OBJECTS=$(C_SOURCES:%.c=%.o)

CC=clang
CFLAGS+=-Ofast
CFLAGS+=-Wall
CFLAGS+=-Werror
CFLAGS+=-Wextra
CFLAGS+=-flto
CFLAGS+=-fno-asynchronous-unwind-tables
CFLAGS+=-fno-exceptions
CFLAGS+=-fno-plt
CFLAGS+=-fno-rtti
CFLAGS+=-fno-stack-protector
CFLAGS+=-fno-unwind-tables
CFLAGS+=-fno-use-cxa-atexit
CFLAGS+=-pedantic
CFLAGS+=-std=c2x

LD=clang
LDFLAGS+=-Wl,-s
LDFLAGS+=-fuse-ld=lld

C_SOURCES=$(wildcard *.c)

TARGETS+=fntt64

all: $(TARGETS)

clean:
	$(RM) $(OBJECTS) $(TARGETS)

test: $(TARGETS)
	@./fntt64 $(shell for i in $(shell seq 126); do head -c8 < /dev/urandom | od -t uL | xargs | cut -d' ' -f2; done | xargs)

.PHONY: all clean test

.c.o:
	$(CC) $(sort $(CFLAGS)) -c $< -o $(<:%.c=%.o)

fntt64: mod_p.o ntt.o
	$(LD) $(sort $(LDFLAGS)) -o $@ $^
