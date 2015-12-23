//David Blader
//260503611
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "sharedjobs.h"

int shmfd, shmid, BUFSIZE;
Shared_t *shared;
const char *path = "/my_shm";

void printmsg(Job *job){
	printf("Server is printing %d pages from Client %d\n", job->dur, job->ID);
}

Job *takejob(){
	Job *to_return;
	Job *j = shared->buffer;
	int i;
	to_return = malloc( sizeof(Job) );
	
	*to_return = j[0];

	for(i = 0; i < ( sizeof(shared->buffer) / sizeof(Job) - 1); i++){
		shared->buffer[i] = j[i+1];
	}
	
	return to_return;
}

void init_semaphores(){
	if(sem_init(&(shared->full), 1, 0) < 0) printf("Full Sem failed: %s", strerror(errno));

	if(sem_init(&(shared->empty), 1, BUFSIZE) < 0) printf("Empty Sem failed: %s", strerror(errno));

	if(sem_init(&(shared->mutex), 1, 1) < 0) printf("Mutex Sem failed: %s", strerror(errno));

}


void attach_sharedmem(){
	size_t SIZE = getpagesize();

	ftruncate(shmfd, sizeof(Job));
	shared = (Shared_t *) mmap(NULL, sizeof(Shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

  	if(shared == MAP_FAILED) {
	    printf("cons: Map failed: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
  	}

}

void setup_sharedmem(){
	
	shmfd = shm_open(path, O_CREAT | O_RDWR, 0666);
	if(shmfd == -1){
		printf("cons: Shared memory failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}	
}

int main(int argc, char *argv[]){
	Job *job;
	int test;
		if(argv[1] != NULL) BUFSIZE = atoi(argv[1]);
		else BUFSIZE = 10;
		
		setup_sharedmem();
	
		attach_sharedmem();

		init_semaphores();
		
		
		while(1){
			sem_getvalue(&(shared->full), &test);
			if(test <= 0){
				printf("Buffer Empty.\n");
			}

			sem_wait(&(shared->full)); 
			sem_wait(&(shared->mutex));
			
				job = takejob();
				printmsg(job);
				
			sem_post(&(shared->mutex));
			sem_post(&(shared->empty));
			
			sleep(job->dur);
		}
}