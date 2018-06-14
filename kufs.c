/* 
File: kufs.c
Developers: Mert Atila Sakaogullari, Pinar Topcam

COMP 304: Operating Systems
Project 3
Spring 2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include <unistd.h>
#include <string.h>

#include "kufs.h"

int kufs_create_disk(char* disk_name, int disk_size){
	FILE* fp = fopen(disk_name, "w+");
	//fprintf(fp, "%s", "file.txt 3000 2 3 4 ");
	if(fp == NULL){ // fopen() failed
		return -1;
	} 
	int errorFlag = ftruncate(fileno(fp), BLOCK_SIZE * disk_size);
	if(errorFlag == -1){ // ftruncate() failed
		return -1;
	}
	errorFlag = fclose(fp);
	if(errorFlag != 0){ // fclose() failed
		return -1;
	}
    return 0; // nothing failed => run successfull
}

int kufs_mount(char* disk_name){
	
	//printf("MOUNT CALLED\n");

	disk_node = (fat*) malloc(sizeof(fat));
	fileFat = *disk_node;

	FILE* fp = fopen(disk_name, "r+");

	//printf("FILE OPENED\n");
	if(fp == NULL){ // fopen() failed
		return -1;
	}

	int errorFlag = fseek(fp, 0, SEEK_END); // seek to end of file
	if(errorFlag == -1){ // fseek() failed
		return -1;
	}
	long int size = ftell(fp); // get current file pointer
	if(size == -1){ // ftell() failed
		return -1;
	}
	errorFlag = fseek(fp, 0, SEEK_SET); // seek back to beginning of file
	if(errorFlag == -1){ // fseek() failed
		return -1;
	}
	double blockCount = (double) size / BLOCK_SIZE;
	int mountedFileSize = ceil(blockCount);

	/* Update two arrays of opened files and unallocated blocks
	----------------------------------------------------------- */
	blockAllocation[0] = 1; //First block allocated to FAT itself
	for(int i=0;i<BLOCK_SIZE;i++){
		openedFiles[i] = 0; // No files are open now
		if(i  > mountedFileSize - 1){
			blockAllocation[i] = -1; // Unavailable blocks are marked
		} 
	}

	char line[BLOCK_SIZE];
	fseek(fp, 0, SEEK_SET );
	errorFlag = fread(line, 1, BLOCK_SIZE, fp);

	if(errorFlag == -1){ // fread() failed
		return -1;
	}
	//printf("Create FAT from: %s\n", line);
	//printf("LINE: %s", line);
	errorFlag = lineToFAT(line); // CREATE FILE FAT

	if(errorFlag == -1){ // lineToFAT() faced an error
		return -1;
	}
	mountedFilePtr = fp;
	return 0;
}

int kufs_create(char* filename){
	/* Get the char pointer of filename into a char array to work with
	------------------------------------------------------------------- */
	int nameLength = -1;
	for(int i=0; i<BLOCK_SIZE; i++){
		if(filename[i] == '\0'){
			nameLength = i;
			break;
		}
	}
	if(nameLength == -1){ // Invalid file name given
		return -1; 
	}

	char newFileName[nameLength];
	for(int i=0; i< nameLength; i++){
		newFileName[i] = filename[i];
	}

	/* Traverse the entries to find if the given file name is in use 
	------------------------------------------------------------------- */
	int fatLength = fileFat.indexEntry;
	for(int i=0; i<fatLength; i++){// check if any of the entry's file name is the same
		entry* tempEntry = &fileFat.fatEntries[i];
		for(int j=0; j<nameLength; j++){ 
			if(tempEntry->fileName[j] != newFileName[j]){
				break;
			} else if(j == nameLength-1 && tempEntry->fileName[j] == newFileName[j]){
				printf("\nWARNING: Filename is in use\n");
				return -1; //File already exists
			}
		}
	}

	/* We know that the filename is not in use, so we add entry to file FAT
	---------------------------------------------------------------------- */
	for(int i=0; i<nameLength; i++){
		fileFat.fatEntries[fatLength].fileName[i] = newFileName[i];
	}
	fileFat.indexEntry++; 
	return 0;
}

