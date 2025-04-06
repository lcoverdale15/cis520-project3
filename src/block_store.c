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
	uint8_t blocks[BLOCK_STORE_NUM_BLOCKS][BLOCK_SIZE_BYTES]; //2D array representing storage
	bitmap_t  *bitmap; //pointer to a bitmap keeping track of what blocks are used vs free
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
	block_store_t *bs = (block_store_t *)malloc(sizeof(block_store_t)); //allocating memory for the block
	if(bs == NULL) return NULL; //checking we allocated correctly

	memset(bs, 0, sizeof(block_store_t)); //setting all values in the block to 0
	
	bs->bitmap = bitmap_create(BLOCK_STORE_NUM_BLOCKS); //setting up the bitmap of the block
	if(bs->bitmap == NULL){ //checking that the bitmap was created correctly, if not, deallocate all allocated memory
		free(bs);
		return NULL;
	}

	bitmap_format(bs->bitmap, 0); //setting values in the bitmap to 0

	for(size_t i = 0; i < BITMAP_NUM_BLOCKS; i++){ //itteratting through the blocks in the bitmap
		bitmap_set(bs->bitmap, BITMAP_START_BLOCK + i); //setting the bitmap
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
	if(bs){ //if the block exists, destroy its bitmap and deallocate its memory
		bitmap_destroy(bs->bitmap);
		free(bs);
	}
}
/*
 This function finds the first free block in the block store and marks it as allocated in the bitmap.
  It returns the index of the allocated block or SIZE_MAX if no free block is available.
*/
size_t block_store_allocate(block_store_t *const bs)
{
	if(bs == NULL || bs->blocks == NULL || bs->bitmap == NULL){ //check that parameters were passed in correctly
		errno = EINVAL; //invalid argument
		return SIZE_MAX; //no free block available
	}
	
	for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){ //iterrate through the blocks
		if((i < BITMAP_START_BLOCK) || (i >= BITMAP_START_BLOCK + BITMAP_NUM_BLOCKS)){ //skipping blocks used to store the bitmap itself
			if(!bitmap_test(bs->bitmap, i)){ //check if the current value is 0
				bitmap_set(bs->bitmap, i); //mark it as used
				return i; //return newly allocated index
			}
		}
	}

	errno = ENOSPC; //no space to allocate to
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
	if(bs == NULL || block_id >= BLOCK_STORE_NUM_BLOCKS || bs->bitmap == NULL){ //Check that parameters were passed correctly
		return false;
	}

	if(block_id >= BITMAP_START_BLOCK && block_id < BITMAP_START_BLOCK + BITMAP_NUM_BLOCKS) return false; //check that the block_id is within acceptable bounds

	if(bitmap_test(bs->bitmap, block_id)){ //if set at the block_id, return false
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
			if(bs == NULL || bs->blocks == NULL || bs->bitmap == NULL){ //check for valid parameters
				return  ;
			}

			if(block_id >= BITMAP_START_BLOCK && block_id < BITMAP_START_BLOCK + BITMAP_NUM_BLOCKS){ //check that block_id is within acceptable range
				return;
			}

			//find the bit, reset it 
			// int bitmapIndex = block_id / (BLOCK_SIZE_BYTES * 8 -1);
			//uint8_t * bitmap = bs->bitmap[bitmapIndex];
			bitmap_reset(bs->bitmap, block_id);	
}
/*
*This function returns the number of blocks that are currently allocated in the block store. 
*It first checks if the pointer to the block store is not NULL and then uses the bitmap_total_set function to count the number of set bits in the bitmap
*/
size_t block_store_get_used_blocks(const block_store_t *const bs)
{

	if(bs == NULL || bs->bitmap == NULL) return SIZE_MAX; //check that the parameters were passed correctly

	// size_t used = 0; //count for total used blocks
	// for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){ //iterate through all blocks
	// 	if(bitmap_test(bs->bitmap, i)){ //test if the bit is set
	// 		used++; //increase the total used count
	// 	}
	// }
	return bitmap_total_set(bs->bitmap);
	// return used; //return the used count

}

