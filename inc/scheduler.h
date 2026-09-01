#pragma once

#include "fd.h"
#include "io.h"
#include <stdint.h>

#define TASK_STACK_SIZE (64 * 1024)

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
  uint32_t cr3;
  int tty;
  fd_table_t fds;
  void *wait_obj;
  int exit_code;
  struct task_control_block *next;
} tcb_t;

typedef struct {
  uint32_t vaddr;
  uint32_t filesz;
  uint32_t memsz;
  const void *data;
} user_segment_t;

int create_task(void (*func)(void));
int create_user_task(uint32_t entry, uint32_t code_base, uint32_t code_size,
                     int argc, char **argv, int tty);
int create_user_task_from_segments(uint32_t entry,
                                   const user_segment_t *segments,
                                   int segment_count, int argc, char **argv,
                                   int tty);
int fork_current(registers_t *r);
void scheduler_init();
void scheduler_exit_current(int exit_code);
void task_exit() __attribute__((noreturn));
uint32_t scheduler_tick(registers_t *r);
fd_table_t *scheduler_current_fds(void);
int scheduler_current_tty(void);
int scheduler_set_tty(int tty);
void scheduler_block(void *wait_obj);
void scheduler_wake(void *wait_obj);
tcb_t *scheduler_get_current(void);
uint32_t scheduler_exec_replace(uint32_t entry, const user_segment_t *segments,
                                 int segment_count, int argc, char **argv,
                                 int tty, registers_t *r);
tcb_t *scheduler_find_by_pid(int pid);
int scheduler_wait_pid(int pid);
