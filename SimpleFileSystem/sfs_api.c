#include "sfs_api.h"
#include "disk_emu.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h> 
#include <unistd.h>
#include <stdio.h> 

const int BLOCKSIZE = 512;
const int NUMBLOCKS = 2048;

int test, maxFiles, iNodeBlocks, maxiNode, nRootBlocks, maxFileSize;
char* FILENAME = "vDisk";
superblock *sb;
dirEntry_t *root;
FilDes *openfds;
iNode_t *inodeTable;
int *freeBlocks, fbSize;


void update_disk(){
	write_blocks(1, iNodeBlocks, inodeTable);
	write_blocks(iNodeBlocks, nRootBlocks, root);
	write_blocks(NUMBLOCKS - fbSize, fbSize, freeBlocks);
}

int next_free_block(){
	int i;
	/*start at nRootBlocks since all blocks
	 *preceding this index will be occupied *and unusable*/ 
	for(i = nRootBlocks; i < NUMBLOCKS - fbSize; i++){ 
		if(freeBlocks[i] == 0){ 
			freeBlocks[i] = 1;
			return i; 
		} 
	}
	return -1; 
} 
void load_freeblocks(){
	read_blocks(NUMBLOCKS - fbSize, fbSize, freeBlocks);
}

int find_next_avail_entry(){
	int i; 
	for(i = 0; i < maxFiles; i++){ 
		if(root[i].inode == -1){ 
			return i;
		}
	} 
	return -1; 
}

int find_next_avail_iNode(){ 
	int i;
	/*start at 1 since inodeTable[0] * will always contain root inode*/ 
	for(i = 1; i < maxiNode; i++){ 
		if(inodeTable[i].id == -1) 
			return i; 
	} 
	return -1; 
} 

int next_fd(){ 
	int i;
	for(i = 0; i < maxFiles; i++){ 
		if(openfds[i].inode == -1){
			return i;
		} 
	}
	return -1; 
}

void init_superblock(){ 
	sb = malloc(BLOCKSIZE); 
	sb->magic = 1234; 
	sb->blockSize = BLOCKSIZE; 
	sb->sfsSize = NUMBLOCKS; 
	sb->root = 0; 
	freeBlocks[0] = 1; 
	test = write_blocks(0, 1, sb);
	free(sb); 
} 

void load_inodetable(){
	read_blocks(1, iNodeBlocks, inodeTable);
}

void init_inodetable(){ 
	int i, inodeTableSize;
    //inode table takes up 1% of total space
	inodeTableSize = (int) (BLOCKSIZE * NUMBLOCKS * .01f);
	inodeTable = malloc(inodeTableSize);
    //store number of inode blocks and maximum number of inodes
	iNodeBlocks = inodeTableSize / BLOCKSIZE; 
	maxiNode = iNodeBlocks * (BLOCKSIZE / sizeof(iNode_t)); 
    //subtract 1 because the first inode is taken by the root directory 
	maxFiles = maxiNode - 1; 
    //store root directory inode at index 0 
	inodeTable[0].id = 0;
	inodeTable[0].size = maxFiles * sizeof(dirEntry_t); 

    //set up inodeTable as well as update free block list 
	for(i = 1; i < maxFiles; i++){ 
		inodeTable[i].id = -1; 
		if(i < iNodeBlocks) freeBlocks[i] = 1; 
     } 
}

void load_directory(){
	read_blocks(iNodeBlocks, nRootBlocks, root);
}

void init_directory(){ 
 	int i, j;
 	root = (dirEntry_t *) malloc(maxFiles * sizeof(dirEntry_t));
 	if(root == NULL){ 
 		printf("Error initializing directory table\n");
 	} 
 	nRootBlocks = ((maxFiles * sizeof(dirEntry_t)) / BLOCKSIZE) + 1;
 	/*max file size = number of direct pointer blocks + number of
 	 *indirect blocks * BLOCKSIZE*/
 	maxFileSize = (12 * BLOCKSIZE) + (BLOCKSIZE / sizeof(int)) * BLOCKSIZE;
 	for(i = 0; i < maxFiles; i++){ 
 		root[i].inode = -1;
 	}
 	j = 0;
 	for(i = iNodeBlocks; i < nRootBlocks; i++){
 		if(j < 12) inodeTable[0].fPtrs[j] = i;
 		freeBlocks[i] = 1;
 		j++; 
 	}
 }

void init_fdtable(){
 	int i;
 	openfds = malloc(maxFiles * sizeof(dirEntry_t)); 
 	for(i = 0; i < maxFiles; i++){
 		openfds[i].inode = -1;
 	} 
 }

