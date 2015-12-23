//David Blader
//260503611
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "my_malloc.h"

int MAX_MALLOCS = 10000;

int main(){
	printf("\nBLOCK_SIZE: %lu\n", BLOCK_SIZE);
	/*some basic operations using first fit*/
		my_mallopt(1);
		void *a =  my_malloc(10);
		void *b =  my_malloc(20);
		void *c =  my_malloc(30);
		char *d =  my_malloc(40);
		puts("\t1");
		my_mallinfo();
						/*we always round UP to the next multiple of four
						 *therefore, a gets 12 bytes, b gets 20, c 32, d 40
						 *total bytes used should then be equal to 104,
						 *freed bytes should be 0, and number of blocks is 4*/

		for(int i = 0; i < 10; i++)
			d[i] = 'A' + i;
		
		puts("Test Data: ");
		display_contents(d); /*should print letters A-J*/

		my_free(c);
		my_free(b);
		puts("\t2");
		my_mallinfo();	/*shows merging of blocks, freeing c frees not only the 32 bytes
						 *but also BLOCK_SIZE bytes (which should be 32). Should now have
						 *3 blocks with 20 + 32 + BLOCK_SIZE = 84 free bytes*/
		
		my_free(d);		
		my_free(a);
		puts("\t3");
		my_mallinfo(); /*we've freed all memory allocations. therefore we have 1 block,
						*0 bytes used, and 84 + 40 + 12 + 2*BLOCK_SIZE = 200 freed bytes*/

	/*showcase first fit example*/
		void *e =  my_malloc(40);
		void *f =  my_malloc(30);
		void *g =  my_malloc(20);
		void *h =  my_malloc(10);
		puts("\t4");
		my_mallinfo(); /*should be exactly the result of the first call to my_mallinfo*/
		
		my_free(e);
		my_free(h);
		puts("\t5");
		my_mallinfo(); /*freeing e and h relinquishes 40 + 12 = 52 bytes of memory
						*because they are not adjacent we still have 4 blocks*/
		
		void *i = my_malloc(4);
		puts("\t6");
		my_mallinfo();/*here we can see that rather than picking the better
					   *block previously pointed to by h, i will pick the first
					   *free block it sees (the block formerly pointed to by e),
					   *making our total number of blocks 5, since e splits.
					   *free bytes = 52 - BLOCK_SIZE - 4 = 16*/

		my_free(f);
		my_free(g);
		my_free(i);
		puts("\t7");
		my_mallinfo(); /*blocks have all merged back together to form 200 bytes of free space*/

	/*showcase best fit example*/
		my_mallopt(0);
		a =  my_malloc(40);
		b =  my_malloc(30);
		c =  my_malloc(20);
		d =  my_malloc(10);

		my_free(a);
		my_free(d);

		puts("\t8");
		my_mallinfo(); /*no surprises here. calling stats for
						*comparitive purposes with regard to the
						*next test*/

		e = my_malloc(4);
		puts("\t9");
		my_mallinfo(); /*here we see that e will pick the block formerly
						*pointed to by d. because d is not big enough to split
						*we still have 4 blocks. the block formerly pointed to by a
						*is the only free one so we have only 40 free bytes*/
		my_free(b);
		my_free(c);
		my_free(e);

	/*error handling*/
		my_malloc(-15);
		my_free(NULL);
		my_free(a); /*freeing something already freed*/ 

	/*stress test*/
		char *buf[MAX_MALLOCS];
		for(int i = 0; i < MAX_MALLOCS; i++){
			 buf[i] = my_malloc(512);
		}
		my_mallinfo();
		
		for(int j = MAX_MALLOCS-1; j >=0; j--){
			my_free(buf[j]);
		}
		
		my_mallinfo();

		return 1;
}