#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "threads/vaddr.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"

bool pagedir_is_accessed (uint32_t *pd, const void *upage);

void frame_table_init(size_t num_of_user_frame);

void frame_table_free(void);

uint32_t evict_single_frame_and_swap_out(void);

