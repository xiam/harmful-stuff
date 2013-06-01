/* android-su.c
 *
 * <carlos@reventlov.com>
 * Trivial "su" backdoor for Android. Device must be already rooted.
 *
 * $ arm-linux-gnueabi-gcc -static -o su android-su.c
 * $ adb push su /system/bin
 *
 * # chmod 4755 /system/bin/su
 *
 * $ adb shell
 * $ su
 * #
 *
 * */

#include <stdio.h>
#include <unistd.h>

#define SHELL "/system/bin/sh\0"

int main() {
  setuid(0);
  setgid(0);
  char *args[] = { SHELL, 0 };
  char *env[] = {
    "LD_LIBRARY_PATH=/system/lib",
    "PATH=/sbin:/system/sbin:/system/bin:/system/xbin",
    0
  };
  execve(args[0], args, env);
  return 0;
}
