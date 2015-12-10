/*
 * crs_spec_io.c
 *
 *  Created on: 10 Dec 2015
 *      Author: dv12rwt
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "crs_spec_io.h"

int read_spec(char *src, struct crs_encoding_spec *spec) {
	int i;
	FILE *f;
	char bitBuf;
	size_t nrRead;
	size_t bitmatrixSize;

	f = fopen(src, "rb");
	if (f == NULL) {
		return -1;
	}
	nrRead = fread(&(spec->k), sizeof(int), 1, f);
	if (nrRead != 1) {
		fclose(f);
		return -1;
	}

	nrRead = fread(&(spec->m), sizeof(int), 1, f);
	if (nrRead != 1) {
		fclose(f);
		return -1;
	}

	nrRead = fread(&(spec->w), sizeof(int), 1, f);
	if (nrRead != 1) {
		fclose(f);
		return -1;
	}

	nrRead = fread(&(spec->width), sizeof(size_t), 1, f);
	if (nrRead != 1) {
		fclose(f);
		return -1;
	}

	nrRead = fread(&(spec->endPadding), sizeof(size_t), 1, f);
	if (nrRead != 1) {
		fclose(f);
		return -1;
	}

	bitmatrixSize = spec->w * spec->m * spec->w * spec->k;
	spec->bitmatrix = (int *) malloc(bitmatrixSize * sizeof(int));
	for (i = 0; i < bitmatrixSize; i++) {
		nrRead = fread(&bitBuf, sizeof(char), 1, f);
		if (nrRead != 1) {
			fclose(f);
			free(spec->bitmatrix);
			return -1;
		}
		spec->bitmatrix[i] = bitBuf;
	}

	fclose(f);
	return 0;
}

int write_spec(struct crs_encoding_spec *spec, char *dest) {
	int i;
	FILE* f;
	size_t nrWritten;
	size_t bitmatrixSize = spec->w * spec->m * spec->w * spec->k;
	char bitToWrite;

	/* Write spec to disk */
	f = fopen(dest, "wb");
	if (f == NULL) {
		return -1;
	}
	nrWritten = fwrite(&(spec->k), sizeof(int), 1, f);
	if (nrWritten != 1) {
		fclose(f);
		return -1;
	}

	nrWritten = fwrite(&(spec->m), sizeof(int), 1, f);
	if (nrWritten != 1) {
		fclose(f);
		return -1;
	}

	nrWritten = fwrite(&(spec->w), sizeof(uint64_t), 1, f);
	if (nrWritten != 1) {
		fclose(f);
		return -1;
	}

	nrWritten = fwrite(&(spec->width), sizeof(uint64_t), 1, f);
	if (nrWritten != 1) {
		fclose(f);
		return -1;
	}

	nrWritten = fwrite(&(spec->endPadding), sizeof(uint64_t), 1, f);
	if (nrWritten != 1) {
		fclose(f);
		return -1;
	}

	for (i = 0; i < bitmatrixSize; i++) {
		bitToWrite = (char) spec->bitmatrix[i];
		nrWritten = fwrite(&bitToWrite, sizeof(char), 1, f);
		if (nrWritten != 1) {
			fclose(f);
			return -1;
		}
	}
	fclose(f);
	return 0;
}
