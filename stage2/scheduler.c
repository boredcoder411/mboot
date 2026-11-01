// Single-CPU round-robin tasks, saved interrupt frames, blocking, and process creation/replacement. Applications still execute at ring 0.
#include "scheduler.h"
#include "cpu/gdt.h"
#include "dev/serial.h"
#include "dev/tty.h"
#include "mem.h"
#include "paging.h"
#include "utils.h"
#include "vfs.h"

void scheduler_switch(void);

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
  /* kmalloc only promises its own small alignment, whereas a page table
     stores a physical address with its low 12 bits reserved for flags.
     Overallocate and round up so the mapped address is page aligned. The
     original allocation pointer is not retained for later freeing. */
  uintptr_t raw = (uintptr_t)kmalloc(PAGE_SIZE * 2);
  void *page = (void *)align_up_uintptr(raw, PAGE_SIZE);
  memset(page, 0, PAGE_SIZE);
  return page;
}

fd_table_t *scheduler_current_fds(void) {
  return current_task ? &current_task->fds : NULL;
}

tcb_t *scheduler_get_current(void) { return current_task; }

tcb_t *scheduler_find_by_pid(int pid) {
  if (!idle_task || pid < 0)
    return NULL;
  tcb_t *t = idle_task;
  do {
    if ((int)t->pid == pid)
      return t;
    t = t->next;
  } while (t != idle_task);
  return NULL;
}

/* Waiters sleep on the PID encoded as a wait object; exit wakes that key.
   This small interface returns the exit code directly and does not reap. */
int scheduler_wait_pid(int pid) {
  if (pid < 0)
    return -1;
  tcb_t *child = scheduler_find_by_pid(pid);
  if (!child)
    return -1;
  if (child->state == TASK_EXITED)
    return child->exit_code;
  scheduler_block((void *)(uintptr_t)pid);
  child = scheduler_find_by_pid(pid);
  if (!child)
    return -1;
  return child->exit_code;
}

