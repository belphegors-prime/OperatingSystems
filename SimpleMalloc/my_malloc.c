//David Blader
//260503611
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "my_malloc.h"

int POLICY; /*0 is best fit, 1 is first fit*/

Block *block_list;
char my_malloc_error[100];
char mem_stats[200];

/*taken from Marwen Burelle's Malloc Tutorial
 *simply "divides" by 4 (>>2) and multiplies it back
 *(<<2) and then adds 4 to get the larger multiple of 4
 *that x falls between, -1 included for the case in which
 *an actual multiple of 4 is pass*/ 
int align4(int x){
	return (((x-1)>>2)<<2)+4;
}
void lower_break(Block *last_block){
	//puts("Decreasing program break\n");
	last_block->prev->next = NULL;
	last_block->prev->length -= last_block->length;
	if(brk(last_block) < 0){
		sprintf(my_malloc_error, "ERROR: something went wrong decreasing pbreak");
		puts(my_malloc_error);
	}
}

void display_contents(void *ptr){
	Block *iblock, *block_ptr = NULL;
	iblock = block_list;

	while(iblock){
		if(&(iblock->data) == ptr && iblock->used){
			block_ptr = iblock;
			break;
		}
		iblock = iblock->next;
	}

	if(block_ptr){
		char *buf = (char *) ptr;
		char content[block_ptr->length];
		for(int i = 0; i < block_ptr->length; i++){
			content[i] = buf[i];
		}
		puts(content);
	}else{
		sprintf(my_malloc_error,
			"ERROR: pointer passed to display_contents points to invalid memory location\n");
		puts(my_malloc_error);
	}
}

//always merge b2 into b1
void merge_blocks(Block *b1, Block *b2){
	if(b1->used == 1 || b2->used == 1){
		sprintf(my_malloc_error,
			"ERROR: at least one block passed to merge_blocks is tagged as used\n");
		puts(my_malloc_error);
		return;
	}

	b1->next = b2->next;
	if(b2->next) b2->next->prev = b1;
	
	b1->length += b2->length + BLOCK_SIZE;
}

void split_block(int size, Block *b){
	Block *left_overs;
	//leftovers starts where data location plus the requested size
	left_overs = (Block *) &(b->data) + size;
	left_overs->length = (unsigned int) b->length - size - BLOCK_SIZE;
	
	//link leftovers to the next block as well as the new one
	left_overs->next = b->next;
	left_overs->prev = b;
	if(b->next) b->next->prev = left_overs;
	left_overs->used = 0;
	b->length = size;
	b->next = left_overs;
}

void *my_malloc(int size){
	Block *iblock, *new_block, *last_block = NULL;
	Block *best_block = NULL;
	int best_fit = INT_MAX;
	if(size <= 0){
		sprintf(my_malloc_error,
		"ERROR: my_malloc requires a positive integer value as argument\n");

		puts(my_malloc_error);
		return NULL;
	}

	iblock = block_list;
	size = align4(size);
	/*----FIRST FIT----*/
	if(POLICY){
		while(iblock){
			if(size <= iblock->length && !(iblock->used)){
				iblock->used = 1;
				if(iblock->length - size >= BLOCK_SIZE + 4){
					split_block(size, iblock);
				}
				iblock->length = size;
				return (void *) &(iblock->data);
			}
			last_block = iblock;
			iblock = iblock->next;
		}
	}
	/*----BEST FIT----*/
	else if(!POLICY){
		while(iblock){
			/*must check that the block is best fit in addition to obvious conditions*/
			if(size <= iblock->length && !(iblock->used) && (iblock->length - size) < best_fit){
				best_fit = iblock->length - size;
				best_block = iblock;
			}
			last_block = iblock;
			iblock = iblock->next;
		}
		/*check to make sure we've actually found a block*/
		if(best_block != NULL){
			best_block->used = 1;
			if(best_block->length - size >= BLOCK_SIZE + 4){
				split_block(size, best_block);
			}
			best_block->length = size;
			return (void *) &(best_block->data);
		}

	}
	/*if we make it here then we must
	 *INCREASE PROGRAM BREAK*/
	new_block = (Block *) sbrk(0);

	if(new_block == (void *) -1 || sbrk(size + BLOCK_SIZE) == (void *) -1){
		sprintf(my_malloc_error, "ERROR: Could not adjust program break\n");
		puts(my_malloc_error);
		return NULL;
	}

	new_block->used = 1;
	new_block->length = size;

	/*if last block is NULL then we have an empty list
	 *otherwise append new block at the end of the list*/
	if(last_block != NULL)
	{
		last_block->next = new_block;
		new_block->prev = last_block;
	}

	else block_list = new_block;
	return (void *) &(new_block->data);
}

void my_free(void *ptr){
	Block *last_block, *block_ptr = NULL;
	Block *iblock = block_list;
	void *new_break;
	
	while(iblock){
		if(&(iblock->data) == ptr && iblock->used){
			block_ptr = iblock;
		}
		last_block = iblock;
		iblock = iblock->next;
	}
	
	/*make sure we were passed a valid pointer*/
	if(block_ptr == NULL){
		sprintf(my_malloc_error,
		"ERROR: pointer passed to my_free at invalid memory location\n");
		puts(my_malloc_error);
		return;
	}
	
	block_ptr->used = 0;
   	/*----MERGE IF NECESSARY----*/
	/*protect against segfaults by checking for NULL values
	 *bad style but w/e. also check if last block exceeds 128KB*/
	if(block_ptr->prev && block_ptr->next){
		if(block_ptr->prev->used == 0 && block_ptr->next->used == 0){
			merge_blocks(block_ptr, block_ptr->next);
			merge_blocks(block_ptr->prev, block_ptr);

			if(last_block->length >= MAX_EXCESS_MEM && last_block->used == 0)
				lower_break(last_block);
			return;
		}
	}
	if(block_ptr->prev){
		if(block_ptr->prev->used == 0){
			merge_blocks(block_ptr->prev, block_ptr);

			if(last_block->length >= MAX_EXCESS_MEM && last_block->used == 0)
				lower_break(last_block);			
			return;
		}
	}
	if(block_ptr->next){
		if(block_ptr->next->used == 0){
			merge_blocks(block_ptr, block_ptr->next);
			if(last_block->length >= MAX_EXCESS_MEM && last_block->used == 0)
				lower_break(last_block);
			return;
		}
	}
	if(last_block->length >= MAX_EXCESS_MEM && last_block->used == 0)
		lower_break(last_block);
}

void my_mallopt(int policy){
	POLICY = policy % 2;
}

void my_mallinfo(){
	int total_used_bytes = 0, total_free_bytes = 0, largest_free = 0, total_blocks = 0;
	Block *iblock = block_list;
	while(iblock){
		if(iblock->used == 1)
			total_used_bytes += iblock->length;
		else{
			if(iblock->length > largest_free)
				largest_free = iblock->length;
			total_free_bytes += iblock->length;
		}
		iblock = iblock->next;
		total_blocks++;
	}

	sprintf(mem_stats,
		"\nBytes Allocated: %d\nFree Bytes: %d\nLargest Free Space: %d\nTotal # Blocks: %d\n",
		total_used_bytes, total_free_bytes, largest_free, total_blocks);

	puts(mem_stats);
}
