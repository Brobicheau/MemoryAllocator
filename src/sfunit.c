#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"



Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    int *y = sf_malloc(4);
    memset(y, 0xFF, 4);
    sf_free(x);
    cr_assert(freelist_head == (void*)x-8);
    sf_free_header *headofx = (sf_free_header*) x-8;
    sf_footer *footofx = (sf_footer*) (x + headofx->header.block_size) - 8;

    // All of the below should be true if there was no coalescing
    cr_assert(headofx->header.alloc == 0);
    cr_assert(headofx->header.block_size == 32);
    cr_assert(headofx->header.padding_size == 15);

    cr_assert(footofx->alloc == 0);
    cr_assert(footofx->block_size == 32);
}





/*This will test if malloc will use the first available block of required size*/
Test(sf_memsuite, free_check, .init = sf_mem_init, .fini = sf_mem_fini) {


    int *x = sf_malloc(4);
    int *y = sf_malloc(4);
    int *z = sf_malloc(4);

    *x = 0;
    *z = 0;


    sf_free(y);

    int *a = sf_malloc(4);
    cr_assert(a == y);
}


Test(sf_memsuite, splinter_check, .init = sf_mem_init, .fini = sf_mem_fini){


    int *w = sf_malloc(32);
    int *x = sf_malloc(4);
;


    sf_free(w);

    int *y = sf_malloc(1);
    int *z = sf_malloc(4);



    cr_assert(w == y);

    sf_free(x);
    sf_free(z);


}

Test(sf_memsuite, info_test, .init = sf_mem_init, .fini = sf_mem_fini){

    info *data = sf_malloc(sizeof(*data));
    int test;
    /*
    ALLOCATIONS SHOULD BE 15
    FREE SHOULD BE 0
    INTERNAL SHOULD BE 16*15 + 8
    EXTERNAL SHOULD BE 4096 - 32*15
    COALESCE SHOULD BE 0   
    */
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    void *d = sf_malloc(4);
    void *e = sf_malloc(4);
    void *f = sf_malloc(4);
    void *g = sf_malloc(4);
    void *h = sf_malloc(4);
    void *i = sf_malloc(4);
    void *j = sf_malloc(4);
    void *k = sf_malloc(4);
    void *l = sf_malloc(4);
    void *m = sf_malloc(4);
    void *n = sf_malloc(4);
    void *o = sf_malloc(4);

    
    if((test = sf_info(data) == 0)){



        cr_assert(data->allocations == 16);
        cr_assert(data->frees == 0);
        cr_assert(data->internal == 444);//15*28 + 1*24
        cr_assert(data->external ==3552);//4096-32*16
        cr_assert(data->coalesce == 0);


    }

    sf_free(a);
    sf_free(b);
    sf_free(c);
    sf_free(d);
    sf_free(e);
    sf_free(f);
    sf_free(g);
    sf_free(h);
    sf_free(i);
    sf_free(j);
    sf_free(k);
    sf_free(l);
    sf_free(m);
    sf_free(n);
    sf_free(o);

    if((test = sf_info(data) == 0)){

        cr_assert(data->allocations == 16);
        cr_assert(data->frees == 15);
        cr_assert(data->internal == 24);
        cr_assert(data->external ==4032);//4096-32*16
        cr_assert(data->coalesce == 14);/*must coalesce every block BUT the first since they are contiguous*/


    }  



    a = sf_malloc(4);
    b = sf_malloc(4);
    c = sf_malloc(4);
    d = sf_malloc(4);
    e = sf_malloc(4);
    f = sf_malloc(4);
    g = sf_malloc(4);
    h = sf_malloc(4);
    i = sf_malloc(4);
    j = sf_malloc(4);
    k = sf_malloc(4);
    l = sf_malloc(4);
    m = sf_malloc(4);
    n = sf_malloc(4);
    o = sf_malloc(4);   

    sf_free(e);
    sf_free(g); 
    sf_free(f);
  

    if((test = sf_info(data) == 0)){

        cr_assert(data->allocations == 31);
        cr_assert(data->frees == 18);
        cr_assert(data->internal == (12*28+24));//15*22
        cr_assert(data->external == 4096-(32*12+64));//4096-32*16
        cr_assert(data->coalesce == 15);


    }  




}

