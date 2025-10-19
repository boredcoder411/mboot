#include "rtc.h"
#include "io.h"
#include "utils.h"

#define RTC_ADDRESS_PORT 0x70
#define RTC_DATA_PORT 0x71

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
  static char time_str[32];
  rtc_time_t t = rtc_time();

  char year[5], month[3], day[3], hour[3], minute[3], second[3];

  itoa_time(t.year, year, 4);
  itoa_time(t.month, month, 2);
  itoa_time(t.day, day, 2);
  itoa_time(t.hour, hour, 2);
  itoa_time(t.minute, minute, 2);
  itoa_time(t.second, second, 2);

  int i = 0;
  time_str[i++] = year[0];
  time_str[i++] = year[1];
  time_str[i++] = year[2];
  time_str[i++] = year[3];
  time_str[i++] = '-';
  time_str[i++] = month[0];
  time_str[i++] = month[1];
  time_str[i++] = '-';
  time_str[i++] = day[0];
  time_str[i++] = day[1];
  time_str[i++] = ' ';
  time_str[i++] = hour[0];
  time_str[i++] = hour[1];
  time_str[i++] = ':';
  time_str[i++] = minute[0];
  time_str[i++] = minute[1];
  time_str[i++] = ':';
  time_str[i++] = second[0];
  time_str[i++] = second[1];
  time_str[i + 2] = '\0';

  return time_str;
}
