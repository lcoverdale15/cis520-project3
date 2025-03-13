#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bitmap.h"
#include "block_store.h"
// include more if you need

struct block_store 
{
	uint8_t blocks[BLOCK_STORE_NUM_BLOCKS][BLOCK_SIZE_BYTES];
	uint8_t  *bitmap;
	uint8_t *secondHalf; 
};


// You might find this handy. I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

block_store_t *block_store_create()
{
	block_store_t *bs = (block_store_t *)malloc(sizeof(block_store_t));
	if(!bs) return NULL;

	memset(bs, 0, sizeof(block_store_t));
	
	bs->bitmap = (uint8_t *)bs->blocks[BITMAP_START_BLOCK];

	for(int i = 0; i < BITMAP_NUM_BLOCKS; i++){
		block_store_request(bs, BITMAP_START_BLOCK + i);
	}

	return bs;
}

void block_store_destroy(block_store_t *const bs)
{
	if(bs){
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
		return SIZE_MAX;
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
				uint8_t new_data = bitmap[byte] & flag;
				//there are only 510 available storage blocs
				// if(i == BITMAP_NUM_BLOCKS-1 && byte == BLOCK_SIZE_BYTES-1 && bit ==6){
				// 	return SIZE_MAX;
				// }
				if(new_data ==0){
					//set the data
					new_data = bitmap[byte] | flag;
					bitmap[byte] = new_data;
					// printf("%d\n",(i * BLOCK_SIZE_BYTES * 8) + byte * 8 + bit);
					return (i *BLOCK_SIZE_BYTES * 8) + byte * 8 + bit;
				}
			}
		}
			bitmap += BLOCK_SIZE_BYTES;	
		}
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

	size_t byte_index = block_id / 8;
	size_t bit_index = block_id % 8;
	uint8_t mask = 1 << bit_index; //creating bitmask at posisiton bit_index
	

	if((bs->bitmap[byte_index] & mask) != 0){
		return false;
	}

	bs->bitmap[byte_index] |= mask;

	return (bs->bitmap[byte_index] & mask);
	//find the first free block, set it to used, return that index

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
			if((block_id>=BITMAP_START_BLOCK+BITMAP_NUM_BLOCKS) || ((int)block_id<BITMAP_START_BLOCK)){
				return  ;
			}

			//find the bit, reset it 
			// int bitmapIndex = block_id / (BLOCK_SIZE_BYTES * 8 -1);
			//uint8_t * bitmap = bs->bitmap[bitmapIndex];
			uint8_t * bitmap = bs->bitmap;
			
			size_t byte_index = block_id / 8;
			size_t bit_index = block_id % 8;
			   uint8_t  flag = 1;
			//shift the flag to the correct position
			flag = flag << bit_index;
			//flip the flag
			flag = ~flag;
			//reset the bit to zero
			uint8_t new_data = bitmap[byte_index] & flag;
			bitmap[byte_index] = new_data; 
			return   ;
	
}

size_t block_store_get_used_blocks(const block_store_t *const bs)
{
	UNUSED(bs);
	return 0;
}

size_t block_store_get_free_blocks(const block_store_t *const bs)
{
	UNUSED(bs);
	return 0;
}

//This function returns the total number of blocks in the block store, which is defined by BLOCK_STORE_NUM_BLOCKS.
size_t block_store_get_total_blocks()
{
	return BLOCK_STORE_NUM_BLOCKS;
}

size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
	UNUSED(bs);
	UNUSED(block_id);
	UNUSED(buffer);
	return 0;
}

size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
	UNUSED(bs);
	UNUSED(block_id);
	UNUSED(buffer);
	return 0;
}

block_store_t *block_store_deserialize(const char *const filename)
{
	UNUSED(filename);
	return NULL;
}

size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
	UNUSED(bs);
	UNUSED(filename);
	return 0;
}
