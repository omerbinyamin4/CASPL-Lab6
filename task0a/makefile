exec: c-libs
	gcc -g -m32 -o mypipe mypipe.o

c-libs: mypipe.c
	gcc -g -m32 -c -o mypipe.o mypipe.c
	
.PHONY: clean
clean:
	rm -rf ./*.o mypipe