int kufs_open(char* filename){
	/* Get the char pointer of filename into a char array to work with
	------------------------------------------------------------------- */
	int fatLength = fileFat.indexEntry;
	for(int i=0; i<fatLength; i++){
		entry* tempEntry = &fileFat.fatEntries[i];
		if(strcmp(filename, (const char*) &tempEntry->fileName) == 0){
			openedFiles[i] = 1;
			filePositions[i] = 0;
			return i;
		}
	}
	printf("\nWARNING: Given file name to open cannot be found\n");
	return -1;
}

int kufs_close(int fd){
	if(openedFiles[fd] != 1){
		printf("\nWARNING: File is not open");
		return -1;
	}
	openedFiles[fd] = 0;
	return 0;
}

int kufs_delete(char* filename){
	int index = -1;
	//printf("\nDELETING FILE: %s\n", filename);

	int fatLength = fileFat.indexEntry;
	for(int i=0; i<fatLength; i++){
		entry* tempEntry = &fileFat.fatEntries[i];
		if(strcmp(filename, (const char*) &tempEntry->fileName) == 0){
			index = i;
		}
	}

	//printf("\nindex to delete: %d\n", index);
	if(index == -1){
		printf("\nWARNING: File not existen in disk\n");
		return -1;
	} else if(openedFiles[index] == 1){
		printf("\nWARNING: File to delete is open\n");
		return -1;
	}

	entry* tempEntry = &fileFat.fatEntries[index];

	for(int i=0; i<tempEntry->indexBlocks; i++){
		int blockNumber = tempEntry->fileBlocks[i];
		blockAllocation[blockNumber] = 0;
	}
	tempEntry->indexBlocks = 0;

	int lastEntry = fileFat.indexEntry -1;
	if(index != lastEntry){
		// Loop over the entries and move the entries after the deleted one to one forward, keeping the order
		for(int i=index; i<lastEntry; i++){
			fileFat.fatEntries[i] = fileFat.fatEntries[i+1];
		}
	}
	fileFat.indexEntry--;
	return 0;
}

int lineToFAT(char line[BLOCK_SIZE]){
	/* Cleans the firt block from unused characters and seperates the part needed to be parsed
	------------------------------------------------------------------------------------------ */
	int fatEnd = -1;
	for(int i=0; i< BLOCK_SIZE-1; i++){
		printf("Searching fat end at : %c\n", line[i]);
		if(line[i] == '\0' && line[i+1]=='\0'){
			fatEnd = i;
			break;
		}
	}
	//printf("FAT END FOUND : %d\n", fatEnd);
	if(fatEnd == -1){
		return -1;
	} else if(fatEnd == 0){ // No FAT generated
		fileFat.indexEntry = 0; // Empty fat, where every entry place is available
		return 0;
	} else { 
		//printf("PARSE UNTIL : %d\n", fatEnd);
		return parseLineToFAT(line, fatEnd);
	}
}

