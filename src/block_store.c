#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bitmap.h"
#include "block_store.h"
// include more if you need

struct block_store 
{
	uint8_t blocks[BLOCK_STORE_NUM_BLOCKS][BLOCK_SIZE_BYTES];
	uint8_t *bitmap;
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

size_t block_store_allocate(block_store_t *const bs)
{
	UNUSED(bs);
	return 0;
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
}

void block_store_release(block_store_t *const bs, const size_t block_id)
{
	UNUSED(bs);
	UNUSED(block_id);
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
