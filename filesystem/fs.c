#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>

typedef struct entry{
  char name[8];
  char attribute;
  time_t last_access;
  int size;
  int blocks[64];
}entry;


typedef struct directory_block{
  entry entries[16];
}directory_block;

int BS = 512;
int DRIVE_SIZE;
int FAT_SIZE;
char *DRIVE_INITIAL;

//seeking functions
int jump_block(int location);

//helping and initialization functions
int mark_block(int location);
int unmark_block(int location);

int zero_drive();
int create_root(char *drive);
directory_block get_db(char *drive);
int get_block(char *drive, char *name);
int find_entry(directory_block dir, char *name);
int find_empty_entry(directory_block drive);

entry empty_entry();

//main functionality need to structure this into repl loop
int create_file(char *drive, char *name);
int delete_file(char *drive, char *name);
int read_file(char *drive, char *name, char *data);
int write_file(char *drive, char *name, char *data, int size);

int create_directory(char *drive, char *name);
int delete_directory(char *drive, char *name);
//int change_directory(char *drive, char *name);
int list_files(char *drive);


void help_message();

//initilization & formatting
int format_drive();
int dispatch(char *drive, int location, char *command, char *operand);

int jump_block(int location){
  //jumps to block, offset by FAT
  return BS * location + FAT_SIZE;
}


int zero_drive(){
  for(int i = 0; i < DRIVE_SIZE; i++){
	DRIVE_INITIAL[i] = 0;
  }
  return 0;
}

int drive_size(char *drive){
  int i = 0;
  while(drive != NULL){
	drive++;
	i++;
  }
  return i;
}

int free_block(){
  for(int i = 0; i < FAT_SIZE; i++){
	if(DRIVE_INITIAL[i] == 0){
	  return i;
	}
  }
  return -1;
}

int mark_block(int location){
  DRIVE_INITIAL[location] = 255;
  return 0;
}

//must be absolute
int unmark_block(int location){
  DRIVE_INITIAL[location] = 0;
  /* if you want to zero every block on deletion
  for(int i = 0; i < BS; i++){
	DRIVE_INITIAL[jump_block(location) + i] = 0;
  }
  */
  return 0;
}

//give a pointer to a space in drive, copies data to a directory struct and returns it
directory_block get_db(char *drive){
  directory_block db;
  memcpy(&db, drive, BS);
  return db;
}

int get_block(char *drive, char *name){
  if(strcmp(name, "root") == 0){
	return 0;
  }
  struct directory_block dir = get_db(drive);
  int i = find_entry(dir, name);
  if(i == -1){
	fprintf(stderr, "%s: directory not found\n", name);
	return -1;
  }else if(dir.entries[i].attribute == 'f'){
	fprintf(stderr, "%s: not a directory\n", name);
	return -1;
  }else{
	entry e = dir.entries[i];
	return e.blocks[0];
  }
}

entry empty_entry(){
  entry e;
  for(int i = 0; i < 8; i++){
	e.name[i] = 0;
  }
  e.attribute = 0;
  e.last_access = 0;
  e.size = 0;
  for(int i = 0; i < 64; i++){
	e.blocks[i] = 0;
  }
  return e;
}

int find_entry(directory_block dir, char *name){
  for(int i = 0; i < 16; i++){
	if(strcmp(dir.entries[i].name, name) == 0){
	  return i;
	}
  }
  return -1;
}

int find_empty_entry(directory_block dir){
  for(int i = 0; i < 16; i++){
	if(dir.entries[i].blocks[0] == 0){
	  return i;
	}
  }
  return -1;
}

int create_root(char *drive){
  int location = 0;
  //int block_location = jump_block(location);
  mark_block(location);

  directory_block root;
  //memcpy(&root, &drive[jump_block(location)], BS);
  memcpy(&drive[jump_block(location)], &root, BS);

  return 0;
}

int create_file(char *drive, char *name){

  //import parent directory, find space for sub
  struct directory_block dir = get_db(drive);
  int i = find_empty_entry(dir);

  //find a free block
  int location = free_block(drive);
  //mark that block for use
  mark_block(location);

  struct entry e;
  memcpy(e.name, name, 8);
  e.attribute = 'f';
  e.last_access = time(NULL);
  e.size = 0;
  e.blocks[0] = location;

  dir.entries[i] = e;

  //write new directory block back to drive
  memcpy(drive, &dir, BS);

  return location;
}

int delete_file(char *drive, char *name){
  directory_block dir = get_db(drive);
  int i = find_entry(dir, name);
  if(i == -1){
	fprintf(stderr, "%s: file not found\n", name);
	return -1;
  }
  //unmark blocks associated with file
  int size = dir.entries[i].size;
  int num_blocks = size / BS + 1;
  for(int j = 0; j < num_blocks; j++){
	unmark_block(dir.entries[i].blocks[j]);
  }

  dir.entries[i] = empty_entry();
  memcpy(drive, &dir, BS);
  return 0;
}

