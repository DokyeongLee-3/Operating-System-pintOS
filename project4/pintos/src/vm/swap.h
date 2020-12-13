#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/stddef.h"
#include "devices/block.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "lib/kernel/bitmap.h"


struct bitmap *my_swap_table;

void palloc_free_multiple (void *pages, size_t page_cnt);
bool install_page (void *upage, void *kpage, bool writable);


void init_swap_table(void);
void swap_in(void *upage, void *kpage, bool writable, block_sector_t sector, void *buffer);
void swap_out(block_sector_t sector, void *buffer);
