#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>
#include <errno.h>
/* Make MIBIB */

char imei[32];
char serial_number[32];

#define AT_CMD_IMEI "AT+BMIMEI\r\n"
#define AT_CMD_SN "AT+BMSN\r\n"
#define AT_CLR "ATZ\n\r"
#define MAX_FD 10
#define MAX_ATTEMPTS 20

int fd;
int debug_mode = 0;
int open_at_port(char *port) {
  fd = open(port, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "%s: Cannot open %s\n", __func__, port);
    return -EINVAL;
  }

  return 0;
}

void close_at_port() { close(fd); }

int send_at_command(char *at_command, size_t cmdlen, char *response,
                    size_t response_sz) {
  int ret;

  if (debug_mode)
    fprintf(stdout, "%s: Sending %s\n", __func__, at_command);
  ret = write(fd, at_command, cmdlen);
  if (ret < 0) {
    fprintf(stderr, "%s: Failed to write\n", __func__);
    return -EIO;
  }

  return 0;
}

int at_port_read(char *response, size_t response_sz) {
  int ret;
  fd_set readfds;
  struct timeval tv;

  tv.tv_sec = 1;
  tv.tv_usec = 500000;
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  ret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    ret = read(fd, response, response_sz);
    if (ret < 0) {
      fprintf(stderr, "%s: Failed to read\n", __func__);
      close(fd);
      return -EIO;
    }
  } else {
    fprintf(stderr, "%s: No response in time\n", __func__);
  }
  if (debug_mode)
    fprintf(stdout, "%s: Received %s\n", __func__, response);

  return ret;
}
void showHelp() {
  fprintf(stdout,
          " Utility to backup and restore BM818 IMEI and serial number\n");
  fprintf(stdout, "Usage:\n");
  fprintf(stdout, "  imeitool -p AT_PORT -b FILENAME \n");
  fprintf(stdout, "  imeitool -p AT_PORT -r FILENAME \n");
  fprintf(stdout, "Arguments: \n"
                  "\t-p [PORT]: AT Port where the modem is connected "
                  "(typically /dev/ttyUSB2)\n"
                  "\t-b [FILE]: Backup to file\n"
                  "\t-r [FILE]: Restore from file\n"
                  "\t-d: Show AT port responses\n"
                  );
  fprintf(stdout, "Examples:\n"
                  "\t ./imeitool -p /dev/ttyUSB2 -b backup_file \n"
                  "\t ./imeitool -p /dev/ttyUSB2 -r backup_file \n");
  fprintf(stdout, "\n\t\t*********************************************\n"
                  "\t\t\t\tWARNING\n"
                  "\t\t*********************************************\n"
                  "\t\t\tUSE AT YOUR OWN RISK\n"
                  "Setting an invalid IMEI can kick you out of a network or "
                  "be illegal in certain countries.\n"
                  "See https://en.wikipedia.org/wiki/International_Mobile_Equipment_Identity#IMEI_and_the_law "
                  "for more info\n");
}

int read_imei() {
  char *response = malloc(1024 * sizeof(char));
  int cmd_ret;
  int i;

  memset(response, 0, 1024);
  cmd_ret = send_at_command(AT_CMD_IMEI, sizeof(AT_CMD_IMEI), response, 1024);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  }
  for (i = 0; i < MAX_ATTEMPTS; i++) {
    memset(response, 0, 1024);
    sleep(1);
    cmd_ret = at_port_read(response, 1024);
    if (strstr(response, "+BMIMEI:") != NULL) { // Sync was ok
      char *test = strstr(response, "+BMIMEI:");
      int bufsize = strlen(test);
      if (bufsize < 38) {
        fprintf(stderr,"WARNING: It seems your IMEI is too short... check the backup file\n");
      } else {
        bufsize = 38;
      }
      int rptr = 0;
      for (i = 8; i < bufsize; i++) {
        if (i % 2 != 0) {
          imei[rptr] = test[i];
          rptr++;
        }
      }
      fprintf(stdout, "IMEI: %s\n", imei);
      free(response);
      return 0;
    }
  }
  free(response);

  return 1;
}

int read_sn() {
  char *response = malloc(1024 * sizeof(char));
  int cmd_ret;
  int i;

  memset(response, 0, 1024);
  cmd_ret = send_at_command(AT_CMD_SN, sizeof(AT_CMD_SN), response, 1024);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  }
  for (i = 0; i < MAX_ATTEMPTS; i++) {
    memset(response, 0, 1024);
    sleep(1);

    cmd_ret = at_port_read(response, 1024);
    if (strstr(response, "+BMSN:") != NULL) { // Sync was ok
      char *test = strstr(response, "+BMSN:");
      int bufsize = strlen(test);
      if (bufsize < 49) {
        fprintf(stderr,"WARNING: It seems your SN is too short... check the backup file\n");
      } else {
        bufsize = 38;
      }

      int rptr = 0;
      for (i = 6; i < bufsize; i++) {
        if (i % 2 != 0) {
          serial_number[rptr] = test[i];
          rptr++;
        }
      }
      fprintf(stdout, "SN: %s\n", serial_number);
      free(response);
      return 0;
    }
  }
  free(response);
  return 1;
}

