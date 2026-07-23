#pragma once

#include "io.h"
#include <stdint.h>

#define TASK_STACK_SIZE 4096

typedef enum {
  TASK_READY,
  TASK_RUNNING,
  TASK_BLOCKED,
  TASK_EXITED
} task_state_t;

typedef struct task_control_block {
  uint32_t esp;
  task_state_t state;
  uint32_t pid;
  uint32_t stack_base;
  struct task_control_block *next;
} tcb_t;

int create_task(void (*func)(void));
void scheduler_init();
void task_exit() __attribute__((noreturn));
uint32_t scheduler_tick(registers_t *r);