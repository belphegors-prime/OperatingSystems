#define MAXFILENAME 20

typedef struct dirEntry_t{
	char fname[MAXFILENAME+1];
	int inode;
} dirEntry_t;

typedef struct iNode_t{
	int id;
	int size;
	int nBlocks;
	int fPtrs[12];
	int indirectPtr;
} iNode_t;

typedef struct FilDes{
	int inode;
	int rwPtr;
} FilDes;


typedef struct superblock{
	int magic;
	int blockSize;
	int sfsSize;
	int iNodeTSize;
	int root;
} superblock;

void mksfs(int fresh);
int sfs_getnextfilename(char *fname);
int sfs_getfilesize(const char* path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fwrite(int fileID, const char *buf, int length);
int sfs_fseek(int fileID, int loc);
int sfs_remove(char *file);