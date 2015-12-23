typedef struct Block{
	unsigned int length;
	int used;
	struct Block *next;
	struct Block *prev;
	void *data;
}Block;

#define BLOCK_SIZE sizeof(Block)
#define MAX_EXCESS_MEM (1<<17) /*max free excess memory = 128 kB = 2^17*/
extern char my_malloc_error[100];