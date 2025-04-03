#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


#include "bitmap.h"
#include "block_store.h"
#include <errno.h>



// include more if you need

struct block_store 
{
	uint8_t blocks[BLOCK_STORE_NUM_BLOCKS][BLOCK_SIZE_BYTES];
	bitmap_t  *bitmap;
	uint8_t *secondHalf; 
};
typedef enum { NONE = 0x00, OVERLAY = 0x01, ALL = 0xFF } BITMAP_FLAGS;

struct bitmap 
{
	unsigned leftover_bits;  // Packing will increase this to an int anyway 2 for second one
	BITMAP_FLAGS flags;	  // Generic place to store flags. Not enough flags to worry about width yet.
	uint8_t *data;
	size_t bit_count, byte_count;
};

// You might find this handy. I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.

/*
	This function creates a new block store and returns a pointer to it. 
	It first allocates memory for the block store and initializes it to zeros using the memset 
	(an alternative method to initialize newly-allocated memory to all 0s is to use calloc instead of malloc). 
	Then it sets the bitmap field of the block store to an overlay of a bitmap with size BITMAP_SIZE_BYTES on the 
	blocks starting at index BITMAP_START_BLOCK. 
	Finally, it marks the blocks used by the bitmap as allocated using the block_store_request function.
*/
block_store_t *block_store_create()
{
	block_store_t *bs = (block_store_t *)malloc(sizeof(block_store_t));
	if(!bs) return NULL;

	memset(bs, 0, sizeof(block_store_t));
	
	bs->bitmap = bitmap_create(BITMAP_SIZE_BYTES);
	if(bs->bitmap == NULL){
		free(bs);
		return NULL;
	}

	for(int i = 0; i < BITMAP_NUM_BLOCKS; i++){
		block_store_request(bs, BITMAP_START_BLOCK + i);
	}

	return bs;
}

/*
	This function destroys a block store by freeing the memory allocated to it. 
	It first checks if the pointer to the block store is not NULL, and if so, 
	it frees the memory allocated to the bitmap and then to the block store.
*/
void block_store_destroy(block_store_t *const bs)
{
	if(bs){
		bitmap_destroy(bs->bitmap);
		free(bs);
	}
}
/*remember, parameter validation, error checking, comments
 This function finds the first free block in the block store and marks it as allocated in the bitmap.
  It returns the index of the allocated block or SIZE_MAX if no free block is available.
*/
size_t block_store_allocate(block_store_t *const bs)
{
	if(bs == NULL || bs->blocks == NULL || bs->bitmap == NULL){
		//review this, unsigned 
		errno = ENOSPC; //No Space
		return SIZE_MAX;
	}
	bitmap_t *bitmap = bs->bitmap;
	int total_bits = BITMAP_NUM_BLOCKS * BLOCK_SIZE_BYTES * 8;
	//go over all the bitmaps
	for(int i = 0; i < total_bits; i++){
		if(bitmap_test(bitmap, i) == false){
			bitmap_set(bitmap, i);
			return i;
		}
	}
		errno = ENOSPC; //No free space
		return SIZE_MAX;
}

/*
	This function marks a specific block as allocated in the bitmap. 
	It first checks if the pointer to the block store is not NULL and if the block_id is within the range of valid block indices. 
	If the block is already marked as allocated, it returns false. 
	Otherwise, it marks the block as allocated and checks that the block was indeed marked as allocated by testing the bitmap. 
	It returns true if the block was successfully marked as allocated, false otherwise.
*/
bool block_store_request(block_store_t *const bs, const size_t block_id)
{
	if(bs == NULL || block_id >= BLOCK_STORE_NUM_BLOCKS || bs->bitmap == NULL){
		return false;
	}

	if(bitmap_test(bs->bitmap, block_id)){
		return false;
	}

	//find the first free block, set it to used, return true
	bitmap_set(bs->bitmap, block_id);
	return true;

}

/*This function marks a specific block as free in the bitmap. It first checks if the pointer to the block store is
 not NULL and if the block_id is within the range of valid block indices. Then, it resets the bit corresponding to 
 the block in the bitmap.
 */
void block_store_release(block_store_t *const bs, const size_t block_id)
{
			if(bs == NULL || bs->blocks == NULL || bs->bitmap == NULL){
				return  ;
			}
			if(!((block_id>=BITMAP_START_BLOCK+BITMAP_NUM_BLOCKS) || ((int)block_id<BITMAP_START_BLOCK))){
				return  ;
			}

			//find the bit, reset it 
			// int bitmapIndex = block_id / (BLOCK_SIZE_BYTES * 8 -1);
			//uint8_t * bitmap = bs->bitmap[bitmapIndex];
			bitmap_reset(bs->bitmap, block_id);
			return   ;
	
}
/*
*This function returns the number of blocks that are currently allocated in the block store. 
*It first checks if the pointer to the block store is not NULL and then uses the bitmap_total_set function to count the number of set bits in the bitmap
*/
size_t block_store_get_used_blocks(const block_store_t *const bs)
{

	if(bs == NULL) return SIZE_MAX;
	
	bitmap_t *bitmap = bs->bitmap;

	return  bitmap_total_set(bitmap);

}

