	.file	"fastHash.c"
	.intel_syntax noprefix
# GNU C11 (Debian 7.2.0-17) version 7.2.1 20171205 (x86_64-linux-gnu)
#	compiled by GNU C version 7.2.1 20171205, GMP version 6.1.2, MPFR version 3.1.6, MPC version 1.0.3, isl version isl-0.18-GMP

# GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
# options passed:  -I ./../include/ -imultilib 32
# -imultiarch i386-linux-gnu fastHash.c -m32 -masm=intel -mtune=generic
# -march=i686 -auxbase-strip fastHash.s -O3 -fno-dwarf2-cfi-asm
# -fno-asynchronous-unwind-tables -fverbose-asm
# options enabled:  -fPIC -fPIE -faggressive-loop-optimizations
# -falign-labels -fauto-inc-dec -fbranch-count-reg -fcaller-saves
# -fchkp-check-incomplete-type -fchkp-check-read -fchkp-check-write
# -fchkp-instrument-calls -fchkp-narrow-bounds -fchkp-optimize
# -fchkp-store-bounds -fchkp-use-static-bounds
# -fchkp-use-static-const-bounds -fchkp-use-wrappers -fcode-hoisting
# -fcombine-stack-adjustments -fcommon -fcompare-elim -fcprop-registers
# -fcrossjumping -fcse-follow-jumps -fdefer-pop
# -fdelete-null-pointer-checks -fdevirtualize -fdevirtualize-speculatively
# -fearly-inlining -feliminate-unused-debug-types -fexpensive-optimizations
# -fforward-propagate -ffp-int-builtin-inexact -ffunction-cse -fgcse
# -fgcse-after-reload -fgcse-lm -fgnu-runtime -fgnu-unique
# -fguess-branch-probability -fhoist-adjacent-loads -fident -fif-conversion
# -fif-conversion2 -findirect-inlining -finline -finline-atomics
# -finline-functions -finline-functions-called-once
# -finline-small-functions -fipa-bit-cp -fipa-cp -fipa-cp-clone -fipa-icf
# -fipa-icf-functions -fipa-icf-variables -fipa-profile -fipa-pure-const
# -fipa-ra -fipa-reference -fipa-sra -fipa-vrp -fira-hoist-pressure
# -fira-share-save-slots -fira-share-spill-slots
# -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
# -fleading-underscore -flifetime-dse -flra-remat -flto-odr-type-merging
# -fmath-errno -fmerge-constants -fmerge-debug-strings
# -fmove-loop-invariants -fomit-frame-pointer -foptimize-sibling-calls
# -foptimize-strlen -fpartial-inlining -fpcc-struct-return -fpeel-loops
# -fpeephole -fpeephole2 -fplt -fpredictive-commoning
# -fprefetch-loop-arrays -free -freorder-blocks -freorder-functions
# -frerun-cse-after-loop -fsched-critical-path-heuristic
# -fsched-dep-count-heuristic -fsched-group-heuristic -fsched-interblock
# -fsched-last-insn-heuristic -fsched-rank-heuristic -fsched-spec
# -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-fusion
# -fschedule-insns2 -fsemantic-interposition -fshow-column -fshrink-wrap
# -fshrink-wrap-separate -fsigned-zeros -fsplit-ivs-in-unroller
# -fsplit-loops -fsplit-paths -fsplit-wide-types -fssa-backprop
# -fssa-phiopt -fstdarg-opt -fstore-merging -fstrict-aliasing
# -fstrict-overflow -fstrict-volatile-bitfields -fsync-libcalls
# -fthread-jumps -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp
# -ftree-builtin-call-dce -ftree-ccp -ftree-ch -ftree-coalesce-vars
# -ftree-copy-prop -ftree-cselim -ftree-dce -ftree-dominator-opts
# -ftree-dse -ftree-forwprop -ftree-fre -ftree-loop-distribute-patterns
# -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
# -ftree-loop-optimize -ftree-loop-vectorize -ftree-parallelize-loops=
# -ftree-partial-pre -ftree-phiprop -ftree-pre -ftree-pta -ftree-reassoc
# -ftree-scev-cprop -ftree-sink -ftree-slp-vectorize -ftree-slsr -ftree-sra
# -ftree-switch-conversion -ftree-tail-merge -ftree-ter -ftree-vrp
# -funit-at-a-time -funswitch-loops -fverbose-asm -fzero-initialized-in-bss
# -m32 -m80387 -m96bit-long-double -malign-stringops
# -mavx256-split-unaligned-load -mavx256-split-unaligned-store
# -mfancy-math-387 -mfp-ret-in-387 -mglibc -mieee-fp -mlong-double-80
# -mno-red-zone -mno-sse4 -mpush-args -msahf -mstv -mtls-direct-seg-refs
# -mvzeroupper

	.text
	.p2align 4,,15
	.globl	fastHash
	.type	fastHash, @function