int create_directory(char *drive, char *name){

  //find spot in parent directory
  directory_block dir = get_db(drive);
  int i = find_empty_entry(dir);

  //initialize new directory's data
  int location = free_block(drive);
  mark_block(location);

  struct entry new_dir;
  memcpy(new_dir.name, name, 8);
  new_dir.attribute = 'd';
  new_dir.last_access = time(NULL);
  new_dir.size = BS;
  new_dir.blocks[0] = location;

  dir.entries[i] = new_dir;

  memcpy(drive, &dir, BS);

  return location;
}

int delete_directory(char *drive, char *name){
  directory_block dir = get_db(drive);
  int i  = find_entry(dir, name);
  //error checking
  if(i == -1){
	fprintf(stderr, "%s: directory not found\n", name);
	return -1;
  }
  //erase previous entry position from directory
  dir.entries[i] = empty_entry();
  memcpy(drive, &dir, BS);

  //need to unmark every possible nested block
  //recursively if they're directories
  /*kind of excessive, i don't really want to do this
  directory_block sub = get_db(&drive[jump_block(e.blocks[0])]);
  char attr;
  for(int i = 0; i < 16; i++){
	attr = sub.entries[i].attribute;
	if(attr == 'f'){
	  delete_file(&drive[jump_block(e.blocks[0])], sub.entries[i].name);
	}else if(attr == 'd'){
	  delete_file(&drive[jump_block(e.blocks[0])], sub.entries[i].name);
	}
  }
  */
  unmark_block(dir.entries[i].blocks[0]);

  return 0;
}

int read_file(char *drive, char *name, char *data){
  //char *contents = (char *) malloc(64);
  directory_block dir = get_db(drive);
  int i = find_entry(dir, name);
  if(i == -1){
	fprintf(stderr, "%s: file not found\n", name);
	return -1;
  }
  entry file = dir.entries[i];
  if(file.attribute == 'd'){
	fprintf(stderr, "%s: not a file\n", name);
	return -1;
  }

  int leftover = file.size % BS;

  //copy data from each block to the pointer given
  int j = 0;
  while(file.blocks[j + 1] > 0){
	//memcpy(contents, &drive[jump_block(file.blocks[j])], BS);
	memcpy(data, &drive[jump_block(file.blocks[j])], BS);
	j++;
  }
  memcpy(data, &drive[jump_block(file.blocks[j])], leftover);
  return 0;
}

int write_file(char *drive, char *name, char *data, int size){
  directory_block dir = get_db(drive);
  int i = find_entry(dir, name);
  if(i == -1){
	fprintf(stderr, "\n%s: file not found\n", name);
	return -1;
  }
  struct entry file = dir.entries[i];
  if(file.attribute == 'd'){
	fprintf(stderr, "\n%s: not a file\n", name);
	return -1;
  }

  //update file entry in directory (blocks, timestamp, and size
  //also, write data to blocks
  file.size = size;
  file.last_access = time(NULL);

  int j;
  for(j = 0; i < size / BS; i++){
	memcpy(&drive[jump_block(file.blocks[j])], data, BS);
	file.blocks[j + 1] = free_block();
	mark_block(file.blocks[j + 1]);
  }
  memcpy(&drive[jump_block(file.blocks[j])], data, BS);
  file.blocks[j + 1] = -1;

  //replace directory in drive
  dir.entries[i] = file;
  memcpy(drive, &dir, BS);

  return size;
}

int list_files(char *drive){
  directory_block db = get_db(drive);

  entry file;
  char *name;
  char time[20];
  char attr;
  int size;

  for(int i = 0; i < 16; i++){
	file = db.entries[i];

	attr = file.attribute;
	if(attr != 'd' && attr != 'f' )
	  continue;

	name = file.name;
	strftime(time, 20, "%Y-%m-%d %H:%M:%S", localtime(&file.last_access));
	size = file.size;


	printf("%s -- ", name);
	if(attr == 'f'){
	  printf("file");
	}else if(file.attribute == 'd'){
	  printf("directory");
	}
	printf(" -- %dB ", size);
	printf("-- %s\n", time);
  }
  return 0;
}

int format(char *drive){
  zero_drive(drive);
  return create_root(drive);
}

int dispatch(char *drive, int location, char *command, char *operand){
  char *buffer = (char *) malloc(32768);
  *buffer = '\0';
  //char buffer[32768];
  if(strcmp(command, "mkdir") == 0){
	create_directory(&drive[jump_block(location)], operand);
  }else if(strcmp(command, "rmdir") == 0){
	delete_file(&drive[jump_block(location)], operand);
  }else if(strcmp(command, "touch") == 0){
	create_file(&drive[jump_block(location)], operand);
  }else if(strcmp(command, "rm") == 0){
	delete_file(&drive[jump_block(location)], operand);
  }else if(strcmp(command, "write") == 0){
	printf("Enter data to be written (Ctrl-D):\n");
	//fscanf(stdin, "  %s  ", buffer);
	fread(buffer, sizeof(char), 32768, stdin);
	write_file(&drive[jump_block(location)], operand, buffer, strlen(buffer));
	printf("\n");
  }else if(strcmp(command, "read") == 0){
	read_file(&drive[jump_block(location)], operand, buffer);
	printf("%s\n", buffer);
  }else{
	printf("%s: Command not found\n", command);
  }
  free(buffer);
  return 0;
}

