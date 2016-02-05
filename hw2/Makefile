CC = gcc
CFLAGS = 
BIN = utfconverter

SRC = $(wildcard *.c)

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean run

clean:
	rm -f *.o $(BIN)

stupid: $(SRC)
	gcc -g $^ -o $(BIN)
run: $(BIN)
	./$(BIN)
