/*
 * crs_encode.h
 *
 *  Created on: 9 Dec 2015
 *      Author: dv12rwt
 */

#ifndef CRS_ENCODE_H_
#define CRS_ENCODE_H_

#define MAX_FILENAME_LENGTH 5 /* d1, ..., d9999 && c1, ..., c9999 */

struct encoding_spec {
	int k; /* nrData (required) */
	int m; /* nrCode (required) */

	/* Following are set on encode */
	int w;
	size_t width; /* in bytes */
	size_t endPadding; /* in bytes */
	int *bitmatrix;
};

int encode(char *src, char *dest, struct encoding_spec *spec);

int fill_encoding_spec(struct encoding_spec *spec, size_t filesize);

int calc_padding(size_t filesize, struct encoding_spec *spec);

int calc_min_w(struct encoding_spec *spec);

char **read_file(FILE *f, struct encoding_spec *spec);

char **create_coding_matrix(struct encoding_spec *spec);

int write_files(char **data, char **coding, struct encoding_spec *spec, char *dest);

void free_data_matrix(char **data, struct encoding_spec *spec);

void free_coding_matrix(char **coding, struct encoding_spec *spec);


#endif /* CRS_ENCODE_H_ */
