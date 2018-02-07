#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "sfmm.h"
#include "sfmmhelper.h"






sf_free_header *first_fit (size_t size);

void place_next_header( sf_free_header* node, size_t size);

void remove_free_node(sf_free_header * node_to_remove);

void coalesce(void * ptr);

void allocate_heap_space();

void  * LB;
void  * UB;


sf_free_header* freelist_head = NULL;
static info data;
int heap_count = 0;


void *sf_malloc(size_t size){

	/*Local variables*/

	void *payload_ptr;						/*pointer to the payload portion of the allocated block*/
	size_t payload_size;					/*size of the payload. (requested_size + padding_size)*/
	size_t block_size;						/*the entire block of memeory (header + footer + payload)*/
	size_t padding_size;					/*the extra memeory allocated to account for alignment*/
	sf_footer * ftr_ptr;					/*Pointer to the footer of the new allocated block*/
	sf_free_header * alloced_free_header;	/* the free header that is about to be allocated*/

	/**********************************************************************/


	if(size >= (PAGE_SIZE*4)){
		errno = 1;
		return NULL;

	}
	/*check if this is the first allocation of the program*/
	if(freelist_head == NULL) {

		/*set the freelist head to the pointer of the heap*/
		freelist_head = (sf_free_header*)((char*)sf_sbrk(0));

		LB = freelist_head;

		/*set the original block size shifted to the right for viewing purposes*/
		freelist_head->header.block_size = (PAGE_SIZE>>4);

		/*set the alloc bit to 0*/
		freelist_head->header.alloc = 0;


		/*set the new head of the freelist*/

		/*get the footer for the first free block*/
		ftr_ptr = (sf_footer*)((char*)sf_sbrk(1)- SF_FOOTER_SIZE);//(sf_footer*)((char*)freelist_head + (4096>>4) - SF_FOOTER_SIZE);

		UB = sf_sbrk(0);
		/*set the block size and allocated bibt*/
		ftr_ptr->block_size = (PAGE_SIZE>>4);
		ftr_ptr->alloc = 0;
		heap_count++;

		data.allocations = 0;
		data.external = PAGE_SIZE;
		data.internal = 0;
		data.frees = 0;
		data.coalesce = 0;		


	}

	/*Check if useable value*/
	if(size == 0){
		errno = 1;

		return NULL;
		/*dont forget to set errno*/
	}
	/*if not a multiple of 16*/
	if((size % 16) != 0){

		/*make it one*/
		payload_size = size + 16 - (size % 16);
		padding_size = payload_size - size;
	}
	/*otherwise set the padding to 0*/
	else{
		padding_size = 0;
		payload_size = size;
	}

	/*TODO: ADD CODE TO ADJUST FOR FRAGMENTATION*/

	/*set the blocksize*/
	block_size = ((SF_HEADER_SIZE + SF_FOOTER_SIZE + payload_size)>>4);

	/*get the appropriate freeblock from the freelist*/
	alloced_free_header = first_fit(block_size);

	if(alloced_free_header == NULL){
		/*SET ERRNO*/
		errno = 1;
		return NULL;
	}

	if( ((alloced_free_header->header.block_size<<4)-(block_size <<4)) < 32){

		padding_size = ((alloced_free_header->header.block_size<<4)-(block_size <<4) + padding_size);
		payload_size = (payload_size + padding_size);
		block_size = ((SF_HEADER_SIZE + SF_FOOTER_SIZE + payload_size)>>4);
	}

	/*modify the freelist accordingly*/
	place_next_header(alloced_free_header, block_size);

	/*remove the allocted free block from the free list*/
	remove_free_node(alloced_free_header);	

	/*set the footer according to block_size*/
	ftr_ptr = (sf_footer*)((char*)alloced_free_header + (block_size<<4) - SF_FOOTER_SIZE);

	//set the blocks header and footer*/
	SET_ALLOCED_BLOCK(alloced_free_header, ftr_ptr, block_size, padding_size);	

	/*set the payload pointer to 8 bytes ahead of the start of the header*/
	payload_ptr = ((char*)alloced_free_header + SF_HEADER_SIZE)

	/*return the new pointer*/
	data.allocations++;
	data.internal = data.internal + (SF_HEADER_SIZE + SF_FOOTER_SIZE + padding_size);
	data.external -= (block_size<<4);
 	return (payload_ptr);
}

