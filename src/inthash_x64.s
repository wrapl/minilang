.intel_syntax
.global inthash_search

//RDI, RSI, RDX, RCX, R8, R9 (R10)

.text
inthash_search:
	// %rdi -> inthash_t Map
	//	(%rdi) -> uint64_t *Keys
	//	8(%rdi) -> void *Values
	//	16(%rdi) -> int Size
	//	20(%rdi) -> int Space
	// %rsi -> uint64_t Key
	mov %ecx, [%rdi + 16]
	mov %r8, [%rdi + 8]
	dec %rcx
	js .empty
	mov %rdi, [%rdi]
	mov %rdx, %rsi
	mov %rax, %rsi
	shr %rdx, 8
	shr %rax, 6
	or %rdx, 1
	jmp .entry
.search:
	add %rax, %rdx
.entry:
	and %rax, %rcx
	cmp %rsi, [%rdi + %rax * 8]
	jb .search
	ja .empty
	mov %rax, [%r8 + %rax * 8]
	ret
.empty:
	xor %eax, %eax
	ret