void help_message(){
  printf("Commands for filesystem operations:\n");
  printf("ls \t\t\t: List files in current directory\n");
  printf("cd [directory|\"root\"]\t: Change to specified directory, or return home\n");
  printf("mkdir [directory]\t: Create directory\n");
  printf("rmdir [directory]\t: Delete directory\n");
  printf("touch [filename]\t: Create normal file\n");
  printf("rm [filename]\t\t: Delete normal file\n");
  printf("write [filename]\t: Write to a file\n");
  printf("read [filename]\t\t: Read from a file and print to screen\n");
  printf("format \t\t\t: Format the file system\n");
  printf("q \t\t\t: Quit\n");
  printf("h \t\t\t: Print this help dialogue\n");
}

int main(int argc, char **argv){
  int fd;
  char *p_map;
  int fsize;

  char *drive_name = "Drive2MB";
  if(argc == 2){
	drive_name = *++argv;
  }

  fd = open(drive_name, O_RDWR | O_APPEND | O_CREAT);
  if(fd < 0){
	fprintf(stderr, "%s: Drive not found\n", *argv);
	return 1;
  }
  //fd = open("Drive10MB", O_RDWR | O_APPEND | O_CREAT);
  //initialize globals
  DRIVE_SIZE = fsize = (int) lseek(fd, 0, SEEK_END);
  FAT_SIZE = fsize / BS;

  p_map = (char *) mmap(0, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  DRIVE_INITIAL = p_map;


  //repl loop for fs operations -- needs to be better
  //
  char command[10];
  char operand[10];

  int location = 0;
  int new_location;

  help_message();
  while(strcmp(command, "q") != 0){
	printf("> ");
	fscanf(stdin, "%s", command);

	if(strcmp(command, "cd") == 0){
	  //printf("Choose a file to \"%s\"\n", command);
	  //printf("> ");
	  fscanf(stdin, "%s", operand);
	  new_location = get_block(&p_map[jump_block(location)], operand);
	  if(new_location != -1){
		location = new_location;
	  }
	}else if(strcmp(command, "format") == 0){
	  printf("Drive \"%s\" is %d bytes long.\n", drive_name, DRIVE_SIZE);
	  printf("It will be formatted into %d byte blocks.\n", BS);
	  //printf("Are you sure you want to format? (y/n): ");
	  location = format(p_map);
	}else if(strcmp(command, "ls") == 0){
	  list_files(&p_map[jump_block(location)]);
	}else if(strcmp(command, "h") == 0){
	  help_message();
	}else if(strcmp(command, "q") == 0){
	}else{
	  /*
	  if(*operand == '\0'){
		printf("Choose a file to \"%s\": " , command);
	  }
	  */
	  fscanf(stdin, "%s", operand);
	  dispatch(p_map, location, command, operand);
	  *operand = '\0';
	}
  }

  /*

  int location = format(p_map);
  printf("Root position --  %d\n", location);

  //printf("%d\n", location);
  //create_file(&p_map[jump_block(location)], "chg");
  //write_file(&p_map[jump_block(location)], "chg", "1111", 5);
  create_file(&p_map[jump_block(location)], "maddie");
  write_file(&p_map[jump_block(location)], "maddie", "gay", 5);
  //printf("%d\n", create_file(&p_map[jump_block(location)], "chg"));
  //delete_file(&p_map[jump_block(location)], "chg");
  //printf("%d\n", create_directory(&p_map[jump_block(location)], "dir"));

  //read_file(&p_map[jump_block(location)], "chg");
  //list_files(&p_map[jump_block(location)]);

  location = create_file(&p_map[jump_block(location)], "inside");
  create_file(&p_map[jump_block(location)], "gottem");
  //list_files(&p_map[jump_block(location)]);

  //char *contents = (char *) malloc(64);
  read_file(&p_map[jump_block(location)], "maddie", contents);
  printf("\n%s\n", contents);
  //free(contents);
  */


  munmap(p_map, DRIVE_SIZE);

  /*
	cleanup schmutz output, phantom files -- solved
	better input methods for format(y/n) -- fuck that
	handle enter -- not really solved, but i don't want it to be a big deal
	allow multi, word entries for write -- solved
	write can't handle more than 8 bytes -- solved
	infinite loop when rm file -- solved
	rmdir just doesn't work -- solved
  */
  //create file, create directory, create root
  //all search of empty space by entry[i].blocks[0] != 0
  //this is bad
  //should be fixed
}