void sf_free(void *ptr){

	/*Local variables*/

	sf_free_header * new_freelist_header;	/* the new header for the freelist */
	sf_free_header * free_header_ptr;					/* pointer for the header of the block to be freed*/
	sf_footer * footer_ptr;					/* pointer to the footer of the block to be freed*/
	char * payload_ptr = (char*)ptr;		/* ptr to the payload of the block, converrted to char pointer for easy arithmetic*/

	/*****************************************************************/

	/*macro that gets the pointer of the header and footer based on payload location*/
	GET_HEADER_AND_FOOTER(free_header_ptr, footer_ptr, payload_ptr);

	if(free_header_ptr->header.alloc != 0 && footer_ptr->alloc != 0){

	/* "free" the block*/
	free_header_ptr->header.alloc = 0;
	footer_ptr->alloc = 0;

	/*adjust freelist*/
	/*set the new headers position to the headers ptr*/
	new_freelist_header = free_header_ptr;

	/*set the current heads prev to the new head*/
	new_freelist_header->prev = freelist_head;

	/*new head of freelist, so next is NULL*/
	new_freelist_header->next = NULL;

	freelist_head->next = new_freelist_header;

	/*change the new head to the current head*/
	freelist_head = new_freelist_header;

	coalesce(free_header_ptr);

	data.internal -= (SF_HEADER_SIZE + SF_FOOTER_SIZE + free_header_ptr->header.padding_size);
	data.external += (free_header_ptr->header.block_size<<4);
	data.frees+=1;
	}
	else {
		errno = 1;
	}

}

