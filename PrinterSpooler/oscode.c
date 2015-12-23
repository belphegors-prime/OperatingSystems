//David Blader
//260503611
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

char *histList[10];

int histcnt, jobcnt;

/*struct Job {
    int pid;
    char *command[];
};

struct Job *createjob(int pid, char* command[]){
    struct Job *j = malloc( sizeof(struct Job) );
    j->pid = pid;
    j->command = malloc( sizeof(command));
    return j;
}
struct Job jobList[10];
*/
void runhistory()
{
    int i;
    for(i = 0; i < 10; i++){
        if(histcnt - i < 0) break;
        if(histList[(histcnt - i) % 10] != NULL)
            printf("%d. %s\n", histcnt -i + 1, histList[(histcnt - i) % 10]);
    }    

}

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    int j;
    int k;

    char *token, *loc;
    char *line, *backup;
    size_t linecap = 0;
    char *exitCMD = "<Control><D>";
    char c;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0) {
        exit(-1);
    }


    if(line[0] == 'r' && line[1] == ' '){
        c = line[2];
        for(k = 1; k <= 10; k++){
            if((histcnt - k) < 0) break;

            if(c == histList[(histcnt - k) % 10][0])
                line = histList[(histcnt -k) % 10];
        }
    }
    

    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;
    
    histList[histcnt % 10] = (char *) malloc( sizeof(line) );

    strcpy(histList[histcnt % 10], line);
    

    histcnt++;

    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    args[i++] = NULL;
   

    if(strcmp(args[0], exitCMD) == 0 || strcmp(args[0], "exit") == 0) exit(1);
    return i;
}


int main()
{
    char *args[20];
    char *oldcmd, *curDir, buf[PATH_MAX];
    char c, x;
    int bg, i, tmp, status, cnt;
    pid_t childPID;
    //struct Job *newjob;

    while(1){
        bg = 0;
        tmp = 0;
        status = 0;
        cnt = getcmd("\n>>  ", args, &bg);
        
        for (i = 0; i < cnt; i++)
            printf("\nArg[%d] = %s\n", i, args[i]);

        if(strcmp("history", args[0]) == 0) runhistory();
        
        else if(strcmp(args[0], "cd") == 0){
            if(chdir(args[1]) != 0)
                printf("Error Changing Directories.\nTry entering the absolute path.\n");
        
        }else if(strcmp(args[0], "pwd") == 0){
            curDir = getcwd(buf, PATH_MAX);
            printf("%s\n", curDir);

        }else if(strcmp(args[0], "jobs") == 0){


        }else if (bg){
            printf("\nBackground enabled..\n"); 
            childPID = fork(); 
            //parent process asks for next command
            if(childPID){
               // newjob = createjob(childPID, args);
                //jobList[jobcnt] = *newjob;
                //jobcnt++;
            	continue;
            }
            //child executes command
            else{
            	tmp = execvp(args[0], args);

                if(tmp < 0){
                    printf("Erroneous Command\n");
                }
            }
        }
        
        else
        {
            printf("\nBackground not enabled \n");
            childPID = fork();
            //parent waits
            if(childPID){
                oldcmd = histList[(histcnt-1)%10]; //store for safe keeping
                printf("histcnt: %d\n",histcnt);

                if(oldcmd == NULL) printf("error retrieving oldcmd\n");
                waitpid(childPID, &status, 0);
                
                if(status != 0){
                    printf("Erroneous Command\n");
                    histcnt--;
                    histList[histcnt%10] = oldcmd;
                }
                printf("\n\n");
            }
            else
            {
                execvp(args[0], args);
                exit(EXIT_FAILURE);
            }
        }
      }
    
    return 0;
}
