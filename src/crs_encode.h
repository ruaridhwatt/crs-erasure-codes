/*
 * crs_encode.h
 *
 *  Created on: 9 Dec 2015
 *      Author: dv12rwt
 */

#ifndef CRS_ENCODE_H_
#define CRS_ENCODE_H_

#include "crs_spec_io.h"

int encode(char *src, char *dest, struct crs_encoding_spec *spec);

int decode(char *src, struct crs_encoding_spec *spec);

int fill_encoding_spec(struct crs_encoding_spec *spec, size_t filesize);

int calc_padding(size_t filesize, struct crs_encoding_spec *spec);

int calc_min_w(struct crs_encoding_spec *spec);

void print_usage(char *progName);

#endif /* CRS_ENCODE_H_ */
