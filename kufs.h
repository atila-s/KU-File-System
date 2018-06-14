/*
File: kufs.h
Developers: Mert Atila Sakaogullari, Pinar Topcam

COMP 304: Operating Systems
Project 3
Spring 2018
*/

#define BLOCK_SIZE 1024

struct fatEntry { 
	char fileName[BLOCK_SIZE];
	int fileSize;
	int indexBlocks; // shows next available index of the fileBlocks array
	int fileBlocks[BLOCK_SIZE];
};

typedef struct fatEntry entry;

struct fat { //fat structure, which is an array of entries and an integer showing next available place in the array
	int indexEntry; // shows next available index of the fatEntries array
	entry fatEntries[BLOCK_SIZE];
};

typedef struct fat fat;

fat* disk_node;

// Global Variables belonging to the currently mounted file
FILE* mountedFilePtr;
fat fileFat;
int openedFiles[BLOCK_SIZE]; // index of the file is the index of it's entry, 0 is closed, 1 is open
int filePositions[BLOCK_SIZE]; // file_positions of opened files, index of the file is the index of it's entry 
int blockAllocation[BLOCK_SIZE]; // Unallocated blocks are 0, allocated blocks are 1, unavailable ones are -1


int kufs_create_disk(char* disk_name, int disk_size);
int kufs_mount(char* disk_name);
int kufs_umount();
int kufs_create(char* filename);
int kufs_open(char* filename);
int kufs_close(int fd);
int kufs_delete(char * filename);
int kufs_write(int fd, void* buf, int n);
int kufs_read(int fd, void* buf, int n);
int kufs_seek(int fd, int n);
void kufs_dump_fat();

// Functions we've created:
int lineToFAT(char line[BLOCK_SIZE]); // Gets the first block of a file to convert to file Fat format
int parseLineToFAT(char* line, int endIndex); // Parses meaningfull part of the first block and creates File Fat
void writeFatToFile(); // Writes the file Fat to first block of the disk to unmount
void cleanMountInfo(); // Cleans two arrays and file Fat and also the information store in file Fat