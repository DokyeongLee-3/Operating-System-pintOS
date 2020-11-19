#include "vm/frame.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pool
  {
    struct lock lock;                   
    struct bitmap *used_map;            
    uint8_t *base;                    
  };

extern struct pool kernel_pool, user_pool;
uint32_t *frame_table;
uint8_t *which_process;
extern uint32_t num_of_pagedir_index;
extern uint16_t num_of_user_pool_pages;
extern uint16_t num_of_kernel_pool_pages;
extern uint32_t *init_page_dir; // page directory base address
extern struct pool kernel_pool, user_pool;

/* frame table을 어떻게 만들면 해당 프레임이
 * 어떤 page들에 의해 reference당하는 걸 알고, 이 frame이 swap out 당할때
 * 그 page들의 mapping을 invalid하게 만들어 줄 수 있을까 */

/* 4.1.5 Each entry in the frame table contains a pointer to the page, if any, that currently occupies it  그냥 주소가 아니라 0으로 채워져있으면 그 frame은 사용중이 아니란 의미*/

/* ----------------------------------------------------------------
 *|                       virtual address[31:0]                   |
 * ________________________________________________________________ */


void frame_table_init(size_t num_of_user_frame){

  frame_table = malloc(num_of_user_frame * sizeof(uint32_t));
  /* 각 frame을 어떤 process가 사용하고 있는지 pid 저장한 table */
  which_process = malloc(num_of_user_frame * sizeof(uint8_t));
  int i = 0;
  for(; i < num_of_user_frame; i = i+1){
    frame_table[i] = 0;
    which_process[i] = 0;
  }
  //printf("<<<<<<<<<<address of frame_table is %p>>>>>>>>>>>>>>>\n", frame_table);
}

void frame_table_free(){
 free(frame_table);
}

/* page table을 보고 reference bit가 0인 entry를 evict 시키고,
 * 모든 entry의 reference bit가 1이면 frame_table의 reference count가 제일
 * 낮은걸 evict 시키고, bitmap도 0으로(unused)로 바꿔주기
 * 이 frame을 참조중인 page들에 대한 reference도 없애주기(그 page들의 pte에
 * valid bit을 0으로 바꿔놓기) */

bool evict_single_frame(){

  bool find_evicted_frame = false;
  int i = 0;
  uint32_t *pt;
  for(; i < num_of_pagedir_index; i = i+1){
    pt = init_page_dir[i];
    int j = 0;
    for( ;j < num_of_pagedir_index; j = j+1){

      if(!pagedir_is_accessed(init_page_dir[i], pt[j])){
        find_evicted_frame = true;
        pt[j] = pt[j] & (~PTE_P);
// pt[j]>>12는 page table의 j번째 entry가 가리키는                                                // physical address인데 이걸 ptov 시키면 user pool이 있는
// kernel virtual address를 얻을 수 있고 그 pool에 used_map을 0으로
        palloc_free_page(ptov(pt[j]>>12));
        break;
      }

    }
  }
  // 만약 위에서 pte 하나하나 들여다봤는데
  // 모든 pte의 accessed bit가 1이면
  // frame_table에서 첫번째 frame evict
  if(!find_evicted_frame){
    palloc_free_page(user_pool.base + PGSIZE);

   /* 1024*1024개의 pte가 가리키는 physical 메모리 하나하나 확인해서
    * physical memory가 physical memory의 frame들에 1대1 매핑되어 있는
    * user_pool의 page중 가장 첫번째 page에 vtop
    * 적용한거랑 같다면 그 pte의 present bit를 0으로 */
   int p = 0;
   for(; p < num_of_pagedir_index; p = p+1){
     pt = init_page_dir[p];
     int q = 0;
     for(; q < num_of_pagedir_index; q = q+1){
       if(pt[q]>>12 == vtop(user_pool.base + PGSIZE))
         pt[q] = pt[q] & (~PTE_P); 
     }
   }
     
  }
  // accessed bit 0인 pte 찾았다면
  // evict하려는 frame을 palloc_free_page
  // palooc_free_page하면 bitmap도 0으로 만들어줌
  return true;

}
    
