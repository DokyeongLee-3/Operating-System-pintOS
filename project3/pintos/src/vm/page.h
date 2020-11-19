#ifndef PAGE_H
#define PAGE_H

#include "lib/kernel/hash.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"
#include <debug.h>


/* Manual A.8.5 example 이용 */

struct file
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

extern struct file;

struct page{
  struct hash_elem hash_elem;
  void *kernel_vaddr;         // load segment 보면 file_read로 kpage에 데이터 써줌, 즉 데>이터가 있는 시작  주소, kernel address가 아니라 swap slot에 대한 주소 정보가 될수도?
  void *user_vaddr; // 나중에 page fault 유발하는 user virtual address
  struct file file_;
  uint16_t size;
  bool writable;
};


unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED);

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);

struct page *page_lookup(const void *address);

#endif