fastHash:
	push	edi	#
	push	esi	#
	push	ebx	#
# fastHash.c:17: uint32_t fastHash (const uint8_t * data, uint32_t len) {
	mov	eax, DWORD PTR 20[esp]	# len, len
	mov	edx, DWORD PTR 16[esp]	# data, data
# fastHash.c:21:     if (len <= 0 || data == NULL) return 0;
	test	eax, eax	# len
	je	.L9	#,
	test	edx, edx	# data
	je	.L9	#,
# fastHash.c:24:     len >>= 2;
	mov	ecx, eax	# len, len
# fastHash.c:23:     rem = len & 3;
	mov	ebx, eax	# rem, len
# fastHash.c:24:     len >>= 2;
	shr	ecx, 2	# len,
# fastHash.c:23:     rem = len & 3;
	and	ebx, 3	# rem,
# fastHash.c:27:     for (;len > 0; len--) {
	test	ecx, ecx	# len
	je	.L10	#,
	lea	ecx, [edx+ecx*4]	# data,
	.p2align 4,,10
	.p2align 3
.L4:
# fastHash.c:28:         hash  += get16bits (data);
	movzx	esi, WORD PTR [edx]	# MEM[base: data_72, offset: 0B], MEM[base: data_72, offset: 0B]
# fastHash.c:31:         data  += 2*sizeof (uint16_t);
	add	edx, 4	# data,
# fastHash.c:28:         hash  += get16bits (data);
	add	esi, eax	# hash, len
# fastHash.c:29:         tmp    = (get16bits (data+2) << 11) ^ hash;
	movzx	eax, WORD PTR -2[edx]	# MEM[base: data_72, offset: 2B], MEM[base: data_72, offset: 2B]
# fastHash.c:30:         hash   = (hash << 16) ^ tmp;
	mov	edi, esi	# tmp175, hash
	sal	edi, 16	# tmp175,
# fastHash.c:29:         tmp    = (get16bits (data+2) << 11) ^ hash;
	sal	eax, 11	# tmp174,
	xor	eax, edi	# _41, tmp175
# fastHash.c:30:         hash   = (hash << 16) ^ tmp;
	xor	eax, esi	# hash, hash
# fastHash.c:32:         hash  += hash >> 11;
	mov	esi, eax	# _13, hash
	shr	esi, 11	# _13,
	add	eax, esi	# len, _13
# fastHash.c:27:     for (;len > 0; len--) {
	cmp	edx, ecx	# data, data
	jne	.L4	#,
.L3:
# fastHash.c:36:     switch (rem) {
	cmp	ebx, 2	# rem,
	je	.L6	#,
	cmp	ebx, 3	# rem,
	je	.L7	#,
	cmp	ebx, 1	# rem,
	je	.L14	#,
.L5:
# fastHash.c:52:     hash ^= hash << 3;
	lea	edx, 0[0+eax*8]	# _33,
# fastHash.c:60: }
	pop	ebx	#
# fastHash.c:52:     hash ^= hash << 3;
	xor	eax, edx	# hash, _33
