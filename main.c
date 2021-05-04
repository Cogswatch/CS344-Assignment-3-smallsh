#include <stdio.h>  // printf
#include <sys/types.h> // pid_t, not used in this example
#include <unistd.h> // getpid, getppid
#include<string.h>

/*

# Program Functionality

  1. The Command Prompt

    Format: command [arg1 arg2 ...] [< input_file] [> output_file] [&]

    Your shell must support command lines with a maximum length of 2048 characters, and a maximum of 512 arguments.

  2. Comments & Blank Lines

    Yup.

  3. Expansion of Variable $$

    Scan through and replace instances.

  4. Built-in Commands

    exit [none]
      kill child processes and exit

    cd [none or dir]
      [none]
        sets pwd to HOME

      [dir]
        dir to change to, support relative and absolute

    status [none]
      Prints last exit condition of a forground process or 0 if fresh
      Ignores conditions of built in functions

  5. Executing Other Commands

    Do this last

    use fork(), exec() and waitpid()

    Use path to find commands, should allow shell scripts

    error 1 if not found

    A child process must terminate after running a command

  6. Input & Output Redirection

  7. Executing Commands in Foreground & Background

    Foregound (/wo &) -> Blocking run

    Background (/w &) -> Non-Blocking run
      When a background process terminates, a message showing the process id and exit status will be printed. This message must be printed just before the prompt for a new command is displayed.

      if input or output is not redirected, then standard input should be redirected to /dev/null

  8. Signals SIGINT & SIGTSTP

*/

#define MAXLENGTH 2048
#define MAXARGS   512

void getCmd();

int main(void) {

  char* cmd[MAXARGS]; // Holds entire command
  char inputFile[256] = "";    // Holds name of input
  char outputFile[256] = "";   // Holds name of output
  int  isBackground; // Holds if background run

  int prevStatus = 0;
  int running = 1;

  while(running) {
    getCmd(cmd, inputFile, outputFile, &isBackground);

    if(cmd[0][0] == '\0')
      continue;

    else if(cmd[0][0] == '#')
      continue;

    else if((strcmp(cmd[0], "exit\n") == 0) || (strcmp(cmd[0], "exit\n") == 0))
      running = 0;

    else if(!strcmp(cmd[0], "cd\n") ) {
      chdir(getenv("HOME"));
    }
    else if(!strcmp(cmd[0], "cd") ) {
      int x = chdir(cmd[1]);
      if (x == -1) {
					printf("Directory not found.\n");
					fflush(stdout);
			}
    }
    else if(!strcmp(cmd[0], "status"))
      printf("Running status\n");

  }

  return 0;
}

// command [arg1 arg2 ...] [< input_file] [> output_file] [&]
void getCmd(char* cmdArgv[],
            char inputFile[], 
            char outputFile[], 
            int* isBackground) {

  *isBackground = 0;

  // Prompt input
  printf(": ");
  fflush(stdout);

  char cmdTxt[MAXLENGTH];
  fgets(cmdTxt, MAXLENGTH, stdin);

  int match = 0;
  for(int i = 0; (!match) && (i < MAXLENGTH); i++) {
    if(cmdTxt[i] == '\n')
      cmdTxt[i] = '\0';
      match = 1;
  }

  // Empty Case
  if(!strncmp(cmdTxt, "\n", strlen("\n"))) {
    cmdArgv[0] = strdup("\n");
    return;
  }

  // Tokenize by ' '
  char space[2] = " ";
  char* token;

  // First Token
  token = strtok(cmdTxt, space);

  // Token Walk
  for(int i = 0; token; i++) {

    // Background run option
    if(!strcmp(token, "&")) {
      *isBackground = 1;
    }
    // input_file
    else if(!strcmp("<", token)) {
      // Advance one word
      token = strtok(NULL, space);
      
      strcpy(inputFile, token);
    }
    // output_file
    else if(!strncmp(token, ">", strlen("&"))) {
      // Advance one word
      token = strtok(NULL, space);
      
      strcpy(outputFile, token);
    }
    // Next Arg
    else {
      cmdArgv[i] = strdup(token);

      // !!! Finish token extraction.
      // for(int j = 0; j < strlen(cmdArgv[i]); j++) {
      //   if((cmdArgv[i][j] == '$') && (cmdArgv[i][j+1] == '$')) {
      //     char* first;
      //     char* second;
      //     strncpy(first, cmdArgv[i], j);
      //     strncpy(second, cmdArgv[i]+i+1, strlen(cmdArgv[i])-j);
      //   }
      // }
    }

    // Lastly, iterate
    token = strtok(NULL, space);
  }

  return;
}