void *sf_realloc(void *ptr, size_t size){


	void * payload_ptr = ptr;
	sf_free_header *  header_ptr;
	sf_free_header * alloced_free_header;
	sf_free_header * next_free_header;
	sf_free_header * new_free_header;
	sf_footer * new_footer;
	sf_footer * new_free_footer;
	sf_footer * footer_ptr;
	sf_footer * next_footer;
	size_t current_block_size;
	size_t payload_size;
	size_t padding_size;
	size_t new_padding_size;
	size_t new_payload_size;
	size_t new_block_size;
	size_t next_block_size;
	unsigned int valid_next;
	void * r_value;


	/*get the header and footer of the current block*/
	GET_HEADER_AND_FOOTER(header_ptr, footer_ptr, payload_ptr);



	new_footer = footer_ptr;

	/*get its block_size*/
	current_block_size = header_ptr->header.block_size << 4;

			/*if not a multiple of 16*/
	if((size % 16) != 0){

		/*make it one*/
		new_payload_size = size + 16 - (size % 16);
		new_padding_size = new_payload_size - size;
	}
	/*otherwise set the padding to 0*/
	else {
		new_padding_size = 0;
		new_payload_size = size;
	}	

	/*if the current is is bigger, WE MUST SHRINK (maybe)*/
	if(current_block_size >= (new_payload_size + SF_FOOTER_SIZE + SF_HEADER_SIZE)){

		/*If shrinking will cause a splinter*/
		if((current_block_size - new_payload_size) < 32){

			GET_NEXT_BLOCK(header_ptr, next_free_header, next_footer);

			if(next_free_header->header.alloc == 0){

				alloced_free_header = header_ptr;
				new_footer = (sf_footer *)((char*)alloced_free_header + new_payload_size);

				new_block_size = (SF_FOOTER_SIZE + SF_HEADER_SIZE + new_payload_size)>>4;
				padding_size = new_padding_size;

				SET_ALLOCED_BLOCK(alloced_free_header, new_footer, new_block_size, padding_size);

				new_free_header = (sf_free_header*)((char*)header_ptr + new_payload_size  + SF_HEADER_SIZE + SF_FOOTER_SIZE);
				new_free_footer = (sf_footer*)next_footer;

				new_free_header->header.block_size = ((next_free_header->header.block_size<<4) - (new_payload_size - new_payload_size));
				new_free_header->header.alloc = 0;
				new_free_header->header.padding_size = 0;
				new_free_footer->alloc = 0;
				new_free_footer->block_size = new_free_header->header.block_size;



			if(freelist_head->next == NULL && freelist_head->prev == NULL){
				freelist_head = new_free_header;
			}
			else {
				freelist_head->next = new_free_header;
				new_free_header->prev = freelist_head;
				freelist_head = new_free_header;
			}	


				payload_ptr = ((char*)alloced_free_header + SF_HEADER_SIZE);

				r_value = payload_ptr;
				data.coalesce++;


			}
			else
			/*just return the pointer without doing anything*/
			/*DO NOTHING*/
			r_value = payload_ptr;

		}
		/*otherwise split it into an alloced block and a free block of at least size 32*/
		else {

			alloced_free_header = header_ptr;


			/*if not a multiple of 16*/
			if((size % 16) != 0){

				/*make it one*/
				payload_size = size + 16 - (size % 16);
				padding_size = payload_size - size;
			}
			/*otherwise set the padding to 0*/
			else {
				padding_size = 0;
				payload_size = size;
			}

			/*set the new block size*/
			new_block_size = ((payload_size + SF_FOOTER_SIZE + SF_HEADER_SIZE)>>4);

			new_footer = (sf_footer*)((char*)alloced_free_header + (new_block_size<<4) - SF_FOOTER_SIZE);

			data.internal -= alloced_free_header->header.padding_size;
			data.internal += padding_size;

			/*set the next block to be a free header*/
			place_next_header(alloced_free_header, new_block_size);

			coalesce((char*)alloced_free_header + (new_block_size<<4));

			/*set up the new allced block*/
			SET_ALLOCED_BLOCK(alloced_free_header, new_footer, new_block_size, padding_size);

			/*set the pointer for the return value (pointer to the paylaod)*/
			payload_ptr = (void*)((char*)alloced_free_header + SF_HEADER_SIZE);

			data.external -= current_block_size;
			data.external += (new_block_size<<4);

			/*SHRINK AND SPLIT*/
			/*set the return value to the payload*/
			r_value = payload_ptr;

		}

	}
	/*otherwise we must expand!*/
	else if(current_block_size < (size + SF_FOOTER_SIZE + SF_HEADER_SIZE)){

		/*get the infomation on the blocks next to the one we are reallocing*/
		GET_NEXT_BLOCK(header_ptr, next_free_header, next_footer);
		next_block_size = (next_free_header->header.block_size<<4);

		/*only need to check if the following block is a valid free block*/
		if((next_free_header !=NULL && next_footer != NULL) &&
			(next_free_header->header.block_size == next_footer->block_size) &&
			(next_free_header->header.alloc == 0 && next_footer->alloc == 0)	&&
			(next_free_header->header.block_size > 0 && next_footer->block_size > 0)){
			valid_next = 0;
		}
		else
			valid_next = 1;


		/* if the next blockis a valid free block, we attempt to use it*/
		if(valid_next == 0){
			/*MIGHT NEED TO REMOVE PADDING FROM OLD BLOCK?*/
			new_block_size = (current_block_size + next_block_size)>>4;
			 /*if it is big enough to house the reqeusted realloc amount*/
			if((size + SF_FOOTER_SIZE + SF_HEADER_SIZE) <= (new_block_size<<4)){

				/*if it will splinter*/
				if(((new_block_size<<4) - (size + SF_HEADER_SIZE + SF_FOOTER_SIZE))<32){

					alloced_free_header = header_ptr;

					/*if not a multiple of 16*/
					if((size % 16) != 0){

						/*make it one*/
						payload_size = size + 16 - (size % 16);
						padding_size = payload_size - size;
					}
					/*otherwise set the padding to 0*/
					else {
						padding_size = 0;
						payload_size = size;
					}

					data.internal -= alloced_free_header->header.padding_size;
					data.internal += padding_size;
					new_footer = (sf_footer*)((char*)alloced_free_header + (new_block_size<<4) -SF_FOOTER_SIZE);
					/*we must allocate entire block*/
					SET_ALLOCED_BLOCK(alloced_free_header, new_footer, new_block_size, padding_size);

					payload_ptr = (void *)((char*)alloced_free_header + SF_HEADER_SIZE);

					/*EXPAND AND SPLINTER WITH TAKE ENTIRE BLOCK*/
					r_value = payload_ptr;
					data.external -= (size - current_block_size);

				}
				/*it will not splinter*/
				else {

					/*set needed data for block*/
					new_block_size = (new_payload_size + SF_HEADER_SIZE + SF_FOOTER_SIZE)>>4;
					alloced_free_header = header_ptr;
					new_footer = (sf_footer *)((char*)alloced_free_header + (new_block_size<<4) - SF_FOOTER_SIZE);



					/*remove the next freeblocks node off the freelist*/

					padding_size = new_padding_size;

					remove_free_node(next_free_header);

					/*set the actual block*/
					SET_ALLOCED_BLOCK(alloced_free_header, new_footer, new_block_size, padding_size);



					/*place the freeblock at the end of the new allocated block*/
					//place_next_header(alloced_cfree_header, (new_block_size));

					/*new free head of this block is = to the footer of the block + the new payload*/
					new_free_header = (sf_free_header*)((char*)alloced_free_header + (new_block_size<<4));
					new_free_header->prev = NULL;
					new_free_header->next = NULL;

					new_free_header->header.block_size = ((next_block_size - (new_payload_size + SF_FOOTER_SIZE + SF_HEADER_SIZE)))>>4;
					new_free_header->header.alloc = 0;
					new_free_header->header.padding_size = 0;
					new_free_footer = (sf_footer*)((char*)new_free_header + (new_free_header->header.block_size<<4) - SF_FOOTER_SIZE);
					new_free_footer->block_size = new_free_header->header.block_size;
					new_free_footer->alloc = 0;

					next_footer->block_size = 0;



					payload_ptr = (void*)((char*)alloced_free_header + SF_HEADER_SIZE);

					/*EXPAND WITHOUT SPLINTER*/
					r_value = payload_ptr;

				}
			}
		}
		/*else it is not a valid block, and we must find a new one*/
		else if (valid_next == 1){

			payload_ptr = sf_malloc(size);
			payload_size = current_block_size - SF_HEADER_SIZE - SF_FOOTER_SIZE;

			memcpy(payload_ptr, ptr, payload_size);

			sf_free(ptr);

			/*MOVE*/
			r_value = payload_ptr;

		}

	}

  return r_value;
}

