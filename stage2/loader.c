#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/keyboard.h"
#include "dev/serial.h"
#include "dev/vga.h"
#include "mem.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

#define STACK_SIZE 4096

extern void enable_fpu();

typedef struct task {
  registers_t context;
  struct task* next;
} task_t;

static task_t *current = NULL;
static task_t *kernel_task = NULL;

void init_scheduler() {
  kernel_task = (task_t *)alloc_page();
  memset(kernel_task, 0, sizeof(task_t));
  kernel_task->next = kernel_task;
  current = kernel_task;
}

void pit_handler(registers_t *r) {
  if(current == NULL) return;

  memcpy(&current->context, r, sizeof(registers_t));
  current = current->next;
  memcpy(r, &current->context, sizeof(registers_t));
}

void create_task(void (*entry)()) {
  task_t *t = (task_t *)alloc_page();
  memset(t, 0, sizeof(task_t));

  void *stack = alloc_page();

  registers_t *initial_context = (registers_t*)((uint32_t)stack + STACK_SIZE - sizeof(registers_t));
  memset(&initial_context->eax, 0, sizeof(uint32_t) * 8);

  initial_context->eip = (uint32_t)entry;
  initial_context->cs = 0x08;
  initial_context->eflags = 0x202;
  initial_context->ss = 0x10;

  initial_context->esp = (uint32_t)initial_context;
  memcpy(&t->context, initial_context, sizeof(registers_t));

  t->next = kernel_task->next;
  kernel_task->next = t;
}

void task1() {
  while (1) {
    serial_print("task 1\n");
  }
}

void task2() {
  while (1) {
    serial_print("task 2\n");
  }
}

void loader_start() {
  for (int i = 0; i < IRQs; i++) {
    pic_set_mask(i);
  }

  pic_remap();
  idt_init();
  install_exception_isrs();
  pit_init();
  install_irq(0, pit_handler);
  pic_clear_mask(0);
  enable_fpu();
  install_keyboard();

  memset(VIDEO_MEMORY, 0, 320 * 200);

  e820_entry_t *mem_map = (e820_entry_t *)0x9000;
  uint16_t entry_count = (*(uint16_t *)0x8E00);

  init_alloc(entry_count, mem_map);

  init_scheduler();

  create_task(task1);
  create_task(task2);

  asm("sti");

  while (1) {
    serial_print("kernel is running\n");
  }
}
