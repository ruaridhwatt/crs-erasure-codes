#ifndef CRS_FILE_IO_H_
#define CRS_FILE_IO_H_

#include <stdio.h>
#include "crs_spec_io.h"

#define MAX_K 9999
#define MAX_M MAX_K
#define MAX_FILENAME_LENGTH 5 /* d1, ..., d9999 && c1, ..., c9999 */

/**
 * Fills the size pointer with the size of the file at filePath
 * @param filePath The path to the file
 * @param size Where the file size should be stored
 * @return 0 on success, otherwise -1 (if the file does not exist)
 */
int get_file_size(char *filePath, size_t *size);

/**
 * Converts the file to a data matrix given the specification, spec
 * @param src The path to the file
 * @param spec The specification to use
 * @return The resulting data matrix, or NULL if unsuccessful
 */
char **file2data_matrix(char *src, struct crs_encoding_spec *spec);

/**
 * @param rows The number of rows to allocate
 * @param columns The size of each row
 * @return the empty byte matrix
 */
char **calloc_matrix(int rows, int columns);

/**
 * Frees the byte matrix, matrix, consisting of 'rows' rows.
 * @param matrix
 * @param rows
 */
void matrix_free(char **matrix, int rows);

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
char **read_files(char *src, int maxNr, size_t fileSize, char prefix, int *present);

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
int repair_files(char *src, char **data, char **coding, struct crs_encoding_spec *spec, int *erasures);

/**
 * Writes the data in the data and coding matrices to files in the dest directory.
 * @param data The data matrix
 * @param coding The coding matrix
 * @param spec The encoding specification
 * @param dest The destination directory
 * @return 0 if successful, otherwise -1
 */
int write_files(char **data, char **coding, struct crs_encoding_spec *spec, char *dest);

/**
 * Writes nrBytes of data to a file at filePath
 * @param data The data to write
 * @param nrBytes The number of bytes to write
 * @param filePath The file path to write to
 * @return 0 if successful, otherwise -1
 */
int write_binary_bytes(void *data, size_t nrBytes, char *filePath);

/**
 * Converts a null terminated array of characters to an integer.
 * @param str The array of characters to be read.
 * @param i A pointer specifying where the conversion should be stored.
 * @return True if the conversion was successful, otherwise false.
 *          A conversion is considered successful iff all the str characters
 *          were used in the conversion and the result was within the range an
 *          int can store.
 */
int str2int(char *str, int *i);

#endif /* CRS_FILE_IO_H_ */
