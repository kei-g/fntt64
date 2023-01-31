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
CFLAGS+=-isystem/usr/local/include
CFLAGS+=-pedantic
CFLAGS+=-std=c2x

COVFLAGS+=-fcoverage-mapping
COVFLAGS+=-fprofile-instr-generate

LD=clang
LDFLAGS+=-L/usr/local/lib
LDFLAGS+=-Wl,-s
LDFLAGS+=-fuse-ld=lld
LDFLAGS+=-lgmp

C_SOURCES=$(wildcard *.c)

TARGETS+=fntt64

all: $(TARGETS)

clean:
	$(RM) $(OBJECTS) $(TARGETS) *.cov *.profile *.v

coverage: fntt64.profile

test: $(TARGETS)
	@./fntt64 $(shell for i in $(shell seq 30); do head -c8 < /dev/urandom | hexdump -v -e '/1 "%02x"' | xargs; done | xargs)

.PHONY: all clean coverage test

.SUFFIXES: .c .cov .pd .pr .profile .v

.c.o:
	$(CC) $(sort $(CFLAGS)) -c $< -o $(<:%.c=%.o)

.c.v:
	$(CC) $(sort $(CFLAGS) $(COVFLAGS)) -c $< -o $(<:%.c=%.v)

.cov.pr:
	@LLVM_PROFILE_FILE=$(<:%.cov=%.pr) ./$< $(shell for i in $(shell seq 30); do head -c8 < /dev/urandom | hexdump -v -e '/1 "%02x"' | xargs; done | xargs)

.pd.profile:
	llvm-cov show ./$(<:%.pd=%.cov) -instr-profile=$< > $(<:%.pd=%.profile)

.pr.pd:
	llvm-profdata merge -sparse $< -o $(<:%.pr=%.pd)

fntt64: mod_p.o ntt.o
	$(LD) $(sort $(LDFLAGS)) -o $@ $^

fntt64.cov: mod_p.v ntt.v
	$(LD) $(sort $(COVFLAGS) $(LDFLAGS)) -o $@ $^
