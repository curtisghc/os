/*
 * myshel.c - designed to be a simple unix shell
 *
 * Take commands from a REPL, parse commands, and dispatch for execution.
 * If a file is passed at time of shell execution, it will run as a script
 *
 * Execution will be modified based off of tokens detected in input,
 * such as for IO redirection, or background execution.
 *
 * For execution, builtin functions are provided and checked, but if none
 * match, execvp is used, which searches for binaries from $PATH
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
//builtin functionality
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

//helping functions for pipeline;
void append_file(char **list, char *token, char *file);
void free_elements(char **arr);

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
  args_parsed--;
  *args_parsed = NULL;
}

//returns 1 if there is a '&' char in the input, indicating the child
//process should not be waited for by the parent.
int check_background(char **input){
  //set background boolean to true if input has '&'
  while(*input != NULL){ //&& **input != '\n'){
	if(**input == '&'){
	  *input = NULL;
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
  while(*input != NULL){
	//for each redirect token
	for(int i = 0; i < 3; i++){
	  //return location and type if token matches element of input
	  if(strcmp(*input, tokens[i]) == 0){
		type[0] = i;
		type[1] = location;
		return type;
	  }
	}
	input++;
	location++;
  }
  //otherwise return -1
  return type;
}

//takes input 2d array, and 1 if it should be a background process
void execute(char **input, int background){
  pid_t cpid = fork();

  if(cpid < 0){
	fprintf(stderr, "%s: Failed to fork\n", *input);
	exit(1);
  }else if(cpid == 0){
	//child process execution of command
	if(run_builtin(input)){
	  //first, check and run if input matches builtin function
	}else if(execvp(*input, input) <= 0){
	  //attempt to execute command from $PATH
	  fprintf(stderr, "%s: Command not found\n", *input);
	}
	exit(1);
  }else if(!background){
	//wait if child should not be in background
	wait(NULL);
  }
}

//sets up input array for execution where output is to a file.
//opens file and passes commands and file to specific function based on
//type of redirection
int redirected_execute(int *type, char **input){
  //type contains to both the kind and location of redirect
  int kind = *type;
  int location = *++type;

  //why doesn't this work?
  type--;
  free(type);

  //stdout -> stdin
  if(kind == 2){
	/* ideal solution
	char **second_command = &input[location + 1];
	*input[location] = '\0';
	return unix_pipeline(input, second_command);
	*/

	//mallocation just to get it to work, not ideal
	char **second_command = (char **)malloc(sizeof(char *) * 64);

	int counter = location;

	//copy elements of input to second command
	while(input[counter + 1] != NULL){
	  *second_command = (char *) malloc(sizeof(char) * 64);

	  strcpy(*second_command, input[counter + 1]);
	  //*second_command = input[location + 1];
	  counter++;
	  second_command++;
	}
	//second_command++;
	*second_command = (char *)malloc(sizeof(char) * 2);
	*second_command = NULL;
	while(counter > location){
	  second_command--;
	  counter--;
	}

	//second_command = &input[location + 1];
	input[location] = NULL;
	//input points to first command
	//second_command points to second command
	return unix_pipeline(input, second_command);
  }else{
	FILE *fp;

	//file name is after redirect token
	char *file = input[location + 1];
	//input becomes command before redirect token
	input[location] = NULL;

	if(kind == 0){
	  //open file for writing stdout to
	  fp = fopen(file, "w");
	  if(fp == NULL){
		fprintf(stderr, "%s: No such file\n", file);
		return -1;
	  }
	  out_to_file(input, fp);
	}else{
	  //open file for reading to stdin
	  fp = fopen(file, "r");
	  if(fp == NULL){
		fprintf(stderr, "%s: No such file\n", file);
		return -1;
	  }
	  in_to_command(input, fp);
	}
	fclose(fp);
  }
  return 0;
}

//execute command and redirect output to file pointed to by fp
int out_to_file(char **command, FILE *fp){
  //establish pipe
  int pipefd[2];
  pid_t cpid;

  //check pipe
  if(pipe(pipefd) == -1){
	fprintf(stderr, "%s: Pipe failure\n", *command);
	return -1;
  }

  //forking, same as execute
  cpid = fork();
  if(cpid < 0){
	fprintf(stderr, "%s: Failed to fork\n", *command);
	exit(1);
  }else if(cpid == 0){
	//close read end
	close(pipefd[0]);
	//redirect stdout to pipe
	dup2(pipefd[1], STDOUT_FILENO);

	//execute command
	if(run_builtin(command)){
	}else if(execvp(*command, command) <= 0){
	  fprintf(stderr, "%s: Command not found\n", *command);
	}
	close(pipefd[0]);
	exit(1);
  }else{
	char buf[1];

	//close write end
	close(pipefd[1]);
	//read pipe char by char to buffer
	while(read(pipefd[0], buf, 1) > 0){
	  //print buffer char to file
	  fprintf(fp, "%c", *buf);
	}
	//fclose(fp); -- moved to redirected_execute
	close(pipefd[0]);
	//wait for child
	wait(NULL);
  }
  return 0;
}

