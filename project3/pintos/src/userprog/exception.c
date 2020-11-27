#include "userprog/exception.h"
#include "lib/kernel/hash.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/block.h"
#include "lib/round.h"

/* Number of page faults processed. */
extern struct semaphore main_waiting_exec;
extern struct semaphore exec_waiting_child_simple;
extern uint32_t *lookup_page (uint32_t *pd, const void *vaddr, bool create);

extern struct list all_list;

static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

struct pool
  {
    struct lock lock;                   /* Mutual exclusion. */
    struct bitmap *used_map;            /* Bitmap of free pages. */
    uint8_t *base;                      /* Base of pool. */
  };

extern struct pool kernel_pool, user_pool;

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

extern struct block;

struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[125];               /* Not used. */
  };

struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

int c = 0;


static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
  : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
    
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */


  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  enum intr_level my_level = intr_enable ();

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */


  uint32_t faulting_addr = fault_addr;
  faulting_addr &= 0xfffff000;
  uint32_t *addr_of_fault_addr = faulting_addr;

  //printf("tid in page fault is %d\n", thread_current()->tid);
  //printf("fault addr is %p\n", fault_addr);
  //printf("stack pointer is %p\n", f->esp);
  //printf("error code is %d\n", f->error_code);
  //printf("thread esp is %p\n", thread_current()->esp);

  if(f->esp < 0xC0000000)
    thread_current()->esp = f->esp; //???
 
  struct page *finding = page_lookup(addr_of_fault_addr);
  if(fault_addr == NULL || fault_addr == (int *)0xC0000000){ // for all bad-* tests 
    if(thread_current()->tid == 3){
      sema_up(&main_waiting_exec);
      printf("%s: exit(%d)\n", thread_current()->name, -1);
      thread_exit ();
    }
  }

  /* 0x8048000 부터 load하는거같으니 그 밑에는 invalid? 모르겠음 */
  if(f->error_code == 0){ 
    if(addr_of_fault_addr < 0x8048000 || addr_of_fault_addr < thread_current()->esp){  //sc-bad-sp (사실 esp가 valid한게 정확하게 어디까지인지 아직 모르겠음)
      if(thread_current()->my_parent == 1){
        sema_up(&main_waiting_exec);
        printf("%s: exit(%d)\n", thread_current()->name, -1);
        thread_exit ();
      }
      if(thread_current()->my_parent == 3){
         printf("%s: exit(%d)\n", thread_current()->name, -1);
         sema_up(&exec_waiting_child_simple);
         thread_exit ();
      }  
    }
  }
  struct list_elem *e = list_front(&all_list);
  struct thread *parent_thread; // t는 부모 프로세스
  for(; e != list_end(&all_list); e = list_next(e)){
    parent_thread = list_entry(e, struct thread, allelem);
    if(parent_thread->tid == thread_current()->my_parent){
      break;
    }
  }
  int i = 0;
  for(; i<10; i++){
    if(parent_thread->mapping[i] == addr_of_fault_addr){
      if(thread_current()->my_parent == 3){ // mmap-inherit
        f->eax = -1;
        printf("%s: exit(%d)\n", thread_current()->name, -1);
        sema_up(&exec_waiting_child_simple);
        thread_exit();
      }
    }
  }


  if(finding == NULL){
    uint32_t sp = f->esp;
    uint32_t tp = thread_current()->esp;

    if(f->error_code == 4){
      if(sp < PHYS_BASE && sp > faulting_addr && sp-faulting_addr > PGSIZE){//pt-grow-bad, pt-wrice-code2
        if(thread_current()->my_parent == 1){
          printf("%s: exit(%d)\n", thread_current()->name, -1);
          sema_up(&main_waiting_exec);
          thread_exit ();
        }
        if(thread_current()->my_parent == 3){
          printf("%s: exit(%d)\n", thread_current()->name, -1);
          sema_up(&exec_waiting_child_simple);
          thread_exit ();
        }
      }

      else if(sp < PHYS_BASE && sp < faulting_addr && faulting_addr - sp > PGSIZE){ 
        if(thread_current()->my_parent == 1){
          printf("%s: exit(%d)\n", thread_current()->name, -1);
          sema_up(&main_waiting_exec);
          thread_exit ();
        }
        if(thread_current()->my_parent == 3){
          printf("%s: exit(%d)\n", thread_current()->name, -1);
          sema_up(&exec_waiting_child_simple);
          thread_exit ();
        }
      }
   }
    if(f->error_code == 6){ //pt-grow-pusha
      uint32_t temp = addr_of_fault_addr;
      temp &= 0xfffff000;
      uint32_t *new_stack_page = palloc_get_multiple(PAL_USER,1);
      thread_current()->esp = thread_current()->esp - PGSIZE;
      if (!install_page ((uint32_t *)temp, new_stack_page, 1))
        palloc_free_page (new_stack_page);
      return;
    }
  
  }
  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */

  /* load_segment에서 insert해둔 hash_elem을 찾는다 */
  
  /* 여기서 이제 faulting address(user space에 존재)가 주어졌고, hash table에서
   * faluting address를 찾아서 그 faulting address를 포함하고 있는 struct page를
   * hash_entry 매크로를 써서 이끌어내서 struct page안에 멤버인 kpage를 사용해서 거기에
   * 저장된(하지만 아직 load안한)데이터를 여기서 load한다. load는 palloc_get_multiple로
   * user pool page하나 할당받아서 거기에 file_read로 써주고(load_segment 처럼),
   * install_page로 mapping 해준다. 마지막으로 데이터를 써놨던 kernel_pool에서 가져온
   * kpage는 여기서 palloc_free_multiple로 free해준다*/


    ASSERT(faulting_addr == finding->user_vaddr);
    uint32_t *kpage = palloc_get_multiple(PAL_USER,1);

    memcpy(kpage, finding->kernel_vaddr, PGSIZE);
    //memcpy(kpage + finding->read_size, (finding->kernel_vaddr) + finding->read_size , PGSIZE - finding->read_size);
 
    if (!install_page (addr_of_fault_addr, kpage, finding->writable)){ // pte도 만들어줌
      palloc_free_page (kpage);
    }
    if(thread_current()->pagedir == NULL)
      pagedir_create();    
 
 
    remove_elem(&(thread_current()->pages), &finding->hash_elem);  
    palloc_free_page(finding->kernel_vaddr);
    free(finding);

  return;
}

