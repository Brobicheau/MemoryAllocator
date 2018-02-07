
#ifndef SFMMHELPER_H
#define SFMMHELPER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

extern int heap_count;
extern void * LB;
extern void * UB;


#define PAGE_SIZE 4096
#define SET_ALLOCED_BLOCK(alloced_free_header,new_footer, new_block_size, padding_size) \
			(alloced_free_header->header.alloc = 1); \
			(alloced_free_header->header.block_size = new_block_size);\
			(alloced_free_header->header.padding_size = padding_size);\
			(alloced_free_header->header.unused_bits = 0);\
			(new_footer->block_size = new_block_size);\
			(new_footer->alloc = 1);



#define GET_HEADER_AND_FOOTER(free_header_ptr, footer_ptr, payload_ptr) \
			(free_header_ptr = (sf_free_header *)(payload_ptr - SF_HEADER_SIZE));\
			(footer_ptr = (sf_footer *)((char *)free_header_ptr +(free_header_ptr->header.block_size<<4) - SF_FOOTER_SIZE));

#define GET_FOOTER(alloced_free_header, new_footer) \
			new_footer = (sf_footer *)((char *)alloced_free_header +(alloced_free_header->header.block_size<<4) - SF_FOOTER_SIZE);\


#define GET_PAYLOAD(alloced_free_header, payload_ptr)\
			payload_ptr = (void *)((char*)alloced_free_header + SF_HEADER_SIZE);

#define ZERO_PREV_BLOCK(prev_free_header, prev_footer) \
			(prev_free_header->header.block_size = 0);\
			(prev_free_header->header.alloc = 0);\
			(prev_free_header->header.padding_size= 0);\
			(prev_free_header->header.unused_bits = 0);\
			(prev_footer->alloc = 0);\
			(prev_footer->block_size = 0);

#define ZERO_NEXT_BLOCK(next_free_header, next_footer) \
			(next_free_header->header.block_size = 0);\
			(next_free_header->header.alloc = 0);\
			(next_free_header->header.padding_size= 0);\
			(next_free_header->header.unused_bits = 0);\
			(next_footer->alloc = 0);\
			(next_footer->block_size = 0);			
		

#define FREE_ALLOCED_BLOCK (header_ptr) 
/*Helper function for finding the first memory block that fits needed size*/

#define GET_SURROUNDING_BLOCKS(current_head, next_header, next_footer, prev_header, prev_footer)\
			next_header = (sf_free_header *)((char*)current_head + (current_head->header.block_size<<4));\
			next_footer = (sf_footer *)((char*)next_header + (next_header->header.block_size<<4) - SF_FOOTER_SIZE);\
			prev_footer = (sf_footer *)((char*)current_head - SF_FOOTER_SIZE);\
			prev_header = (sf_free_header *)((char*)prev_footer - (prev_footer->block_size<<4) + SF_FOOTER_SIZE);\

#define GET_NEXT_BLOCK(current_head, next_header, next_footer) \
			next_header = (sf_free_header *)((char*)current_head + (current_head->header.block_size<<4));\
			next_footer = (sf_footer *)((char*)next_header + (next_header->header.block_size<<4) - SF_FOOTER_SIZE);





#endif 