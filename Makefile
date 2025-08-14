CFLAGS	= -o out -Wall -Wextra -Werror -lpng -lcjson -lm -I../include
MAIN	= a.c
CC	= clang

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
	clang-tidy $(MAIN) -- -I. -I../include

r:
	./out