uint32_t scheduler_exec_replace(uint32_t entry, const user_segment_t *segments,
                                int segment_count, int argc, char **argv,
                                int tty, registers_t *r) {
  if (!current_task || current_task == idle_task)
    return 0;
  if (argc < 0)
    argc = 0;
  if (argc > MAX_TASK_ARGS)
    argc = MAX_TASK_ARGS;
  if (!segments || segment_count <= 0)
    return 0;

  /* The caller's argument pointers belong to the old image, so copy the
     strings before switching page directories. */
  char *kargv[MAX_TASK_ARGS];
  int kargc = 0;
  for (int i = 0; i < argc && i < MAX_TASK_ARGS; i++) {
    if (!argv || !argv[i])
      break;
    kargv[i] = copy_arg(argv[i]);
    if (!kargv[i])
      return 0;
    kargc++;
  }

  uint32_t old_cr3 = current_task->cr3;
  /* This temporary clone is created and immediately cleared by the current
     implementation. It is not the new executable image, and cleanup does
     not reclaim all allocations made for the clone. */
  uint32_t clone_tmp = paging_clone_directory(old_cr3);
  (void)clone_tmp;
  paging_free_directory(clone_tmp);

  uint32_t new_cr3 = paging_create_directory();

  uint32_t image_base = 0xFFFFFFFFU;
  uint32_t image_end = 0;
  /* Determine one page-aligned span enclosing every PT_LOAD segment. This
     also allocates pages for any holes between segments. */
  for (int i = 0; i < segment_count; i++) {
    uint32_t seg_base = page_align_down(segments[i].vaddr);
    uint32_t seg_end = page_align_up(segments[i].vaddr + segments[i].memsz);
    if (seg_base < image_base)
      image_base = seg_base;
    if (seg_end > image_end)
      image_end = seg_end;
  }
  if (image_base >= image_end)
    return 0;

  /* Allocate the full page-aligned span, including gaps between segments.
     Zeroed pages also supply the ELF memory beyond each segment's filesz. */
  uint32_t page_count = (image_end - image_base) / PAGE_SIZE;
  uint32_t *image_pages = (uint32_t *)kmalloc(page_count * sizeof(uint32_t));
  if (!image_pages)
    return 0;
  for (uint32_t i = 0; i < page_count; i++) {
    void *page = alloc_user_page();
    image_pages[i] = (uint32_t)page;
    paging_map_page(new_cr3, image_base + i * PAGE_SIZE, image_pages[i],
                    PAGE_USER | PAGE_WRITABLE);
  }

  /* The new directory is not active yet. Copy through the physical page
     addresses recorded in image_pages, splitting writes at page boundaries. */
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

  /* The stack allocation is mapped at the same numeric address in the new
     directory. A forked child has stack_base == 0 and needs a fresh stack. */
  uint32_t stack_base = current_task->stack_base;
  if (stack_base == 0) {
    uint8_t *new_stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    if (!new_stack)
      return 0;
    stack_base = (uint32_t)new_stack;
    current_task->stack_base = stack_base;
  }
  for (uint32_t addr = stack_base & 0xFFFFF000U;
       addr < stack_base + TASK_STACK_SIZE; addr += PAGE_SIZE) {
    paging_map_page(new_cr3, addr, addr, PAGE_USER | PAGE_WRITABLE);
  }

  /* After the saved registers and iret frame are consumed, crt0 sees argc
     followed by argv pointers and a null terminator at the top of stack. */
  uint32_t *sp = (uint32_t *)(stack_base + TASK_STACK_SIZE);
  *--sp = 0;
  *--sp = 0;
  for (int i = kargc - 1; i >= 0; --i) {
    *--sp = (uint32_t)kargv[i];
  }
  *--sp = (uint32_t)kargc;
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

  uint32_t new_sp = (uint32_t)sp;

  /* Commit the replacement only after building its image and initial frame.
     The task keeps its PID and, unless a TTY is supplied, its descriptors. */
  current_task->cr3 = new_cr3;
  current_task->esp = new_sp;
  if (tty >= 0) {
    current_task->tty = tty;
    vfs_bind_tty(&current_task->fds, tty);
  }

  paging_switch(new_cr3);
  paging_free_directory(old_cr3);

  if (r) {
    r->eip = entry;
    r->cs = GDT_CODE_SEG;
    r->eflags = 0x00000202;
    r->eax = 0;
  }

  kfree(image_pages);

  INFO("SCHED",
       "exec replaced PID %u (old CR3 0x%X -> new CR3 0x%X, entry 0x%X, "
       "new esp 0x%X, image 0x%X+0x%X)",
       current_task->pid, old_cr3, new_cr3, entry, new_sp, image_base,
       image_end - image_base);

  return new_sp;
}

int scheduler_current_tty(void) { return current_task ? current_task->tty : 0; }

int scheduler_set_tty(int tty) {
  if (!current_task)
    return -1;
  if (!tty_valid(tty))
    return -1;
  current_task->tty = tty;
  fd_table_t *fds = scheduler_current_fds();
  if (fds)
    vfs_bind_tty(fds, tty);
  return 0;
}

void scheduler_init(void) {
  /* The boot context becomes the idle task and anchors the circular list.
     Its stack pointer is saved on its first timer interrupt or switch. */
  current_task = (tcb_t *)kmalloc(sizeof(tcb_t));
  current_task->esp = 0;
  current_task->state = TASK_RUNNING;
  current_task->pid = next_pid++;
  current_task->stack_base = 0;
  current_task->cr3 = paging_get_kernel_directory();
  current_task->tty = 0;
  current_task->exit_code = 0;
  current_task->wait_obj = NULL;
  vfs_init_fds(&current_task->fds, current_task->tty);
  current_task->next = current_task;
  idle_task = current_task;

  INFO("SCHED", "Scheduler initialized (idle PID %u)", current_task->pid);
}

