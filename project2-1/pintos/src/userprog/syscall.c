#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"


struct file
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

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

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) //intr_handlers 배열에 0x30(interrupt)에 해당하는 handler는 syscall_handler라고 등록
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf("syscall num : %d\n", *(uint32_t *)(f->esp));
  //printf("now esp is %p\n", (uint32_t *)(f->esp));

  char *front;
  char *next;
  front = strtok_r(thread_current()->name, " ", &next);

  if(((uint32_t *)(f->esp) > 0xc0000000) || ((uint32_t *)(f->esp) < 0x8048000) || ((uint32_t *)(f->esp) == NULL)){
    //printf("invalid user given pointer!!!!\n");
    printf("%s: exit(%d)\n", front, -1);
    thread_exit();
  }

  //printf("current thread is %p\n", &thread_current()->name);
  //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 200, 1);


  else if(*(uint32_t *)f->esp == 0){  //SYS_HALT
    shutdown_power_off();
  }



  else if(*(uint32_t *)f->esp == 1){ //SYS_EXIT
    //int exit_tid = thread_current()->tid;
    //(uint32_t *)(f->eax) = 0;
    if( ((uint32_t *)f->esp+4  > 0xc0000000) || ((uint32_t *)f->esp+4 < 0x8048000) || ( (uint32_t *)(f->esp) == NULL)){
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }
    else{
      printf("%s: exit(%d)\n", front, *((uint32_t *)f->esp+1)); 
      thread_exit();
    }

  }


  
  else if(*(uint32_t *)f->esp == 4){ //SYS_CREATE
    if(*((uint32_t *)f->esp+4) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = false;
      thread_exit();
    }
    if(pagedir_get_page(thread_current()->pagedir, *((uint32_t *)f->esp+4)) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = false;
      thread_exit();
    }
    enum intr_level old_level = intr_enable();
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);

    f->eax = filesys_create (*((uint32_t *)f->esp+4), *((uint32_t *)f->esp+5));
    intr_set_level(old_level); 
  }




  else if(*(uint32_t *)f->esp == 6){ //SYS_OPEN
    enum intr_level old_level = intr_enable();
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);  
    if(*((uint32_t *)(f->esp+4)) == NULL){
      f->eax = -1;
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    } 

//printf("file name is %s\n", *((uint32_t *)f->esp+1));

//printf("now file descriptor is %d\n", thread_current()->tid);

    if(pagedir_get_page( thread_current()->pagedir, *((uint32_t *)f->esp+1) ) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = -1;
      thread_exit();
    }
    
    struct file *file_ = filesys_open(*((uint32_t *)(f->esp+4)));
    static int return_val;
    //printf("open : %s\n", *((uint32_t *)(f->esp+4)));
    if(file_ == NULL)
      f->eax = -1;

    else{
//hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
      if((struct file *)file_->inode->open_cnt == 1){
        f->eax = thread_current()->tid; // return file descriptor 
        return_val = f->eax;
       //printf("return val at first is %d\n", return_val);
      }
      else{
        //struct thread *t = list_entry(&(file_->inode->elem), struct thread, elem);
        //printf("tid is %d\n", t->tid);
        //file_ = file_reopen(file_);
        //printf("reopen return val is %d\n", return_val+1);
        f->eax = return_val+1;
        return_val = return_val+1; //why pass????????
      }

     }

    //printf("open counter: %d\n",file_->inode->open_cnt);

    intr_set_level(old_level); 
  }





  else if(*(uint32_t *)f->esp == 8){

  }





  else if(*(uint32_t *)f->esp == 9){ // SYS_WRITE 
    //int count = 0; // count length of an argument
    //int tmp = 72;
   
    //while(*((char *)f->esp+tmp) != ' '){
      //printf("if i die\n");
      //count = count+1; 
      //tmp = tmp+1;
    //}
    if((uint32_t *)(f->esp+20) > 0xc0000000 || (uint32_t *)(f->esp+20) < 0x8048000 || (uint32_t *)(f->esp+20) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }

    if(*(uint32_t *)(f->esp+24) > 0xc0000000 || *(uint32_t *)(f->esp+24) < 0x8048000 || *(uint32_t *)(f->esp+24) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }
    
    if((uint32_t *)(f->esp+28) > 0xc0000000 || (uint32_t *)(f->esp+28) < 0x8048000 || (uint32_t *)(f->esp+28) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }

    if(*(uint32_t *)(f->esp+20) == 0x1)
      putbuf(*(uint32_t *)(f->esp+24), *(uint32_t *)(f->esp+28));
      f->eax = *(uint32_t *)(f->esp+28);
  }





  else if(*(uint32_t *)f->esp == 12){ //SYS_CLOSE  why all pass without any code?
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1); 
    //printf("file descriptor is %p\n", ((uint32_t *)(f->esp+8)));
    //printf("tid is %d\n", thread_current()->tid);
    //printf("file descriptor is %p\n", *((uint32_t *)(f->esp+8)));
    //file_close((struct file *)*((uint32_t *)(f->esp+8))); fd를 가지고 struct file*를 끌어낼 수 있나
  }





}

//void my_exit(int status){
//  exit(1);
//  NOT_REACHED ();
//}