int save_backup_file(char *filename) {
  char buf[1024];
  int bytes;
  int fd = open(filename, O_CREAT | O_RDWR,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IRWXO);
  if (fd < 0) {
    fprintf(stderr, "Error opening %s\n", filename);
    return 1;
  }
  bytes = snprintf(buf, 1024, "%s\n%s\n", imei, serial_number);
  if (write(fd, buf, bytes) < 0) {
    fprintf(stderr, "Error writing the backup file!\n");
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}

int read_backup_file(char *filename) {
  char line[128];
  FILE *fd = fopen(filename, "r");
  if (fd == NULL) {
    fprintf(stderr, " *** Error opening %s!\n", filename);
    return 1;
  }
  if (fgets(line, sizeof line, fd) != NULL) {
    strncpy(imei, line, 32);
    imei[strlen(imei) - 1] = '\0';
  }
  if (fgets(line, sizeof line, fd) != NULL) {
    strncpy(serial_number, line, 32);
    serial_number[strlen(serial_number) - 1] = '\0';
  }

  fprintf(stdout, " -> IMEI: %s\n -> Serial Number: %s\n", imei, serial_number);
  return 0;
}

int write_imei_to_modem() {
  char *response = malloc(1024 * sizeof(char));
  memset(response, 0, 1024);

  char tempbuf[64];
  char at_command_buf[128];
  int count = 0;
  int cmd_ret;
  int i;
  memset(tempbuf, 0, 64);
  memset(at_command_buf, 0, 128);
  for (int i = 0; i < strlen(imei); i++) {
    tempbuf[count] = '3';
    count++;
    tempbuf[count] = imei[i];
    count++;
  }
  if (debug_mode)
    fprintf(stdout, "IMEI %s > IMEI_RAW %s\n", imei, tempbuf);
  count = snprintf(at_command_buf, 128, "AT+BMIMEI=%s\r\n", tempbuf);
  cmd_ret = send_at_command(at_command_buf, count, response, 1024);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  } else if (debug_mode){
    fprintf(stdout, "Response: %s\n", response);
  }
  for (i = 0; i < MAX_ATTEMPTS; i++) {
    memset(response, 0, 1024);
    sleep(1);
    cmd_ret = at_port_read(response, 1024);
    if (strstr(response, "+BMIMEI:") != NULL) { // Sync was ok
      free(response);
      return 0;
    }
  }

  return 0;
}


int write_sn_to_modem() {
  char *response = malloc(1024 * sizeof(char));
  memset(response, 0, 1024);

  char tempbuf[64];
  char at_command_buf[128];
  int count = 0;
  int cmd_ret;
  int i;
  memset(tempbuf, 0, 64);
  memset(at_command_buf, 0, 128);
  for (int i = 0; i < strlen(serial_number); i++) {
    tempbuf[count] = '3';
    count++;
    tempbuf[count] = serial_number[i];
    count++;
  }
  if (debug_mode)
    fprintf(stdout, "SN %s > SN_RAW %s\n", serial_number, tempbuf);
  count = snprintf(at_command_buf, 128, "AT+BMSN=%s\r\n", tempbuf);
  cmd_ret = send_at_command(at_command_buf, count, response, 1024);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  } else if (debug_mode) {
    fprintf(stdout, "Response: %s\n", response);
  }
  for (i = 0; i < MAX_ATTEMPTS; i++) {
    memset(response, 0, 1024);
    sleep(1);
    cmd_ret = at_port_read(response, 1024);
    if (strstr(response, "+BMSN:") != NULL) { // Sync was ok
      free(response);
      return 0;
    }
  }

  return 0;
}
int main(int argc, char **argv) {
  uint8_t backup_mode = 0;
  uint8_t restore_mode = 0;
  char *at_port;
  char *filename;

  int c;
  fprintf(stdout, "IMEI tool for Broadmobi BM818\n");
  fprintf(stdout, "-----------------------------\n");
  if (argc < 4) {
    showHelp();
    return 0;
  }

  while ((c = getopt(argc, argv, "p:b:r:dh")) != -1)
    switch (c) {
    case 'p':
      if (optarg == NULL) {
        fprintf(
            stderr,
            "I can't do my job if you don't tell me which port to connect to!\n");
      }
      at_port = optarg;
      break;
    case 'b':
      if (optarg == NULL) {
        fprintf(stderr, "You need to give me a filename to backup the data\n");
      }
      filename = optarg;
      backup_mode = 1;
      break;
    case 'r':
      if (optarg == NULL) {
        fprintf(stderr, "You need to give me a filename to restore the data\n");
      }
      filename = optarg;
      restore_mode = 1;
      break;
    case 'd':
      debug_mode = 1;
      break;
    case 'h':
    default:
      showHelp();
      return 0;
    }
  if (open_at_port(at_port) != 0) {
    fprintf(stderr, "Error opening AT Port!\n");
    return 1;
  }
  if (!backup_mode && !restore_mode) {
    fprintf(stderr, "*** ERROR *** \n You need to tell me if you want to "
                    "backup or restore something!\n");
    return 1;
  } else if (backup_mode && restore_mode) {
    fprintf(stderr, "*** ERROR *** \n I can't backup up and restore the same "
                    "data at the same time!\n");
    return 1;
  } else if (backup_mode) {
    fprintf(stdout, "Trying to retrieve the IMEI and SN from the modem...\n");
    if (read_imei() == 0 && read_sn() == 0) {
      if (save_backup_file(filename) != 0) {
        fprintf(stderr, "Error while trying save the data to disk\n");
      } else {
        fprintf(stdout, "Finished!\n");
      }
    } else {
      fprintf(stderr, "Error while retrieving data from the modem\n");
    }
  } else if (restore_mode) {
    fprintf(stdout, "* Opening backup file...\n");
    if (read_backup_file(filename) == 0) {
      // Write to modem
      if (write_imei_to_modem() != 0 || write_sn_to_modem()) {
        fprintf(stdout, " *** Error writing the backup to the modem ***\n");
      } else {
        fprintf(stdout, " IMEI and serial number restored! \n");

      }
    }
  }
  close_at_port();
  return 0;
}
