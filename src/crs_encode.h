/*
 * crs_encode.h
 *
 *  Created on: 9 Dec 2015
 *      Author: dv12rwt
 */

#ifndef CRS_ENCODE_H_
#define CRS_ENCODE_H_

#include "crs_spec_io.h"
#define MAX_FILENAME_LENGTH 5 /* d1, ..., d9999 && c1, ..., c9999 */

int encode(char *src, char *dest, struct crs_encoding_spec *spec);

int decode(char *src, struct crs_encoding_spec *spec);

int fill_encoding_spec(struct crs_encoding_spec *spec, size_t filesize);

int calc_padding(size_t filesize, struct crs_encoding_spec *spec);

int calc_min_w(struct crs_encoding_spec *spec);

char **file2data_matrix(FILE *f, struct crs_encoding_spec *spec);

char **alloc_coding_matrix(struct crs_encoding_spec *spec);

int write_files(char **data, char **coding, struct crs_encoding_spec *spec, char *dest);

void free_data_matrix(char **data, struct crs_encoding_spec *spec);

void free_coding_matrix(char **coding, struct crs_encoding_spec *spec);


#endif /* CRS_ENCODE_H_ */