# fastHash.c:53:     hash += hash >> 5;
	mov	edx, eax	# _34, hash
	shr	edx, 5	# _34,
	add	eax, edx	# hash, _34
# fastHash.c:54:     hash ^= hash << 4;
	mov	edx, eax	# _35, hash
	sal	edx, 4	# _35,
	xor	eax, edx	# hash, _35
# fastHash.c:55:     hash += hash >> 17;
	mov	edx, eax	# _36, hash
	shr	edx, 17	# _36,
	add	edx, eax	# hash, hash
# fastHash.c:56:     hash ^= hash << 25;
	mov	eax, edx	# _37, hash
	sal	eax, 25	# _37,
	xor	edx, eax	# hash, _37
# fastHash.c:57:     hash += hash >> 6;
	mov	eax, edx	# _38, hash
	shr	eax, 6	# _38,
	add	eax, edx	# <retval>, hash
# fastHash.c:60: }
	pop	esi	#
	pop	edi	#
	ret
	.p2align 4,,10
	.p2align 3
.L9:
	pop	ebx	#
# fastHash.c:21:     if (len <= 0 || data == NULL) return 0;
	xor	eax, eax	# <retval>
# fastHash.c:60: }
	pop	esi	#
	pop	edi	#
	ret
	.p2align 4,,10
	.p2align 3
.L14:
# fastHash.c:46:         case 1: hash += (signed char)*data;
	movsx	edx, BYTE PTR [ecx]	# *data_73, *data_73
	add	edx, eax	# hash, len
# fastHash.c:47:                 hash ^= hash << 10;
	mov	eax, edx	# _31, hash
	sal	eax, 10	# _31,
	xor	eax, edx	# hash, hash
# fastHash.c:48:                 hash += hash >> 1;
	mov	edx, eax	# _32, hash
	shr	edx	# _32
	add	eax, edx	# len, _32
	jmp	.L5	#
	.p2align 4,,10
	.p2align 3
.L10:
# fastHash.c:27:     for (;len > 0; len--) {
	mov	ecx, edx	# data, data
	jmp	.L3	#
	.p2align 4,,10
	.p2align 3
.L7:
# fastHash.c:37:         case 3: hash += get16bits (data);
	movzx	edx, WORD PTR [ecx]	# MEM[(const uint16_t *)data_73], MEM[(const uint16_t *)data_73]
	add	eax, edx	# hash, MEM[(const uint16_t *)data_73]
# fastHash.c:38:                 hash ^= hash << 16;
	mov	edx, eax	# _16, hash
	sal	edx, 16	# _16,
	xor	edx, eax	# hash, hash
# fastHash.c:39:                 hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
	movsx	eax, BYTE PTR 2[ecx]	# MEM[(const uint8_t *)data_73 + 2B], MEM[(const uint8_t *)data_73 + 2B]
	sal	eax, 18	# tmp178,
	xor	eax, edx	# hash, hash
# fastHash.c:40:                 hash += hash >> 11;
	mov	edx, eax	# _23, hash
	shr	edx, 11	# _23,
	add	eax, edx	# len, _23
# fastHash.c:41:                 break;
	jmp	.L5	#
	.p2align 4,,10
	.p2align 3
.L6:
# fastHash.c:42:         case 2: hash += get16bits (data);
	movzx	edx, WORD PTR [ecx]	# MEM[(const uint16_t *)data_73], MEM[(const uint16_t *)data_73]
	add	edx, eax	# hash, len
# fastHash.c:43:                 hash ^= hash << 11;
	mov	eax, edx	# _26, hash
	sal	eax, 11	# _26,
	xor	eax, edx	# hash, hash
# fastHash.c:44:                 hash += hash >> 17;
	mov	edx, eax	# _27, hash
	shr	edx, 17	# _27,
	add	eax, edx	# len, _27
# fastHash.c:45:                 break;
	jmp	.L5	#
	.size	fastHash, .-fastHash
	.ident	"GCC: (Debian 7.2.0-17) 7.2.1 20171205"
	.section	.note.GNU-stack,"",@progbits
