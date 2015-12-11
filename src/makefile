OUT=crs_erasure_codes
COMPILER=gcc
FLAGS=-g -Wall -pedantic 
LIBS=-lJerasure -I/usr/include/jerasure/

BIN_DIR=../bin

all: $(OUT)

$(OUT): crs_encode.o crs_spec_io.o
	$(COMPILER) $(FLAGS) $(LIBS) -o $(BIN_DIR)/$(OUT) $(BIN_DIR)/crs_encode.o $(BIN_DIR)/crs_spec_io.o

crs_encode.o: crs_encode.c crs_encode.h crs_spec_io.h
	$(COMPILER) $(FLAGS) -o $(BIN_DIR)/crs_encode.o -c crs_encode.c

crs_spec_io.o: crs_spec_io.c crs_spec_io.h
	$(COMPILER) $(FLAGS) -o $(BIN_DIR)/crs_spec_io.o -c crs_spec_io.c
	