int sf_info(info* meminfo){

	if(meminfo != NULL) {
		meminfo->allocations = data.allocations;
		meminfo->frees = data.frees;
		meminfo->internal = data.internal;
		meminfo->external = data.external;
		meminfo->coalesce = data.coalesce;
		return  0;
	}
	else {
		return-1;
	
	}


}


sf_free_header *first_fit(size_t block_size){

	int flag = 1;
	sf_free_header *current_head;
	current_head = freelist_head;
	size_t current_block_size;
	size_t requested_block_size;
	sf_free_header* r_value;

	/****************************************************************/

	requested_block_size = (block_size << 4);

	/*get the size of the head of the free list*/
	current_block_size = (current_head->header.block_size<<4);

	/*while a big enough block hasnt been found, or the next block isnt null*/
	while(flag != 0){

		/*if the current free block is big enough*/
		if(requested_block_size <= current_block_size){

			/* set the flag to 0 and set the return value to the current block*/
			flag = 0;
			r_value = current_head;
		}
		/*else if its at the enf of the free list*/
		else if(current_head->next == NULL){
			/*TODO: DEAL WITH ENDING FOOTER OF ALLOCATED BLOCK

			  MAY AHVE TO CHANGE THIS TO WHILE LOOP */
			/*allocate more space on the heap*/
			if(heap_count == 4){
				 errno = 28;
				
				 return  NULL;
			}
			allocate_heap_space();
			data.external += PAGE_SIZE;
			current_head = first_fit(block_size);

			/*set flag to zero and return the last block*/
			flag = 0;
			r_value = current_head;			

		}
		/*otherwise move to the next block*/
		else{
			current_head = current_head->next;
			current_block_size = (current_head->header.block_size<<4);
		}

	}


	return r_value;  
}



