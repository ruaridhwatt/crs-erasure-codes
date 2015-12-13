/*
 * crs_file_io.h
 *
 *  Created on: 13 Dec 2015
 *      Author: dv12rwt
 */

#ifndef CRS_FILE_IO_H_
#define CRS_FILE_IO_H_

#include <stdio.h>
#include "crs_spec_io.h"


#define MAX_FILENAME_LENGTH 5 /* d1, ..., d9999 && c1, ..., c9999 */

int get_file_size(char *filePath, size_t *size);

char **file2data_matrix(char *src, struct crs_encoding_spec *spec);

char **calloc_matrix(int rows, int columns);

void matrix_free(char **matrix, int rows);

char **read_files(char *src, int maxNr, size_t fileSize, char prefix, int *present);

int repair_files(char *src, char **data, char **coding, struct crs_encoding_spec *spec, int *erasures);

int write_files(char **data, char **coding, struct crs_encoding_spec *spec, char *dest);

int write_binary_bytes(void *data, size_t nrBytes, char *filePath);

int str2int(char *str, int *i);


#endif /* CRS_FILE_IO_H_ */
