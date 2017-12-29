	global main
	section .text

main:

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '3'
	JNE ko 
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '4'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'C'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '3'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '_'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'M'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '1'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'G'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'H'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'T'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'Y'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '_'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'M'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '0'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'R'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'P'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'h'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '1'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'n'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'G'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '_'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, 'g'
	JNE ko
	JMP decrypt

	PUSH RSI ; each function is basically check(char*, void* next_fnc, char key)
	PUSH RDX ; top of stack contains the next routine to decrypt and the key
	MOV AL, byte [RDI]
	CMP AL, '0'
	JNE ko
	JMP decrypt

	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
decrypt:
	POP RAX ; al = key
	POP RDI ; rdi = void* next_fnc
	MOV R8, 0
loop:
	CMP R8, 17
	JE bailout
	MOV BL, byte [RDI]
	MOV CL, AL
	XOR BL, CL
	MOV byte [RDI], BL
	INC R8
	INC RDI
	JMP loop

ko:
	MOV RAX, 60
    	MOV RDI, 1
    	SYSCALL

bailout:
	RET
