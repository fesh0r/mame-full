/* i'll actually make something out of this when i have time, its the decryption for some old igs game ;-) */


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

unsigned short *file1_data;
int file1_size;

unsigned short *result_data;

int main(int argc, char **argv)
{
  int fd;
  int i;
  char msg[512];

  if(argc != 3) {
    fprintf(stderr, "Usage:\n%s <file1.bin> <result.bin>\n", argv[0]);
    exit(1);
  }

  sprintf(msg, "Open %s", argv[1]);
  fd = open(argv[1], O_RDONLY);
  if(fd<0) {
    perror(msg);
    exit(2);
  }

  file1_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  file1_data = malloc(file1_size);
  read(fd, file1_data, file1_size);
  close(fd);

  result_data = malloc(file1_size);
  for(i=0; i<file1_size/2; i++) {
    unsigned short x = file1_data[i];

    if((i & 0x10c0) == 0x0000)
      x ^= 0x0001;

    if((i & 0x0010) == 0x0010 || (i & 0x0130) == 0x0020)
      x ^= 0x0404;

    if((i & 0x00d0) != 0x0010)
      x ^= 0x1010;

    if(((i & 0x0008) == 0x0008)^((i & 0x10c0) == 0x0000))
      x ^= 0x0100;

    result_data[i] = x;
  }

  sprintf(msg, "Open %s", argv[2]);
  fd = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fd<0) {
    perror(msg);
    exit(2);
  }

  write(fd, result_data, file1_size);
  close(fd);

  return 0;
}

