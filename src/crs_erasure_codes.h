#ifndef CRS_ERASURE_CODES_H_
#define CRS_ERASURE_CODES_H_

#include "crs_spec_io.h"

/**
 * Encodes the file at src to dest directory. Also writes the spec to a file in dest. Spec k and m values must be
 * initialised, The rest will be filled.
 * @param src The file to encode
 * @param dest The directory to create and fill with the data, coding and spec files.
 * @param spec The spec (k and m) to be used in encoding
 * @return 0 if successful, otherwise -1
 */
int encode(char *src, char *dest, struct crs_encoding_spec *spec);

/**
 * Decodes (repairs) the file set in the src directory using the specified spec.
 * @param src The directory containing the coding, data and spec files.
 * @param spec An empty spec struct to read the spec file into.
 * @return 0 if successful, otherwise -1
 */
int decode(char *src, struct crs_encoding_spec *spec);

/**
 * Calculates the encoding specifications from the size of the file to be encoded.
 * @param spec The spec struct to fill
 * @param filesize The size of the file to encode
 * @return 0 if successful, otherwise -1
 */
int fill_encoding_spec(struct crs_encoding_spec *spec, size_t filesize);

/**
 * Calculates the end padding required to fill out the file data to the required size. Stores this information in the
 * given spec struct.
 * @param filesize The size of the file to encode
 * @param spec The spec to be updated
 * @return 0 if successful, otherwise -1
 */
int calc_padding(size_t filesize, struct crs_encoding_spec *spec);

/**
 * Updates the spec with the minimum possible word size given k, m, width and end padding are already filled.
 * @param spec The spec to be updated
 * @return 0 if successful, otherwise -1
 */
int calc_min_w(struct crs_encoding_spec *spec);

/**
 * Prints the usage to stdout.
 * @param progName The name of the program binary
 */
void print_usage(char *progName);

#endif /* CRS_ERASURE_CODES_H_ */
