// Based on examples from inotify(7) man pages

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

// Events to watch
#define WATCH_ME                                                          \
  IN_OPEN | IN_CLOSE | IN_MOVE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF | \
      IN_ACCESS | IN_ATTRIB | IN_CREATE | IN_MOVE_SELF

// Definitions
#define MAX_FILES 10
#define MAX_FLEN  256

#define MIN_ARGC 2
#define MAX_ARGC MAX_FILES + 1

#define BUF_SIZE 4096

// Remove the trailing slash from a string
#define RM_TRAILING_SLASH(str)     \
  if (str[strlen(str) - 1] == '/') \
  str[strlen(str) - 1] = '\0'
#define PRINT_IF_EVENT(event_mask, bit, bit_str) \
  if (event_mask & bit)                          \
  printf("<%s> ", bit_str)

typedef enum { E_DIR, E_FILE } object_type;

// Data structures
struct file_desc {
  int wd; // The file watch descriptor
  int len;
  char fname[MAX_FLEN]; // Name of the file
  object_type type;
};

// Function prototypes
int check_args(int argc, char **argv);
void print_usage(char *arg1);
void print_timestamp(const char *ends);
void print_fevent(const struct inotify_event *fevent);
object_type get_type(const char *fname);
void handle_fevents(int fd_api, struct file_desc *file_d, int len_arr);

// Main
int main(int argc, char **argv) {
  struct file_desc file_desc_arr[MAX_FILES] = {0};
  int fd_api, poll_ret;
  nfds_t nr_fds_poll;
  struct pollfd fds_poll[2];
  char buf;

  if (check_args(argc, argv) != 0) {
    exit(EXIT_FAILURE);
  }

  // init file struct
  for (int i = 0; i < argc - 1; i++) {

    file_desc_arr[i].len = strlen(argv[i + 1]) + 1;
    if (file_desc_arr[i].len > MAX_FLEN) {
      fprintf(stderr, "[ERROR] Filename too long\n");
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }

    strncpy(file_desc_arr[i].fname, argv[i + 1], file_desc_arr[i].len);
  }

  fd_api = inotify_init1(IN_NONBLOCK);
  if (fd_api == -1) {
    perror("inotify_init1 has failed");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < argc - 1; i++) {
    printf("Monitoring [%d]: %s\n", i, file_desc_arr[i].fname);

    // Assign watch descriptor for each file
    file_desc_arr[i].wd =
        inotify_add_watch(fd_api, file_desc_arr[i].fname, WATCH_ME);
    if (file_desc_arr[i].wd == -1) {
      fprintf(stderr, "Failed to watch '%s': %s\n", file_desc_arr[i].fname,
              strerror(errno));
      exit(EXIT_FAILURE);
    }

    file_desc_arr[i].type = get_type(file_desc_arr[i].fname);
  }

  nr_fds_poll = 2;

  fds_poll[0].fd     = STDIN_FILENO; /*Console input*/
  fds_poll[0].events = POLLIN;
  fds_poll[1].fd     = fd_api; /*inotify input*/
  fds_poll[1].events = POLLIN;

  printf("Monitoring...\n");
  for (;;) {
    poll_ret = poll(fds_poll, nr_fds_poll, -1);
    if (poll_ret == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("Poll failed to get I/O");
      exit(EXIT_FAILURE);
    }

    if (poll_ret) {
      if (fds_poll[0].revents & POLLIN) {
        // Console input available
        // Empty stdin and exit
        while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n') {
          continue;
        }
        printf("Exiting\n");
        break;
      }
      if (fds_poll[1].revents & POLLIN) {
        // Inotify events
        // Do something with Inotify events
        printf("Event came in\n");
        handle_fevents(fd_api, file_desc_arr, argc - 1);
      }
    }
  }

  close(fd_api);

  exit(EXIT_SUCCESS);
}

// Function definitions
int check_args(int argc, char **argv) {

  // Check arg count
  if (argc < MIN_ARGC) {
    fprintf(stderr, "[ERROR] Too few arguments\n");
    print_usage(argv[0]);
    return -1;
  }
  if (argc >= MAX_ARGC) {
    fprintf(stderr, "[ERROR] Too many arguments\n");
    print_usage(argv[0]);
    return -1;
  }

  // Check help arg
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "-h", strlen("-h")) == 0) {
      print_usage(argv[0]);
      return -1;
    }
    if (strncmp(argv[i], "--help", strlen("--help")) == 0) {
      print_usage(argv[0]);
      return -1;
    }
  }

  return 0;
}

/*
 * Print usage for the user
 */
