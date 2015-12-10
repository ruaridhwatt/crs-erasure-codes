#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdint.h>
#include <jerasure.h>
#include <cauchy.h>
#include <errno.h>
#include "crs_encode.h"

/* args: src dest k m */
int main(int argc, char **argv) {
   int c, res, i;
   int encode = -1;
   char *src = NULL;
   char *dest = NULL;
   struct encoding_spec spec;

   while ((c = getopt(argc, argv, "edk:m:")) != -1)
      switch (c) {
      case 'e':
         if (encode == -1) {
            encode = 1;
         } else {
            print_usage(argv[0]);
            exit(1);
         }
         break;
      case 'd':
         if (encode == -1) {
            encode = 0;
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
      }

   for (i = optind; index < argc; index++) {
      if (i == optind) {
         src = argv[i];
      } else if (i == optind + 1) {
         dest = argv[i];
      } else {
         print_usage(argv[0]);
         exit(1);
      }
   }

   if (encode = ) {
      fprintf(stderr, "usage: ");
      return EXIT_FAILURE;
   }

   sscanf(argv[3], "%d", &spec.k);
   sscanf(argv[4], "%d", &spec.m);
   res = encode(argv[1], argv[2], &spec);
   if (res == 0) {
      printf("Success!\n");
      fprintf(stdout, "Padding=%d bytes\n", (int) spec.endPadding);
      fprintf(stdout, "Word size, w=%d \n", spec.w);
      fprintf(stdout, "File size=%d \n", (int) spec.width);
   }
   return res;
}

void print_usage(char *progName) {
   fprints(stdout, "Usage:\n");
   fprints(stdout, "\t%s [options] [src] [dest]\n");
   fprints(stdout, "Options:\n");
   fprints(stdout, "\t-e\t encode\n");
   fprints(stdout, "\t-d\t decode (when decoding only the source folder is required)\n");
   fprints(stdout, "\t-k\t the number of data files (when encoding only)\n");
   fprints(stdout, "\t-m\t the number of coding files (when encoding only)\n");
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
   int res = -1;
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
   return 0;;
}

int encode(char *src, char *dest, struct encoding_spec *spec) {

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
      return -1;
   }

   /* Get input file stats */
   if (fstat(fileno(srcfile), &srcStat) < 0) {
      fprintf(stderr, "Could not get file info for %s\n%s\n", src, strerror(errno));
      return -1;
   }

   /* Create output directory */
   res = mkdir(dest, S_IRWXU | S_IRWXG);
   if (res < 0) {
      fprintf(stderr, "Could not create directory %s\n%s\n", dest, strerror(errno));
      return -1;
   }

   res = fill_encoding_spec(spec, srcStat.st_size);
   if (res < 0) {
      fprintf(stderr, "Error: Could not fill encoding spec in crs_encode.encode\n");
      return -1;
   }

   data = read_file(srcfile, spec);
   fclose(srcfile);
   if (data == NULL) {
      fprintf(stderr, "Could not create data matrix from input file\n%s\n", strerror(errno));
      return -1;
   }

   coding = create_coding_matrix(spec);
   if (coding == NULL) {
      fprintf(stderr, "Could not create coding matrix\n%s\n", strerror(errno));
      free_data_matrix(data, spec);
      return -1;
   }

   /* Create bitmatrix */
   matrix = cauchy_good_general_coding_matrix(spec->k, spec->m, spec->w);
   if (matrix == NULL) {
      fprintf(stderr, "Could not create cauchy matrix\n%s\n", strerror(errno));
      free_data_matrix(data, spec);
      free_coding_matrix(coding, spec);
      return -1;
   }
   spec->bitmatrix = jerasure_matrix_to_bitmatrix(spec->k, spec->m, spec->w, matrix);
   if (spec->bitmatrix == NULL) {
      fprintf(stderr, "Could not create bitmatrix\n%s\n", strerror(errno));
      free_data_matrix(data, spec);
      free_coding_matrix(coding, spec);
      return -1;
   }
   free(matrix);

   /* Encode using schedule */
   schedule = jerasure_smart_bitmatrix_to_schedule(spec->k, spec->m, spec->w, spec->bitmatrix);
   if (schedule == NULL) {
      fprintf(stderr, "Could not create schedule from bitmatrix\n%s\n", strerror(errno));
      free(spec->bitmatrix);
      free_data_matrix(data, spec);
      free_coding_matrix(coding, spec);
      return -1;
   }
   jerasure_schedule_encode(spec->k, spec->m, spec->w, schedule, data, coding, spec->width, (spec->width / spec->w));
   jerasure_free_schedule(schedule);

   res = write_files(data, coding, spec, dest);

   free(spec->bitmatrix);
   free_data_matrix(data, spec);
   free_coding_matrix(coding, spec);

   return res;
}

