#include "scheduler.h"
#include "cpu/gdt.h"
#include "dev/serial.h"
#include "mem.h"
#include "paging.h"
#include "utils.h"
#include "vfs.h"

#define MAX_TASK_ARGS 16

static tcb_t *current_task;
static tcb_t *idle_task;
static uint32_t next_pid;

static char *copy_arg(const char *arg) {
  int len = strlen(arg);
  char *copy = (char *)kmalloc((uint32_t)len + 1);
  memcpy(copy, arg, (size_t)len + 1);
  return copy;
}

static void task_init_fds(tcb_t *tcb) {
  if (current_task && current_task != idle_task)
    vfs_fork_fds(&tcb->fds);
  else
    vfs_init_fds(&tcb->fds, tcb->tty);
}

static uint32_t page_align_down(uint32_t x) { return x & ~(PAGE_SIZE - 1); }

static uint32_t page_align_up(uint32_t x) {
  return (uint32_t)align_up_uintptr(x, PAGE_SIZE);
}

static void *alloc_user_page(void) {
  uintptr_t raw = (uintptr_t)kmalloc(PAGE_SIZE * 2);
  void *page = (void *)align_up_uintptr(raw, PAGE_SIZE);
  memset(page, 0, PAGE_SIZE);
  return page;
}

fd_table_t *scheduler_current_fds(void) {
  return current_task ? &current_task->fds : NULL;
}

int scheduler_current_tty(void) { return current_task ? current_task->tty : 0; }

void scheduler_init(void) {
  current_task = (tcb_t *)kmalloc(sizeof(tcb_t));
  current_task->esp = 0;
  current_task->state = TASK_RUNNING;
  current_task->pid = next_pid++;
  current_task->stack_base = 0;
  current_task->cr3 = paging_get_kernel_directory();
  current_task->tty = 0;
  vfs_init_fds(&current_task->fds, current_task->tty);
  current_task->next = current_task;
  idle_task = current_task;

  INFO("SCHED", "Scheduler initialized (idle PID %u)", current_task->pid);
}

int create_task(void (*func)(void)) {
  tcb_t *tcb = (tcb_t *)kmalloc(sizeof(tcb_t));
  uint8_t *stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);

  uint32_t *sp = (uint32_t *)(stack + TASK_STACK_SIZE);

  *--sp = (uint32_t)task_exit;
  *--sp = 0x00000202;
  *--sp = GDT_CODE_SEG;
  *--sp = (uint32_t)func;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;

  tcb->esp = (uint32_t)sp;
  tcb->state = TASK_READY;
  tcb->pid = next_pid++;
  tcb->stack_base = (uint32_t)stack;
  tcb->cr3 = paging_get_kernel_directory();
  tcb->tty = scheduler_current_tty();
  task_init_fds(tcb);

  tcb->next = idle_task->next;
  idle_task->next = tcb;

  INFO("SCHED", "Created task PID %u @ 0x%X (stack 0x%X, func 0x%X)", tcb->pid,
       (uint32_t)tcb, tcb->stack_base, (uint32_t)func);
  return tcb->pid;
}

int create_user_task(uint32_t entry, uint32_t code_base, uint32_t code_size,
                     int argc, char **argv, int tty) {
  user_segment_t segment = {
      .vaddr = code_base,
      .filesz = code_size,
      .memsz = code_size,
      .data = (const void *)code_base,
  };
  return create_user_task_from_segments(entry, &segment, 1, argc, argv, tty);
}

