#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_ARGUMENTS 256

typedef struct cmdLine {
    char pipe;                               /* boolean incicating pipe*/
    char * firstCmd[MAX_ARGUMENTS];          /* command line arguments (arg 0 is the command)*/
    int firstArgCount;	                     /* number of arguments */
    char firstBlocking;	                     /* boolean indicating blocking/non-blocking */
    char *firstInputRedirect;	             /* input redirection path. NULL if no input redirection */
    char *firstOutputRedirect;	             /* output redirection path. NULL if no output redirection */
    
    char * secondCmd[MAX_ARGUMENTS];         /* command line arguments (arg 0 is the command)*/
    int secondArgCount;	                     /* number of arguments */
    char secondBlocking;	                 /* boolean indicating blocking/non-blocking */
    char *secondInputRedirect;	             /* input redirection path. NULL if no input redirection */
    char *secondOutputRedirect;	             /* output redirection path. NULL if no output redirection */
} cmdLine;

typedef struct cmdLL {
    cmdLine* currCmd;
    struct cmdLL* next;
} cmdLL;


cmdLL* changeDir(cmdLine* cmd, cmdLL* list);
cmdLL* p_execute(cmdLine* cmd, cmdLL* list);
cmdLL* runExecute(cmdLine* cmd, cmdLL* list);
cmdLL* execute(cmdLine* cmd, cmdLL* list);
int shouldQuit(cmdLine* cmd);
int isCd(cmdLine* cmd);
cmdLine* parseLine(char* line);
cmdLL* addCommand(cmdLine* cmd ,cmdLL* list);
int isHistory(cmdLine* cmd);
cmdLL* printCmdList(cmdLine* cmd ,cmdLL* list);
int isReuse(cmdLine* cmd);
cmdLL* reuseCmd(cmdLine* cmd ,cmdLL* list);



int debugFlag = 0;

cmdLL* runExecute(cmdLine* cmd, cmdLL* list) {
    if (cmd->pipe) return p_execute(cmd, list);
    else if (isCd(cmd)) return changeDir(cmd, list);
    else if (isHistory(cmd)) return printCmdList(cmd, list);
    else if (isReuse(cmd)) return reuseCmd(cmd, list);
    else return execute(cmd, list);
}

cmdLL* p_execute(cmdLine* cmd, cmdLL* list) {
    int pidChild1, pidChild2, status;
    int write_dup;
    int p[2];

    if (pipe(p) < 0) {
        perror("Failed to pipe: ");
        exit(1);
    }

    /* first child */ 
    if (!(pidChild1=fork())) {
        fclose(stdout);
        write_dup = dup(p[1]);
        close(p[1]);
        if (cmd->firstInputRedirect != NULL) {
            fclose(stdin);
            fopen(cmd->firstInputRedirect, "r");
        }
        execvp(cmd->firstCmd[0], cmd->firstCmd);
        perror("");
        exit(1);
    }

    close(p[1]);

    /* second child */
    if (!(pidChild2=fork())) {
        fclose(stdin);
        write_dup = dup(p[0]);
        close(p[0]);
        if (cmd->secondOutputRedirect != NULL) {
            fclose(stdout);
            fopen(cmd->secondOutputRedirect, "a");
        }
        execvp(cmd->secondCmd[0], cmd->secondCmd);
        perror("");
        exit(1);
    }

    close(p[0]);

    // if (debugFlag) fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
    waitpid(pidChild1, &status, 0);
    waitpid(pidChild2, &status, 0);

    return addCommand(cmd, list);
}

cmdLL* execute(cmdLine* cmd, cmdLL* list) {
    int pid, status;
    if (!(pid=fork())) {
        if (cmd->firstInputRedirect != NULL) {
            fclose(stdin);
            fopen(cmd->firstInputRedirect, "r");
        }
        if (cmd->firstOutputRedirect != NULL) {
            fclose(stdout);
            fopen(cmd->firstOutputRedirect, "a");
        }
        execvp(cmd->firstCmd[0], cmd->firstCmd);
        perror("");
        exit(1);
    }
    if (cmd->firstBlocking == 1) waitpid(pid, &status, 0);
    return addCommand(cmd, list);
}

int shouldQuit(cmdLine* cmd) {
    return strcmp(cmd->firstCmd[0], "quit") == 0;
}

