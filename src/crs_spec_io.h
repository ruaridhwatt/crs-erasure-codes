#ifndef SRC_CRS_SPEC_IO_H_
#define SRC_CRS_SPEC_IO_H_

/**
 * The encoding specification
 */
struct crs_encoding_spec {

	int k; /* nrDataFiles (required) */
	int m; /* nrCodeFiles (required) */

	/* Following are set on encode */
	int w;
	size_t width; /* in bytes */
	size_t endPadding; /* in bytes */
	int *bitmatrix;
};

/**
 * Reads the spec file at src to spec
 * @param src The spec file path
 * @param spec Where the spec file should be read into
 * @return 0 if successful, otherwise -1
 */
int read_spec(char *src, struct crs_encoding_spec *spec);

/**
 * Writes the encoding specification to file.
 * @param spec The spec struct
 * @param dest The file destination
 * @return 0 if successful, otherwise -1
 */
int write_spec(struct crs_encoding_spec *spec, char *dest);

#endif /* SRC_CRS_SPEC_IO_H_ */
