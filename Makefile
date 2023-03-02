CC=gcc
CFLAGS=-Wall -Werror -pedantic -std=gnu17 -fsanitize=address -g -lm
LDLIBS=-lcriterion
OBJS=my_parser.o my_calc.o builtins.o errors.o

all: ${OBJS}

test: test.o ${OBJS}
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

compiler: compiler.o ${OBJS}
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

ref: test.o ref_${OBJS}
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) ${OBJS} ref_$(OBJS)

.PHONY: all test ref compiler