Test(sf_memsuite, realloc_test, .init = sf_mem_init, .fini = sf_mem_fini){

 


    
}

Test(sf_memsuite, realloc_ex_and_split, .init = sf_mem_init, .fini = sf_mem_fini){


    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    void *d = sf_malloc(4);
    void *e = sf_malloc(4);
    void *f = sf_malloc(4);

    /* REALLOC EXPAND AND SPLIT CASE*/


    /*
    free up 64 bytes of space directly after d
    malloc of 4 bytes = 12bytes in padding + 16 bytes for header/footer = 32*2
    */

    sf_free(c);
    sf_free(d);
    sf_free(e);

    /*realloc it with 32 bytes to show the realloc expands and splits*/
    b = sf_realloc(b , 32);


    /*get the headerr of the allocated block (the pointer b - 8 bytes)*/
    sf_free_header * head = (sf_free_header*)((char*)b-8);

    /*get the free block head (head of the allocated block + the size of the allocated block)*/
    sf_free_header * free_head = (sf_free_header*)((char*)head + (head->header.block_size<<4));

    /*if working correcty, head block should be 64 bytes*/

    cr_assert((head->header.block_size<<4) == 48);


    /* and the free header should be 32 bytes (96 - 64 for the allocated byte)*/
    cr_assert(free_head->header.block_size<<4 == 48);




    sf_free(a);
    sf_free(b);
    sf_free(c);
    sf_free(d);
    sf_free(e);
    sf_free(f);

  
    
}

Test(sf_memsuite, realloc_shrink_and_split, .init = sf_mem_init, .fini = sf_mem_fini){
  

    /*allocate a bunch of space to work with*/
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    void *d = sf_malloc(4);
    void *e = sf_malloc(4);
    void *f = sf_malloc(4);
    void *g = sf_malloc(4);
    void *h = sf_malloc(4);

    /*free 96 bytes of space*/
    sf_free(c);
    sf_free(d);
    sf_free(e);

    /*realloc it with 48 bytes so that a total of 64 is used including padding/header/footer*/
    b = sf_realloc(b , 48);

    /* the head willl be the b pointer - 8 bytes*/
    sf_free_header *head = (sf_free_header*)((char*)b-8);

    /*get the free block head (head of the allocated block + the size of the allocated block)*/
    sf_free_header *free_head = (sf_free_header*)((char*)head + (head->header.block_size<<4));

    cr_assert((head->header.block_size<<4) == 64);

    /*now shrink it*/
    b = sf_realloc(b,4);

    /*get the free block head (head of the allocated block + the size of the allocated block)*/
    free_head = (sf_free_header*)((char*)head + (head->header.block_size<<4));

    /*the block should now be 32 bytes. (4 byte allocation will be 32 if padding is applied)*/
    cr_assert((head->header.block_size<<4) == 32);


    /*the free block should now be 64 bytes. Origianl 32 free - 32 extra free bytes for shrink*/
    cr_assert((free_head->header.block_size<<4) == 64);
    


    sf_free(a);
    sf_free(b);
    sf_free(c);
    sf_free(d);
    sf_free(e);
    sf_free(f);
    sf_free(g);
    sf_free(h);
 
}

Test(sf_memsuite, realloc_ex_no_split, .init = sf_mem_init, .fini = sf_mem_fini){


    /*allocate a bunch of space to work with*/
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    void *d = sf_malloc(4);
    void *e = sf_malloc(4);
    void *f = sf_malloc(4);
    void *g = sf_malloc(4);
    void *h = sf_malloc(4);

    /*EXPAND WITH NO SPLINTER OR SPLIT*/

    sf_free(c);
    sf_free(d);

    b = sf_realloc(b, 48);

    sf_free_header *head = (sf_free_header*)((char*)b-8);


    cr_assert((head->header.block_size<<4) == 64);

    sf_free(a);
    sf_free(b);
    sf_free(c);
    sf_free(d);
    sf_free(e);
    sf_free(f);
    sf_free(g);
    sf_free(h);    
    
}





