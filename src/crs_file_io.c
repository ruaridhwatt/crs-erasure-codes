#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "crs_file_io.h"

/**
 * Fills the size pointer with the size of the file at filePath
 * @param filePath The path to the file
 * @param size Where the file size should be stored
 * @return 0 on success, otherwise -1 (if the file does not exist)
 */
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

/**
 * Converts the file to a data matrix given the specification, spec
 * @param src The path to the file
 * @param spec The specification to use
 * @return The resulting data matrix, or NULL if unsuccessful
 */
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

/**
 * Writes the data in the data and coding matrices to files in the dest directory.
 * @param data The data matrix
 * @param coding The coding matrix
 * @param spec The encoding specification
 * @param dest The destination directory
 * @return 0 if successful, otherwise -1
 */
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
	for (i = 0; i < spec->k - 1; i++) {
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
	snprintf(filePath, pathLen, "%s/d%d", dest, i + 1);
	res = write_binary_bytes(data[i], spec->width - spec->endPadding, filePath);
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

/**
 * Writes nrBytes of data to a file at filePath
 * @param data The data to write
 * @param nrBytes The number of bytes to write
 * @param filePath The file path to write to
 * @return 0 if successful, otherwise -1
 */
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

/**
 * Reads maxNr of files from the src directory which begin with the given prefix character followed by a number.
 * The present array is updated with 1s for each file found. Used to read the data and coding files d1-d<k> and c1-c<m>.
 * @param src The source directory
 * @param maxNr The maximum number of files to read (k for data files, m for coding files)
 * @param fileSize The expected size of the files (the width of the matrix)
 * @param prefix The character prefix (d for data files and c for coding files)
 * @param present The empty binary array (filled with 0 for missing, 1 for present)
 * @return A matrix of maxNr rows and fileSize columns containing the files read
 */
char **read_files(char *src, int maxNr, size_t fileSize, char prefix, int *present) {
	char **data;
	DIR *d;
	struct dirent *dent;
	FILE* f;
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
			res = fread(data[fileNr - 1], sizeof(char), fileSize, f);
			if (res == 0) {
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

/**
 * Repairs the data and coding matrices with the specified erasures given the encoding spec. Writes the repaired files
 * to the src directory.
 * @param src The directory of files to be repaired
 * @param data The data matrix
 * @param coding The coding matrix
 * @param spec The encoding spec
 * @param A NULL terminates array of missing file indices (0 = d1, 1 = d2, ..., k = c1, ..., k+m-1 = cm)
 * @return 0 if successful, otherwise -1
 */
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
		if (row < spec->k - 1) {
			snprintf(filePath, pathLen, "%s/d%d", src, row + 1);
			res = write_binary_bytes(data[row], spec->width, filePath);
		} else if (row == spec->k - 1) {
			snprintf(filePath, pathLen, "%s/d%d", src, row + 1);
			res = write_binary_bytes(data[row], spec->width - spec->endPadding, filePath);
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

/**
 * @param rows The number of rows to allocate
 * @param columns The size of each row
 * @return the empty byte matrix
 */
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

/**
 * Frees the byte matrix, matrix, consisting of 'rows' rows.
 * @param matrix
 * @param rows
 */
void matrix_free(char **matrix, int rows) {
	int i;
	for (i = 0; i < rows; i++) {
		free(matrix[i]);
	}
	free(matrix);
}

/**
 * Converts a null terminated array of characters to an integer.
 * @param str The array of characters to be read.
 * @param i A pointer specifying where the conversion should be stored.
 * @return True if the conversion was successful, otherwise false.
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

