#include <stdio.h>  // printf
#include <sys/types.h> // pid_t, not used in this example
#include <unistd.h> // getpid, getppid

/*

# Program Functionality

  1. The Command Prompt

    Format: command [arg1 arg2 ...] [< input_file] [> output_file] [&]

    Your shell must support command lines with a maximum length of 2048 characters, and a maximum of 512 arguments.

  2. Comments & Blank Lines

  3. Expansion of Variable $$

  4. Built-in Commands

  5. Executing Other Commands

  6. Input & Output Redirection

  7. Executing Commands in Foreground & Background

  8. Signals SIGINT & SIGTSTP

*/

int main(void) {
  printf("My pid is %d\n", getpid());
  printf("My parent's pid is %d\n", getppid());
  return 0;
}