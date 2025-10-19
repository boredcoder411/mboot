#pragma once

#include <stdint.h>

#define RTC_ADDRESS_PORT 0x70
#define RTC_DATA_PORT 0x71

typedef struct {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint32_t year;
} rtc_time_t;

char *fetch_rtc();
