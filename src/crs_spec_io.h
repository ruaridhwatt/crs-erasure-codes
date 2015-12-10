/*
 * crs_spec_io.h
 *
 *  Created on: 10 Dec 2015
 *      Author: dv12rwt
 */

#ifndef SRC_CRS_SPEC_IO_H_
#define SRC_CRS_SPEC_IO_H_

struct crs_encoding_spec {

	int k; /* nrData (required) */
	int m; /* nrCode (required) */

	/* Following are set on encode */
	int w;
	size_t width; /* in bytes */
	size_t endPadding; /* in bytes */
	int *bitmatrix;
};

int read_spec(char *src, struct crs_encoding_spec *spec);

int write_spec(struct crs_encoding_spec *spec, char *dest);

#endif /* SRC_CRS_SPEC_IO_H_ */
