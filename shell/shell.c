#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

void parse(char *args, char **args_parsed){
  while (*args != '\0'){
    while (*args == ' ' || *args == '\t' || *args == '\n'){
      *args++ = '\0';
    }
    *args_parsed++ = args;
    while (*args != '\0' && *args != ' ' && *args != '\t' && *args != '\n'){
      args++;
    }
  }
  *args_parsed = '\0';
}

void execute(char **args){
  int pid = fork();
  int status;
  if(pid < 0){
	printf("Failed to fork");
	exit(1);
  }else if(pid > 0){
	while(wait(&status) != pid);
  }else{
	if(execvp(*args, args) < 0){
	  printf("Failed to execute");
	  exit(1);
	}
  }
}
int main(int argc, char **argv){
  char args[1024];
  char *args_parsed[64];
  /*
  char *a = "ls";
  char *b = "-a";
  char *c = "\0";
  char *args[] = {"urxvt","\0"};
  execvp("urxvt", args);
  */
  printf("prompt> ");
  fgets(args, 1024, stdin);
  parse(args, args_parsed);
  /*
  while(*args_parsed != NULL){

    printf("%s\n", *args_parsed);
    args_parsed++;

  }
  */
  return 0;
}