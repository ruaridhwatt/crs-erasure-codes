OUT=crs_erasure_codes
COMPILER=gcc
FLAGS=-Wall -pedantic -I/usr/include/jerasure/
LIBS=-lJerasure

BIN_DIR=../bin

all: $(OUT)

$(OUT): crs_encode.c crs_encode.h
	$(COMPILER) $(FLAGS) $(LIBS) crs_encode.c -o $(BIN_DIR)/$(OUT)