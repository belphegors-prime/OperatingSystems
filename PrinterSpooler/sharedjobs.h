#ifndef SHAREDJOBS_H
#define SHAREDJOBS_H
typedef struct{
	int ID;
	int dur;
} Job;

typedef struct{
	sem_t full, empty, mutex;
	Job buffer[10];
} Shared_t;
#endif