//execute command using file pouinted to by fp as input
int in_to_command(char **command, FILE *fp){
  //establish pipe and buffer
  int pipefd[2];
  pid_t cpid;

  //check pipe
  if(pipe(pipefd) == -1){
	fprintf(stderr, "%s: Pipe failure\n", *command);
	return -1;
  }

  //forking, same as in execute
  cpid = fork();
  if(cpid < 0){
	fprintf(stderr, "%s: Failed to fork\n", *command);
	exit(1);
  }else if(cpid == 0){
	//close write end, child reads
	close(pipefd[1]);
	//redirect pipe to stdin
	dup2(pipefd[0], STDIN_FILENO);

	//execute command
	if(run_builtin(command)){
	}else if(execvp(*command, command) <= 0){
	  fprintf(stderr, "%s: Command not found\n", *command);
	}
	close(pipefd[0]);
	exit(1);

  }else{
	char buf[1];
	//close read end
	close(pipefd[0]);
	//write file char by char to pipe
	while(EOF != (*buf = fgetc(fp))){
	  write(pipefd[1], buf, 1);
	}
	//fclose(fp); -- moved to redirected_execute
	close(pipefd[1]);
	//wait for child to execute
	wait(NULL);
  }
  return 0;
}

//takes two commands, redirects stdout of first to stdin of second
int unix_pipeline(char **first, char **second){
  /* ideal solution
	 FILE *tempfp = tmpfile();
	 out_to_file(first, tempfp);
	 in_to_command(second, tempfp);
	 fclose(tempfp);
   */
  /* better than nothing
	 FILE *tempfp;
	 tempfp = fopen("temp", "w");
	 out_to_file(first, tempfp);
	 fclose(tempfp);
	 //rewind(tempfp);
	 tempfp = fopen("temp", "r");
	 in_to_command(second, tempfp);
	 fclose(tempfp);
  */
  //worst solution requires reallocation in redirected_execute()

  append_file(first, ">", "temp");
  append_file(second, "<", "temp");

  dispatch(first);
  dispatch(second);
  remove("temp");

  free_elements(second);
  free(second);

  return 0;
}

//helper for pipeline appends redirection tokens and files to input arrays
void append_file(char **list, char *token, char *file){
  while(*list != NULL){
	list++;
  }
  *list = token;
  list++;
  *list = file;
  list++;
  *list = NULL;
}

//helper for pipeline, current solution requires reallocation
void free_elements(char **arr){
  while(*arr != NULL){
	free(*(arr++));
  }
}

//compare input to list of builtin functions, and execute if matched
//return 1 if match found
int run_builtin(char **input){
  char *command = *input;
  if(strcmp(command, "pwd") == 0)
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

//open file and execute line by line
int run_script(char *file){
  char args[1024];
  char *args_parsed[64];
  FILE *fp = fopen(file, "r");

  if(fp == NULL){
	fprintf(stderr, "%s: No such file\n", file);
	return -1;
  }else{
	//parse and execute script file line by line
	while(fgets(args, 1024, fp) != NULL){
	  parse(args, args_parsed);
	  dispatch(args_parsed);
	}
	fclose(fp);
	return 0;
  }
}

//do checks and decide how the input array should be executed
int dispatch(char **input){
  if(strcmp(*input, "cd") == 0){
	//must be run by parent process
	cd(input);
  }else{
	int *redirect = check_redirect(input);
	int background = check_background(input);
	if(*redirect != -1){
	  //check for output redirection and execute there if required
	  redirected_execute(redirect, input);
	}else{
	  //otherwise execute normally
	  execute(input, background);
	}
  }
  return 0;
}

int main(int argc, char **argv){
  char args[1024];
  char *args_parsed[64];
  char pwd[1024];
  //if shell passed with argument script file
  if(argc >= 2){
	run_script(argv[1]);
	//otherwise prompt user
  }else{
	//main repl loop
	while(1){
	  getcwd(pwd, 1024);
	  printf("%s > ", pwd);
	  fgets(args, 1024, stdin);
	  parse(args, args_parsed);

	  if(strcmp(args_parsed[0], "exit") == 0 ){
		return 0;
	  }

	  dispatch(args_parsed);
	}
  }
}
