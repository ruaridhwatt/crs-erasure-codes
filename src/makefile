OUT=crs-erasure-codes
COMPILER=gcc
FLAGS=-g -Wall -pedantic -I/usr/include/jerasure/
LIBS=-lJerasure

BIN_DIR=../bin

all: $(OUT)

$(OUT): crs_encode.o crs_file_io.o crs_spec_io.o
	$(COMPILER) $(FLAGS) -o $(BIN_DIR)/$(OUT) $(BIN_DIR)/crs_encode.o $(BIN_DIR)/crs_spec_io.o $(BIN_DIR)/crs_file_io.o $(LIBS)

crs_encode.o: crs_encode.c crs_encode.h crs_spec_io.h
	$(COMPILER) $(FLAGS) -o $(BIN_DIR)/crs_encode.o crs_encode.c -c

crs_file_io.o: crs_spec_io.c crs_spec_io.h crs_spec_io.h
	$(COMPILER) $(FLAGS) -o $(BIN_DIR)/crs_file_io.o crs_file_io.c -c

crs_spec_io.o: crs_spec_io.c crs_spec_io.h
	$(COMPILER) $(FLAGS) -o $(BIN_DIR)/crs_spec_io.o crs_spec_io.c -c
	