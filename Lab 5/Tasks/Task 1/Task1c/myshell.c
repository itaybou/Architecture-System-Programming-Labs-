#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <linux/limits.h>

#include "LineParser.h"

typedef int bool;
#define true 1
#define false 0

#define ERROR -1
#define SUCCESS 0

#define NOP 2

#define MAX_INPUT_LEN 2048 * sizeof(char)

struct command_func {
  char *command;
  bool (*function)(cmdLine*);
};

bool execute(cmdLine *pCmdLine);
bool check_shell_command(cmdLine* pCmdLine);
void parse_flags(int arg_count, char ** args);
void print_debug(int pid, char * const* proc_args, int arg_count);

//Shell commands
bool change_dir(cmdLine* pCmdLine);
bool quit(cmdLine* pCmdLine);


bool debug = false; // DEtermines if the program is in debug mode.
struct command_func commands[] = {{ "quit", &quit }, { "cd", &change_dir } };

int main (int argc, char** argv)
{
   char wd_path[PATH_MAX];
   char input [MAX_INPUT_LEN];
   cmdLine * parsed_cmd_line;

   bool quit = false;
   parse_flags(argc, argv);

   while(quit == false)
   {
      getcwd(wd_path, PATH_MAX);
      printf("%s> ", wd_path);
      fgets(input, MAX_INPUT_LEN, stdin);
      if (strcmp(input, "\n") != 0) {
         parsed_cmd_line = parseCmdLines(input);
         quit = execute(parsed_cmd_line);
         freeCmdLines(parsed_cmd_line);
      }
   }

   exit(EXIT_SUCCESS);
}

void parse_flags(int arg_count, char ** args)
{
   int i;
   for(i = 0; i < arg_count; ++i)
      if(strcmp(args[i], "-d") == 0)
         debug = true;
}

bool execute(cmdLine *pCmdLine)
{
   pid_t pid;
   int wstatus = 0;
   bool ret_val;

   ret_val = check_shell_command(pCmdLine);
   if(ret_val != NOP)
      return ret_val;

   pid = fork();
   if(pid != 0) {
      print_debug(pid, (*pCmdLine).arguments, (*pCmdLine).argCount);
      if((*pCmdLine).blocking == 1)
         do waitpid(pid, &wstatus, WUNTRACED);
         while(!WIFSIGNALED(wstatus) && !WIFEXITED(wstatus));
   }
   if (pid == ERROR)
      perror("Error while creating a proccess fork");
   else if (pid == 0) { // Try to exec only if you are the child proccess
      if(execvp((*pCmdLine).arguments[0], (*pCmdLine).arguments) == ERROR) {
         perror("Error executing proccess");
         _exit(EXIT_FAILURE); // exit "abnormally"
      }
      exit(EXIT_SUCCESS);
   }

   return false;
}

bool check_shell_command(cmdLine* pCmdLine)
{
   int i;
   const int commands_size = sizeof(commands)/sizeof(struct command_func);
   for(i = 0; i < commands_size; ++i)
   {
      if(strcmp((*pCmdLine).arguments[0], commands[i].command) == 0)
         return (commands[i].function)(pCmdLine);
   }
   return NOP;
}

bool change_dir(cmdLine* pCmdLine)
{
   char * path;
   int i;

   if((*pCmdLine).argCount >= 2)
   {
      path = (char *)calloc(PATH_MAX, sizeof(char));
      for(i = 1; i < (*pCmdLine).argCount; ++i) {
         if(i != 1)
            strcat(path, " ");
         strcat(path, (*pCmdLine).arguments[i]);
      }
      if(chdir(path) == ERROR)
         fprintf(stderr, "Given path is illegal or does not exist. %s\n", path);
      free(path);
   }
   else fprintf(stderr, "Illegal change dir command, missing directory to change to. (command: cd <DIR>)\n");
   return false;
}

bool quit(cmdLine* pCmdLine)
{
   return true;
}

void print_debug(int pid, char * const* proc_args, int arg_count)
{
   int i;
   if(debug == true)
   {
      fprintf(stderr, "DEBUG:\nCurrent PID: %d, Executing command: %s\nArguments: ", pid, proc_args[0]);
      for(i = 1; i < arg_count; ++i) {
         if(i != arg_count - 1)
            fprintf(stderr, "%s [%d], ", proc_args[i], i);
         else fprintf(stderr, "%s [%d]\n", proc_args[i], i);
      }
   }
}
