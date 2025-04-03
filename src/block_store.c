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
	if(bs == NULL) return NULL;

	memset(bs, 0, sizeof(block_store_t));
	
	bs->bitmap = bitmap_create(BLOCK_STORE_NUM_BLOCKS);
	if(bs->bitmap == NULL){
		free(bs);
		return NULL;
	}

	bitmap_format(bs->bitmap, 0);

	for(size_t i = 0; i < BITMAP_NUM_BLOCKS; i++){
		bitmap_set(bs->bitmap, BITMAP_START_BLOCK + i);
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
		errno = EINVAL; //invalid argument
		return SIZE_MAX;
	}
	
	for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){
		if((i < BITMAP_START_BLOCK) || (i >= BITMAP_START_BLOCK + BITMAP_NUM_BLOCKS)){
			if(!bitmap_test(bs->bitmap, i)){
				bitmap_set(bs->bitmap, i);
				return i;
			}
		}
	}

	errno = ENOSPC;
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

	if(block_id >= BITMAP_START_BLOCK && block_id < BITMAP_START_BLOCK + BITMAP_NUM_BLOCKS) return false;

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

			if(block_id >= BITMAP_START_BLOCK && block_id < BITMAP_START_BLOCK + BITMAP_NUM_BLOCKS){
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

	if(bs == NULL || bs->bitmap == NULL) return SIZE_MAX;

	size_t used = 0;
	for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){
		if(bitmap_test(bs->bitmap, i)){
			used++;
		}
	}

	return used;

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
	if(filename == NULL) return NULL;

	block_store_t *bs = block_store_create();
	if(bs == NULL){
		return NULL;
	}

	int fd = open(filename, O_RDONLY);
	if(fd == -1){
		block_store_destroy(bs);
		return NULL;
	}

	for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){
		ssize_t bytes_read = read(fd, bs->blocks[i], BLOCK_SIZE_BYTES);
		if(bytes_read != BLOCK_SIZE_BYTES){
			close(fd);
			block_store_destroy(bs);
			return NULL;
		}

		for(size_t j = 0; j < BLOCK_SIZE_BYTES; j++){
			if(bs->blocks[i][j] != 0){
				bitmap_set(bs->bitmap, i);
				break;
			}
		}
	}

	for(size_t i = 0; i < BLOCK_SIZE_BYTES; i++){
		bitmap_set(bs->bitmap, BITMAP_START_BLOCK + i);
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
	if(bs == NULL || bs->blocks == NULL || bs->bitmap == NULL|| filename == NULL){
		//review this, unsigned 
		return 0;
	}

	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd == -1){
		return 0;
	}

	for(size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++){
		ssize_t bytes_written = write(fd, bs->blocks[i], BLOCK_SIZE_BYTES);
		if(bytes_written != BLOCK_SIZE_BYTES){
			close(fd);
			return 0;
		}
	}

	close(fd);
	return BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES;

}
