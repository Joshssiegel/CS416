all: test

test:
	gcc test.c -L../ -lmy_vm -lpthread -m32 -lm -std=c99 -o test

clean:
	rm -rf test
