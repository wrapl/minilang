.intel_syntax
.global inthash_search

//RDI, RSI, RDX, RCX, R8, R9 (R10)

.set INDEX_SHIFT, 6
.set INCR_SHIFT, 8

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
	sub %rcx, 1
	jc .empty
	mov %rdi, [%rdi]
	mov %rax, %rsi
	shr %rax, INDEX_SHIFT
	and %rax, %rcx
	cmp %rsi, [%rdi + %rax * 8]
	je .found
	ja .empty
.search:
	mov %rdx, %rsi
	shr %rdx, INCR_SHIFT
	or %rdx, 1
.loop:
	add %rax, %rdx
	and %rax, %rcx
	cmp %rsi, [%rdi + %rax * 8]
	jb .loop
	ja .empty
.found:
	mov %rax, [%r8 + %rax * 8]
	ret
.empty:
	xor %eax, %eax
	ret
