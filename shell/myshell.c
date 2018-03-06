#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include "utilities.h"

void parse(char *args, char **args_parsed);
//check flags
int *check_redirect(char **input);
int check_background(char **input);

void execute(char **input, int background);

//pipe operations
int redirected_execute(int *type, char **input);
int out_to_file(char **command, FILE *fp);
int in_to_command(char **command, FILE *fp);
int unix_pipeline(char **first, char **second);

int run_builtin(char **input);
int run_script(char *file);
int dispatch(char **input);


//each whitespace-separated word in args will be placed as elements of
//args_parsed, a 2d array.
void parse(char *args, char **args_parsed){
  //replace whitespace with '\0'
  while (*args != '\0'){
	while (*args == ' ' || *args == '\t' || *args == '\n'){
	  *args++ = '\0';
	}
	//add partitioned strings to 2d array
	*args_parsed++ = args;
	//skip over extra white space
	while (*args != '\0' && *args != ' ' && *args != '\t' && *args != '\n'){
		args++;
	}
  }
  //end of 2d array
  *args_parsed = '\0';
}

//returns 1 if there is a '&' char in the input, indicating the child
//process should not be waited for by the parent.
int check_background(char **input){
  //set background boolean to true if input has '&'
  while(**input != '\0' && **input != '\n'){
	if(**input == '&'){
	  return 1;
	}
	input++;
  }
  return 0;
}

//returns array indicating type of redirection and location of redirect token
int *check_redirect(char **input){
  char *tokens[3] = {">", "<", "|"};
  int *type = (int *) malloc(sizeof(int) * 2);
  //initialize type
  *type = -1;

  int location = 0;
  //for earch word of input
  while(**input != '\0'){
	//for each redirect token
	for(int i = 0; i < 3; i++){
	  if(strcmp(*input, tokens[i]) == 0){
		type[0] = i;
		type[1] = location;
		return type;
	  }
	}
	input++;
	location++;
  }
  return type;

  /*
  //for each token
  for(int i = 0; i < (signed int) (sizeof(tokens)/sizeof(*tokens)); i++){
	//for each word of input
	while(**input != '\0'){
	  if(strcmp(*input, tokens[i]) == 0){
		type[0] = i;
		type[1] = location;
		return type;
	  }
	  input++;
	  location++;
	}
  }
  return type;
  */
}

void execute(char **input, int background){
  pid_t pid = fork();
  //int background = check_background(input);

  if(pid < 0){
	fprintf(stderr, "ERROR: %s: Failed to fork\n", *input);
	exit(1);
  }else if(pid == 0){
	//child process execution of command
	if(run_builtin(input)){
	  //first, check and run if input matches builtin function
	}else if(execvp(*input, input) <= 0){
	  //attempt to execute command from $PATH
	  fprintf(stderr, "ERROR: %s: Failed to execute\n", *input);
	  exit(1);
	}
  }else if(!background){
	//wait if child should not be in background
	wait(NULL);
  }
}

int redirected_execute(int *type, char **input){
  //type contains to both the kind and location of redirect
  int kind = *type;
  int location = *++type;
  //free(type);

  //stdout -> stdin
  if(kind == 2){
	//pipe stdout -> stdin
	char **second_command = &input[location + 1];
	input[location] = '\0';
	//input points to first command
	//second command points to first command
	return unix_pipeline(input, second_command);
  }else{
	FILE *fp = (FILE *) malloc(sizeof(char) * 64);
	//file name is after redirect token
	char *file = input[location + 1];
	//input becomes command before redirect token
	input[location] = '\0';

	if(kind == 0){
	  //open file for writing stdout to
	  fp = fopen(file, "w");
	  if(fp == NULL){
		fprintf(stderr, "ERROR: %s: File not found\n", file);
		return -1;
	  }
	  out_to_file(input, fp);
	}else{
	  //open file for reading to stdin
	  fp = fopen(file, "r");
	  if(fp == NULL){
		fprintf(stderr, "ERROR: %s: File not found\n", file);
		return -1;
	  }
	  in_to_command(input, fp);
	}
	fclose(fp);
  }
  return -1;
}