int parseLineToFAT(char* line, int endIndex){
	/* Parses the given line in a loop, working on every character in three parsing operations
	------------------------------------------------------------------------------------------ */
	fileFat.indexEntry = 0;
	int i = 0;
	while(i < endIndex){
		/* First parse is the file name
		-------------------------------*/
		int index = fileFat.indexEntry;
		entry* tempEntry = &fileFat.fatEntries[index];
		tempEntry->indexBlocks = 0;
		int nameLength = -1;
		for(int j=i; j<BLOCK_SIZE; j++){
			if(line[j] == ' '){
				nameLength = j;
				break;
			}
		}
		//printf("nameLength end: %d, whereas i is %d\n", nameLength, i);
		for(int j=i; j< BLOCK_SIZE+i; j++){
			if(j < nameLength){
				tempEntry->fileName[j-i] = line[j];
			} else {
				tempEntry->fileName[j-i] = '\0';
			}
		}
		i=nameLength;
		//printf("File name inserted: %s\n", tempEntry->fileName);
		/* Second parse is the file size in bytes
		-----------------------------------------*/
		int sizeEnd = -1;
		for(int j=i+1; j<BLOCK_SIZE; j++){
			if(line[j] == ' '){
				sizeEnd = j;
				break;
			}
		}
		//printf("sizeEnd end: %d\n", sizeEnd);
		char tempfileSize[sizeEnd-nameLength-1];
		for(int j=nameLength+1; j< sizeEnd; j++){
			tempfileSize[j-nameLength-1] = line[j];
		}
		int fileSizeBytes = atoi(tempfileSize);
		tempEntry->fileSize = fileSizeBytes;
		i = sizeEnd + 1;

		//printf("File size inserted: %d, i became %d\n", tempEntry->fileSize, i);

		/* Third parse is the allocated blocks
		-------------------------------------- */
		double blockCount = (double) fileSizeBytes / BLOCK_SIZE;

		int neededBlocks = ceil(blockCount);
		//printf("needed blocks: %d\n", neededBlocks);
		int blockEnd ;
		for(int j=0; j<neededBlocks;j++){
			for(int k=i; k<BLOCK_SIZE; k++){
				if(line[k] == ' '){
					blockEnd = k;
					break;
				}
			}
			char tempBlock[blockEnd-i];
			for(int k=i; k<blockEnd; k++){
				tempBlock[k-i] = line[k];
			}
			//printf("block end: %d\n", blockEnd);
			i=blockEnd + 1;
			int blockNumber = atoi(tempBlock);
			//printf("added block: %d\n", blockNumber);
			blockAllocation[blockNumber] = 0;
			tempEntry->fileBlocks[tempEntry->indexBlocks] = blockNumber;
			tempEntry->indexBlocks++;
		}
		fileFat.indexEntry++;
		//printf("FILE FAT INDEEEEEEEX : %d \n", fileFat.indexEntry);
	}
	return 0;
}

int kufs_umount(){
	/* To unmount a disk, all the files must be closed
	--------------------------------------------------------- */
	for(int i=0; i<BLOCK_SIZE; i++){
		if(openedFiles[i] != 0){
			printf("\nWARNING: Cannot unmount, open files remaining:");
			// Printinf the file names of opened files
			for(int j=i; j<BLOCK_SIZE; j++){
				if(openedFiles[j] != 0){
					printf(" %s", fileFat.fatEntries[j].fileName);
				}
			}
			printf("\n");
			return -1;
		}
	}

	/* Now that there is no open file
	--------------------------------------------------------- */
	writeFatToFile(); // Write current File Fat to the file before closing
	fclose(mountedFilePtr); // Close file 
	cleanMountInfo(); // Clean Global Variables 
	free(disk_node);
	return 0;
}

int kufs_read(int fd, void* buf, int n){
	if(n < 1){ // invalid input
		return 0;
	}

	/* To read the file, it must be open
	----------------------------------- */
	if(openedFiles[fd] == 0){
		printf("\nWARNING: Cannot read, file is not open\n");
		return -1;
	}

	char* bufPtr = (char *)buf; // for usability
	
	/* First, adjust parameters and check the maximum amount of bytes readable
	--------------------------------------------------------------------------- */
	int bytesToRead = n;
	entry* tempEntry = &fileFat.fatEntries[fd];
	int filePosition = filePositions[fd];
	if(filePosition + bytesToRead > tempEntry->fileSize){ // cannot read any more bytes than this
		bytesToRead = tempEntry->fileSize - filePosition;
		printf("\n Bytes to read has beed decreased to: %d", bytesToRead);
	}
	if(bytesToRead == 0){ // no byte is readable so return immediately
		return 0;
	}

	int bytesRead = 0; // count to keep track of bytes read
	int allocatedBytes = tempEntry->indexBlocks * 1024;
	int remainingBytes = allocatedBytes - filePosition;
	int allocatedBlockCount = tempEntry->indexBlocks;
	int blockIndex = tempEntry->indexBlocks - (remainingBytes / 1024);

	/* Second, if the file_position is in a block, begin from it and read the block to the end
	---------------------------------------------------------------------------------------- */
	if(remainingBytes > 0 && (remainingBytes % 1024 != 0) ){
		blockIndex--;
		int blockNumber = tempEntry->fileBlocks[blockIndex];
		char tempLine[BLOCK_SIZE];
		fseek(mountedFilePtr, blockNumber * 1024, SEEK_SET);
		fgets(tempLine, BLOCK_SIZE + 1, mountedFilePtr);
		//printf("\n kufs_read : READ CURRENT LINE: %s\n", tempLine);

		for(int i=(filePosition % 1024); i<BLOCK_SIZE; i++){
			bufPtr[bytesRead] = tempLine[i];
			bytesRead++;
			if(bytesRead == bytesToRead){ // if enough bytes are read, return;
				bufPtr[bytesRead] = '\0'; // can cause buffer overflow (intentionally), aim is to display properly
				return bytesRead;
			}
		}
		blockIndex++;
	}

	/* Lastly, keep reading the block contents until either enough bytes are read or all the blocks have been traversed
	------------------------------------------------------------------------------------------------------------------- */
	for(int i=blockIndex; i<allocatedBlockCount; i++){
		int blockNumber = tempEntry->fileBlocks[i];
		char tempLine[BLOCK_SIZE];
		fseek(mountedFilePtr, blockNumber * 1024, SEEK_SET);
		fgets(tempLine, BLOCK_SIZE + 1, mountedFilePtr);
		for(int j=0; j<BLOCK_SIZE; j++){
			bufPtr[bytesRead] = tempLine[j];
			bytesRead++;
			if(bytesRead == bytesToRead){
				bufPtr[bytesRead] = '\0'; // can cause buffer overflow (intentionally), aim is to display properly
				return bytesRead;
			}
		}
	}
	bufPtr[bytesRead] = '\0'; // can cause buffer overflow (intentionally), aim is to display properly
	return bytesRead;
}

