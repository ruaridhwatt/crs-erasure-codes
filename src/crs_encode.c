#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <jerasure.h>
#include <cauchy.h>
#include <errno.h>
#include <unistd.h>
#include "crs_encode.h"
#include "crs_file_io.h"
#include "crs_spec_io.h"

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
				return -1;
			}
			break;
		case 'd':
			if (mode == -1) {
				mode = 0;
			} else {
				print_usage(argv[0]);
				return -1;
			}
			break;
		case 'k':
			res = str2int(optarg, &(spec.k));
			if (res < 0) {
				print_usage(argv[0]);
				return -1;
			}
			break;
		case 'm':
			res = str2int(optarg, &(spec.m));
			if (res < 0) {
				print_usage(argv[0]);
				return -1;
			}
			break;
		case '?':
			print_usage(argv[0]);
			return -1;
		default:
			return -1;
			break;
		}

	for (i = optind; i < argc; i++) {
		if (i == optind) {
			src = argv[i];
		} else if (i == optind + 1) {
			dest = argv[i];
		} else {
			print_usage(argv[0]);
			return -1;
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
	}
	return res;
}

int encode(char *src, char *dest, struct crs_encoding_spec *spec) {

	int res = 0;
	size_t fileSize;
	char **data = NULL;
	char **coding = NULL;
	int *matrix;
	int **schedule;

	/* Check args are reasonable */
	if (spec->k <= 0 || spec->k >= 9999 || spec->m <= 0 || spec->m > spec->k) {
		fprintf(stderr, "Error: Unsuitable arguments used for crs_encode.encode\n");
		return -1;
	}

	/* Calculate encoding specs */
	res = get_file_size(src, &fileSize);
	if (res < 0) {
		fprintf(stderr, "Could get size of file: %s\n%s\n", src, strerror(errno));
		return -1;
	}
	res = fill_encoding_spec(spec, fileSize);
	if (res < 0) {
		fprintf(stderr, "Error: Could not calculate encoding specs\n");
		return -1;
	}

	/* Read data from file */
	data = file2data_matrix(src, spec);
	if (data == NULL) {
		fprintf(stderr, "Could not create data matrix from input file\n%s\n", strerror(errno));
		return -1;
	}

	/* Alloc coding matrix */
	coding = calloc_matrix(spec->m, spec->width);
	if (coding == NULL) {
		fprintf(stderr, "Could not create coding matrix\n%s\n", strerror(errno));
		matrix_free(data, spec->k);
		return -1;
	}

	/* Create bitmatrix */
	matrix = cauchy_good_general_coding_matrix(spec->k, spec->m, spec->w);
	if (matrix == NULL) {
		fprintf(stderr, "Could not create cauchy matrix\n%s\n", strerror(errno));
		matrix_free(data, spec->k);
		matrix_free(coding, spec->m);
		return -1;
	}
	spec->bitmatrix = jerasure_matrix_to_bitmatrix(spec->k, spec->m, spec->w, matrix);
	if (spec->bitmatrix == NULL) {
		fprintf(stderr, "Could not create bitmatrix\n%s\n", strerror(errno));
		free(matrix);
		matrix_free(data, spec->k);
		matrix_free(coding, spec->m);
		return -1;
	}
	free(matrix);

	/* Encode using schedule */
	schedule = jerasure_smart_bitmatrix_to_schedule(spec->k, spec->m, spec->w, spec->bitmatrix);
	if (schedule == NULL) {
		fprintf(stderr, "Could not create schedule from bitmatrix\n%s\n", strerror(errno));
		free(spec->bitmatrix);
		matrix_free(data, spec->k);
		matrix_free(coding, spec->m);
		return -1;
	}
	jerasure_schedule_encode(spec->k, spec->m, spec->w, schedule, data, coding, spec->width, (spec->width / spec->w));
	jerasure_free_schedule(schedule);

	res = write_files(data, coding, spec, dest);
	if (res < 0) {
		fprintf(stderr, "Could not write encoded files\n%s", strerror(errno));
	}

	free(spec->bitmatrix);
	matrix_free(data, spec->k);
	matrix_free(coding, spec->m);

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
	free(filePath);
	if (res < 0) {
		fprintf(stderr, "Could not read spec file\n%s", strerror(errno));
		return -1;
	}

	present = (int *) calloc(spec->k, sizeof(int));
	if (present == NULL) {
		return -1;
	}
	erasures = (int *) malloc((spec->k + spec->m + 1) * sizeof(int));
	if (erasures == NULL) {
		free(present);
		return -1;
	}

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
		matrix_free(data, spec->k);
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

	if (erasures[0] == -1) {
		fprintf(stdout, "Nothing to do!\n");
	} else {
		res = jerasure_schedule_decode_lazy(spec->k, spec->m, spec->w, spec->bitmatrix, erasures, data, coding, spec->width, (spec->width / spec->w),
				1);

		if (res == 0) {
			fprintf(stdout, "Repairing files...\n");
			res = repair_files(src, data, coding, spec, erasures);
		}
	}

	free(erasures);
	matrix_free(data, spec->k);
	matrix_free(coding, spec->m);
	free(spec->bitmatrix);
	return res;
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

void print_usage(char *progName) {
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "\t%s [options] [src] [dest]\n", progName);
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-e\t encode\n");
	fprintf(stdout, "\t-d\t decode (when decoding only the source folder is required)\n");
	fprintf(stdout, "\t-k\t the number of data files (when encoding only)\n");
	fprintf(stdout, "\t-m\t the number of coding files (when encoding only)\n");
}

