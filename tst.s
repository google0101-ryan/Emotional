.intel_syntax noprefix

.data
LC0:
	.asciz "0x%08lx, 0x%08lx\n"
.text
foo:
	sub rsp, 8
	mov rdx, rsi
	mov rsi, rdi
	lea rdi, [rip+LC0]
	mov eax, 0
	call printf
	add rsp, 8
	ret
.global main
main:
	mov rdi, 0xABABABAB
	mov rsi, 0xBEEFBEEF
	call foo
	ret