int kufs_write(int fd, void* buf, int n){
	if(n < 1){ // Invalid input
		return 0;
	}

	/* To write the file, it must be open
	----------------------------------- */
	if(openedFiles[fd] == 0){
		printf("\nWARNING: Cannot write, file is not open\n");
		return -1;
	}

	char* bufPtr = (char *)buf; // for usability

	//printf("\nLine received: %s where offset is :%d \n", bufPtr, filePositions[fd]);

	/* First, we find the offset of the file to write, and the entry to write
	------------------------------------------------------------------------ */
	int bytesToWrite = n;
	entry* tempEntry = &fileFat.fatEntries[fd];
	int filePosition = filePositions[fd];
	int allocatedBytes = tempEntry->indexBlocks * 1024;
	//printf("\aAllocatedBytes: %d\n", allocatedBytes);
	int remainingBytes = allocatedBytes - filePosition;

	int writtenBytes = 0; // keeps track of the number of bytes written
	int allocatedBlockCount = tempEntry->indexBlocks;
	int blockIndex = tempEntry->indexBlocks - (remainingBytes / 1024);
	//printf("\nRemainingBytes: %d\n",remainingBytes);

	/* If there is a non-full but also a not empty block; we first fill that block
	--------------------------------------------------------------------------------- */
	if(remainingBytes > 0 && (remainingBytes % 1024 != 0) ){ 
		blockIndex = tempEntry->indexBlocks - (1 + remainingBytes / 1024); // index of that block
		int blockContentEnd = filePosition % 1024; // end index of the block's existing content
		//printf("\nblockContentEnd: %d\n",blockContentEnd);
		int blockNumber = tempEntry->fileBlocks[blockIndex]; 

		fseek(mountedFilePtr, blockNumber * 1024,SEEK_SET); // move the cursor to the block
		char tempLine[blockContentEnd];
		fgets(tempLine, blockContentEnd + 1, mountedFilePtr); // read the block content
		//printf("\nREAD CURRENT LINE: %s , with content end index : %d\n", tempLine, blockContentEnd);

		char writeLine[BLOCK_SIZE+1]; // create string to write to the block
		for(int i=0; i<blockContentEnd; i++){
			writeLine[i] = tempLine[i]; // copy the existing content up to it's end
		}

		for(int i=blockContentEnd; i<BLOCK_SIZE; i++){
			writeLine[i] = bufPtr[writtenBytes]; // write new content of the buffer
			writtenBytes++; // increase amount of written bytes by 1
			if(writtenBytes == bytesToWrite){
				for(int k=i+1; k<BLOCK_SIZE-1; k++){ // fill the remaining of the string by null
					writeLine[k] = '\0';
				}
				writeLine[BLOCK_SIZE] = '\0';
				//printf("\nWriting filled Line: %s\n", writeLine);
				fseek(mountedFilePtr, blockNumber * 1024, SEEK_SET); // move the cursor back to the block
				int temp = fprintf(mountedFilePtr, "%s", writeLine); // write the string generated
				//printf("\n SUCCESFULLY : %d\n", temp);
				if(filePosition + writtenBytes > tempEntry->fileSize){ // adjust file size if needed 
					tempEntry->fileSize = filePosition + writtenBytes;
				}
				filePositions[fd] += writtenBytes; // update file_position (offset)
				return writtenBytes;
			}
		}
		//printf("\nBlock Full-ed: Writing Line: %s\n", writeLine);
		fseek(mountedFilePtr, blockNumber * 1024, SEEK_SET); // move the cursor back to the block
		int awdawd = fprintf(mountedFilePtr, "%s", writeLine); // write the string generated
		//printf("\n SUCCESFULLY : %d\n", awdawd);

		blockIndex++; // increase the block index after the current block is full
	}

	/* Loop until the last byte is written: create content and write it block by block 
	---------------------------------------------------------------------------------- */
	while(writtenBytes < bytesToWrite){ 
		int blockToWrite;
		/* If present use the allocated block, otherwise allocate new block 
		------------------------------------------------------------------------------ */
		if(blockIndex < allocatedBlockCount){ 
			blockToWrite = tempEntry->fileBlocks[blockIndex];
			blockIndex++;
		} else{ // allocate new block
			blockToWrite = -1;
			for(int i=0; i<BLOCK_SIZE; i++){ // find unallocated block
				if(blockAllocation[i] == 0){
					blockAllocation[i] = 1;
					blockToWrite = i;
					tempEntry->fileBlocks[tempEntry->indexBlocks] = i;
					tempEntry->indexBlocks++;
					break;
				}
			}
			if(blockToWrite == -1){ // no available block left
				printf("\nWARNING: No allocable block left, refuse to write\n");

				if(filePosition + writtenBytes > tempEntry->fileSize){ // adjust the filesize if needed
					tempEntry->fileSize = filePosition + writtenBytes;
				}
				filePositions[fd] += writtenBytes; // update the file_position
				return writtenBytes; //return the amount of bytes written
			}
		}
		/* Create content to write to the block
		---------------------------------------- */
		char writeLine[BLOCK_SIZE+1]; //create string as content to the block
		for(int i=0; i<BLOCK_SIZE; i++){ // get the character from the buffer
			writeLine[i] = bufPtr[writtenBytes];
			writtenBytes++;
			if(writtenBytes == bytesToWrite){ // if all the bytes are copied to string
				for(int k=i+1; k<BLOCK_SIZE; k++){ // fill the remaining of string as null
					writeLine[k] = '\0';
				}
				writeLine[BLOCK_SIZE] = '\0';
				//printf("\nWriting complete Line of size %lu is : %s\n",sizeof(writeLine), writeLine);
				fseek(mountedFilePtr, blockToWrite * 1024, SEEK_SET); // move cursor to the block
				fprintf(mountedFilePtr, "%s", writeLine); // write string to the block

				if(filePosition + writtenBytes > tempEntry->fileSize){ // adjust the filesize if needed
					tempEntry->fileSize = filePosition + writtenBytes;
				}
				filePositions[fd] += writtenBytes; // update file_position
				return writtenBytes; // return amount of bytes written
			}
		}
		//printf("\nWriting Line and moving on: %s\n", writeLine);
		fseek(mountedFilePtr, blockToWrite * 1024, SEEK_SET);
		fprintf(mountedFilePtr, "%s", writeLine);
	}

	if(filePosition + writtenBytes > tempEntry->fileSize){ // adjust the filesize if needed
		tempEntry->fileSize = filePosition + writtenBytes;
	}
	filePositions[fd] += writtenBytes; // update file_position
	return writtenBytes; // return amount of bytes written
}