int isCd(cmdLine* cmd) {
    return strcmp(cmd->firstCmd[0], "cd") == 0;
}

int isHistory(cmdLine* cmd) {
    return strcmp(cmd->firstCmd[0], "history") == 0;
}

int isReuse(cmdLine* cmd) {
    return strncmp(cmd->firstCmd[0], "!", 1) == 0;
}

cmdLL* changeDir(cmdLine* cmd, cmdLL* list) {
    int status = chdir(cmd->firstCmd[1]);
    if (status == -1) perror("Failed to change directory: ");
    return addCommand(cmd, list);
}

cmdLine* parseLine(char* line) {
    line[strlen(line)-1] = '\0';
    int i = 0;
    char* argument;
    char* command;
    char* copy = strdup(line);
    cmdLine* CmdLine = (cmdLine*)malloc( sizeof(cmdLine));
    CmdLine->pipe = 0;
    command = strtok(line, "|");
    if (command != NULL) {
        argument = strtok(command, " ");
        while (argument != NULL) {
            if (strcmp(argument, "<") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->firstInputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->firstInputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;
            }
            if (strcmp(argument, ">") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->firstOutputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->firstOutputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;  
            }
            CmdLine->firstCmd[i] = (char*) malloc(strlen(argument) + 1);
            strcpy(CmdLine->firstCmd[i], argument);
            CmdLine->firstArgCount++;
            argument = strtok(NULL, " ");
            i++;
        }
        i = 0;
    }
    command = strchr(copy, '|');
    if (command != NULL) {
        command += 2;
        CmdLine->pipe = 1;
        argument = strtok(command, " ");
        while (argument != NULL) {
            if (strcmp(argument, "<") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->secondInputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->secondInputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;
            }
            if (strcmp(argument, ">") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->secondOutputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->secondOutputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;  
            }
            CmdLine->secondCmd[i] = (char*) malloc(strlen(argument) + 1);
            strcpy(CmdLine->secondCmd[i], argument);
            CmdLine->secondArgCount++;
            argument = strtok(NULL, " ");
            i++;
        }
    }
    return CmdLine;
}

cmdLL* addCommand(cmdLine* cmd ,cmdLL* list) {
    cmdLL* newLink = (struct cmdLL*) malloc (sizeof(struct cmdLL));
    newLink->currCmd = cmd;
    newLink->next = NULL;
    cmdLL* currLink = list;
    /* empty list */
    if (currLink == NULL) {
        return newLink;
    }
    /* list have at least 1 cmd */
    while (currLink->next != NULL) {
        currLink = currLink->next;
    }
    currLink->next = newLink;
    return list;
}

cmdLL* printCmdList(cmdLine* cmd ,cmdLL* list) {
    cmdLL* currLink = list;
    int i;
    while (currLink != NULL) {
        for (i = 0; i < currLink->currCmd->firstArgCount; i++) {
            printf("%s ", currLink->currCmd->firstCmd[i]);
        }
        if (currLink->currCmd->pipe) {
            printf("| ");
            for (i = 0; i < currLink->currCmd->secondArgCount; i++) {
                printf("%s ", currLink->currCmd->secondCmd[i]);
            }
        }
        printf("\n");
        currLink = currLink->next;
    }
    return addCommand(cmd, list);
}

cmdLL* reuseCmd(cmdLine* cmd ,cmdLL* list) {
    cmdLL* currLink = list;
    int index = atoi(cmd->firstCmd[0]+1);
    while (index > 0) {
        currLink = currLink->next;
    }
    return runExecute(currLink->currCmd, list);
}


int main(int argc, char **argv) {
    char dirBuff[PATH_MAX];
    char userBuff[2048];
    int shouldQuitLoop = 0;
    cmdLine* parsedLine;
    cmdLL* commandList;

    if (argc > 1) debugFlag = !(strcmp(argv[1], "-d"));

    while (!shouldQuitLoop) {
        getcwd(dirBuff, PATH_MAX);
        printf("%s ", dirBuff);
        fgets(userBuff, 2048, stdin);
        parsedLine = parseLine(userBuff);
        if (parsedLine->firstArgCount <= 0)
            continue;
        shouldQuitLoop = shouldQuit(parsedLine);
        if (!shouldQuitLoop) {
            commandList = runExecute(parsedLine, commandList);
        }
    }
    return 0;
}