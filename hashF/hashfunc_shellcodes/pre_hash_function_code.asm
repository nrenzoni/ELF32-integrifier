bits 32

[SECTION .text]

global _start

_start:

	call word start ; overide dw to word by appending 0x66 to beginning of opcode (shortens bytecode from 5 bytes to 4)

start:

	; use eax to get start address of original .text section (before any injections)

	pop eax 		; eax now contains address of this instruction

		
	; add (total size of this file's compiled asm bytecodes) - 4 (offset to current address in eax) to eax
	add eax, 35

	; add size of to-be injected hash function to eax. 0x3210 is dummy-placeholder replaced by runtime of integrifier program
	add eax, 0x3210
	
hash_verify:

	; amount of bytes to calcuate hash digest on (sizeof .text section before injections). replaced during runtime of integrifier program
	push 0x1234

	; initial address where to begin hash digest calculation
	push eax	

	call word hash_function ; hash function digest result stored in eax
	add esp, 8

	cmp eax, 0x9876 ; 0x9876 is place-holder of hash digest, calculated and replaced during runtime of integrifier
	je $+300 ; jmp to original entry point of .text section before any injections. offset needs to be calculated and replaced during integrifier runtime
	
	; else hash digest mismatch. exit()
	xor eax, eax
	mov al, 1 		; exit is syscall 1
	xor ebx, ebx
	int 0x80		; syscall
	
	
hash_function:

	; hash function injected during runtime here (compiled func must use cdecl calling convention and should return normally) ...
