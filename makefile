CC=gcc
CFLAGS=-c -Wall -pedantic -I/usr/include/jerasure
LDFLAGS=-lJerasure

PROGRAM_NAME=crs_erasure_server
SRC_DIR=src
BIN_DIR=bin

all: $(PROGRAM_NAME)

$(PROGRAM_NAME):
	mkdir -p $(BIN_DIR)
	cd $(SRC_DIR) && $(MAKE)

clean:
	rm -R $(BIN_DIR)