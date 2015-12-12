/*
 * crs_encode.h
 *
 *  Created on: 9 Dec 2015
 *      Author: dv12rwt
 */

#ifndef CRS_ENCODE_H_
#define CRS_ENCODE_H_

#include <stdio.h>
#include "crs_spec_io.h"
#define MAX_FILENAME_LENGTH 5 /* d1, ..., d9999 && c1, ..., c9999 */

int encode(char *src, char *dest, struct crs_encoding_spec *spec);

int decode(char *src, struct crs_encoding_spec *spec);

int repair_files(char *src, char **data, char **coding, struct crs_encoding_spec *spec, int *erasures);

int fill_encoding_spec(struct crs_encoding_spec *spec, size_t filesize);

int calc_padding(size_t filesize, struct crs_encoding_spec *spec);

int calc_min_w(struct crs_encoding_spec *spec);

char **file2data_matrix(FILE *f, struct crs_encoding_spec *spec);

char **read_files(char *src, int maxNr, size_t fileSize, char firstChar, int *present);

char **calloc_matrix(int rows, int columns);

int write_files(char **data, char **coding, struct crs_encoding_spec *spec, char *dest);

int write_file(char **matrix, int row, size_t width, char *filePath);

void free_matrix(char **matrix, int rows);

int str2int(char *str, int *i);

void print_usage(char *progName);

#endif /* CRS_ENCODE_H_ */
