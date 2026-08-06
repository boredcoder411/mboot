#include "scheduler.h"
#include "cpu/gdt.h"
#include "dev/serial.h"
#include "mem.h"
#include "paging.h"
#include "utils.h"

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

void scheduler_init(void) {
  current_task = (tcb_t *)kmalloc(sizeof(tcb_t));
  current_task->esp = 0;
  current_task->state = TASK_RUNNING;
  current_task->pid = next_pid++;
  current_task->stack_base = 0;
  current_task->cr3 = paging_get_kernel_directory();
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

  tcb->next = idle_task->next;
  idle_task->next = tcb;

  INFO("SCHED", "Created task PID %u @ 0x%X (stack 0x%X, func 0x%X)", tcb->pid,
       (uint32_t)tcb, tcb->stack_base, (uint32_t)func);
  return tcb->pid;
}

int create_user_task(uint32_t entry, uint32_t code_base, uint32_t code_size,
                     int argc, char **argv) {
  if (argc < 0)
    return -1;
  if (argc > MAX_TASK_ARGS)
    argc = MAX_TASK_ARGS;

  tcb_t *tcb = (tcb_t *)kmalloc(sizeof(tcb_t));
  uint8_t *stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
  uint32_t dir = paging_create_directory();

  for (uint32_t off = 0; off < code_size; off += PAGE_SIZE) {
    paging_map_page(dir, code_base + off, code_base + off,
                    PAGE_USER | PAGE_WRITABLE);
  }
  for (uint32_t addr = (uint32_t)stack; addr < (uint32_t)stack + TASK_STACK_SIZE;
       addr += PAGE_SIZE) {
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

  tcb->next = idle_task->next;
  idle_task->next = tcb;

  INFO("SCHED", "Created user task PID %u @ 0x%X (stack 0x%X, entry 0x%X, "
                "CR3 0x%X, code 0x%X+0x%X)",
       tcb->pid, (uint32_t)tcb, tcb->stack_base, entry, tcb->cr3, code_base,
       code_size);

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

void scheduler_exit_current(int exit_code) {
  if (!current_task || current_task == idle_task)
    return;

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
