ASM = nasm -f elf64 -o

.PHONY: clean
clean:
	rm -f ./*.o

lib.o: lib.asm
	$(ASM) lib.o $^
dict.o: dict.asm lib.o 
	$(ASM)  dict.o $<
main.o: main.asm dict.o words.inc
	$(ASM) main.o $<
program: main.o dict.o lib.o
	ld -o $@ $^
