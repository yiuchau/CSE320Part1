CC = gcc
CFLAGS = -Wall -Werror -DDEBUG
BIN = utfconverter

SRC = $(wildcard *.c)

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o $(BIN)

