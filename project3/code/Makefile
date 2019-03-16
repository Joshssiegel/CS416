CC = gcc
CFLAGS = -g -c -m32
# CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
AR = ar -rc
RANLIB = ranlib

all: my_vm.a

my_vm.a: my_vm.o
	$(AR) libmy_vm.a my_vm.o
	$(RANLIB) libmy_vm.a

my_vm.o: my_vm.h

	$(CC)	$(CFLAGS)  my_vm.c

clean:
	rm -rf *.o *.a