CC = gcc
CFLAGS = -Wall -Werror
DFLAGS = -Wall -Werror -g -DCSE320
BIN = utfconverter debug

SRC = $(wildcard *.c)

all: $(BIN)

utfconverter: $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

debug:
	$(CC) $(DFLAGS) utfconverter.c -o $@

clean:
	rm -f *.o $(BIN)