int kufs_seek(int fd, int n){
	/* To seek the file_position of a file, it must be open
	------------------------------------------------------- */
	if(openedFiles[fd] == 0){
		printf("\nWARNING: Cannot seek, file is not open\n");
		return -1;
	}

	int seekedFileSize = fileFat.fatEntries[fd].fileSize;
	if(n > seekedFileSize){ // if n is bigger than file size return file size
		filePositions[fd] = seekedFileSize;
		return seekedFileSize;
	}
	filePositions[fd] = n;
	return n;
}

void cleanMountInfo(){
	/* PART A : Clean two arrays of opened files and block allocations, with other global variables
	---------------------------------------------------------------------------------------------- */
	for(int i=0; i<BLOCK_SIZE; i++){
		openedFiles[i] = 0;
		blockAllocation[i] = 0;
	}
	mountedFilePtr = NULL;

	/* PART B : Clean File FAT
	------------------------------------------------------------------- */
	int tempFatIndex = fileFat.indexEntry;
	for(int i=0; i<tempFatIndex; i++){
		entry* tempEntry= &fileFat.fatEntries[i];
		tempEntry->fileSize = 0;
		char unmountName[BLOCK_SIZE];
		for(int k=0; k<BLOCK_SIZE; k++){
			tempEntry->fileName[k] = unmountName[k];
		}
		int tempEntryIndex = tempEntry->indexBlocks;
		for(int j=0; j<tempEntryIndex; j++){
			tempEntry->fileBlocks[j] = 0;
		}
		tempEntry->indexBlocks = 0;
	}
	fileFat.indexEntry = 0;
}

