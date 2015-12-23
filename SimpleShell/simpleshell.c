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
#include <signal.h>

typedef struct Job {
    pid_t pid;
    char *command;
    struct Job *next;
} Job;

typedef struct jbLinkList {
	Job *first;
} jbLinkList;


jbLinkList jobList;
char *histList[10];
int histcnt;
char *oldcmd;
pid_t childPID;
Job *oldjob;

//for handling / detecting erroneous background cmds
void badcmd(){
	int status;
	waitpid(childPID, &status, WNOHANG);
	status = WEXITSTATUS(status);
	if(status != 0){
		printf("Erroneous Command\n");
		histcnt--;
		histList[histcnt % 10] = oldcmd;
	}
}

void createjob(int pid, char *command, Job *oldjob){
    Job *j = malloc( sizeof(struct Job) );
    j->pid = pid;
    j->command = command;
    j->next = oldjob;
    jobList.first = j;
}


void printjobs(){
	Job *j = jobList.first;	

	while(j != NULL){		
		printf("PID: %d, Job: %s\n", j->pid, j->command);
		if(j->next != NULL) j = j->next;
		else break;	
	}

}

void updatejobs(){
	int status;
	Job *old, *tmp;
	Job *j = jobList.first;
	old = NULL;

	while(j != NULL){
		//if waitpid() returns a nonzero value the job is done
		if(waitpid(j->pid, &status, WNOHANG)){
			//if not NULL we are at an internal node
			if(old != NULL){
				old->next = j->next;
			}
			//else we are at the first node, so update jobList so its first node
			//points to the next
			else{
				jobList.first = j->next;
			}
			tmp = j->next;
			free(j);
			j = tmp;
		}else{
			old = j;
			j = j->next;
		}	
	}
}


void runhistory(){
    int i;
    for(i = 0; i < 10; i++){
        if(histcnt - i < 0) break;
        if(histList[(histcnt - i) % 10] != NULL)
            printf("%d. %s\n", histcnt -i + 1, histList[(histcnt - i) % 10]);
    }    
}


int getcmd(char *prompt, char *args[], int *background){
    int length, i = 0;
    int j, k;
    char *token, *loc, *line;
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
   
    free(line);
    if(strcmp(args[0], exitCMD) == 0 || strcmp(args[0], "exit") == 0) exit(EXIT_SUCCESS);
    return i;
}

void fg(int ppid){
	int status;
	Job *j = jobList.first;	
	while(j != NULL){		
		if(j->pid == ppid){
			waitpid((pid_t) ppid, &status, 0);
			break;
		}else{
			j = j->next;
		}
	}
}

int main(){
    char *args[20];
    char *curDir, buf[PATH_MAX];
    int bg, i, status, cnt;
    

    jobList.first = NULL;

    signal(SIGCHLD, badcmd);

    while(1){
        bg = 0;
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
        	updatejobs();
        	printjobs();
		}
		else if(strcmp(args[0], "fg") == 0) fg(atoi(args[1]));
        
        else if (bg){
            printf("\nBackground enabled..\n"); 
            childPID = fork();

            //parent process asks for next command
            if(childPID){
                oldcmd = histList[histcnt-1 % 10];
                oldjob = jobList.first;
		        createjob(childPID, histList[histcnt-1 % 10], jobList.first);
            	continue;
            }

            //child executes command
            else{
            	execvp(args[0], args);
            	_Exit(EXIT_FAILURE);
            }
        }
        
        else{
            printf("\nBackground not enabled \n");
            childPID = fork();
            //parent waits
            if(childPID){
            	
                oldcmd = histList[(histcnt-1)%10];
        
                if(oldcmd == NULL) printf("error retrieving oldcmd\n");
                waitpid(childPID, &status, 0);
                
                if(status != 0){
                    printf("Erroneous Command\n");
                    histcnt--;
                    histList[histcnt%10] = oldcmd;
                }
            }
            else{
                execvp(args[0], args);
                _Exit(EXIT_FAILURE);
            }
        }
        printf("\n\n");
      }
    
    return 0;
}