void mksfs(int fresh) { 
 	if(fresh){ 
 		freeBlocks = (int *) malloc(NUMBLOCKS * sizeof(int));
 		fbSize = sizeof(freeBlocks) / BLOCKSIZE;
 		//printf("inode size = %d\n", sizeof(iNode_t)); 
 		init_fresh_disk(FILENAME, BLOCKSIZE, NUMBLOCKS);
 		init_superblock();
 		init_inodetable(); 
 		init_directory();
 		init_fdtable();

 		update_disk();
 	}else{ 
 		init_disk(FILENAME, BLOCKSIZE, NUMBLOCKS);
 		read_blocks(0, 1, sb);
 		load_inodetable();
 		load_directory();
 		load_freeblocks();
 	} 
 	return; 
 }
int find_first_entry(){
	int i;
	for(i = 0; i < maxFiles; i++){
		if(root[i].inode != -1) return i;
	}
	return -1;
}
int sfs_getnextfilename(char *fname) {
 	int i, j, ent;
 	//if passed null file name just retrieve first entry
 	if(fname == NULL){
 		ent = find_first_entry();
 		if(ent < 0){
 			printf("Directory is empty\n");
 			return 0;
 		}
 		strcpy(fname, root[ent].fname);
 		j = root[ent].inode;
 		return inodeTable[j].size;
 	}
 	for(i = 0; i < maxFiles; i++){ 
 		if(strcmp(root[i].fname, fname) == 0){ 
 			if(i != maxFiles - 1){ 
 				strcpy(fname, root[i+1].fname);
 				j = root[i+1].inode;
 				return inodeTable[j].size;
 			}else return 0; 
 		} 
 	} 
 	printf("File %s not found\n", fname);
 	return 0; 
 }

int sfs_getfilesize(const char* path) { 
 	int i, fileIndex;
 	for(i = 0; i < maxFiles; i++){ 
 		if(strcmp(path, root[i].fname) == 0){ 
 			fileIndex = root[i].inode; 
 			return inodeTable[fileIndex].size; 
 		} 
 	} 
 	printf("File %s not found\n", path); 
 	return 0; 
 }

int sfs_fopen(char *name) { 
 	int i, j, in ,fd, fsize, ent;
 	if(strlen(name) > MAXFILENAME){
 		printf("File name too long\n");
 		return -1;
 	}
 	for(i = 0; i < maxFiles; i++){ 
 		if(strcmp(root[i].fname, name) == 0){
 			in = root[i].inode;

  			//check if file is already opened 
 			for(j = 0; j < maxFiles; j++){ 
 				if(openfds[j].inode == in){ 
 					printf("File is already open\n"); 
 					return -1; 
 				}
 			}
 			fd = next_fd();
 			fsize = inodeTable[in].size;
 			openfds[fd].inode = in;
 			openfds[fd].rwPtr = fsize;
 			return fd;
 		}
      }

      //if not found in root directory, create the file
      //init inode entry 
      in = find_next_avail_iNode(); 
      if(in <= 0){ 
       //printf("Error allocating inode for file %s\n", name);
      	return -1; 
      } 
      inodeTable[in].id = in; 
      inodeTable[in].size = 0; 
      inodeTable[in].nBlocks = 0; 
      inodeTable[in].indirectPtr = -1; 
      for(i = 0; i < 12; i++){ 
      	inodeTable[in].fPtrs[i] = -1; 
      } 
      //init directory entry 
      ent = find_next_avail_entry(); 
      root[ent].inode = in; 
         
      strcpy(root[ent].fname, name); 
        
        //init file descriptor entry 
      fd = next_fd();
      openfds[fd].inode = in; 
      openfds[fd].rwPtr = 0; 
      return fd; 
  }

int sfs_fclose(int fileID){ 
  	if(openfds[fileID].inode == -1){ 
  		printf("File %d is already closed\n", fileID); 
  		return -1; 
  	} 
  	openfds[fileID].inode = -1; 
  	openfds[fileID].rwPtr = 0; 
  	return 0; 
  }

