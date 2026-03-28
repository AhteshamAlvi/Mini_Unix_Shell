CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic
SRC     = d8sh.c parser.c lexer.c executor.c command.c
OBJ     = $(SRC:.c=.o)
TARGET  = d8sh

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

test: $(TARGET)
	bash tests/run_tests.sh

.PHONY: all clean test
