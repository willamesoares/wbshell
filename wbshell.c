/**
 * Developed as a project for an Operating System class
 * Willame Soares Barroso
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define BUFFER_SIZE 50

//static char buffer[BUFFER_SIZE];
int nprompt = 1, historyIndex = 0;    /* nprompt - counts number of prompts |
                                         historyIndex - keeps track of number of commands*/
typedef struct node     /* define struct type to store history of commands */
{
    char command[MAX_LINE];
    int index;
    struct node *next;
} historyList;

historyList * head = NULL; //create head to the Linked List

void insertNewCommand(char inputBuffer[])  /* function to store imputBuffer into the Linked List */
{
    historyIndex++;
    historyList * temp;
    if(head == NULL) /* malloc space for head */
    {
        head = (historyList *) malloc(sizeof(historyList));
    }
    temp = head;
    int j;
    while(temp->next != NULL) /* iterate until the last node in list */
    {
        temp = temp->next;
    }
    temp->index = historyIndex; /* define number of command */
    if(historyIndex > 10)       /* show up to 10 commands in the screen */
    {
        head = head->next;
    }
    for(j = 0; j < strlen(inputBuffer); j++) /* inserting command */
    {
        temp->command[j] = inputBuffer[j];
    }
    temp->next = (historyList *) malloc(sizeof(historyList)); /* malloc new space for next node */
    temp = temp->next;
    temp->next = NULL;
}


/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */
void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    /*------------------------insert inputBufffer into Linked List------------------------*/
    if(inputBuffer[0] != 'r' && inputBuffer[1] != ' ') /* r command should not be included into history list*/
        insertNewCommand(inputBuffer);
    /*---------------------------command added to the historyList---------------------------*/

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0)
    {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i=0; i<length; i++)
    {
        switch (inputBuffer[i])
        {
        case ' ':
        case '\t' :               /* argument separators */
            if(start != -1)
            {
                args[ct] = &inputBuffer[start];    /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;
        case '\n':                 /* should be the final char examined */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;
        default :             /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *background  = 1;
                start = -1;
                inputBuffer[i] = '\0';
            }
        }
    }
    args[ct] = NULL; /* just in case the input line was > 80 */
}

void handle_SIGQUIT() /* function to handle signal <Ctrl><\> */
{
    historyList * temp;
    temp = head;
    while(temp->next != NULL) /* printing historyList */
    {
        printf("%d %s",temp->index, temp->command);
        temp = temp->next;
    }
    nprompt++;
    printf("wbshell[%d]:\n ", nprompt); /* printing prompt here because function returns directly to setup */

}


int main(void)
{
    int status;
    char inputBuffer[MAX_LINE];  /* buffer to hold the command entered */
    int background;              /* equals 1 if a command is followed by '&' */
    char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */

    pid_t pid;                   /* store parent and child pid */
    int i = 0;

    /* set up the signal handler */
    struct sigaction handler;
    handler.sa_handler = handle_SIGQUIT;
    handler.sa_flags = SA_RESTART;
    sigaction(SIGQUIT, &handler, NULL);

    printf("Welcome to wbshell. My pid is %d.\n", getpid());
    while (1)             /* Program terminates normally inside setup */
    {
        background = 0;

        printf("wbshell[%d]:\n ", nprompt);
        setup(inputBuffer,args,&background);       /* get next command */

        if(strcmp(args[0], "r") == 0) /* r command - the implementation used does not support yell command*/
        {

            if(args[1] == NULL)       /* r executed without parameters - execute last command (!!) */
            {
                historyList * temp1 = head->next;
                while(temp1->index != historyIndex) /* iterate until last node/command */
                {
                    temp1= temp1->next;
                }
                system(temp1->command);     /* call system to execute command */
                insertNewCommand(temp1->command); /* insert command into history */
            }
            else /* if any argument has been passed to r command */
            {
                historyList * temp1 = head;
                char aux = *args[1]; /* get command number*/
                while(temp1->index != (aux - 48)) /* iterate until node/command specified by the index */
                {
                    temp1= temp1->next;
                }
                system(temp1->command);     /* call system to execute command */
                insertNewCommand(temp1->command); /* insert command into history */
            }
        }
        else if(strcmp(args[0], "yell") == 0) /* yell built-in command */
        {
            int i = 1;
            while(args[i]) /* iterate through all arguments passed */
            {
                int j = 0;
                while(args[i][j]) /* iterate through all chars in each argument */
                {
                    printf("%c", toupper(args[i][j]));
                    j++;
                }
                i++;
                printf(" ");
            }
            printf("\n");
        }
        else if(strcmp(args[0], "exit") == 0) /* exit built-in command */
        {
            char command[MAX_LINE];
            snprintf(command, MAX_LINE, "ps -p %d -o pid,ppid,pcpu,pmem,etime,user,command", getpid()); /* getting pid and concatenating it into command string*/
            system(command);
            exit(1);
        }
        else  /* fork a child process */
        {

            pid = fork();
            if (pid < 0)   /* error occurred */
            {
                fprintf(stderr, "Fork Failed.\n");
            }
            else if (pid == 0)   /* child process */
            {
                execvp(args[0], args);
            }
            else if (background == 0) /* run process in foreground - parent will wait*/
            {
                printf("[Child pid = %d, background = FALSE]\n", pid);
                /* parent will wait for the child to complete*/
                waitpid(pid, &status, 0);
                printf("Child complete.\n");
            }
            else                    /* run process in background - parent won't wait */
            {
                printf("[Child pid = %d, background = TRUE]\n", pid);
            }
        }
        nprompt++;
    }
}
