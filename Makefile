CFLAGS	= -o out -Wall -Wextra -Werror -lpng -lcjson -I../include
MAIN	= a.c
CC	= gcc

all: rel r

dbg: d
	gdb ./out

bin: rel
	doas cp -i out /usr/bin/$(name)

rel:
	$(CC) -O3 $(MAIN) $(CFLAGS)
d:
	$(CC) -g $(MAIN) $(CFLAGS)
mac:
	$(CC) -dM -E $(MAIN)
sca:
	scan-build -v -V make rel
	clang-tidy $(MAIN) -- -I.

r:
	./out
