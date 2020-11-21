#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include <hash.h>
#include <inttypes.h>

/*

user_pool의 frame에 user page를 올린다.
그 frame과 해당 user page의 page table entry 사이의 mapping이 필요함.
=> frame table

eviction 할 때 어떤 frame을 비울지 결정
-> 이때 쫓겨나는 user page가 있다면 이 사실을 알려줘야 함
=> 해당 page table entry에 대한 pointer를 저장하고 있어야 함

page table entry에 접근할 수 있는 건 pagedir.c 의 pagedir_set_page?


==== data structure of frame table ====
list -> search time이 너무 길어진다
hash -> 한 번도 안 써봤지만 이게 제일 나을 듯 ㅅㅂ


*/




static struct hash frame_table;


unsigned int
frame_hash (const struct hash_elem *f_, void *aux UNUSED)
{
    const struct frame *f = hash_entry (f_, struct frame, hash_elem);
    return hash_int ((int)(&f->frame_addr));
}

bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
    const struct frame *a = hash_entry (a_, struct frame, hash_elem);
    const struct frame *b = hash_entry (b_, struct frame, hash_elem);
    return a->frame_addr < b->frame_addr;
}

struct frame *
frame_lookup (const void *addr)
{
    struct frame f;
    struct hash_elem *e;

    f.frame_addr = addr;
    e = hash_find(&frame_table, &(f.hash_elem));
    return e != NULL ? hash_entry (e, struct frame, hash_elem) : NULL;
}



void
frame_table_init (void){
    hash_init(&frame_table, frame_hash, frame_less, NULL);
}






/* PROCESS FRAME TABLE */

/*
palloc_get_page () 를 쓰면 알아서 page를 할당함
그런데 upgae랑 kpage를 mapping하는 건 install_page () 에서 이루어짐
=> install_page ()에서 mapping을 추가하자
=> 다만 eviction이 필요한 경우 palloc_get_page ()에서 빈 frame을 찾아야 함
    그래서 palloc.c 의 수정이 필요함
    palloc.c 건드리긴 좀 그러니까 frame.c에 palloc_get_page ()에 대한 wrapper 함수를 만들자
*/



// Lock이 필요할까?

void *
frame_get_one (enum palloc_flags flags)
{
    return palloc_get_page(flags);
}

void *
frame_get_multiple (enum palloc_flags flags, size_t page_cnt)
{   
    return palloc_get_multiple(flags, page_cnt);
}

void
frame_free_one (void *page)
{
    palloc_free_page (page);
}

void
frame_free_multiple (void *pages, size_t page_cnt)
{
    palloc_free_multiple (pages, page_cnt);
}



void
frame_table_add (void *kpage, void *pte)
{
    struct frame *f = malloc (sizeof(struct frame));
    f->frame_addr = (int)(kpage);
    f->mapped_pte = pte;

    hash_insert (&frame_table, &(f->hash_elem));
}

void
frame_table_delete (void *kpage){
    struct frame *f = frame_lookup ((int)(kpage));
    hash_delete (&frame_table, &(f->hash_elem));
    free (f);
}