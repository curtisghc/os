#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

int BS = 512;

int zero_drive(char *drive);
int drive_size(char *drive);
char *increment_block(char *drive);

int zero_drive(char *drive){
  while(drive != NULL){
	*drive = 0;
	increment_block(drive);
  }
  return 0;
}

int drive_size(char *drive){
  int count = 0;
  char *drive_chars = (char *) drive;
  while(drive_chars != NULL){
	drive_chars++;
	count++;
  }
  return count;
}

char *increment_block(char *drive){
  //int blocks = (drive_size(drive))/BS;
  //brute force
  return (drive + 512);
}

void print_drive_contents(char *drive){
  //int blocks = (drive_size(drive))/BS;
  unsigned char *fs = (unsigned char *) drive;
  //int size = drive_size(drive);
  while(drive != NULL){
	printf("%c", *fs);
	fs = (unsigned char *) increment_block(drive);
  }
}

int main(int argc, char **argv){
  int fd;
  char *p_map;

  fd = open("testfile", O_RDWR | O_APPEND | O_CREAT);
  p_map = (char *) mmap(0, 1024*1024*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  zero_drive(p_map);
  printf("%s\n", p_map);

  printf("%c\n", (char) -1);

  munmap(p_map, 1024*1024*2);

  //void *drive = malloc(sizeof(char) * 1024 *1024 * 2);
}
