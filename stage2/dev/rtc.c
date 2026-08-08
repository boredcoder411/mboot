#include "dev/rtc.h"
#include "io.h"
#include "mem.h"
#include "utils.h"

#define RTC_ADDRESS_PORT 0x70
#define RTC_DATA_PORT 0x71
#define RTC_TIME_STR_SIZE 32

static uint8_t get_rtc_register(uint8_t reg) {
  outb(RTC_ADDRESS_PORT, reg);
  return inb(RTC_DATA_PORT);
}

void itoa_time(uint32_t value, char *buf, uint8_t digits) {
  for (int i = digits - 1; i >= 0; i--) {
    buf[i] = '0' + (value % 10);
    value /= 10;
  }
  buf[digits] = '\0';
}

rtc_time_t rtc_time() {
  rtc_time_t time;

  uint8_t second = get_rtc_register(0x00);
  uint8_t minute = get_rtc_register(0x02);
  uint8_t hour = get_rtc_register(0x04);
  uint8_t day = get_rtc_register(0x07);
  uint8_t month = get_rtc_register(0x08);
  uint8_t year = get_rtc_register(0x09);

  time.second = bcd_to_bin(second);
  time.minute = bcd_to_bin(minute);
  time.hour = bcd_to_bin(hour);
  time.day = bcd_to_bin(day);
  time.month = bcd_to_bin(month);
  time.year = 2000 + bcd_to_bin(year);

  return time;
}

char *fetch_rtc() {
  static char *time_str = NULL;
  if (time_str == NULL)
    time_str = (char *)kmalloc(RTC_TIME_STR_SIZE);

  rtc_time_t t = rtc_time();

  int i = 0;
  itoa_time(t.year, &time_str[i], 4);
  i += 4;
  time_str[i++] = '-';
  itoa_time(t.month, &time_str[i], 2);
  i += 2;
  time_str[i++] = '-';
  itoa_time(t.day, &time_str[i], 2);
  i += 2;
  time_str[i++] = ' ';
  itoa_time(t.hour, &time_str[i], 2);
  i += 2;
  time_str[i++] = ':';
  itoa_time(t.minute, &time_str[i], 2);
  i += 2;
  time_str[i++] = ':';
  itoa_time(t.second, &time_str[i], 2);

  return time_str;
}
