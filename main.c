#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAXLENGTH 2048
#define MAXARGS   512

// Prototypes
void getCmd();
void execCmd();
void onSIGTSTP(int signo);
void dup2helper(char fileName[], int dupMode, int flags, int mode);

// Global to keep track of state within signals.
int backgroundEnable = 1;

int main(void) {

  char* cmd[MAXARGS]; // Holds entire command
  for(int i = 0; i < MAXARGS; i++) { cmd[i] = NULL; }

  char inputFile[256] = "";    // Holds name of input
  char outputFile[256] = "";   // Holds name of output
  int  isBackground;           // Holds if background run

  int prevStatus = 0;          // Holds status of last ran extrenal function
  int running = 1;             // Exit condition
  int parentPid = getpid();    // Store parent process PID

  // SIGINT ^C
    // Parent ignores
    // Background children ignore
    // Foreground children accept
  struct sigaction str_sigint;
	str_sigint.sa_handler = SIG_IGN;
	sigfillset(&str_sigint.sa_mask);
	str_sigint.sa_flags = 0;
	sigaction(SIGINT, &str_sigint, NULL);

  // SIGTSTP ^V
    // Parent provides telemetry
    // Foregound children ignore
    // background children ignore
  struct sigaction str_sigtstp;
  str_sigtstp.sa_handler = onSIGTSTP;
  sigfillset(&str_sigtstp.sa_mask);
  str_sigtstp.sa_flags = 0;
  sigaction(SIGTSTP, &str_sigtstp, NULL);

  do {  // Event Loop
    // Get command from line
    getCmd(cmd, inputFile, outputFile, &isBackground, parentPid);

    // \0
    if(cmd[0][0] == '\0')
      continue;
   
    // #
    else if(cmd[0][0] == '#')
      continue;
    
    // exit
    else if(!strcmp(cmd[0], "exit"))
      running = 0;
    
    // cd
		else if (!strcmp(cmd[0], "cd")) {
			if (cmd[1]) {
				if (chdir(cmd[1]) == -1) {
					printf("Directory not found.\n");
					fflush(stdout);
				}
			} else {
				chdir(getenv("HOME"));
			}
		}
    
    // Check for status
    else if((strcmp(cmd[0], "status") == 0)) {
      if (WIFEXITED(prevStatus)) {
        printf("exit value %d\n", WEXITSTATUS(prevStatus));
      } else {
        printf("terminated by signal %d\n", WTERMSIG(prevStatus));
      }
      fflush(stdout);
    }
    
    // External Command detected
    else {
      execCmd(cmd, inputFile, outputFile, &isBackground, parentPid, &prevStatus, str_sigint);
      // printf("Command: %s\n", cmd[0]);
    }

    for(int i = 0; i < MAXARGS; i++) { cmd[i] = NULL; }
    isBackground = 0;
    inputFile[0] = '\0';
    outputFile[0] = '\0';

  } while(running);

  return 0;
}

// command [arg1 arg2 ...] [< input_file] [> output_file] [&]
void getCmd(char* cmdArgv[],
            char inputFile[], 
            char outputFile[], 
            int* isBackground,
            int parentPid) {

  // Holds text pre-processing
  char cmdTxt[MAXLENGTH];

  // Prompt input
  printf(": ");
  fflush(stdout);

  fgets(cmdTxt, MAXLENGTH, stdin);

  for(int i = 0; (i < MAXLENGTH); i++) {
    if(cmdTxt[i] == '\n')
      cmdTxt[i] = '\0';
  }

  // Empty Case
  if(!strcmp(cmdTxt, "")) {
    cmdArgv[0] = strdup("");
    return;
  }

  // Tokenize by ' '
  char sp[2] = " ";
  char* token = strtok(cmdTxt, sp);

  // Token Walk
  for(int i = 0; token != NULL; i++) {

    // &
    if(!strcmp(token, "&")) {
      *isBackground = 1;
    }

    // <
    else if(!strcmp(token, "<")) {
      token = strtok(NULL, sp);
      strcpy(inputFile, token);
    }

    // >
    else if(!strncmp(token, ">", strlen("&"))) {
      token = strtok(NULL, sp);
      strcpy(outputFile, token);
    }
    
    // Arg[i]
    else {
      cmdArgv[i] = strdup(token);

      for (int j = 0; cmdArgv[i][j]; j++) {
				if (cmdArgv[i][j] == '$' &&
					 cmdArgv[i][j+1] == '$') {
					cmdArgv[i][j] = '\0';
          // This eats the next three characters after the first match and I cant figure out why...
					snprintf(cmdArgv[i], 256, "%s%d%s", cmdArgv[i], parentPid, cmdArgv[i]+j+1);
          printf("%s\n", cmdArgv[i]);
				}
			}
    }

    // Lastly, iterate
    token = strtok(NULL, sp);
  }

  return;
}

void execCmd(char* cmdArgv[],
            char   inputFile[], 
            char   outputFile[], 
            int*   isBackground,
            int    parentPid,
            int*   exitStatus,
            struct sigaction sa_child) {

  pid_t spawnPid = -5;

  spawnPid = fork();
  switch (spawnPid)
  {
  // Fork Fail Mode
  case -1:
    perror("Failed Fork");
    exit(1);
    break;

  // Child Process
  case 0:
    // setup child sigaction and re-enable ^C for sub-processes
    sa_child.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa_child, NULL);

    // Register input dup
    dup2helper(inputFile, 0, (O_RDONLY), -1);

    // Rgeister output dup
    dup2helper(outputFile, 1, (O_WRONLY | O_CREAT | O_TRUNC), 0666);


    // Search directories and run
    if (execvp(cmdArgv[0], (char* const*)cmdArgv)) {
          printf("%s: no such file or directory\n", cmdArgv[0]);
          fflush(stdout);
          exit(2);
    }

  break;

  // Parent Process
  default:

    if (*isBackground && backgroundEnable) {
      pid_t actualPid = waitpid(spawnPid, exitStatus, WNOHANG);
      printf("background pid is %d\n", spawnPid);
      fflush(stdout);
    }
    // Otherwise execute it like normal
    else {
      pid_t actualPid = waitpid(spawnPid, exitStatus, 0);
    }

		// Check for terminated background processes!	
		while ((spawnPid = waitpid(-1, exitStatus, WNOHANG)) > 0) {
			printf("child %d terminated\n", spawnPid);
      fflush(stdout);
			if (WIFEXITED(exitStatus)) {
        printf("exit value %d\n", WEXITSTATUS(exitStatus));
      } else {
        printf("terminated by signal %d\n", WTERMSIG(exitStatus));
      }
      fflush(stdout);
		}

    break;
  }

  return;
}

void onSIGTSTP(int signo) {
  int size;
  char* message;
	if (backgroundEnable == 1) {
		message = "\nEntering foreground-only mode (& is now ignored)\n";
	}
	else {
		message = "\nExiting foreground-only mode\n";
	}
  backgroundEnable = !backgroundEnable;
  write (1, message, strlen(message)-2);
  fflush(stdout);
}

void dup2helper(char fileName[], int dupMode, int flags, int mode) {
  int value;
  int result;
  
  if (strcmp(fileName, "")) {
      // open it
      if(mode == -1) {
        value = open(fileName, flags);
      } else {
        value = open(fileName, flags, mode);
      }
      if (value == -1) {
        perror("Unable to open output file\n");
        exit(1);
      }
      // assign it
      result = dup2(value, dupMode);
      if (result == -1) {
        perror("Unable to assign output file\n");
        exit(2);
      }
      // trigger its close
      fcntl(value, F_SETFD, FD_CLOEXEC);
    }
}