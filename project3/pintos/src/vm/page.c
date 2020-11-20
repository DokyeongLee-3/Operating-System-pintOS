#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/stddef.h"
#include "vm/page.h"
#include "lib/kernel/hash.h"


/*
unsigned my_divisor(const void *buf_, size_t, size){
  const unsigned char *buf = buf_;
  unsigned hash = buf

  ASSERT (buf != NULL);
  hash
}
*/


unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED){
  const struct page *p = hash_entry(p_, struct page, hash_elem);
  printf("######## argument is %p ###########\n", p->user_vaddr);
  return hash_bytes(&p->user_vaddr, sizeof p->user_vaddr);
}

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->user_vaddr < b->user_vaddr;
}

struct page *page_lookup(const void *address){
  struct page p;
  struct hash_elem *e;

  p.user_vaddr = address;

  e = hash_find(&(thread_current()->pages), &p.hash_elem);
 
  return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}