/*
*This function returns the number of blocks that are currently free in the block store. It first checks if the pointer to the block store is not NULL and then calculates the 
*difference between the total number of blocks and the number of used blocks using the block_store_get_used_blocks and BLOCK_STORE_NUM_BLOCKS.
*/
size_t block_store_get_free_blocks(const block_store_t *const bs)
{
		if(bs == NULL) return SIZE_MAX; //check correct parameter passing
	    return BLOCK_STORE_NUM_BLOCKS - block_store_get_used_blocks(bs); //return the total number of blocks minus the amount of used blocks
}

//This function returns the total number of blocks in the block store, which is defined by BLOCK_STORE_NUM_BLOCKS.
size_t block_store_get_total_blocks()
{
	return BLOCK_STORE_NUM_BLOCKS; //return the total number of blocks
}

//This function reads the contents of a block into a buffer. It returns the number of bytes successfully read.
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
	if(bs == NULL || buffer == NULL || block_id >= BLOCK_STORE_NUM_BLOCKS){ //check that the parameters were passed correctly
		return 0;
	}

	memcpy(buffer, bs->blocks[block_id], BLOCK_SIZE_BYTES); //copy the from the block at block_id to the buffer for amount BLOCK_SIZE_BYTES
	return BLOCK_SIZE_BYTES; //return the amount copied
}

//This function writes the contents of a buffer to a block. It returns the number of bytes successfully written.
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{

	if(bs == NULL || buffer == NULL || block_id >= BLOCK_STORE_NUM_BLOCKS){ //check for valid parameters
		errno = EINVAL; //Invalid argument
		return 0;
	}
	memcpy(bs->blocks[block_id], buffer, BLOCK_SIZE_BYTES); //copy from the buffer to the block at index block_id for amount BLOCK_SIZE_BYTES

	return BLOCK_SIZE_BYTES; //return the amount copied
}
//This function deserializes a block store from a file. It returns a pointer to the resulting block_store_t struct.

block_store_t *block_store_deserialize(const char *const filename)
{
	if(filename == NULL) return NULL; //check that the filename was passed correctly

	block_store_t *bs = block_store_create(); //create a block store
	if(bs == NULL){ //check that the block store was created correctly
		return NULL;
	}

	int fd = open(filename, O_RDONLY); //open the file in readonly mode

	
	if(fd == -1){ //check that the file was opened correctly, if not destroy the block store
	perror("error opening file");
		block_store_destroy(bs);
		return NULL;
	}

	for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){ //Read one block of data from the file into block i.
		ssize_t bytes_read = read(fd, bs->blocks[i], BLOCK_SIZE_BYTES); //read from the file and store in the block at index i
		
		if(bytes_read != BLOCK_SIZE_BYTES){ //check that we read enough values, if not close the file and destroy the block
			close(fd);
		
			block_store_destroy(bs);
			return NULL;
		}

		for(size_t j = 0; j < BLOCK_SIZE_BYTES; j++){ //iterate through the blocks
			if(bs->blocks[i][j] != 0){ //check if the block contains any non zero byte and set as in use if found
				bitmap_set(bs->bitmap, i);
				break;
			}
		}
	}

	for(size_t i = 0; i < BLOCK_SIZE_BYTES; i++){ //iterate through the blocks
		bitmap_set(bs->bitmap, BITMAP_START_BLOCK + i); //mark bitmap storage as in use
	}

	close(fd);
	
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
	if(bs == NULL || bs->blocks == NULL || bs->bitmap == NULL|| filename == NULL){ //check that parameters were passed correctly
		return 0;
	}

	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); //open the file in write only: https://stackoverflow.com/questions/28466715/using-open-to-create-a-file-in-c
	
	if(fd == -1){ //check that file was opened correctly
			perror("error opening file");

		return 0;
	}

	for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){ //write one block of data into the file
		ssize_t bytes_written = write(fd, bs->blocks[i], BLOCK_SIZE_BYTES);

	
		
		if(bytes_written != BLOCK_SIZE_BYTES){ //check that the amount written matches what we expect, if not close the file
			close(fd);
			
			return 0;
		}
	}

	close(fd); //close the file
	
	return BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES; //size of file written in bytes

}
