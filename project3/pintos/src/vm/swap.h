#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/stddef.h"
#include "devices/block.h"


struct bitmap *my_swap_table;

void init_swap_table();
void swap_in(void *upage, void *kpage, bool writable, block_sector_t sector, void *buffer);
void swap_out(uint32_t *pd, uint32_t *uaddr, block_sector_t sector, void *buffer);
