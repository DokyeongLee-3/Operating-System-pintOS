#include "vm/swap.h"
#include "lib/kernel/list.h"
#include "threads/pte.h"



typedef unsigned long elem_type;

struct block
  {
    struct list_elem list_elem;         /* Element in all_blocks. */

    char name[16];                      /* Block device name. */
    enum block_type type;                /* Type of block device. */
    block_sector_t size;                 /* Size in sectors. */

    const struct block_operations *ops;  /* Driver operations. */
    void *aux;                          /* Extra data owned by driver. */

    unsigned long long read_cnt;        /* Number of sectors read. */
    unsigned long long write_cnt;       /* Number of sectors written. */
  };

struct bitmap
  {
    size_t bit_cnt;     /* Number of bits. */
    elem_type *bits;    /* Elements that represent bits. */
  };

void init_swap_table(){
  struct block *swap = block_get_role(3);
  uint32_t number_of_sectors = swap->size;
  my_swap_table = bitmap_create(number_of_sectors);
}

void swap_in(void *upage, void *kpage, bool writable, block_sector_t sector, void *buffer){
  block_read(block_get_role(3), sector, buffer); 
  my_swap_table->bits[sector] = 0;
  if(!install_page(upage, kpage, writable)){
    palloc_free_page (kpage);
  }
}

void swap_out(uint32_t *pd, uint32_t *uaddr, block_sector_t sector, void *buffer){
  block_write(block_get_role(3), sector, buffer);
  my_swap_table->bits[sector] = 1;
  // 원래 있던 물리 메모리에선 자리 빼주기 
  uint32_t *pte = lookup_page (pd, uaddr, false);
  *pte = *pte & ~PTE_P; // not present 상태로 만들어주기
}
