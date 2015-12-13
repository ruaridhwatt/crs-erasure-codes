/*
 * crs_file_io.c
 *
 *  Created on: 13 Dec 2015
 *      Author: dv12rwt
 */
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "crs_file_io.h"

int get_file_size(char *filePath, size_t *size) {
	FILE *f;
	struct stat fileStats;

	f = fopen(filePath, "rb");
	if (f == NULL) {
		return -1;
	}

	if (fstat(fileno(f), &fileStats) < 0) {
		fclose(f);
		return -1;
	}
	fclose(f);
	*size = fileStats.st_size;
	return 0;
}

char **file2data_matrix(char *src, struct crs_encoding_spec *spec) {
	FILE *f;
	int i, res;
	char **data;
	size_t bytesRead;

	f = fopen(src, "rb");
	if (f == NULL) {
		return NULL;
	}

	/* Create data matrix */
	data = calloc_matrix(spec->k, spec->width);
	if (data == NULL) {
		return NULL;
	}

	/* Fill data matrix according to specs */
	res = 0;
	for (i = 0; i < spec->k; i++) {
		bytesRead = fread(data[i], sizeof(char), spec->width, f);
		if (bytesRead == 0) {
			res = -1;
			break;
		}
	}
	fclose(f);

	if (res < 0) {
		matrix_free(data, spec->k);
		data = NULL;
	}
	return data;
}

int write_files(char **data, char **coding, struct crs_encoding_spec *spec, char *dest) {
	int i, res;
	size_t pathLen;
	char *filePath;

	/* Create output directory */
	res = mkdir(dest, S_IRWXU | S_IRWXG);
	if (res < 0) {
		return -1;
	}

	pathLen = strlen(dest) + MAX_FILENAME_LENGTH + 2;
	filePath = (char *) calloc(pathLen, sizeof(char));
	if (filePath == NULL) {
		return -1;
	}

	/* Write spec file */
	snprintf(filePath, pathLen, "%s/spec", dest);
	res = write_spec(spec, filePath);
	if (res < 0) {
		free(filePath);
		return -1;
	}

	/* Write data files */
	for (i = 0; i < spec->k; i++) {
		snprintf(filePath, pathLen, "%s/d%d", dest, i + 1);
		res = write_binary_bytes(data[i], spec->width, filePath);
		if (res < 0) {
			break;
		}
	}
	if (res < 0) {
		free(filePath);
		return -1;
	}

	/* Write coding files */
	for (i = 0; i < spec->m; i++) {
		snprintf(filePath, pathLen, "%s/c%d", dest, i + 1);
		res = write_binary_bytes(coding[i], spec->width, filePath);
		if (res < 0) {
			break;
		}
	}
	free(filePath);
	return res;
}

int write_binary_bytes(void *data, size_t nrBytes, char *filePath) {
	FILE *f;
	size_t written;

	f = fopen(filePath, "wb");
	if (f == NULL) {
		return -1;
	}
	written = fwrite(data, sizeof(char), nrBytes, f);

	fclose(f);
	return (written == nrBytes) ? 0 : -1;
}

char **read_files(char *src, int maxNr, size_t fileSize, char prefix, int *present) {
	char **data;
	DIR *d;
	struct dirent *dent;
	FILE* f;
	struct stat fs;
	int res, fileNr;
	char *filePath;

	int pathLen = strlen(src) + MAX_FILENAME_LENGTH + 2;
	filePath = (char *) calloc(pathLen, sizeof(char));

	data = calloc_matrix(maxNr, fileSize);
	if (data == NULL) {
		return NULL;
	}

	d = opendir(src);
	if (d == NULL) {
		matrix_free(data, maxNr);
		return NULL;
	}

	while ((dent = readdir(d)) != NULL) {
		if (dent->d_name[0] == prefix) {
			res = str2int(&(dent->d_name[1]), &fileNr);
			if (res < 0) {
				break;
			}
			if (fileNr < 1 || fileNr > maxNr) {
				res = -1;
				break;
			}
			snprintf(filePath, pathLen, "%s/%s", src, dent->d_name);
			f = fopen(filePath, "rb");
			if (f == NULL) {
				res = -1;
				break;
			}
			res = fstat(fileno(f), &fs);
			if (res < 0 || fs.st_size != fileSize) {
				res = -1;
				fclose(f);
				break;
			}
			res = fread(data[fileNr - 1], sizeof(char), fileSize, f);
			if (res != fileSize) {
				res = -1;
				fclose(f);
				break;
			}
			fclose(f);
			present[fileNr - 1] = 1;
		}
	}
	closedir(d);
	free(filePath);
	if (res < 0) {
		matrix_free(data, maxNr);
		data = NULL;
	}
	return data;
}

int repair_files(char *src, char **data, char **coding, struct crs_encoding_spec *spec, int *erasures) {
	char *filePath;
	int row;
	int res = 0;
	int i = 0;

	int pathLen = strlen(src) + MAX_FILENAME_LENGTH + 2;
	filePath = (char *) calloc(pathLen, sizeof(char));
	if (filePath == NULL) {
		return -1;
	}

	while (erasures[i] != -1) {
		row = erasures[i];
		if (row < spec->k) {
			snprintf(filePath, pathLen, "%s/d%d", src, row + 1);
			res = write_binary_bytes(data[row], spec->width, filePath);
		} else {
			snprintf(filePath, pathLen, "%s/c%d", src, row - spec->k + 1);
			res = write_binary_bytes(coding[row - spec->k], spec->width, filePath);
		}
		fprintf(stdout, "\t%s\n", filePath);
		if (res < 0) {
			break;
		}
		i++;
	}
	free(filePath);
	return res;
}

char **calloc_matrix(int rows, int columns) {
	int i;
	int res = 0;
	char **matrix;

	matrix = (char **) malloc(rows * sizeof(char *));
	if (matrix == NULL) {
		return NULL;
	}
	for (i = 0; i < rows; i++) {
		matrix[i] = (char *) calloc(columns, sizeof(char));
		if (matrix[i] == NULL) {
			res = -1;
			break;
		}
	}
	if (res < 0) {
		/* Failed, rewind */
		for (i--; i >= 0; i--) {
			free(matrix[i]);
		}
		free(matrix);
		matrix = NULL;
	}
	return matrix;
}

void matrix_free(char **matrix, int rows) {
	int i;
	for (i = 0; i < rows; i++) {
		free(matrix[i]);
	}
	free(matrix);
}

/*
 * Converts a null terminated array of characters to an integer.
 *
 * str:     The array of characters to be read.
 *
 * i:       A pointer specifying where the conversion should be stored.
 *
 * Returns: True if the conversion was successful, otherwise false.
 *          A conversion is considered successful iff all the str characters
 *          were used in the conversion and the result was within the range an
 *          int can store.
 */
int str2int(char *str, int *i) {
	long l;
	char *pEnd;

	if (str == NULL) {
		return -1;
	}

	errno = 0;

	l = strtol(str, &pEnd, 10);
	if (pEnd == str || *pEnd != '\0' || errno == ERANGE) {
		return -1;
	}

	if (l > INT_MAX || l < INT_MIN) {
		errno = ERANGE;
		return -1;
	}
	*i = (int) l;
	return 0;
}

