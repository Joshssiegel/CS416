CC = gcc
CFLAGS = -g

all:
	$(CC) $(CFLAGS) -o test_case test_case.c;
	$(CC) $(CFLAGS) -o simple_test simple_test_orig.c

warmup:
	$(CC) $(CFLAGS) -o simple_test simple_test_orig.c

clean:
	rm -rf simple_test
	rm -rf test_case
