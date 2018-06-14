# COMP 304 : Operating Systems
# Project : KU File System

This project is about creating a file system on a .txt file. Virtual disks can be created, mounted and unmounted. Moreover, each virtual disk can be written or read. In details, the file systems works on blocks of 1024 bytes, where blocks are assigned to files in the virtual disk and read by keepind track and updating these blocks. Detailed description can be found in the [project description](https://github.com/Matiatus/KU-File-System/blob/master/Project_Description.pdf).

## Getting Started

### How to Run the Project?: 

```
gcc -o kufs.o -c kufs.c
gcc test.c kufs.o
```
To run the project, first the library we’ve written has to be compiled with gcc command and -c flag. -o flag is used to set name of the output. Then the testing .c file should be compiled together with the compiled version of our library. Important thing to mention here is as we don’t change the directory, kufs.c , kufs.h and the testing .c file should be in the same directory. Again here -o flag can be used to set name to the final output. Then the output is run with the following command:
```
./<output name>
```

## Design of the Project:

### Data Structures
```c
BLOCK_SIZE = 1024
```
Block size in this project is 1024 bytes, so it is defined so at the beginning of the header file
```c
struct fatEntry { 
	char fileName[BLOCK_SIZE];
	int fileSize;	
	int fileBlocks[BLOCK_SIZE]; 
	int indexBlocks;
};
```
fatEntry, is the data structure of an entry in FAT. Each entry belongs to a file in the disk. 

It has a file name, which is kept in an array of characters, which is called filename

An array of integers, where each element is the number of the allocated blocks, which is called fileBlocks

An integer, which is the index of the next free cell in the array of allocated blocks, which is called indexBlocks
```c
struct fat { 
	int indexEntry;
	entry fatEntries[BLOCK_SIZE];
};
```
Then we have a data structure of FAT, which has an array of entries called fatEntries and an integer to keep track of the next free cell in that array, called indexEntry

### Global Variables
```c
fat * disk_node;
```
Pointer to the allocated memory to our data structure of fat. This pointer is updated upon initialization of that memory when kufs_mount() is called, and freed upon unmounting the virtual disk with kufs_umount().
```c
FILE* mountedFilePtr;
```
General C pointer to a file, which is going to be the virtual disk file we are going to work on. This pointer is assigned upon mounting a disk, used as a cursor on the disk file and moved according to the read or write operation, as needed; at the end it is used by kufs_umount() to close the disk file.
```c
fat fileFat;
```
A global variable of our fat data structure. This is updated when disk_node is assigned and all the work is done on it. Basically it is the data structure of fat, which is pointed by the disk_node.
```c
int openedFiles[BLOCK_SIZE]; 
```
Global array which is used to keep track of opened. Each element of index i, corresponds to the condition of the file with the file descriptor of i. If the element of index i, in openedFiles is 1, it means the file with file descriptor i is open. If the file, with file descriptor i, is close, then openedFiles[i] is 0.
```c
int filePositions[BLOCK_SIZE]; 
```
Global array to keep track of file_position of opened files. filePositions[i] correspond to the file_position of the file with the file descriptor of i.
```c
int blockAllocation[BLOCK_SIZE]; 
```
Global array keeping track of the block conditions of the virtual disk. Block with the number i, is allocated to a file if blockAllocation[i] is 1, is available and not allocated if blockAllocation[i] is 0 and if blockAllocation[i] is -1, this means that this block is not available in the disk as its number exceeds the number of block in the disk.

### Main Functions
```c
int kufs_create_disk(char* disk_name, int disk_size);
```
This function takes the name of the disk and the size of the disk to be created , and opens a file that will be used to perform the disk operations, filling it with null characters depending on the given disk_size. If an error occurs upon file creation, file close or file truncate operations, the method returns -1.
```c
int kufs_mount(char* disk_name);
```
This method takes the name of the disk, disk_name, to be mounted. First of all, memory allocation for the fat pointer type global variable disk_node is performed by malloc operation. After assigning it, the file with the name disk_name is opened and the content of the first block is obtained to create the FAT. Then , the number of blocks available for this disk is determined. In order to calculate the number of blocks that is available in this disk, the total size of the disk is determined by seeking to the end of the file by using fseek() method and getting the current position of the file by using ftell() function. The obtained size is divided by BLOCK_SIZE to obtain the number of blocks available for this disk. This value is held by the variable blockCount. Another variable, mountedFileSize, holds ceiling of it. This is the actual size of the file mounted, in terms of blocks. 

A globally declared array holding the files which are open, openedFiles, is filled with “0” since there is can be no files that are open before mounting is completed.  Indices of another globally declared array called blockAllocation that denotes the availability of the blocks, is filled with -1 if indices are greater than the number of blocks, mountedFileSize.  The reason behind this is that,  no more blocks than the mountedFileSize, can be allocated, so  the remaining of the array is set so. Also the first element of blockAllocation, with the index 0, is set to 1 because that block is allocated to FAT.

To create the FAT, first line of the disk is read since FAT is always written to first block of the disk. If there is no error at file read, that line is passed to lineToFAT() method to fill work with the line and fill the fat data structure that we have in the memory, which is pointed by  disk_node and declared as fileFat. Lastly, a variable called mountedFilePtr, that always points to the current location that we deal with, becomes equal to the beginning of the file. This will be used by other functions.
```c
int kufs_umount();
```
In order to unmount a disk, all the content of the FAT will be written to the disk. Before delegating writeFatToFile() method for writing the content of the FAT to disk, openedFiles array is checked to detect if there is a file that is currently open. If at least one file is open, a warning message is printed to express that there are open files so unmounting cannot be performed and also prints the names of the open files to the command line. Then, the method returns -1 to indicate that unmounting is not completed. 

If there are no files that are open, first writeFatToFile() method is called to write the content of the fat to first block of the disk. Then, the file used for the disk operations is closed. After that, the global variables used in file operations, such as openedFiles, blockAllocation and fat data structure called fileFat are cleaned by cleanMountInfo() method. Lastly, the memory allocated for the fat data structure, pointed by disk_node is freed. 
```c
int kufs_create(char* filename);
```
This method gets the filename of the file to be created. If the input is invalid, method returns -1 since creation operation cannot be performed. 

If the input is a valid input, then the filename is moved to a variable called newFileName. In order to create the file with the given filename, all of the entries currently present in FAT are checked to detect this filename is used or not. If there is a match between a filename that is created before and the newFileName, a warning message is printed and method returns -1 since creation operation cannot be performed. If there is no match, an entry with the given filename is created and added to fileFat. Then the next index available in the fileFat’s entries is incremented by 1.
```c
int kufs_open(char* filename);
```
Input of the method is the name of the file, filename,  that will be opened. In order to find the entry that holds the information of that file, fat data structure denoted by fatFile is traversed. If the file is found in one of the entries, index of the entry in the FAT is used to
change the status of the file as open by changing the content as “1” in openedFiles array that corresponds to the file we open. Here index of the file in the fileFat’s fatEntries, correspond to its related element’s index in openedFiles. 
Change the current position of file in file_positions array as “0” to denote that the current location we deal with is the beginning of the file. 
Return the index of the file in FAT.  

If the file with filename cannot be found,a warning message is printed and  -1 is returned. 

int kufs_close(int fd); 

For this method, fd denotes the index of the file in FAT.  In order to close a file, we first check if the file is currently open. If it is not open, a warning message is printed and method returns -1 since closing operation cannot be performed. 

Since index of a file in FAT, openedFiles and file_positions are the same, fd is used to change the status of the file as close by changing the content in openedFiles as “0”. Then, method returns 0 to indicate that closing operation is performed without an error. 
```c
int kufs_delete(char * filename);
```
In order to delete a file, index of the file in fileFat and openedFiles array is determined by traversing in fileFat and comparing each entry’s filename with the input filename. If the file is not present in disk, a warning message is printed and method returns -1 since deletion cannot be performed. If the given file is currently open, it cannot be deleted. That’s why,again the status of the file in openedFiles is checked and method returns -1 after a warning message is printed. 

After detecting the index of the file and ensuring that the file is not open, all the blocks allocated for that file is becoming available. In order to set the blocks available, the block numbers held by the fileBlocks array is used as the number stored in the array, correspond to the indices to be changed in blockAllocation array. Setting the indices in blockAllocation to “0”, we mark these block numbers are available for allocation.

Lastly, if the file that is deleted is not in the last index of fileFat, each remaining entry after the deleted one, is moved to one index ahead to the beginning, to keep the same order in the entry table.

Before returning, the integer in fileFat, indexEntry, which is showing the index of  next available element in fatEntries is decremented by 1, as we have decremented the total entry size by 1.

void kufs_dump_fat();  

This method traverses the fileFat variable to print the filename and allocated blocks of each file in the disk and listed in FAT. The output has the form of the given example at the project description.
```c
int kufs_write(int fd, void* buf, int n); 
```
Input validation: If bytes to write(input n) is less than 1 or if the file corresponding to the file descriptor fd is not open, it return -1.

When kufs_write is doing its work, an integer called writtenBytes is used to keep track of the amount of bytes written, so that it will be used to check if the work should be terminated or not. It will also be used to keep track of where we are when we are getting content from input buf. Each time a character is copied from input buf and added to the content we are working with, writtenBytes gets incremented by 1.

First, the offset aka file_position is obtained. Using the information of the entry, bytes allocated to the file is calculated, and remaining allocated usable bytes are called remainingBytes. 
There might be a block allocated but not fully filled, so if it is that case: 
The content existent in the block is copied to tempLine, and new contents gets appended to it. Content gets appended until that block is full or the aimed amount of bytes to write is reached.

When content is ready to write, the file pointer mountedFilePtr is seeked to the block’s position and the content gets written
Secondly, either there was no fully filled block or we filled it, now we will create block content from zero. Following processes will be done in a loop, until the number of bytes to write is reached.

The block to write is picked as follows:
•	If there is a already allocated and available block, it is used, this is done by having an integer called blockIndex and making it point to the first available allocated block in the entry’s fileBlocks array.
•	Otherwise a new block is allocated.
o	If there is no available block left to allocate, warning is printed and -1 is returned, after updating the file size.

After picking the block, a char array, writeLine is created and the content gets copied from input buf, continuing from where it was left. After the content is fully filled or the amount if bytes to write is completed, the content created is written to the block’s position with calling fseek() and fprintf(). 

When writing operation is completed, either the disk became full, no blocks left to allocate or amount of bytes to write is reached; file size is updated if needed, file_position (aka offset of the file) in the filePositions global array is updated and successfully written amount of bytes is returned. 
```c
int kufs_read(int fd, void* buf, int n);
```
Input validation: If bytes to read(input n) is less than 1 or if the file corresponding to the file descriptor fd is not open, it return -1. Then, an integer called bytesToRead is assigned which is the maximum bytes less than or equal to n, which can be read. This is calculated using n, file size and file_position. If there is not any possibility to read any bytes, it returns -1.

When kufs_read is doing its work, an integer called bytesRead  is used to keep track of the amount of bytes read, so that it will be used to check if the work should be terminated or not. It will also be used to keep track of where we are when we are writing content to the input buf. bytesRead is updated each time a single character is copied to the input buf. 

First, it is checked if the file_position is pointing to somewhere on the block which is not the beginning. If it is pointing to somewhere on the block, the content of the block is read to tempLine, then beginning from the place pointed by file_position, content of the block is copied to the input buf until bytesRead reaches bytesToRead.  This block become read totally so the integer containing the index of the block we are working, called the blockIndex, is incremented by 1.

Secondly, as now the point we are reading is beginning of a block, the followings are done in a for loop, until either the bytesRead reaches bytesToRead, or all the blocks have been traversed:
•	Block number is obtained from the allocated blocks of the entry.
•	Block content is read from the file.
•	Content is copied to the input buf.

Whenever kufs_read finishes its work, before returning bytesRead, which is the amount of bytes successfully read, a null character is added to the end of buf just like any string in C. Adding ‘\0’, to an input buffer of size 5 where the function is asked to read 5 bytes, can cause buffer overflow, which still done intentionally. The user using these functions should be careful about the buffer overflows just as he/she should be carefull with the standard C libraries.
```c
int kufs_seek(int fd, int n);
```
Input validation: If the file corresponding to the file descriptor fd is not open, it return -1. 

If the integer given n, is larger than the filesize, the file_position, which is the fd’th element in array filePositions is assigned to be the file size. As the size begins from 1 to exist but the file_position begins from 0, assigning it directly to the fil_size make it point to the end of file. 

Otherwise, the file_position is assigned to be n. As the file_position begins from 0, assigning it to n means making it point the end of n’th byte.

### Helper Functions
```c
int lineToFAT(char line[BLOCK_SIZE]); 
```
This function accepts a line, defined to be a char array of size BLOCK_SIZE. This line is the content read from the first block of the file. It  finds upto which index is the line meaningful as a FAT. After finding it, it passes that information to the parseLineToFAT() with the line itself. This will return o, if the disk’s Fat is successfully created or -1 otherwise, so this function return the return value of parseLineToFAT().
```c
int parseLineToFAT(char* line, int endIndex);
```
This function gets a line, which is the content of the first block in the disk and an endIndex, which is the index where the meaningful fat content ends. Beginning from index of 0, this function does the followings in a while loop, until the endIndex is reached:

•	File name of the entry is read from the line, which is the string from the current index to the first occurrence of ‘ ‘ (space).
•	File size of the entry is read, which is the string from the current index to to the next ‘ ‘ character.
•	File blocks allocated to the entry is read. This is done first by calculating the number of blocks to read from the file size of the entry and then reading that calculated amount of integers separated by ‘ ’.

If the given line is converted to the FAT format successfully the function return 0.
```c
void cleanMountInfo();
```
This function loops on global arrays of openedFiles and blockAllocation, and sets all their elements to zero. So the global arrays are cleaned. (filePositions array is assigned to zero after opening a file so cleaning it here is not necessary)
Then it traverses the entries of fileFat, and it cleans every entry’s information. This is not necessary as well as the memory allocation to fileFat will be free’d but we’ve still implemented as a matter of choice.
```c
void writeFatToFile();
```
This function first move the cursor of the file to the beginning and then create a string, fatString, to fill and and integer stringIndex, to keep track of the index of the first available element of that string. stringIndex is increased by one for each character added to the string.

Followings are done in a for loop for each entry:
•	Entry’s file name length is understood, by looping once and reaching ‘\0’ in it
•	Entry’s file name is copied to the fatString using name length.
•	Entry’s file size is copied to fatString,
•	Traversing based on the amount of blocks allocated, block numbers of the entry are added to the fatString.

When all the entries’ contents are added to fatString, it is written to the disk file and then the disk file is closed using mountedFilePtr.

No control mechanism is implemented here for checking whether stringIndex passes BLOCK_SIZE or not, the reason for that is in the discussion forum, it is stated as follows:
“...we will not consider the case where you write too many files 
such that the FAT will not fit in the first block”



More detailed explanations about the code can be found in the comments. 

## Deployment

### Built With

* [Sublime Text](https://www.sublimetext.com)

### Authors
* Mert Atila Sakaoğulları
* Pınar Topçam
