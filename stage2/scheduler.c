#include "scheduler.h"
#include "cpu/gdt.h"
#include "dev/serial.h"
#include "mem.h"
#include "utils.h"

static tcb_t *current_task;
static tcb_t *idle_task;
static uint32_t next_pid;

void scheduler_init(void) {
  current_task = (tcb_t *)kmalloc(sizeof(tcb_t));
  current_task->esp = 0;
  current_task->state = TASK_RUNNING;
  current_task->pid = next_pid++;
  current_task->stack_base = 0;
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

  tcb->next = idle_task->next;
  idle_task->next = tcb;

  INFO("SCHED", "Created task PID %u @ 0x%X (stack 0x%X, func 0x%X)",
       tcb->pid, (uint32_t)tcb, tcb->stack_base, (uint32_t)func);

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

  return current_task->esp;
}

void task_exit(void) {
  current_task->state = TASK_EXITED;
  INFO("SCHED", "Task PID %u exited", current_task->pid);
  while (1) {
    asm volatile("cli; hlt");
  }
}