/* Build an artificial interrupt-return frame so the first switch enters
   func just as if it had been interrupted and then resumed. */
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
  tcb->exit_code = 0;
  tcb->wait_obj = NULL;
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

  /* As in exec, copy each segment to zero-filled physical pages. Zero fill
     provides the ELF memory beyond filesz, including typical .bss data. */
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

  /* Below argc/argv, synthesize the register frame expected by irq.asm.
     iret consumes entry, code selector and EFLAGS (0x202 enables IRQs),
     leaving argc at the top of the stack for user/src/crt0.s. */
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
  tcb->exit_code = 0;
  tcb->wait_obj = NULL;
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

static void scheduler_schedule(void) {
  if (!current_task)
    return;

  /* Walk the circular list once, skipping blocked and exited tasks. */
  tcb_t *next = current_task;
  do {
    next = next->next;
  } while (next != current_task && next->state != TASK_READY &&
           next->state != TASK_RUNNING);

  if (next != current_task) {
    current_task = next;
    current_task->state = TASK_RUNNING;
  } else if (current_task->state == TASK_BLOCKED) {
    /* No other runnable task exists; keep the current one alive. This only
       happens for the idle task during early boot. */
    current_task->state = TASK_RUNNING;
  }

  /* CR3 must match the task before the assembly code resumes its stack and
     instruction pointer; writing it also refreshes address translations. */
  if (current_task->cr3 != paging_get_directory()) {
    paging_switch(current_task->cr3);
  }
}

uint32_t scheduler_tick(registers_t *r) {
  if (!current_task)
    return (uint32_t)r;

  /* The IRQ stub restores whichever frame pointer this function returns. */
  current_task->esp = (uint32_t)r;

  if (current_task->state == TASK_RUNNING) {
    current_task->state = TASK_READY;
  }

  scheduler_schedule();

  return current_task->esp;
}

/* Picks the next runnable task and returns its saved stack pointer. Called
   from the assembly context switch after the old task's frame is stored. */
uint32_t scheduler_perform_switch(uint32_t old_esp) {
  current_task->esp = old_esp;
  scheduler_schedule();
  return current_task->esp;
}

/* Voluntarily give up the CPU until `wait_obj` is signalled via
   scheduler_wake. Does not return until another task (or the timer) resumes
   this one. */
void scheduler_block(void *wait_obj) {
  CLI();
  current_task->wait_obj = wait_obj;
  current_task->state = TASK_BLOCKED;
  scheduler_switch();
}

/* Mark every task blocked on `wait_obj` as runnable again. Safe to call from
   interrupt context; it never performs a context switch itself. */
void scheduler_wake(void *wait_obj) {
  if (!wait_obj)
    return;

  tcb_t *t = idle_task;
  do {
    if (t->state == TASK_BLOCKED && t->wait_obj == wait_obj)
      t->state = TASK_READY;
    t = t->next;
  } while (t != idle_task);
}

/* Clone marked pages and the saved syscall frame. The copied frame returns
   zero in the child, while the dispatcher returns the child PID to parent. */
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
  /* The child's directory is not active yet. Walk its page tables to find
     the physical copy of the stack page and write the child's frame there. */
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
  child->exit_code = 0;
  child->wait_obj = NULL;
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
  current_task->exit_code = exit_code;
  scheduler_wake((void *)(uintptr_t)current_task->pid);
  INFO("SCHED", "Task PID %u exited with code %d", current_task->pid,
       exit_code);
}

void task_exit(void) {
  current_task->state = TASK_EXITED;
  current_task->exit_code = 0;
  scheduler_wake((void *)(uintptr_t)current_task->pid);
  INFO("SCHED", "Task PID %u exited", current_task->pid);
  while (1) {
    asm volatile("cli; hlt");
  }
}
