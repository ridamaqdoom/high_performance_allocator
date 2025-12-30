all:
	gcc -O2 allocator.c test.c -o test

clean:
	rm -f test