void remove_free_node(sf_free_header* node_to_remove){

	sf_free_header *temp;	/*temp variable for switching around linked list*/
	sf_free_header *temp2;	/*temp variable for switching around linked list*/
	sf_footer * footer_to_remove;

	/*****************************************************************/

	/*if only the prev on the list is NULL*/
	if(node_to_remove->prev == NULL && node_to_remove->next != NULL){

		/*THIS SHOULD BE THE TAIL OF THE FREELIST*/

		/*set the temp to the next node in the list*/
		temp = node_to_remove->next;

		/* set the next nodes prev(the node to remove) to NULL*/
		temp->prev = NULL;

		footer_to_remove = (sf_footer *)((char*)node_to_remove + (node_to_remove->header.block_size<<4) - SF_FOOTER_SIZE);
		footer_to_remove->block_size =0;
		/*set the node to removes next to NULL, removing it from list*/
		node_to_remove->next = NULL;
		node_to_remove->prev = NULL;
	}
	/*if only the next on the list is NULL*/
	else if(node_to_remove->prev != NULL && node_to_remove->next == NULL){

		/*THIS IS THE HEAD OF THE FREELIST*/

		/*set the temp to prev node*/
		temp = node_to_remove->prev;

		/*set the temps next to NULL*/
		temp->next = NULL;

		footer_to_remove = (sf_footer *)((char*)node_to_remove + (node_to_remove->header.block_size<<4) - SF_FOOTER_SIZE);
		footer_to_remove->block_size = 0;

		/* set the nodes prev to NULL effectively removingg it from list*/
		node_to_remove->prev = NULL;
		node_to_remove->next = NULL;

		/*set the new head*/
		freelist_head = temp;
	}
	/* if neither arre NULL*/
	else if(node_to_remove->next!=NULL && node_to_remove->prev !=NULL){

		/* set the first temp to the node were removings previous pointer*/		
		temp = node_to_remove->prev;

		/* set the second temp to the node werer removings next pointer*/
		temp2 = node_to_remove->next;

		/*set the first temps next pointer to the second temp (jumping over the node were removing)*/
		temp->next = temp2;

		/* set the second temps previous pointer to the first temp (again jumping over the node to be removed)*/
		temp2->prev = temp;


		footer_to_remove = (sf_footer *)((char*)node_to_remove + (node_to_remove->header.block_size<<4) - SF_FOOTER_SIZE);
		footer_to_remove->block_size = 0;
		/*set the node to removes pointers to NULL removing it from the list completely*/
		node_to_remove->next = NULL;
		node_to_remove->prev = NULL;
									

	}





}

void place_next_header(sf_free_header *alloced_free_header, size_t block_size){

	/* if the next byte after the footer of the block that was
		just allocated is 0, make it the new head of the free list,
		otherwise the currnt heads prev is the new head.*/

		unsigned int alloc_bit;						/*the bit used for checking if the block is free*/
		size_t old_block_size;						/*size of the freeblock before the allocation*/
		size_t new_block_size;						/*the size of the freeblock AFTER the allocation(if there is any lefft)*/
		sf_free_header * new_head;					/*the new header for the new freeblock*/
		sf_footer * footer_ptr;						/*the new footer for the new freeblock, and its pointer*/
		size_t requested_block_size;				/*the size of the block that the user requested*/
		requested_block_size = (block_size << 4);	/*requested blocksize is the blocksize gotten from the user shifted to the left by 4 bits*/
		char * free_space_ptr;						/*points to the first byte after the alloced_free_headers block*/
		

		/*********************************************************************/

		/*get the blocksize of the freeblock we are about to allocate*/
		old_block_size = (alloced_free_header->header.block_size<<4);			

		/*set the new blocks size to the difference between
		 the old block size and the requested block size*/
		new_block_size = (old_block_size - requested_block_size);				

		/*find the poisition of first byte after newly alloced block*/
		free_space_ptr = ((char*)alloced_free_header + (requested_block_size));

		/*get the alloc bit of that byte to determine its allocation property*/
		alloc_bit = (*free_space_ptr & 0x01);

		/*if its allocated*/
		if(alloc_bit == 1){
			/*then just remove the current head of the freelist*/
			/*TODO: check for prev == null*/
			freelist_head = alloced_free_header->prev;
		}
		/*else if the block is not allocated yet*/
		else if(alloc_bit == 0){

			/*set the new head as a pointer to the byte right after the footer*/

			new_head = (sf_free_header*)(free_space_ptr); 
			new_head->prev = 0;
			new_head->next = 0;

			/*get the pointer to the footer of the free block*/
			footer_ptr = (sf_footer*)((char*)alloced_free_header + old_block_size - SF_FOOTER_SIZE);

			/*set the footer to the new info*/
			footer_ptr->block_size = (new_block_size >> 4);
			footer_ptr->alloc = 0;

			/*set the new free block header to the new info*/
			new_head->header.block_size = (new_block_size >> 4);
			new_head->header.alloc = 0;

			/*arrange the freelist accordingly*/
			if(freelist_head->next == NULL && freelist_head->prev == NULL){
				freelist_head = new_head;
			}
			else {
				freelist_head->next = new_head;
				new_head->prev = freelist_head;
				freelist_head = new_head;
			}

		}

}