int sfs_fread(int fileID, char *buf, int length){ 
  	int j, k, l, index, rPtr, aBlockInd, zBlockInd, blxToRd, jBlock; 
  	int aIndirect, zIndirect, indirectBlk; 
  	int bytesRead; iNode_t inode; 
  	char *buf2, *jBuf, *indirectBuf; 
  	if(openfds[fileID].inode == -1){ 
  		printf("File %d is not opened\n", fileID); 
  		return -1; 
  	} 
  	index = openfds[fileID].inode; 
  	rPtr = openfds[fileID].rwPtr; 
  	inode = inodeTable[index]; 
  	/*if(rPtr + length > inode.size){ 
  		printf("Read request out of bounds\n");
  		printf("size: %d, rPtr: %d\n", inode.size, rPtr);
  		return -1; 
  	} */
         /*start block will be whichever the file's pointer is currently located on 
         *last block is the block which contains file's pointer + length*/ 
  	aBlockInd = rPtr / BLOCKSIZE; 
  	zBlockInd = (rPtr + length) / BLOCKSIZE; 
  	blxToRd = (zBlockInd - aBlockInd) + 1;

  	buf2 = (char *) malloc((blxToRd * BLOCKSIZE));
  	jBuf = (char *) malloc(BLOCKSIZE);
  	

  	indirectBuf = malloc(BLOCKSIZE);

  	bytesRead = 0;
  	l = 0;
         //READ IS CONTAINED IN DIRECT FILE POINTERS 
  	if(aBlockInd < 12 && zBlockInd < 12){
         //file's data does not have to be sequential on disk must go 
         //through each file pointer and read each block individually 
  		for(j = aBlockInd; j <= zBlockInd; j++){
  			jBlock = inode.fPtrs[j]; 

  			if(jBlock < 0){
  				continue;
  			}
  			test = read_blocks(jBlock, 1, jBuf);
			  			
			if(test < 0){			
  				printf("Error reading file %d\n", fileID); 
  				test = 0;
  			}
  			/*make my own implementation of strcat
  			 *because strcat gets grumpy with me*/
  			k = 0;
  			while(jBuf[k] != '\0'){
  				buf2[l] = jBuf[k];
  				k++;
  				l++;
  			}
  		}
       //READ IS PARTIALLY DIRECT PARTIALLY INDIRECT
     }

     else if (aBlockInd < 12){  
         //READ FROM DIRECT BLOCKS 
     	for(j = aBlockInd; j < 12; j++){ 
     		jBlock = inode.fPtrs[j];
     		if(jBlock < 0){
  				continue;
  			}
     		test = read_blocks(jBlock, 1, jBuf);
     		 
     		if(test < 0){			
     			printf("Error reading file %d\n", fileID); 
     			test = 0;
     		}
     		k = 0;
  			while(jBuf[k] != '\0'){
  				buf2[l] = jBuf[k];
  				k++;
  				l++;
  			}
     	}

		//READ FROM INDIRECT BLOCKS
     	indirectBlk = inode.indirectPtr;
     	test = read_blocks(indirectBlk, 1, indirectBuf);
     	if(test < 0 || indirectBlk < 0){
     		printf("Error reading file %d's indirect pointer block\n", fileID);
     		test = 0;
     	}
		//for each desired data block pointed to indirectly
     	for(j = 0; j <= zBlockInd - 12; j++){
		//find its address
     		indirectBuf += j * sizeof(int);
     		jBlock = (int) *indirectBuf;
     		indirectBuf -= j * sizeof(int);
     		if(jBlock < 0){
  				continue;
  			}
		//retrieve from disk
     		read_blocks(jBlock, 1, jBuf);
     		k = 0;
  			while(jBuf[k] != '\0'){
  				buf2[l] = jBuf[k];
  				k++;
  				l++;
  			}
     	}

	//READ ENTIRELY ON INDIRECT BLOCK
     }else{
     	indirectBlk = inode.indirectPtr;
     	aIndirect = aBlockInd - 12;
     	zIndirect = zBlockInd - 12;
     	read_blocks(indirectBlk, 1, indirectBuf);

     	for(j = aIndirect; j <= zIndirect; j++){
     		indirectBuf += j * sizeof(int);
     		jBlock = (int) *indirectBuf;
     		indirectBuf -= j * sizeof(int);
     		if(jBlock < 0){
  				continue;
  			}
     		read_blocks(jBlock, 1, jBuf);
     		k = 0;
  			while(jBuf[k] != '\0'){
  				buf2[l] = jBuf[k];
  				k++;
  				l++;
  			}
     	}
     }
   	/*set read pointer to be relative to block*/
    rPtr = rPtr % BLOCKSIZE;
  	for(j = 0; j < length; j++){
		buf[j] = buf2[rPtr];
		rPtr++;
		bytesRead++;
		if(bytesRead >= inode.size) break;
	}
	//free buffers and update pointer
    free(buf2);
    free(jBuf);
    free(indirectBuf);
    openfds[fileID].rwPtr += length;
    return bytesRead;
}

