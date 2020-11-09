.intel_syntax
.global inthash_search
.global inthash_search2

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
	jc .L1
	mov %rdi, [%rdi]
	mov %rax, %rsi
	shr %rax, INDEX_SHIFT
	and %rax, %rcx
	cmp %rsi, [%rdi + %rax * 8]
	je .L2
	ja .L1
.L3:
	mov %rdx, %rsi
	shr %rdx, INCR_SHIFT
	or %rdx, 1
.L4:
	add %rax, %rdx
	and %rax, %rcx
	cmp %rsi, [%rdi + %rax * 8]
	jb .L4
	ja .L1
.L2:
	mov %rax, [%r8 + %rax * 8]
	ret
.L1:
	xor %eax, %eax
	ret

.text
inthash_search2:
	// %rdi -> inthash_t Map
	//	(%rdi) -> uint64_t *Keys
	//	8(%rdi) -> void *Values
	//	16(%rdi) -> int Size
	//	20(%rdi) -> int Space
	// %rsi -> uint64_t Key
	mov %ecx, [%rdi + 16]
	mov %r8, [%rdi + 8]
	sub %rcx, 1
	jc .L5
	mov %rdi, [%rdi]
	mov %rax, %rsi
	shr %rax, INDEX_SHIFT
	and %rax, %rcx
	cmp %rsi, [%rdi + %rax * 8]
	je .L6
	ja .L5
.L7:
	mov %rdx, %rsi
	shr %rdx, INCR_SHIFT
	or %rdx, 1
.L8:
	add %rax, %rdx
	and %rax, %rcx
	cmp %rsi, [%rdi + %rax * 8]
	jb .L8
	ja .L5
.L6:
	mov %rax, [%r8 + %rax * 8]
	xor %edx, %edx
	dec %rdx
	ret
.L5:
	xor %eax, %eax
	xor %edx, %edx
	ret