void allocate_heap_space(){

	/*Local Variables*/
	void *heap_ptr;			/* pointer to the spot on the heap were memeory was just allocated*/
	sf_free_header * new_free_header;
	sf_footer *new_footer;

	/*********************************************************************/


	/*MIGHT NEED TO MAKE THE HEADER A FREE HEADER*/


	/* request more space from the heap, if sucessful set pointer*/
	heap_ptr = (sf_sbrk(1));/*<<<<<<<<<<<< should this be + or - 8?*/
			heap_count++;
	UB = heap_ptr;




	new_free_header = (sf_free_header*)((char*)heap_ptr - PAGE_SIZE);
	new_footer = (sf_footer*)((char*)new_free_header + PAGE_SIZE - SF_FOOTER_SIZE);

	new_free_header->header.block_size = (PAGE_SIZE>>4);
	new_free_header->header.alloc = 0;
	new_free_header->header.padding_size = 0;
	new_free_header->header.unused_bits = 28;

	new_footer->block_size = (PAGE_SIZE>>4);
	new_footer->alloc = 0;

	coalesce(new_free_header);

}

void coalesce(void *ptr) {

	/*Local Variables*/
	sf_footer *next_footer; /*footer for the next footer*/
	sf_footer *prev_footer; /*footer for the previous block*/
	sf_footer *new_footer;
	size_t prev_block_size; /*size of the previous block*/
	size_t next_block_size; /*size of the next block*/
	size_t freed_block_size;/*size of the current freed block*/
	size_t new_block_size;
	sf_free_header * freed_block = (sf_free_header *)ptr; /*free header for the block we are trying to coalesce*/
	sf_free_header * next_free_header;
	sf_free_header * prev_free_header;
	sf_free_header * new_free_header;
	int valid_prev = 1;
	int valid_next = 1;

	/******************************************************************/

	/*get the block of the block to be coalesced*/
	freed_block_size = (freed_block->header.block_size<<4);

	/*get previous blocks footer to check if it is allocated or not*/
	prev_footer = (sf_footer *)((char*)freed_block - SF_FOOTER_SIZE);

	/*get the next blocks header to check if its allocated or not*/
	next_free_header = (sf_free_header *)((char*)freed_block + freed_block_size);	

	if((void*)prev_footer < LB || (void*)prev_footer > UB){

			 prev_free_header = NULL;
			 prev_footer = NULL;
		}
	else {

		/*get the size of the PREVIOUS block*/
		prev_block_size = (prev_footer->block_size << 4);		

		/*find the header of the PREVIOUS block*/
		prev_free_header = (sf_free_header*)((char*)prev_footer - prev_block_size + SF_FOOTER_SIZE);	

	}

	if((void*)next_free_header < LB || (void*)next_free_header > UB) {
			next_free_header = NULL;
			next_footer = NULL;

	}
	else {

		/*get the size of the NEXT block*/
		next_block_size = (next_free_header->header.block_size << 4);

		next_footer = (sf_footer*)((char*)next_free_header + next_block_size - SF_FOOTER_SIZE);
	}



	/*check if the pervious blocks values are all valid.
	head_block_size = footer_block_size
	alloc in header and footer = 0
	block_size for header and footer > 0*/

	if((prev_free_header != NULL && prev_footer !=NULL)&&
		(prev_free_header->header.block_size == prev_footer->block_size) &&
		(prev_free_header->header.alloc == 0 && prev_footer->alloc == 0)	&&
		(prev_free_header->header.block_size > 0 && prev_footer->block_size > 0)){
			valid_prev = 0;
		}
	if((next_free_header !=NULL && next_footer != NULL) &&
		(next_free_header->header.block_size == next_footer->block_size) &&
		(next_free_header->header.alloc == 0 && next_footer->alloc == 0)	&&
		(next_free_header->header.block_size > 0 && next_footer->block_size > 0)){
			valid_next = 0;
		}

	if((valid_prev == 0) && (valid_next == 0)) {

		/******GET ALL NEEDED INFORMATION FOR COALESCE*/


		/*get the freed_blocks footer*/
		//freed_footer = (sf_footer*)((char*)freed_block + freed_block_size - SF_FOOTER_SIZE);


		/*******REMOVE ALL FOOTERS AND HEADERS THAT ARE TO BE COALESCED FROM FREELIST*/

		/* remove all the free_headers and set their footers to 0*/

		remove_free_node(prev_free_header);

		remove_free_node(freed_block);
	
		remove_free_node(next_free_header);


		/*set the new block_size to the sum of all the used block sizes*/
		new_block_size = ((freed_block_size + prev_block_size + next_block_size) >> 4);

		new_free_header = prev_free_header;

		/*set the new free headers data */
		new_free_header->header.block_size = new_block_size;
		new_free_header->header.alloc = 0;
		new_free_header->header.padding_size = 0;

		/****FIND Way TO REMOVE FOOTERS LATER*/

		new_footer = next_footer;

		/*and the footers*/
		new_footer->alloc = 0;
		new_footer->block_size = new_block_size;


		/*if there is only one block on the freelist, then we just merged the head block, 
			thus the new head will be the block just created*/
		if(freelist_head->next == NULL && freelist_head->prev == NULL){
			freelist_head = new_free_header;
		}
		else {
			/*otherwise modify the freelist using LIFO policy*/
			freelist_head->next = new_free_header;
			new_free_header->prev = freelist_head;
			freelist_head = new_free_header;
		}
		data.coalesce++;


	}
	else if (valid_prev == 1 && valid_next == 0){

		/*THIS IS THE HEAD OF THE FREELIST*/

		/*remove these nodes from the tree*/
		remove_free_node(freed_block);
		remove_free_node(next_free_header);


		/*add the nodes to be coalesceds block_size's to get the new size*/
		new_block_size = ((freed_block_size + next_block_size) >>4);

		new_free_header = (sf_free_header *) freed_block;

		/*set the new header data*/
		new_free_header->header.block_size = new_block_size;
		new_free_header->header.alloc = 0;
		new_free_header->header.padding_size = 0;

		/*set the footer pointer and its data*/
		new_footer = next_footer;
		new_footer->block_size = new_block_size;
		new_footer->alloc = 0;

		/*if only one block in freelist, we merged the head, thus the new block is the head*/
		if(freelist_head->next == NULL && freelist_head->prev == NULL){
			freelist_head = new_free_header;
		}
		else {
			freelist_head->next = new_free_header;
			new_free_header->prev = freelist_head;
			freelist_head = new_free_header;
		}
		data.coalesce++;

	}
	else if (valid_prev == 0 && valid_next == 1){


		/*LIKELY MERGING THE TAIL*/

		remove_free_node(prev_free_header);

		remove_free_node(freed_block);


		new_block_size = ((freed_block_size + prev_block_size)>>4);

		new_free_header = (sf_free_header *)prev_free_header;

		new_free_header->header.block_size = new_block_size;

		new_free_header->header.alloc = 0;
		new_free_header->header.padding_size = 0;

		new_footer = (sf_footer*)((char*)freed_block + freed_block_size - SF_FOOTER_SIZE);

		new_footer->block_size = new_block_size;
		new_footer->alloc = 0;


		if(freelist_head->next == NULL && freelist_head->prev == NULL){
			freelist_head = new_free_header;
		}
		else {
			freelist_head->next = new_free_header;
			new_free_header->prev = freelist_head;
			freelist_head = new_free_header;
		}	

		data.coalesce++;
	}

}

	