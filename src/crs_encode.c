#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <jerasure.h>
#include <cauchy.h>
#include <errno.h>
#include <dirent.h>
#include "crs_encode.h"

int main(int argc, char **argv) {
	int c, res, i;
	int mode = -1;
	char *src = NULL;
	char *dest = NULL;
	struct crs_encoding_spec spec;

	spec.k = 0;
	spec.m = 0;

	while ((c = getopt(argc, argv, "edk:m:")) != -1)
		switch (c) {
		case 'e':
			if (mode == -1) {
				mode = 1;
			} else {
				print_usage(argv[0]);
				exit(1);
			}
			break;
		case 'd':
			if (mode == -1) {
				mode = 0;
			} else {
				print_usage(argv[0]);
				exit(1);
			}
			break;
		case 'k':
			res = str2int(optarg, &(spec.k));
			if (res < 0) {
				print_usage(argv[0]);
				exit(1);
			}
			break;
		case 'm':
			res = str2int(optarg, &(spec.m));
			if (res < 0) {
				print_usage(argv[0]);
				exit(1);
			}
			break;
		case '?':
			print_usage(argv[0]);
			exit(1);
		default:
			abort();
			break;
		}

	for (i = optind; i < argc; i++) {
		if (i == optind) {
			src = argv[i];
		} else if (i == optind + 1) {
			dest = argv[i];
		} else {
			print_usage(argv[0]);
			exit(1);
		}
	}

	switch (mode) {
	case 0:
		if (src == NULL) {
			print_usage(argv[0]);
			res = -1;
		} else {
			res = decode(src, &spec);
		}
		break;
	case 1:
		if (src == NULL || dest == NULL) {
			print_usage(argv[0]);
			res = -1;
		} else {
			res = encode(src, dest, &spec);
		}
		break;
	default:
		print_usage(argv[0]);
		res = -1;
		break;
	}
	if (res == 0) {
		printf("Success!\n");
		fprintf(stdout, "Padding=%d bytes\n", (int) spec.endPadding);
		fprintf(stdout, "Word size, w=%d \n", spec.w);
		fprintf(stdout, "File size=%d \n", (int) spec.width);
	}
	return res;
}

int encode(char *src, char *dest, struct crs_encoding_spec *spec) {

	int res = 0;
	FILE *srcfile;
	struct stat srcStat;
	char **data = NULL;
	char **coding = NULL;
	int *matrix;
	int **schedule;

	/* Check args are reasonable */
	if (spec->k <= 0 || spec->k >= 9999 || spec->m <= 0 || spec->m > spec->k) {
		fprintf(stderr, "Error: Unsuitable arguments used for crs_encode.encode\n");
		return -1;
	}

	/* Open input file */
	srcfile = fopen(src, "r");
	if (srcfile == NULL) {
		fprintf(stderr, "Could not open %s for reading\n%s\n", src, strerror(errno));
		fclose(srcfile);
		return -1;
	}

	/* Get input file stats */
	if (fstat(fileno(srcfile), &srcStat) < 0) {
		fprintf(stderr, "Could not get file info for %s\n%s\n", src, strerror(errno));
		fclose(srcfile);
		return -1;
	}

	/* Create output directory */
	res = mkdir(dest, S_IRWXU | S_IRWXG);
	if (res < 0) {
		fprintf(stderr, "Could not create directory %s\n%s\n", dest, strerror(errno));
		fclose(srcfile);
		return -1;
	}

	/* Calculate encoding specs */
	res = fill_encoding_spec(spec, srcStat.st_size);
	if (res < 0) {
		fprintf(stderr, "Error: Could not fill encoding spec in crs_encode.encode\n");
		fclose(srcfile);
		return -1;
	}

	/* Read data from file */
	data = file2data_matrix(srcfile, spec);
	if (data == NULL) {
		fprintf(stderr, "Could not create data matrix from input file\n%s\n", strerror(errno));
		return -1;
	}
	fclose(srcfile);

	/* Alloc coding matrix */
	coding = calloc_matrix(spec->m, spec->width);
	if (coding == NULL) {
		fprintf(stderr, "Could not create coding matrix\n%s\n", strerror(errno));
		free_matrix(data, spec->k);
		return -1;
	}

	/* Create bitmatrix */
	matrix = cauchy_good_general_coding_matrix(spec->k, spec->m, spec->w);
	if (matrix == NULL) {
		fprintf(stderr, "Could not create cauchy matrix\n%s\n", strerror(errno));
		free_matrix(data, spec->k);
		free_matrix(coding, spec->m);
		return -1;
	}
	spec->bitmatrix = jerasure_matrix_to_bitmatrix(spec->k, spec->m, spec->w, matrix);
	if (spec->bitmatrix == NULL) {
		fprintf(stderr, "Could not create bitmatrix\n%s\n", strerror(errno));
		free_matrix(data, spec->k);
		free_matrix(coding, spec->m);
		return -1;
	}
	free(matrix);

	/* Encode using schedule */
	schedule = jerasure_smart_bitmatrix_to_schedule(spec->k, spec->m, spec->w, spec->bitmatrix);
	if (schedule == NULL) {
		fprintf(stderr, "Could not create schedule from bitmatrix\n%s\n", strerror(errno));
		free(spec->bitmatrix);
		free_matrix(data, spec->k);
		free_matrix(coding, spec->m);
		return -1;
	}
	jerasure_schedule_encode(spec->k, spec->m, spec->w, schedule, data, coding, spec->width, (spec->width / spec->w));
	jerasure_free_schedule(schedule);

	res = write_files(data, coding, spec, dest);

	free(spec->bitmatrix);
	free_matrix(data, spec->k);
	free_matrix(coding, spec->m);

	return res;
}