int sfs_fwrite(int fileID, const char *buf, int length){
	int index, wPtr;
	int aBlockInd, zBlockInd, blxToAlloc, freeBlock;
	int aIndirect, zIndirect, indirectBlk;
	int j, jBlock, jPtr, k;
	int bytesWrit;
	iNode_t inode;
	char *jBuf;
	char *indirectBuf;
	//void* ptr;
	if(openfds[fileID].inode == -1){
		printf("File %d is not opened\n", fileID);
		return -1;
	}
	index = openfds[fileID].inode;
	if(index <= 0){
		printf("invalid inode index\n");
		return -1;
	}
	wPtr = openfds[fileID].rwPtr;
	if(wPtr + length > maxFileSize){
		printf("Write request exceeds maximum file size\n");
		return -1;
	}
	inode = inodeTable[index];
	
	/*start block will be whichever the file's pointer is currently located on
	 *last block is the block which contains file's pointer + length*/
	aBlockInd = wPtr / BLOCKSIZE;
	zBlockInd = (wPtr + length) / BLOCKSIZE;

	/*buffers for storing data blocks and indirect block*/
	jBuf = (char *) malloc(BLOCKSIZE);	
	indirectBuf = malloc(BLOCKSIZE);

	if(jBuf == NULL || indirectBuf == NULL){
		printf("malloc error while writing to file %d\n", fileID);
	}

	/*================ALLOCATE BLOCKS IF NECCESSARY================*/
		if(zBlockInd >= inode.nBlocks){
			/*inode.nBlocks = last block index + 1 so if zBlockInd = 2 and nBlocks = 2
			 *then we need to increase nBlocks to 3*/
			blxToAlloc = zBlockInd - inode.nBlocks + 1;
			//buffer to store indirect block data
			indirectBlk = inode.indirectPtr;
			//check if we need to init the inode's indirect block pointer
			if(inode.nBlocks + blxToAlloc >= 12 && indirectBlk == -1){
				indirectBlk = next_free_block();
				if(indirectBlk == -1) printf("Error allocating indirect block\n");
				else inode.indirectPtr = indirectBlk;
			}
			//begin allocating blocks
			for(j = inode.nBlocks; j <= zBlockInd; j++){
				freeBlock = next_free_block();
				if(freeBlock < 0){
					printf("well shit\n");
				}
				if(j < 12) inode.fPtrs[j] = freeBlock;
				
				else{
					//store freeblock pointer to indirect block
					indirectBlk = inode.indirectPtr;
					test = read_blocks(indirectBlk, 1, indirectBuf);
					if(test < 0){
						printf("Error retrieving indirect block\n");
						test = 0;
					}
					//COPY FREE BLOCK TO BUFFER
					indirectBuf += (j - 12) * sizeof(int);
					memcpy(indirectBuf, &freeBlock, sizeof(int));
					indirectBuf -= (j - 12) * sizeof(int);
					test = write_blocks(indirectBlk, 1, indirectBuf);
					if(test < 0){
						printf("Error writing to indirect pointer block\n");
						test = 0;
					}
				}
			}
			//update the number of blocks
			inode.nBlocks += blxToAlloc;
		}

	/*================BEGIN WRITING TO DISK================*/
	/*want to express write pointer relative to the block it is located in*/
	jPtr = wPtr % BLOCKSIZE;
	k = 0; 					//index for looping through write buf
	bytesWrit = 0;			//return value

	//WRITE IS CONTAINED IN DIRECT FILE POINTERS
	if(aBlockInd < 12 && zBlockInd < 12){
		//read in block by block
		for(j = aBlockInd; j <= zBlockInd; j++){
			jBlock = inode.fPtrs[j];
			test = read_blocks(jBlock, 1, jBuf);
			if(test < 0){
				printf("Error reading in block %d for writing\n", jBlock);
				test = 0;
			}
			//copy buffer into jBuf
			for(; jPtr < BLOCKSIZE; jPtr++){
				if(k < length){
					//copy buffer into jBuf
					jBuf[jPtr] = buf[k];
					k++;
					bytesWrit++;
				}
				/*once we've exhausted buf we
				 *can break from this loop*/
				else break;
			}
			jPtr = 0; //reset jPtr
			write_blocks(jBlock, 1, jBuf);
			if(k == length) break;
		}
	//WRITE IS PARTIALLY DIRECT PARTIALLY INDIRECT
	}else if(aBlockInd < 12){
		//WRITE PORTION OF BLOCKS POINTED TO BY FILE POINTERS
		for(j = aBlockInd; j < 12; j++){
			jBlock = inode.fPtrs[j];
			read_blocks(jBlock, 1, jBuf);
			/*note we do not check if k < length here it can be assumed k is less than length
			 *otherwise we are looking through indirect ptrs*/
			for(; jPtr < BLOCKSIZE; jPtr++){
				jBuf[jPtr] = buf[k];
				k++;
				bytesWrit++;
			}
			jPtr = 0;
			write_blocks(jBlock, 1, jBuf);
		}
		//WRITE PORTION OF BLOCKS STORED IN INDIRECT POINTER
		indirectBlk = inode.indirectPtr;
		read_blocks(indirectBlk, 1, indirectBuf);
		for(j = 0; j <= zBlockInd - 12; j++){
			//get the j-th integer in the indirect block & store in jBlock
			indirectBuf += j * sizeof(int);
			jBlock = (int) *indirectBuf;
			indirectBuf -= j * sizeof(int);
			//read from jBlock into jBuf
			read_blocks(jBlock, 1, jBuf);
			for(jPtr = 0; jPtr < BLOCKSIZE; jPtr++){
				if(k < length){
					jBuf[jPtr] = buf[k];
					k++;
					bytesWrit++;
				}else break;
			}
			write_blocks(jBlock, 1, jBuf);
			if(k == length) break;
		}
	//WRITE IS CONTAINED ON INDIRECT BLOCK
	}else{
		/*the indices of the first and last blocks with respect to the start
		 *of the indirect block*/
		aIndirect = aBlockInd - 12;
		zIndirect = zBlockInd - 12;

		indirectBlk = inode.indirectPtr;
		read_blocks(indirectBlk, 1, indirectBuf);

		for(j = aIndirect; j <= zIndirect; j++){

			indirectBuf += j * sizeof(int);
			jBlock = (int) *indirectBuf;
			indirectBuf -= j * sizeof(int);
			read_blocks(jBlock, 1, jBuf);
			for(; jPtr < BLOCKSIZE; jPtr++){
				if(k < length){
					jBuf[jPtr] = buf[k];
					k++;
					bytesWrit++;
				}else break;
			}
			jPtr = 0;
			write_blocks(jBlock, 1, jBuf);
			if(k == length) break;
		}
	}

	if(wPtr + length > inode.size){
		inode.size = wPtr + length;
	}

	free(jBuf);
	free(indirectBuf);
	openfds[fileID].rwPtr += length;
	inodeTable[index] = inode;
	update_disk();
	return bytesWrit;
}

