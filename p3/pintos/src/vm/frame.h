#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/malloc.h"
#include <hash.h>
#include <inttypes.h>


/* FRAME TABLE WITH HASH DATA STRUCTURE */

struct frame
{
    struct hash_elem hash_elem;
    uint32_t* frame_addr;
    uint32_t* mapped_pte;
};

void frame_table_init (void);

unsigned int frame_hash (const struct hash_elem *, void * UNUSED);

bool frame_less (const struct hash_elem *, const struct hash_elem *, void * UNUSED);

struct frame *frame_lookup (const void *);





/* get one frame */
void *frame_get_one (enum palloc_flags);

void *frame_get_multiple (enum palloc_flags, size_t);

void frame_free_one (void *);

void frame_free_multiple (void *, size_t);



void frame_table_add (void *, void *);

void frame_table_delete (void *);


#endif /* VM_FRAME_H */