/*
*This function returns the number of blocks that are currently free in the block store. It first checks if the pointer to the block store is not NULL and then calculates the 
*difference between the total number of blocks and the number of used blocks using the block_store_get_used_blocks and BLOCK_STORE_NUM_BLOCKS.
*/
size_t block_store_get_free_blocks(const block_store_t *const bs)
{
		if(bs == NULL) return SIZE_MAX;
	    return BLOCK_STORE_NUM_BLOCKS - block_store_get_used_blocks(bs);
}

//This function returns the total number of blocks in the block store, which is defined by BLOCK_STORE_NUM_BLOCKS.
size_t block_store_get_total_blocks()
{
	return BLOCK_STORE_NUM_BLOCKS;
}

//This function reads the contents of a block into a buffer. It returns the number of bytes successfully read.
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
	if(bs == NULL || buffer == NULL || block_id >= BLOCK_STORE_NUM_BLOCKS){
		return 0;
	}

	memcpy(buffer, bs->blocks[block_id], BLOCK_SIZE_BYTES);

	return BLOCK_SIZE_BYTES;
}

//This function writes the contents of a buffer to a block. It returns the number of bytes successfully written.
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{

	if(bs == NULL || buffer == NULL || block_id >= BLOCK_STORE_NUM_BLOCKS){
		errno = EINVAL; //Invalid argument
		return 0;
	}
	memcpy(bs->blocks[block_id], buffer, BLOCK_SIZE_BYTES);

	return BLOCK_SIZE_BYTES;
}
//This function deserializes a block store from a file. It returns a pointer to the resulting block_store_t struct.

block_store_t *block_store_deserialize(const char *const filename)
{
	//load back from filenam to block
	//open the file
	//create a block_store_t struct
	//read from the file, 
	//how to read from the file ? 
	//check if we need to pad it if it's sie %block_size has remainder
	//how do I know that padding is present ? 
	//read the file, and cll write while requesting bolck_store_write
	//and pass the block store we created, return the blockstore
	
	if(filename == NULL){
		    return 0;
		}
		block_store_t * bs = block_store_create();
		if(bs == NULL){
			perror("failed to create block store");
			return 0;
		}
		int fd = open(filename,O_RDONLY);
		if(fd == -1){
				perror("failed to open file");
				free(bs);
				return 0;
			}
		//read the file, while we haven't reached the end, request a block of memory
		//how to check for padding bytes ? 
			int block_id = block_store_allocate(block_store_t *const bs);
			void * buffer = malloc(sizeof(char)*32);
			read(fd,buffer,BLOCK_SIZE_BYTES);
				
			 block_store_read( bs,  block_id, buffer);

			

		return bs;
}


/*
*This function serializes a block store to a file. It returns the size of the resulting file in bytes.
* Note: If a test case expects a specific number of bytes to be written but your file is smaller, pad
* the rest of the file with zeros until the file is of the expected size. Modify your block_store_deserialize
* function accordingly to accept padding if present.
*/
size_t block_store_serialize(const block_store_t *const bs, const char *const filename)

{
	//go through blocks one by one
		if(bs == NULL || bs->blocks == NULL || bs->bitmap == NULL|| filename == NULL){
		//review this, unsigned 
		return 0;
			}
		int fileSize = 0;
		int fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC);
		if(errno){
				perror("failed to open file");
				return 0;
			}

			uint8_t *bitmap = bs->bitmap;
			//go over all the bitmaps
			for(int i = 0; i<BITMAP_NUM_BLOCKS ;i++){
				//go over every byte in a block 
				for(int byte = 0; byte < BLOCK_SIZE_BYTES;byte++){
				//go over every bit in the byte
					for(int bit = 0; bit <8 ; bit ++){
						uint8_t flag = 1;
						//shift the flag to the correct bit
						flag = flag << bit;
						//see if the bit is set
						uint8_t data = bitmap[byte] & flag;
						
						if(data !=0){
							printf("found an a located bit");
							//set the data
							char * buffer = malloc(sizeof(char)*32);
						
							block_store_read( bs,  (i *BLOCK_SIZE_BYTES * 8) + byte * 8 + bit, buffer);

							int x;
							//reposition read file offset, specifying the offset to offset bytes
							if((x = write(fd,buffer,BLOCK_SIZE_BYTES)) != BLOCK_SIZE_BYTES){
									//write the other blocks
									char * padding = calloc(sizeof(char),(BLOCK_SIZE_BYTES - x));
									write(fd,padding,BLOCK_SIZE_BYTES - x);
								
							}
							fileSize +=BLOCK_SIZE_BYTES;
							
						}
					}
		  }
			bitmap += BLOCK_SIZE_BYTES;	
		}
							close(fd);

			return fileSize;
	

}