int sfs_fseek(int fileID, int loc){
	int i;
	iNode_t inode;
	i = openfds[fileID].inode;
	inode = inodeTable[i];

	if(loc < 0 || loc > inode.size){
		printf("Requested location is out of file bounds\n");
		return -1;
	}
	openfds[fileID].rwPtr = loc;
	return 0;
}

int sfs_remove(char *file) {
	int i, index;
	int j, jBlock, indirectBlk;
	iNode_t inode;
	//void *eraser;
	char *indirectBuf = (char *) malloc(BLOCKSIZE);
	char *zero = (char *) calloc(1, BLOCKSIZE);
	//\m/ Seek & Destroy \m/
	for(i = 0; i < maxFiles; i++){
		if(strcmp(root[i].fname, file) == 0){
			
			index = root[i].inode;
			inode = inodeTable[index];
			
			//clear root directory entry
			bzero(&(root[i].fname), MAXFILENAME);
			root[i].inode = -1;

			//ready indirect ptr block if one exists
			indirectBlk = inode.indirectPtr;
			if(indirectBlk > 0) write_blocks(indirectBlk, 1, indirectBuf);
			//clear all data blocks
			for(j = 0; j < inode.nBlocks; j++){
				if(j < 12){
					jBlock = inode.fPtrs[j];
					if(jBlock < 0 || jBlock >= NUMBLOCKS){
						continue;
					}
					write_blocks(jBlock, 1, zero);
					inode.fPtrs[j] = -1;
				}else{
					indirectBuf += (j - 12) * sizeof(int);
					memcpy(&jBlock, indirectBuf, sizeof(int));
					indirectBuf -= (j - 12) * sizeof(int);
					if(jBlock < 0 || jBlock >= NUMBLOCKS){
						continue;
					}
					write_blocks(jBlock, 1, zero);
				}
				//clear free block list
				freeBlocks[jBlock] = 0;
			}
			
			inode.id = -1;
			inode.nBlocks = 0;
			inode.size = 0;
			inode.indirectPtr = -1;

			inodeTable[index] = inode;

			update_disk();
		}
	}
	printf("File %s not found\n", file);
	return -1;
}
