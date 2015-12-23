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

int jID, jDur, shmfd;
const char *path = "/my_shm";
Shared_t *shared;

void release_shm(){
	close(shmfd);
	munmap(shared, sizeof(Shared_t));
}

void attach_share_mem(){
	
	
	shmfd = shm_open(path, O_RDWR, 0666);

	if(shmfd == -1){
		printf("cons: Shared memory failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}	

	shared = (Shared_t *) mmap(0, sizeof(Shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	
	if(shared == MAP_FAILED) {
	    printf("cons: Client Map failed: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
  	}
}

int main(int arc, char *argv[]){
	Job *buf;
	int full_slots, test;

	jID = atoi(argv[1]);
	jDur = atoi(argv[2]);
	
	attach_share_mem();

	sem_getvalue(&(shared->empty), &test);
	if(test == 0) printf("Buffer full, Time 2 Chill B)\n");
	
	sem_wait(&(shared->empty));
	sem_wait(&(shared->mutex));
		//CRIT SECTION
		sem_getvalue(&(shared->full), &full_slots);

		buf = shared->buffer;

		buf[full_slots].ID = jID;
		
		buf[full_slots].dur = jDur;

		printf("\nClient %d requests a %d page job\n", jID, jDur);
		fflush(stdout);

	sem_post(&(shared->full));
	sem_post(&(shared->mutex));

	release_shm();
	
	return 0;
}