void writeFatToFile(){
	/* Move the cursor to the beginning of the file to write on the first block
	--------------------------------------------------------------------------- */
	int entryCount = fileFat.indexEntry;
	fseek(mountedFilePtr, 0, SEEK_SET);
	char fatString[BLOCK_SIZE];
	int stringIndex = 0;

	/* Traversing on each entry, adds up the final string to be written as file Fat
	------------------------------------------------------------------------------- */
	for(int i=0; i<entryCount; i++){
		entry* tempEntry = &fileFat.fatEntries[i];
		int nameLength;
		for(int j=0; j<BLOCK_SIZE; j++){
			if(tempEntry->fileName[j] == '\0'){
				nameLength = j;
				break;
			}
		}
		//printf("entry file name : %s\n", tempEntry->fileName);
		
		/* First: Gets the filename of the entry
		---------------------------------------- */
		for(int j=0; j<nameLength; j++){
			fatString[j+stringIndex] = tempEntry->fileName[j];
		}
		fatString[stringIndex+nameLength] = ' ';
		stringIndex = stringIndex + nameLength + 1;
		
		/* Second: Gets the fileSize of the entry
		---------------------------------------- */
		int length = snprintf( NULL, 0, "%d", tempEntry->fileSize)+1;
		char sizeString[length];
		snprintf(sizeString, length, "%d", tempEntry->fileSize);

		for(int j=0; j<length; j++){
			fatString[stringIndex+j] = sizeString[j];
		}
		stringIndex = stringIndex + length;
		fatString[stringIndex-1] = ' ';

		/* Third: Gets the blocks allocated in the entry
		----------------------------------------------- */
		int blockCount = tempEntry->indexBlocks;
		for(int k=0; k<blockCount; k++){
			int blockNumber = tempEntry->fileBlocks[k];
			length = snprintf( NULL, 0, "%d", blockNumber) + 1;
			char blockString[length];
			snprintf(blockString, length, "%d", blockNumber);
			for(int j=0; j<length; j++){
				fatString[stringIndex+j] = blockString[j];
			}
			stringIndex = stringIndex + length;
			fatString[stringIndex-1] = ' ';
		}
	}
	/* String to write as FAT is ready, packs it up and writes to the file
	--------------------------------------------------------------------- */
	fatString[stringIndex-1] = ' ';
	for(int k=stringIndex; k<BLOCK_SIZE; k++){
		fatString[k] = '\0';
	}
	//printf("FAT STRING: %s\n", fatString);
	
	fprintf(mountedFilePtr, "%s", fatString);
	fclose(mountedFilePtr);
}

void kufs_dump_fat(){
	int fatLength = fileFat.indexEntry;
	for(int i=0; i<fatLength; i++){
		entry* tempEntry = &fileFat.fatEntries[i];
		printf("%s:", tempEntry->fileName);
		int blockCount = tempEntry->indexBlocks;
		for(int j=0; j<blockCount; j++){
			printf(" %d", tempEntry->fileBlocks[j]);
		}
		printf("\n");
	}
}