void print_usage(char *arg1) {
  printf("Usage:\n\t%s <FILE> [FILES]\n", arg1);
  printf("Example:\n\t%s syslog.txt passwd.txt\n", arg1);
}

/*
 * Get the type of object. Currently this returns E_DIR if it is a directory or
 * E_FILE if it is not a directory.
 * Param:
 *   - fname -- Path of the object
 * Returns:
 *    E_DIR  -- if the path is a directory
 *    E_FILE -- if the path is not a directory
 */
object_type get_type(const char *fname) {
  struct stat path;
  stat(fname, &path);
  return (S_ISREG(path.st_mode) == 0) ? E_DIR : E_FILE;
}

/*
 * Print a timestamp.
 * Param:
 *   - ends -- End string.
 */
void print_timestamp(const char *ends) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("[%ld.%ld]%s", tv.tv_sec, tv.tv_usec, ends);
}

/*
 * Print the type of event.
 * Param:
 *   - fevent -- The inotify event related
 */
void print_fevent(const struct inotify_event *fevent) {
  // File was accessed
  PRINT_IF_EVENT(fevent->mask, IN_ACCESS, "IN_ACCESS");

  // Metadata was changed, such as permissions(chmod), timestamps(utimensat),
  // extended attributes(setxattr), link count(link/unlink), user group
  // id(chown).
  PRINT_IF_EVENT(fevent->mask, IN_ATTRIB, "IN_ATTRIB");
  // File/dir opened for writing was now closed
  PRINT_IF_EVENT(fevent->mask, IN_CLOSE_WRITE, "IN_CLOSE_WRITE");
  // File/dir not opened for writing was now closed
  PRINT_IF_EVENT(fevent->mask, IN_CLOSE_NOWRITE, "IN_CLOSE_NOWRITE");
  // File/dir was created in watched directory
  PRINT_IF_EVENT(fevent->mask, IN_CREATE, "IN_CREATE");
  // File/dir was deleted from watched directory
  PRINT_IF_EVENT(fevent->mask, IN_DELETE, "IN_DELETE");
  // Watched file/dir itself was deleted or moved
  PRINT_IF_EVENT(fevent->mask, IN_DELETE_SELF, "IN_DELETE_SELF");
  // File/dir was modified (write or truncate)
  PRINT_IF_EVENT(fevent->mask, IN_MODIFY, "IN_MODIFY");
  // Watched file/dir was moved
  PRINT_IF_EVENT(fevent->mask, IN_MOVE_SELF, "IN_MOVE_SELF");
  // Generated for the directory containing the old filename when a file is
  // renamed
  PRINT_IF_EVENT(fevent->mask, IN_MOVED_FROM, "IN_MOVED_FROM");
  // Generated for the directory containing the old filename when a file is
  // renamed
  PRINT_IF_EVENT(fevent->mask, IN_MOVED_TO, "IN_MOVED_TO");
  // File/dir was opened
  PRINT_IF_EVENT(fevent->mask, IN_OPEN, "IN_OPEN");
}

/*
 * Handle incoming events. Prints out events in a human readable way.
 * Params:
 *   - fd_api  -- INotify API file descriptor
 *   - file_d  -- Array of file_desc
 *   - len_arr -- Length of file_d
 */
void handle_fevents(int fd_api, struct file_desc *file_d, int len_arr) {
  char buffer[BUF_SIZE]
      __attribute__((aligned(__alignof__(struct inotify_event))));
  const struct inotify_event *fevent;

  // Loop while events can be read from inotify fd
  for (;;) {
    ssize_t len = read(fd_api, buffer, sizeof(buffer));
    if (len == -1 && errno != EAGAIN) {
      perror("Read has failed");
      exit(EXIT_FAILURE);
    }

    // If nonblocking read() found no events to read, we exit the loop
    if (len <= 0)
      break;

    // Loop over events in the buffer
    for (char *ptr = buffer; ptr < buffer + len;
         ptr += sizeof(struct inotify_event) + fevent->len) {
      fevent = (const struct inotify_event *)ptr;

      // First we print the current timestamp
      print_timestamp(" -- ");

      // Next we print the event type
      print_fevent(fevent);

      // Print the watched file/dir
      for (size_t i = 0; i < len_arr; ++i) {
        if ((file_d[i].wd == fevent->wd) && (file_d[i].type == E_DIR)) {
          RM_TRAILING_SLASH(file_d[i].fname);
          printf("%s/", file_d[i].fname);
          break;
        }
      }

      // Print filename
      if (fevent->len) {
        printf("%s", fevent->name);
      }

      // print type of object
      if (fevent->mask & IN_ISDIR) {
        printf(" [directory]\n");
      } else {
        printf(" [file]\n");
      }
    }
  }
}
