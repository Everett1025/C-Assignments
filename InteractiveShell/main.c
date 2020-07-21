#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_COMMAND 1024 // max number of letters to be supported
#define MAX_ARGUMENT 64  // max number of commands to be supported

// EXECUTES THE COMMAND
pid_t execute(char **args, int in, int out, int closeFD)
{
    // EXIT AND FREE MEMORY IF THE USER INPUTS EXIT
    if (!strcmp(args[0], "exit"))
    {
        printf("bye");
        free(args[0]);
        free(args);
        exit(0);
    }
    // CHANGE DIRECTORY COMMAND
    if (!strcmp(args[0], "cd"))
    {
        if(chdir(args[1] ? args[1] : getenv("HOME")) != 0){
            perror("chdir() failed");
        }
        return -1;
    }
    // CREATE CHILD PROCESS
    pid_t pid = fork();

    if (pid == 0)
    {
        char *path = getenv("MYPATH");
        char *ePath = strdup(path);
        char *currentPath = strtok(ePath, ":"); // being weird on tabs

        if (closeFD != -1)
        {
            close(closeFD);
        }

        if (in != -1)
        {
            dup2(in, STDIN_FILENO);
        }

        if (out != -1)
        {
            dup2(out, STDOUT_FILENO);
        }

        while (currentPath != NULL)
        {
            char *temp;
            asprintf(&temp, "%s/%s", currentPath, args[0]);
            currentPath = strtok(NULL, ":");

            struct stat sb;

            if (lstat(temp, &sb) == 0 && sb.st_mode & S_IXUSR)
            {
                execv(temp, args);
            }
            else /* non-executable */
            {
            }
            free(temp);
        }
        fprintf(stderr, "ERROR: command \"%s\" not found\n", args[0]);
        exit(EXIT_FAILURE);
    }

    // PARENT PROCESS
    if (in != -1)
    {
        close(in);
    }

    if (out != -1)
    {
        close(out);
    }
    return pid;
}

// RETURNS TRUE IF PROCESS SHOULD BE RAN IN THE BACKGROUND
bool backgroundCheck(char *userCommand)
{
    if (userCommand[strlen(userCommand) - 1] == '&')
    {
        userCommand[strlen(userCommand) - 1] = '\0';
        return true;
    }
    return false;
}

// DETERMINES IF THERE ARE TWO PROCESSES BY CHECKING IF A PIPE IS PRESENT
char *pipeCheck(char *userCommand)
{
    char *pipe;

    pipe = strchr(userCommand, '|');
    if (pipe != NULL)
    {
        *pipe = '\0';
        return pipe + 1;
    }
    return pipe;
}

// PARSES USER COMMANDS INTO ARGUMENTS AND DETERMINES IF BACKGROUND OR FOREGROUND PROCESS
char **parseInput(char *userCommand)
{
    char **args;
    args = calloc(MAX_ARGUMENT, sizeof(char *));
    char *delimeters = " \n\t\r";
    char *argument;
    argument = strtok(userCommand, delimeters);
    int index = 0;

    while (argument != NULL)
    {

        args[index] = argument;
        argument = strtok(NULL, delimeters);
        index++;
    }
    return args;
}

// TAKES IN USER INPUT RETURNS AN CHAR * ARRAY OF DATA
char *readInput()
{
    char *buffer = malloc(sizeof(char) * MAX_COMMAND);
    char c;
    int index = 0;

    if (!buffer)
    {
        fprintf(stderr, "Ran out of memory or some memory error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();
        if (c == '\r')
        {
            continue;
        }
        if (c == EOF || c == '\n')
        {
            buffer[index] = '\0';
            return buffer;
        }
        buffer[index] = c;
        index++;
    }
}
// DETERMINES IF A PROCESS HAS FINISHED RUNNING
bool isFinished()
{
    int status;
    pid_t finishedProcess = waitpid(-1, &status, WNOHANG);
    if (finishedProcess > 0)
    {
        if (WIFEXITED(status))
        {
            printf("[process %d terminated with exit status %d]\n", finishedProcess, WEXITSTATUS(status));
        }
        else
        {
            printf("[process %d terminated abnormally]\n", finishedProcess);
        }
        return true;
    }
    return false;
}
// REPEATEDLY ASKS USER FOR COOMANDS & EXECUTES THEM, UNTIL EXIT IS ENTERED BY THE USER
void shellLoop()
{
    char *userCommand;
    char *userCommand2;
    char **args;
    char **args2;
    while (1)
    {
        bool background;
        char workingDirectory[PATH_MAX];
        while(isFinished());
        
        if (getcwd(workingDirectory, sizeof(workingDirectory)) != NULL)
        {
            printf("%s$ ", workingDirectory);
        }
        else
        {
            fprintf(stderr, "Error get cwd()");
        }
        userCommand = readInput();
        if (*userCommand == '\0')
        {
            continue;
        }
        background = backgroundCheck(userCommand);
        userCommand2 = pipeCheck(userCommand);
        args = parseInput(userCommand);

        if (userCommand2)
        {
            args2 = parseInput(userCommand2);
            int fdpipe[2];
            pipe(fdpipe);

            pid_t command1 = execute(args, -1, fdpipe[1], fdpipe[0]);
            pid_t command2 = execute(args2, fdpipe[0], -1, fdpipe[1]);
            if (background == false)
            {
                waitpid(command1, NULL, 0);
                waitpid(command2, NULL, 0);

            }
            else
            {
                printf("[running background process \"%s\"]\n", args[0]);
                printf("[running background process \"%s\"]\n", args2[0]);
            }
            free(args);
            free(args2);
        }
        else
        {
            pid_t command1 = execute(args, -1, -1, -1);
            if (background == false)
            {
                waitpid(command1, NULL, 0);
            }
            else
            {
                printf("[running background process \"%s\"]\n", args[0]);
            }
            free(args);
        }
        free(userCommand);
    }
    return;
}

// MAIN PROGRAM
int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    // START SHELL
    shellLoop();
    return EXIT_SUCCESS;
}