int decode(char *src, struct crs_encoding_spec *spec) {
	int res, i, j;
	char *filePath;
	char **data;
	char **coding;
	int *present;
	int *erasures;

	int pathLen = strlen(src) + MAX_FILENAME_LENGTH + 2;
	filePath = (char *) calloc(pathLen, sizeof(char));

	/* Read spec file */
	snprintf(filePath, pathLen, "%s/spec", src);
	res = read_spec(filePath, spec);
	if (res < 0) {
		fprintf(stderr, "Could not read spec file\n%s", strerror(errno));
		free(filePath);
		return -1;
	}
	free(filePath);

	present = (int *) calloc(spec->k, sizeof(int));
	erasures = (int *) malloc((spec->k + spec->m) * sizeof(int));

	data = read_files(src, spec->k, spec->width, 'd', present);
	if (data == NULL) {
		return -1;
	}
	j = 0;
	for (i = 0; i < spec->k; i++) {
		if (present[i] == 0) {
			/* Erased */
			erasures[j] = i;
			j++;
		} else {
			/* Reset */
			present[i] = 0;
		}
	}

	coding = read_files(src, spec->m, spec->width, 'c', present);
	if (coding == NULL) {
		free_matrix(data, spec->k);
		return -1;
	}
	for (i = 0; i < spec->m; i++) {
		if (present[i] == 0) {
			/* Erased */
			erasures[j] = i + spec->k;
			j++;
		}
	}
	erasures[j] = -1;
	free(present);

	jerasure_schedule_decode_lazy(spec->k, spec->m, spec->w, spec->bitmatrix, erasures, data, coding, spec->width, (spec->width / spec->w), 1);
	/* TODO write back repaired erasures */

	free(erasures);
	free_matrix(data, spec->k);
	free_matrix(coding, spec->m);
	free(spec->bitmatrix);
	return 0;
}