//execute command and redirect output to file pointed to by fp
int out_to_file(char **command, FILE *fp){
  //establish pipe
  int pipefd[2];
  pid_t cpid;

  //check pipe
  if(pipe(pipefd) == -1){
	fprintf(stderr, "ERROR: %s: Pipe failure\n", *command);
	return -1;
  }

  //forking, same as execute
  cpid = fork();
  if(cpid < 0){
	fprintf(stderr, "ERROR: %s: Failed to fork\n", *command);
	exit(1);
  }else if(cpid == 0){
	//close read end
	close(pipefd[0]);
	//redirect stdout to pipe
	dup2(pipefd[1], STDOUT_FILENO);

	//execute command
	if(execvp(*command, command) <= 0){
	  //attempt to execute command from $PATH
	  fprintf(stderr, "ERROR: %s: Failed to execute\n", *command);
	  exit(1);
	}
	close(pipefd[0]);
  }else{
	char buf[1];

	//close write end
	close(pipefd[1]);
	//read pipe char by char to buffer
	while(read(pipefd[0], buf, 1) > 0){
	  //print buffer char to file
	  fprintf(fp, "%c", *buf);
	}
	//fclose(fp);
	close(pipefd[0]);
	//wait for child
	wait(NULL);
  }
  return 0;
}

int in_to_command(char **command, FILE *fp){
  //establish pipe and buffer
  int pipefd[2];
  pid_t cpid;

  //check pipe
  if(pipe(pipefd) == -1){
	fprintf(stderr, "ERROR: %s: Pipe failure\n", *command);
	return -1;
  }

  //forking, same as in execute
  cpid = fork();
  if(cpid < 0){
	fprintf(stderr, "ERROR: %s: Failed to fork\n", *command);
	exit(1);
  }else if(cpid == 0){
	//close write end, child reads
	close(pipefd[1]);

	//redirect pipe to stdin
	dup2(pipefd[0], STDIN_FILENO);

	//execute command
	if(execvp(*command, command) <= 0){
	  //attempt to execute command from $PATH
	  fprintf(stderr, "ERROR: %s: Failed to execute\n", *command);
	  exit(1);
	}
	close(pipefd[1]);
  }else{
	char buf[1];
	//close read end
	close(pipefd[0]);
	//write file char by char to pipe
	while(EOF != (*buf = fgetc(fp))){
	  write(pipefd[1], buf, 1);
	}
	//fclose(fp);
	close(pipefd[1]);
	//wait for child to execute
	wait(NULL);
  }
  return 0;
}

//takes two commands, redirects stdout of first to stdin of second
int unix_pipeline(char **first, char **second){
  FILE *tempfp = tmpfile();
  out_to_file(first, tempfp);
  rewind(tempfp);
  in_to_command(second, tempfp);
  fclose(tempfp);

  return 0;
}


//compare input to list of builtin functions, and execute if matched
//return 1 if match found
int run_builtin(char **input){
  char *command = *input;
  if(strcmp(command, "cd") == 0)
	cd(input);
  else if(strcmp(command, "pwd") == 0)
	pwd();
  else if(strcmp(command, "cls") == 0)
	cls();
  else if(strcmp(command, "dir") == 0)
	dir();
  else if(strcmp(command, "environ") == 0)
	_environ();
  else if(strcmp(command, "echo") == 0)
	echo(input);
  else if(strcmp(command, "help") == 0)
	help(input);
  else if(strcmp(command, "pause") == 0)
	_pause();
  else
	return 0;
  return 1;
}

//
int run_script(char *file){
  char args[1024];
  char *args_parsed[64];
  FILE *fp = fopen(file, "r");
  if(fp != NULL){
	return 1;
	fprintf(stderr, "ERROR: %s: No such file\n", file);
  }else{
	//parse and execute script file line by line
	while(fgets(args, 1024, fp) != NULL){
	parse(args, args_parsed);
	dispatch(args_parsed);
	}
  }
  return 0;
}

int dispatch(char **input){
  int *redirect = check_redirect(input);
  if(*redirect != -1){
	//check for output redirection and execute there if required
	if(redirected_execute(redirect, input) != 0){
	  fprintf(stderr, "ERROR: %s: Failed to redirect\n", *input);
	  return -1;
	}
  }else{
	//otherwise execute normally
	execute(input, check_background(input));
  }
  return 0;
}

int main(int argc, char **argv){
  char args[1024];
  char *args_parsed[64];
  //if shell passed with argument script file
  if(argc >= 2){
	run_script(argv[1]);
	//otherwise prompt user
  }else{
	while(1){
		printf("MYSHELL > ");
		fgets(args, 1024, stdin);
		printf("\n");
		parse(args, args_parsed);

		if(strcmp(args_parsed[0], "exit") == 0 ){
		  return 0;
		}

		dispatch(args_parsed);
	}
  }
}

  //fix issue with exit- for each command entered, exit must be entered
  //an extra time to actually exit
  //something to do with mock prompts, i think

//still need to add io redirection
