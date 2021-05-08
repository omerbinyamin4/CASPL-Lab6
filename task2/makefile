exec: c-libs
	gcc -g -m32 -o start myshell.o
	rm -f myshell.o 
	
c-libs: myshell.c
	gcc -g -m32 -c -o myshell.o myshell.c
	
.PHONY: clean
clean:
	rm -rf ./*.o start
