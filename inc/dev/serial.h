#pragma once

#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_BLUE    "\033[34m"

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
} log_level_t;

int init_serial();
void write_serial(char a);
void message_preamble(const char *module, log_level_t level);
void serial_printf(const char *fmt, ...);

#define INFO(module, fmt, ...)                    \
    do {                                          \
        message_preamble(module, LOG_INFO);          \
        serial_printf(fmt, ##__VA_ARGS__);        \
        write_serial('\n');                       \
    } while (0)

#define WARN(module, fmt, ...)                    \
    do {                                          \
        message_preamble(module, LOG_WARN);          \
        serial_printf(fmt, ##__VA_ARGS__);        \
        write_serial('\n');                       \
    } while (0)

#define ERROR(module, fmt, ...)                   \
    do {                                          \
        message_preamble(module, LOG_ERROR);          \
        serial_printf(fmt, ##__VA_ARGS__);        \
        write_serial('\n');                       \
    } while (0)

#define DEBUG(module, fmt, ...)                   \
    do {                                          \
        message_preamble(module, LOG_DEBUG);          \
        serial_printf(fmt, ##__VA_ARGS__);        \
        write_serial('\n');                       \
    } while (0)