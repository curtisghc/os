#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>

struct entry{
  char name[8];
  char ext[3];
  char attribute;
  time_t last_access;
  int size;
  int start;
};

struct block{
  char data[508];
  int next_block;
};

int BS = 512;
int DRIVE_SIZE = 1024 * 1024 * 2;
int FAT_SIZE = 4096;


//seeking functions
void next_block(FILE *drive);
void prev_block(FILE *drive);
void fat(FILE *drive);
void root(FILE *drive);
void find_fat(FILE *drive, int location);
void find_block(FILE *drive, int location);

//initialization and helping functions
int zero_fat(FILE *drive);
int init_root(FILE *drive);
int search_fat(FILE *drive);
int mark_fat(FILE *drive, int location);
int unmark_fat(FILE *drive, int location);
//main functionality
int create_file(FILE *drive, struct entry e);
int delete_file(FILE *drive, struct entry e);
int write_data(FILE *drive, struct entry e, char *data, int size);
int read_data(FILE *drive, struct entry e, char *data, int size);
//basically main, but i want to use this as like a header
int intialize_fs(FILE *drive);

void next_block(FILE *drive){
  fseek(drive, BS, SEEK_CUR);
}
void prev_block(FILE *drive){
  fseek(drive, -BS, SEEK_CUR);
}
void fat(FILE *drive){
  fseek(drive, 0, SEEK_SET);
}
void root(FILE *drive){
  fseek(drive, FAT_SIZE, SEEK_SET);
}
//location in these two functions corresponds to both the byte in the fat
//where that block is allocated, and also, the block in data where that file
//is located

//every time a block at location is allocated for a file, it should be
//similarly marked on the fat at the locationth byte of the fat.
void find_fat(FILE *drive, int location){
  if(location < FAT_SIZE){
	fseek(drive, location, SEEK_SET);
  }
}
void find_block(FILE *drive, int location){
  int destination = location * BS;
  root(drive);
  if(destination < DRIVE_SIZE){
	fseek(drive, destination, SEEK_CUR);
  }
}

//write zero bytes over entire fat segment
int zero_fat(FILE *drive){
  char zero = 0;
  for(int i = 0; i < FAT_SIZE; i++){
	fwrite(&zero, 1, 1, drive);
  }
  return 0;
}

//marks the location in fat as a used block
int mark_fat(FILE *drive, int location){
  find_fat(drive, location);
  char c = 'c';
  fwrite(&c, 1, 1, drive);
  return location;
}

int unmark_fat(FILE *drive, int location){
  find_fat(drive, location);
  char c = 0;
  fwrite(&c, 1, 1, drive);
  return location;
}

//what the fuck
int init_root(FILE *drive){
  root(drive);
  return 0;
}

//gives the location of an unallocated block, must still be marked
int search_fat(FILE *drive){
  int i;
  int value;
  fat(drive);
  for(i = 0; i < FAT_SIZE; i++){
	value = fread(NULL, 2, 1, drive);
	if(value == 0){
	  return i;
	}
  }
  return -1;
}

int create_file(FILE *drive, struct entry e){
  int location = search_fat(drive);
  //mark used in fat
  find_fat(drive, location);
  mark_fat(drive, location);

  //do some kind of directory stuff here
  //write metadata to directory

  //location should still be the numbered block that has the data

  e.start = location;
  return location;

}

int delete_file(FILE *drive, struct entry e){
  //do something really important about the directory structure and root file
  //!!!!!!
  //this only allows for overwriting, needs to delete from directory too
  int location = e.start;
  struct block *b = (struct block *) malloc(512);

  do{
	unmark_fat(drive, location);
	find_fat(drive, location);
	fread(b, 512, 1, drive);
	location = b->next_block;
  }while(location != -1);

  free(b);
  return 0;
}

//takes fs, file struct, data pointer, size of data
int write_data(FILE *drive, struct entry e, char *data, int size){
  //jump to starting block
  int location = e.start;
  find_block(drive, location);

  //create block struct
  struct block *b = (struct block *) malloc(512);

  while(size > 508){
	//copy from data to block struct
	memcpy(b->data, data, 508);
	//find next block, put it into block ll
	b->next_block = search_fat(drive);
	//return to current block
	find_block(drive, location);
	//write contents of block struct to real block
	fwrite(b, 512, 1, drive);
	//decrement size
	size -= 508;
	//move to new block
	location = b->next_block;
	find_block(drive, location);
	//mark table that it's allocated
	mark_fat(drive, b->next_block);

	//consider that the pointer for b->data may need to be reset
  }
  memcpy(b->data, data, size);
  b->next_block = -1;
  fwrite(b, size, 1, drive);

  free(b);
  return 0;
}

//reads data starting at the beginning of file e, and reads size bytes into
//data
int read_data(FILE *drive, struct entry e, char *data, int size){
  find_block(drive, e.start);
  //read first block from drive
  struct block *b = (struct block *)malloc(512);
  fread(b, 512, 1, drive);
  if(size > 508){
	while(b->next_block != -1){
	  memcpy(data, b->data, 508);
	  find_block(drive, b->next_block);
	  fread(b, 512, 1, drive);
	  size -= 508;
	}
  }
  memcpy(data, b->data, size);
  free(b);
  return size;
}



int initialize_fs(FILE *drive){
  return 0;
}

int main(){
  FILE *drive = fopen("Drive2MB", "w");

  //clinically proven to work!
  zero_fat(drive);

  struct entry e;
  strcpy(e.name, "abcdefg");
  strcpy(e.ext, "tx");
  e.attribute = 'f';

  struct entry f;
  strcpy(e.name, "direct");
  strcpy(e.ext, "dr");
  f.attribute = 'd';

  struct entry root;
  strcpy(e.name, "/");
  strcpy(e.ext, "");
  root.attribute = 'd';

  fclose(drive);
}