char **read_files(char *src, int maxNr, size_t fileSize, char firstChar, int *present) {
	char **data;
	DIR *d;
	struct dirent *dent;
	FILE* f;
	struct stat fs;
	int res, row;
	char *filePath;

	int pathLen = strlen(src) + MAX_FILENAME_LENGTH + 2;
	filePath = (char *) calloc(pathLen, sizeof(char));

	d = opendir(src);
	if (d == NULL) {
		return NULL;
	}

	data = calloc_matrix(maxNr, fileSize);
	if (data == NULL) {
		closedir(d);
		return NULL;
	}
	while ((dent = readdir(d)) != NULL) {
		if (dent->d_name[0] == firstChar) {
			res = str2int(&(dent->d_name[1]), &row);
			if (res < 0 || row >= maxNr) {
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
			res = fread(data[row - 1], sizeof(char), fileSize, f);
			if (res != fileSize) {
				res = -1;
				fclose(f);
				break;
			}
			fclose(f);
			present[row - 1] = 1;
		}
	}
	closedir(d);
	free(filePath);
	if (res < 0) {
		free_matrix(data, maxNr);
		data = NULL;
	}
	return data;
}

int fill_encoding_spec(struct crs_encoding_spec *spec, size_t filesize) {

	int res;

	/* Set endPadding and set width */
	res = calc_padding(filesize, spec);
	if (res < 0) {
		return -1;
	}
	spec->width = (filesize + spec->endPadding) / spec->k;

	/* Calculate minimum word size  */
	res = calc_min_w(spec);
	if (res < 0) {
		return -1;
	}
	return 0;
}

int calc_padding(size_t filesize, struct crs_encoding_spec *spec) {
	int div = spec->k * sizeof(long);
	spec->endPadding = div - (filesize % div);
	if ((filesize + spec->endPadding) % sizeof(long) != 0) {
		return -1;
	}
	return 0;
}

int calc_min_w(struct crs_encoding_spec *spec) {
	int n = 4;
	spec->w = 2;
	while (n < spec->k + spec->m || spec->width % spec->w != 0) {
		n <<= 1;
		spec->w++;
	}
	if (spec->w > 32) {
		return -1;
	}
	return 0;
}

char **file2data_matrix(FILE *f, struct crs_encoding_spec *spec) {
	int i;
	int res = 0;
	char **data;
	size_t bytesRead;

	/* Create data matrix */
	data = (char **) malloc(spec->k * sizeof(char *));
	if (data == NULL) {
		return NULL;
	}
	for (i = 0; i < spec->k; i++) {
		data[i] = (char *) calloc(spec->width, sizeof(char));
		if (data[i] == NULL) {
			res = -1;
			break;
		}
		bytesRead = fread(data[i], sizeof(char), spec->width, f);
		if (bytesRead == 0) {
			res = -1;
			break;
		}
	}
	if (res < 0) {
		/* Failed, rewind */
		for (i--; i >= 0; i--) {
			free(data[i]);
		}
		free(data);
		data = NULL;
	}
	return data;
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

int write_files(char **data, char **coding, struct crs_encoding_spec *spec, char *dest) {
	int i;
	FILE* f;
	int res = 0;
	size_t nrWritten;

	int pathLen = strlen(dest) + MAX_FILENAME_LENGTH + 2;

	char *filePath = (char *) calloc(pathLen, sizeof(char));
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
		f = fopen(filePath, "wb");
		if (f == NULL) {
			res = -1;
			break;
		}
		nrWritten = fwrite((void *) data[i], sizeof(char), spec->width, f);
		if (nrWritten != spec->width) {
			res = -1;
			fclose(f);
			break;
		}
		fclose(f);
	}
	if (res < 0) {
		free(filePath);
		return -1;
	}

	/* Write coding files */
	for (i = 0; i < spec->m; i++) {
		snprintf(filePath, pathLen, "%s/c%d", dest, i + 1);
		f = fopen(filePath, "wb");
		if (f == NULL) {
			res = -1;
			break;
		}
		nrWritten = fwrite((void *) coding[i], sizeof(char), spec->width, f);
		if (nrWritten != spec->width) {
			res = -1;
			fclose(f);
			break;
		}
		fclose(f);
	}

	free(filePath);
	return res;
}

void free_matrix(char **matrix, int rows) {
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

void print_usage(char *progName) {
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "\t%s [options] [src] [dest]\n", progName);
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-e\t encode\n");
	fprintf(stdout, "\t-d\t decode (when decoding only the source folder is required)\n");
	fprintf(stdout, "\t-k\t the number of data files (when encoding only)\n");
	fprintf(stdout, "\t-m\t the number of coding files (when encoding only)\n");
}