int create_user_task_from_segments(uint32_t entry,
                                   const user_segment_t *segments,
                                   int segment_count, int argc, char **argv,
                                   int tty) {
  if (argc < 0)
    return -1;
  if (argc > MAX_TASK_ARGS)
    argc = MAX_TASK_ARGS;
  if (!segments || segment_count <= 0)
    return -1;

  tcb_t *tcb = (tcb_t *)kmalloc(sizeof(tcb_t));
  uint8_t *stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
  uint32_t dir = paging_create_directory();

  uint32_t image_base = 0xFFFFFFFFU;
  uint32_t image_end = 0;
  for (int i = 0; i < segment_count; i++) {
    uint32_t seg_base = page_align_down(segments[i].vaddr);
    uint32_t seg_end = page_align_up(segments[i].vaddr + segments[i].memsz);
    if (seg_base < image_base)
      image_base = seg_base;
    if (seg_end > image_end)
      image_end = seg_end;
  }
  if (image_base >= image_end)
    return -1;

  uint32_t page_count = (image_end - image_base) / PAGE_SIZE;
  uint32_t *image_pages = (uint32_t *)kmalloc(page_count * sizeof(uint32_t));
  for (uint32_t i = 0; i < page_count; i++) {
    void *page = alloc_user_page();
    image_pages[i] = (uint32_t)page;
    paging_map_page(dir, image_base + i * PAGE_SIZE, image_pages[i],
                    PAGE_USER | PAGE_WRITABLE);
  }

  for (int i = 0; i < segment_count; i++) {
    uint32_t dst_vaddr = segments[i].vaddr;
    const uint8_t *src = (const uint8_t *)segments[i].data;
    uint32_t remaining = segments[i].filesz;
    while (remaining > 0) {
      uint32_t page_idx = (page_align_down(dst_vaddr) - image_base) / PAGE_SIZE;
      uint32_t page_off = dst_vaddr & (PAGE_SIZE - 1);
      uint32_t chunk = PAGE_SIZE - page_off;
      if (chunk > remaining)
        chunk = remaining;
      memcpy((void *)(image_pages[page_idx] + page_off), src, chunk);
      src += chunk;
      dst_vaddr += chunk;
      remaining -= chunk;
    }
  }

  for (uint32_t addr = (uint32_t)stack & 0xFFFFF000U;
       addr < (uint32_t)stack + TASK_STACK_SIZE; addr += PAGE_SIZE) {
    paging_map_page(dir, addr, addr, PAGE_USER | PAGE_WRITABLE);
  }

  uint32_t *sp = (uint32_t *)(stack + TASK_STACK_SIZE);

  *--sp = 0;
  *--sp = 0;
  for (int i = argc - 1; i >= 0; --i) {
    *--sp = (uint32_t)copy_arg(argv[i]);
  }
  *--sp = (uint32_t)argc;

  *--sp = 0x00000202;
  *--sp = GDT_CODE_SEG;
  *--sp = entry;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;

  tcb->esp = (uint32_t)sp;
  tcb->state = TASK_READY;
  tcb->pid = next_pid++;
  tcb->stack_base = (uint32_t)stack;
  tcb->cr3 = dir;
  task_init_fds(tcb);
  if (tty >= 0) {
    tcb->tty = tty;
    vfs_bind_tty(&tcb->fds, tty);
  } else {
    tcb->tty = scheduler_current_tty();
  }

  tcb->next = idle_task->next;
  idle_task->next = tcb;

  INFO("SCHED",
       "Created user task PID %u @ 0x%X (stack 0x%X, entry 0x%X, "
       "CR3 0x%X, image 0x%X+0x%X)",
       tcb->pid, (uint32_t)tcb, tcb->stack_base, entry, tcb->cr3, image_base,
       image_end - image_base);

  return tcb->pid;
}

uint32_t scheduler_tick(registers_t *r) {
  if (!current_task)
    return (uint32_t)r;

  current_task->esp = (uint32_t)r;

  if (current_task->state == TASK_RUNNING) {
    current_task->state = TASK_READY;
  }

  tcb_t *next = current_task;
  do {
    next = next->next;
  } while (next->state != TASK_READY && next != current_task);

  if (next->state == TASK_READY) {
    current_task = next;
    current_task->state = TASK_RUNNING;
  } else {
    current_task->state = TASK_RUNNING;
  }

  if (current_task->cr3 != paging_get_directory()) {
    paging_switch(current_task->cr3);
  }

  return current_task->esp;
}

int fork_current(registers_t *r) {
  if (!current_task || current_task == idle_task)
    return -1;

  uint32_t dir = paging_clone_directory(current_task->cr3);

  tcb_t *child = (tcb_t *)kmalloc(sizeof(tcb_t));

  // All tasks run at ring 0, so int $0x80 does not push ss/esp and the
  // registers_t esp/ss fields are garbage. The child must resume with the
  // parent's user esp: place its iret frame in the child's own copy of the
  // user stack page, 48 bytes below the parent's esp at syscall entry.
  uint32_t user_esp = (uint32_t)&r->eip + 12;
  uint32_t frame_vaddr = user_esp - 48;

  uint32_t *cdir = (uint32_t *)dir;
  uint32_t pde = cdir[frame_vaddr >> 22];
  uint32_t *ctab = (uint32_t *)(pde & 0xFFFFF000U);
  uint32_t frame_paddr = ctab[(frame_vaddr >> 12) & 0x3FF] & 0xFFFFF000U;
  registers_t *child_frame =
      (registers_t *)(frame_paddr + (frame_vaddr & 0xFFFU));

  memcpy(child_frame, r, 48);
  child_frame->eax = 0;

  child->esp = (uint32_t)child_frame;
  child->state = TASK_READY;
  child->pid = next_pid++;
  child->stack_base = 0;
  child->cr3 = dir;
  child->tty = current_task->tty;
  vfs_fork_fds(&child->fds);

  child->next = idle_task->next;
  idle_task->next = child;

  INFO("SCHED", "fork: PID %u -> child PID %u (CR3 0x%X)", current_task->pid,
       child->pid, dir);

  return child->pid;
}

void scheduler_exit_current(int exit_code) {
  if (!current_task || current_task == idle_task)
    return;

  vfs_exit_fds(&current_task->fds);
  current_task->state = TASK_EXITED;
  INFO("SCHED", "Task PID %u exited with code %d", current_task->pid,
       exit_code);
}

void task_exit(void) {
  current_task->state = TASK_EXITED;
  INFO("SCHED", "Task PID %u exited", current_task->pid);
  while (1) {
    asm volatile("cli; hlt");
  }
}