int fill_encoding_spec(struct encoding_spec *spec, size_t filesize) {

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

int calc_padding(size_t filesize, struct encoding_spec *spec) {
   int div = spec->k * sizeof(long);
   spec->endPadding = div - (filesize % div);
   if ((filesize + spec->endPadding) % sizeof(long) != 0) {
      return -1;
   }
   return 0;
}

int calc_min_w(struct encoding_spec *spec) {
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

char **read_file(FILE *f, struct encoding_spec *spec) {
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

char **create_coding_matrix(struct encoding_spec *spec) {
   int i;
   int res = 0;
   char **coding;

   coding = (char **) malloc(spec->m * sizeof(char *));
   if (coding == NULL) {
      return NULL;
   }
   for (i = 0; i < spec->m; i++) {
      coding[i] = (char *) calloc(spec->width, sizeof(char));
      if (coding[i] == NULL) {
         res = -1;
         break;
      }
   }
   if (res < 0) {
      /* Failed, rewind */
      for (i--; i >= 0; i--) {
         free(coding[i]);
      }
      free(coding);
      coding = NULL;
   }
   return coding;
}

int write_files(char **data, char **coding, struct encoding_spec *spec, char *dest) {
   int i;
   FILE* f;
   int res = 0;
   size_t nrWritten;
   size_t bitmatrixSize = spec->w * spec->m * spec->w * spec->k;
   char bitToWrite;
   uint64_t specToWrite;

   int pathLen = strlen(dest) + MAX_FILENAME_LENGTH + 2;

   char *filePath = (char *) calloc(pathLen, sizeof(char));
   if (filePath == NULL) {
      return -1;
   }

   /* Write spec to disk */
   snprintf(filePath, pathLen, "%s/spec", dest);
   f = fopen(filePath, "wb");
   if (f == NULL) {
      free(filePath);
      return -1;
   }
   specToWrite = spec->k;
   nrWritten = fwrite(&specToWrite, sizeof(uint64_t), 1, f);
   if (nrWritten != 1) {
      fclose(f);
      free(filePath);
      return -1;
   }
   specToWrite = spec->m;
   nrWritten = fwrite(&specToWrite, sizeof(uint64_t), 1, f);
   if (nrWritten != 1) {
      fclose(f);
      free(filePath);
      return -1;
   }
   specToWrite = spec->w;
   nrWritten = fwrite(&specToWrite, sizeof(uint64_t), 1, f);
   if (nrWritten != 1) {
      fclose(f);
      free(filePath);
      return -1;
   }
   specToWrite = spec->width;
   nrWritten = fwrite(&specToWrite, sizeof(uint64_t), 1, f);
   if (nrWritten != 1) {
      fclose(f);
      free(filePath);
      return -1;
   }
   specToWrite = spec->endPadding;
   nrWritten = fwrite(&specToWrite, sizeof(uint64_t), 1, f);
   if (nrWritten != 1) {
      fclose(f);
      free(filePath);
      return -1;
   }
   for (i = 0; i < bitmatrixSize; i++) {
      bitToWrite = (char) spec->bitmatrix[i];
      nrWritten = fwrite(&bitToWrite, sizeof(char), 1, f);
      if (nrWritten != 1) {
         fclose(f);
         free(filePath);
         return -1;
      }
   }
   fclose(f);

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

void free_data_matrix(char **data, struct encoding_spec *spec) {
   int i;
   for (i = 0; i < spec->k; i++) {
      free(data[i]);
   }
   free(data);
}

void free_coding_matrix(char **coding, struct encoding_spec *spec) {
   int i;
   for (i = 0; i < spec->m; i++) {
      free(coding[i]);
   }
   free(coding);
}

