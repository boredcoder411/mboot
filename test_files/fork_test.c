// Exercise fork and distinguish the parent and child return values.
#include "libc.h"

int main(void) {
  write(1, "fork_test: before fork\n", 23);
  int pid = fork();

  /* Both processes continue here: fork returns zero in the child and the
     child's PID in the parent. Scheduling determines their print order. */
  if (pid == 0) {
    write(1, "fork_test: I am the child\n", 26);
  } else if (pid > 0) {
    write(1, "fork_test: I am the parent, child pid=", 38);
    char buf[16];
    int n = 0;
    int p = pid;
    if (p == 0)
      buf[n++] = '0';
    /* Extract decimal digits backwards, then reverse them for printing. */
    while (p > 0) {
      buf[n++] = '0' + (p % 10);
      p /= 10;
    }
    for (int i = 0; i < n / 2; i++) {
      char t = buf[i];
      buf[i] = buf[n - 1 - i];
      buf[n - 1 - i] = t;
    }
    buf[n++] = '\n';
    write(1, buf, (size_t)n);
  } else {
    write(1, "fork_test: fork failed\n", 23);
  }

  for (volatile int i = 0; i < 5000000; i++)
    ;
  return 0;
}
