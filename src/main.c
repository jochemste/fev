

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 10
#define MAX_FLEN  256

#define MIN_ARGC 2
#define MAX_ARGC MAX_FILES + 1

// Data structures
struct file_desc {
  int fd; // The file descriptor
  int len;
  char fname[MAX_FLEN]; // Name of the file
};

// Function prototypes
int check_args(int argc, char **argv);
void print_usage(void);

// Main
int main(int argc, char **argv) {
  struct file_desc file_desc_arr[MAX_FILES] = {0};

  if (check_args(argc, argv) != 0)
    exit(EXIT_FAILURE);

  // init file struct
  for (int i = 0; i < argc - 1; i++) {

    file_desc_arr[i].len = strlen(argv[i + 1]) + 1;
    if (file_desc_arr[i].len > MAX_FLEN) {
      fprintf(stderr, "[ERROR] Filename too long\n");
      print_usage();
      exit(EXIT_FAILURE);
    }

    strncpy(file_desc_arr[i].fname, argv[i + 1], file_desc_arr[i].len);
  }

  for (int i = 0; i < argc - 1; i++) {
    printf("Monitoring [%d]: %s\n", i, file_desc_arr[i].fname);
  }

  exit(EXIT_SUCCESS);
}

// Function definitions
int check_args(int argc, char **argv) {
  if (argc < MIN_ARGC) {
    fprintf(stderr, "[ERROR] Too few arguments\n");
    print_usage();
    return -1;
  }
  if (argc >= MAX_ARGC) {
    fprintf(stderr, "[ERROR] Too many arguments\n");
    print_usage();
    return -1;
  }
  return 0;
}
void print_usage(void) {}
