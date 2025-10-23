#include "dev/e1k.h"
#include "dev/serial.h"
#include "utils.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void e1k_init(uint8_t bus, uint8_t device, uint8_t func, uint16_t vendor,
              uint16_t device_id) {
  ERROR("E1K", "Not implemented yet");
  HALT()
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void e1k_send(uint16_t io_base, const uint8_t *frame, uint16_t len) {}
