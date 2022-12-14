#include "userprog/syscall.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "filesys/inode.h"
#include "threads/synch.h"
#include "userprog/process.h"
#include "vm/page.h"
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"
#include "lib/round.h"
#include "lib/limits.h"
#include "lib/ustar.h"
#include "filesys/directory.h"


typedef unsigned long elem_type;
#define ELEM_BITS (sizeof (elem_type) * CHAR_BIT)

struct dir_entry
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
  };

struct bitmap
  {
    size_t bit_cnt;     /* Number of bits. */
    elem_type *bits;    /* Elements that represent bits. */
  };

struct pool
  {
    struct lock lock;
    struct bitmap *used_map;
    uint8_t *base;
  };

extern struct list all_list;
extern struct pool kernel_pool, user_pool;


#define MAP_FAILED ((mapid_t) -1)
typedef int mapid_t;
//struct file
//  {
//    struct inode *inode;        /* File's inode. */
//    off_t pos;                  /* Current position. */
//    bool deny_write;            /* Has file_deny_write() been called? */
//  };

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

static inline elem_type
bit_mask (size_t bit_idx)
{
  return (elem_type) 1 << (bit_idx % ELEM_BITS);
}

//extern bool load_success;
extern struct semaphore main_waiting_exec;
extern struct semaphore exec_waiting_child_simple;
extern struct semaphore multichild[46]; //multi-recurse.c, rox-multichild.c?????? ??????

static inline size_t
elem_idx (size_t bit_idx)
{
  return bit_idx / ELEM_BITS;
}


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) //intr_handlers ????????? 0x30(interrupt)??? ???????????? handler??? syscall_handler?????? ??????
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
enum intr_level my_level = intr_disable();
//printf("syscall number is %d and tid is %d\n", *(uint32_t *)f->esp, thread_current()->tid);

  struct list_elem *e = list_front(&all_list);
  struct thread *parent;
    for(; e != list_end(&all_list); e = list_next(e)){
      parent = list_entry(e, struct thread, allelem);
      if(parent->tid == thread_current()->my_parent){ // t??? ?????? ?????????
          break;
      }
    }
  if(parent->abnormal_child == thread_current()->tid){
    //printf("I am an abnormal thread: %d\n", thread_current()->tid);
    parent->exit_status_of_child[thread_current()->tid%44] = 0;
    thread_exit();
  }



/* multi-oom?????? consume_some_resoucre??? ?????? ???????????? abnormal?????? terminate
 ??? ?????? ?????????????????? ?????? ??????????????? syscall_handler ?????? ????????? ??? ?????? ????????? ??? kernel resource??? free*/

/*
  if(thread_current()->abnormal_child != 0){ //?????? ?????? ????????? ??? ???????????? 0
// process_execute?????? abnormal child tid??? ?????? ?????????????


    enum intr_level my_level = intr_disable();

    struct list_elem *e = list_front(&all_list);
    struct thread *abnormal;
    for(; e != list_end(&all_list); e = list_next(e)){
      abnormal = list_entry(e, struct thread, allelem);
      if(abnormal->tid == thread_current()->abnormal_child){
        break;
      }
    }

    int k = 0;
    for(;k < 10; k++){
      if(abnormal->file_descriptor_table[k] != NULL){
        struct file *wasted = abnormal->file_descriptor_table[k];
        if(wasted->deny_write == true){
          file_allow_write(wasted);
          inode_close(wasted->inode);
          free(wasted);
        }
      }
    }

    intr_set_level(my_level);
  } 
*/


  char *front;
  char *next;
  front = strtok_r(thread_current()->name, " ", &next);

  if((*(uint32_t *)(f->esp) > 0xc0000000) || ((uint32_t *)(f->esp) < 0x8048000) || ((uint32_t *)(f->esp) == NULL)){
    printf("%s: exit(%d)\n", front, -1);
    thread_exit();
  }


  else if(*(uint32_t *)f->esp == 0){  //SYS_HALT
    shutdown_power_off();
  }


  else if(*(uint32_t *)f->esp == 1){ //SYS_EXIT void exit(int status)

//printf("sys exit and my tid is %d\n", thread_current()->tid);

  enum intr_level old_level = intr_disable();

    thread_current()->load_success = true;

  if((uint32_t *)(f->esp+4) >= 0xC0000000){ //for sc-bad-arg case
    printf("%s: exit(%d)\n", front, -1);
    f->eax = -1;
    if(thread_current()->tid == 3)
      sema_up(&main_waiting_exec);
    thread_exit();
  }

   
  else{
//printf("sys exit else and status is %d\n", *(uint32_t *)(f->esp+4));

      printf("%s: exit(%d)\n", front, *(uint32_t *)(f->esp+4));

      int16_t my_parent = 0;
      my_parent = thread_current()->my_parent;

     //printf("my tid is %d and my parent is %d\n", thread_current()->tid, my_parent);
      struct list_elem *e = list_front(&all_list);
      struct thread *t;
      for(; e != list_end(&all_list); e = list_next(e)){
        t = list_entry(e, struct thread, allelem);
        if(t->tid == my_parent){ // t??? ?????? ?????????
          break;
        }
      }

      t->exit_status_of_child[(thread_current()->tid)%44] = *(uint32_t *)(f->esp+4);
//????????? ????????? exit_status_of_child??? ?????? tid index??? ???????????? ?????? status??? ??????????????? ???
//?????? spawn_child??? recursion??? ?????? ????????? 47->45->43...????????? ?????? ??? return ??????, ?????? ???>???, 45??? sys_exit?????? ????????? ?????? sys_wait??? ???????????? 43??? wait??? ??????????????? 30(depth)??? ???>????????? ?????? 43??? exit_status_of_child??? ?????????


// consume_resource_and_die??? case4 ?????? ??????????????? sys_exit call?????? ?????? ????????? ??????
// -> ????????? sema_up??? ?????? ??????. ?????? ?????? ????????? tid=21??????, ????????? tid=22, 23 ??? ?????? 22??? multi-oom ~ -k ?????? ???????????? ?????????????????? ????????? tid=21??? sema_down??? ??????
// -> ?????? sema_up??? ???????????? ?????? 21??? 23 ????????? ?????? semaphore??? sema_up ????????? ??? ??????

     /* exit?????? user pool??? load?????? page palloc_free_multiple ?????????
      * palloc_free_multiple?????? frame_table ????????? ????????? ??? */ 

    
     while(!hash_empty(&thread_current()->pages)){
       int i = 0;
       for(; i < thread_current()->pages.bucket_cnt; i++){

         while(!list_empty(&thread_current()->pages.buckets[i])){
           struct page *temp = list_front(&thread_current()->pages.buckets[i]);
           palloc_free_page(temp->kernel_vaddr);
           hash_delete(&thread_current()->pages, &temp->hash_elem);
           // spt_entry, ??? kernel_pool page ????????? ??????, hash?????? ?????????(hash_delete)
         }
       }
     }


      int8_t index = 0;
      struct file *wasted;

      if(my_parent == 1){
        sema_up(&main_waiting_exec);
      }

     else if(my_parent %44 == 3){
        sema_up(&exec_waiting_child_simple); //exec-multiple?????? pid=3??? ???????????? ???????????? exec?????? pid =4,5,6...?????? ????????? ??? ??? ??????-> pid=4?????? ?????? sema_up?????? pid=5??? ?????????????????? sema_up?????? ?????? ??? sema_up????????? ??? -> ??? ????????? ????????? ??????????????? exit?????? ????????? sema_up????????????.
      }
// ????????? ????????? ???????????? ????????? ??????...
      else if(my_parent %44 == 4){
        sema_up(&multichild[0]);
      }
      else if(my_parent %44 == 5){
        sema_up(&multichild[1]);
      }
      else if(my_parent %44 == 6){
        sema_up(&multichild[2]);
      }
      else if(my_parent %44 == 7){
        sema_up(&multichild[3]);
      }
      else if(my_parent %44 == 8){
        sema_up(&multichild[4]);
      }
      else if(my_parent %44 == 9){
        sema_up(&multichild[5]);
      }
      else if(my_parent %44 == 10){
        sema_up(&multichild[6]);
      }
      else if(my_parent %44 == 11){
        sema_up(&multichild[7]);
      }
      else if(my_parent %44 == 12){
        sema_up(&multichild[8]);
      }
      else if(my_parent %44 == 13){
        sema_up(&multichild[9]);
      }
      else if(my_parent %44 == 14){
        sema_up(&multichild[10]);
      }
      else if(my_parent %44 == 15){
        sema_up(&multichild[11]);
      }
      else if(my_parent %44 == 16){
        sema_up(&multichild[12]);
      } 
      else if(my_parent %44 == 17){
        sema_up(&multichild[13]);
      }
      else if(my_parent %44 == 18){
        sema_up(&multichild[14]);
      }

      else if(my_parent %44 == 19){
        sema_up(&multichild[15]);
      }

      else if(my_parent %44 == 20){
        sema_up(&multichild[16]);
      }
      else if(my_parent %44 == 21){
        sema_up(&multichild[17]);
      }
      else if(my_parent %44 == 22){
        sema_up(&multichild[18]);
      }
      else if(my_parent %44 == 23){
        sema_up(&multichild[19]);
      }
      else if(my_parent %44 == 24){
        sema_up(&multichild[20]);
      }
      else if(my_parent %44 == 25){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[21]);
      }
      else if(my_parent %44 == 26){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[22]);
      }
      else if(my_parent %44 == 27){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[23]);
      }
      else if(my_parent %44 == 28){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[24]);
      }
      else if(my_parent %44 == 29){
        sema_up(&multichild[25]);
      }
      else if(my_parent %44 == 30){
        sema_up(&multichild[26]);
      }

      else if(my_parent %44 == 31){ 
        sema_up(&multichild[27]);
      }

      else if(my_parent %44 == 32){
        sema_up(&multichild[28]);
      }
      else if(my_parent %44 == 33){
        sema_up(&multichild[29]);
      }
      else if(my_parent %44 == 34){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[30]);
      }
      else if(my_parent %44 == 35){
        sema_up(&multichild[31]);
      }
      else if(my_parent %44 == 36){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[32]);
      }
      else if(my_parent %44 == 37){
        sema_up(&multichild[33]);
      }
      else if(my_parent %44 == 38){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[34]);
      }
      else if(my_parent %44 == 39){
        sema_up(&multichild[35]);
      }
      else if(my_parent %44 == 40){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[36]);
      }
      else if(my_parent %44 == 41){
        sema_up(&multichild[37]);
      }
      else if(my_parent %44 == 42){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[38]);
      }
      else if(my_parent %44 == 43){
        sema_up(&multichild[39]);
//printf("sema value is %d\n", (&multichild[39])->value);
      }
      else if(my_parent %44 == 0){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[40]);
      }
      else if(my_parent > 44 && my_parent%44 == 1){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[41]);
      }
/*
      else if(my_parent > 44 && my_parent%44 == 2){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[42]);
      }
      else if(my_parent > 44 && my_parent%44 == 3){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[43]);
      }
      else if(my_parent > 44 && my_parent%44 == 4){
//printf("sema_up and my tid is %d\n", thread_current()->tid);
        sema_up(&multichild[44]);
      }
*/
//printf("any way thread_exit is called\n");
      thread_exit();
    }
  }






  else if(*(uint32_t *)f->esp == 2){ //SYS_EXEC

//printf("SYS_EXEC start!\n");

    if(pagedir_get_page(thread_current()->pagedir, *(uint32_t *)(f->esp+4)) == NULL){

      if(thread_current()->tid == 3){
        sema_up(&main_waiting_exec);
      }
      else{
        sema_up(&exec_waiting_child_simple);
      }

      printf("%s: exit(%d)\n", front, -1);
      f->eax = -1;
      thread_exit();
    }
    

    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 200, 1); 
//printf("exec depth(k) is %d\n", *(uint32_t *)(f->esp+28));
    if(*(uint32_t *)(f->esp+28) == 31){ //multi-oom?????? depth??? 30 ????????? child_pid = spawn_child(n+1, RECURSE)????????? 

      f->eax = -1;
      struct thread *parent; // depth=30??? ??????????????? ???????????? ?????? exit status??? ??????????????? ??????
      struct list_elem *e = list_front(&all_list);
      for(; e != list_end(&all_list); e = list_next(e)){
        parent = list_entry(e, struct thread, allelem);
        if(parent->tid == thread_current()->my_parent){
//printf("my parent is %d and *(uint32_t *)(f->esp+28)-1 is %d\n",thread_current()->my_parent, *(uint32_t *)(f->esp+28)-1);
//printf("%d will give %d to %d..\n", thread_current()->tid, (*(uint32_t *)(f->esp+28))-1, thread_current()->my_parent);
          parent->exit_status_of_child[(thread_current()->tid)%44] = *(uint32_t *)(f->esp+28)-1;
        }
      } 
    }

    else{
      int pid = process_execute(*((uint32_t *)(f->esp+4)));

      if(pid == TID_ERROR){
        printf("%s: exit(%d)\n", front, -1);
        f->eax = -1;
        thread_exit();
      }
      struct list_elem *e = list_front(&all_list);
      struct thread *t;
      for(; e != list_end(&all_list); e = list_next(e)){
        t = list_entry(e, struct thread, allelem);
        if(t->tid == pid)
          break;
      }
      if(t->tid == pid && t->load_success == true){ // ?????? exec-missing?????? child??? load ?????? ?????? ??? child??? load_success ????????? false??? ????????? child_waiting_exit????????? sema?????? block??? ?????? ?????? ????????? ??? child??? exit_status???????????? sema??? ???????????? child thread??? exit?????? ???
        f->eax = pid;
      } 

      else if(thread_current()->exit_status_of_child[pid%44] != -1){ // exit_status??? sys_exit?????? ??????-> ??????????????? sys_exit?????? user_program??? ?????? thread??? ???????????? -1??? ?????? ????????? ??????????????????
        f->eax = pid;
      }

      else{ //exec-missing?????? ????????? load??? ???????????? sys_exit?????? ??????????????? ??????
        f->eax = -1;
      }

    }
  }



  else if(*(uint32_t *)f->esp == 3){ // SYS_WAIT int wait(pid_t pid)


//printf("sys wait and waiting %d\n", *((uint32_t *)(f->esp+4)));

    if(thread_current()->exit_status_of_child[(*(uint32_t *)(f->esp+4))%44] == -1){ // multi-oom?????? ?????? ??????????????? abnormal?????? sys_exit????????? ????????? ??????(multi-oom??????if(wait(child_pid)!= -1) ?????? ????????? ?????? ?????????
      f->eax = -1;
    }
   
    else{
 
      struct thread *t;
      struct list_elem *e = list_front(&all_list);
      bool find = false;
      for(; e != list_end(&all_list); e = list_next(e)){
        t = list_entry(e, struct thread, allelem);
        if(t->tid == *(uint32_t *)(f->esp+4)){
          find = true;
          break;
        }
      }
      if(find == false){ //multi-oom ???????????? abnormal?????? ????????? thread(sys_exit??? ???????????? ?????? ??????)??? wait????????? ??? ?????? or ?????? exit??? ????????? ???????????? ????????? or wait-twice.c ?????? ?????? wait??? ????????? ??? wait ????????? ?????? thread??? ???????????? all_list?????? ?????? ??? ?????? ????????? exit_status_of_child???????????? ????????? exit_status ????????????
  
        f->eax = thread_current()->exit_status_of_child[(*(uint32_t *)(f->esp+4))%44];
        //wait-twice.c ?????? ?????? ?????? wait?????? reap??? ????????? ??? wait????????? ?????? ?????????
        //?????? ????????? ?????? ?????? ????????? ?????????
        thread_current()->exit_status_of_child[(*(uint32_t *)(f->esp+4))%44] = -1;
      }

      else if(*((uint32_t *)f->esp+4) == -1){
        printf("%s: exit(%d)\n", front, -1);
        f->eax = -1;
        thread_exit();
      }
   
      else {
        f->eax = process_wait(*(uint32_t *)(f->esp+4)); //????????? pid(4)??? ????????? ??????
      }
      //printf("sys_exit end...\n"); 
    }
  }

  
  else if(*(uint32_t *)f->esp == 4){ //SYS_CREATE

//hex_dump(f->esp, f->esp, 200, 1);
//printf("sys create and name is %s\n", *(((uint32_t *)f->esp)+4)); 

    if(*((uint32_t *)f->esp+4) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      f->eax = false;
      thread_exit();
    }
    if(pagedir_get_page(thread_current()->pagedir, *((uint32_t *)f->esp+4)) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      f->eax = false;
      thread_exit();
    }
    enum intr_level old_level = intr_enable();
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);

    f->eax = filesys_create (*((uint32_t *)f->esp+4), *((uint32_t *)f->esp+5));


//fsutil_ls(NULL);


    intr_set_level(old_level); 
  }



  else if(*(uint32_t *)f->esp == 5){ //SYS_REMOVE
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
    enum intr_level old_level = intr_enable();
    //printf("%p\n", *(uint32_t *)(f->esp+4));
    //
    int i = 0;
    f->eax = filesys_remove(*(uint32_t *)(f->esp+4));

 
    intr_set_level(old_level);
  } 


  else if(*(uint32_t *)f->esp == 6){ //SYS_OPEN
  
//printf("sys open and my tid is %d\n", thread_current()->tid);
//printf("I will open %s\n", *((uint32_t *)(f->esp+4)));

    enum intr_level old_level = intr_enable();
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 200, 1);  

    if(*((uint32_t *)(f->esp+4)) >= 0xc0000000){ // multi-oom?????? 0xc0000000??? open??? ????????? ???

//printf("wrong address in sys_open and my tid is %d\n", thread_current()->tid);

      f->eax = -1;
      //thread_exit();
    }

    else if(*((uint32_t *)(f->esp+4)) == NULL){
      f->eax = -1;
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      thread_exit();
    } 

//printf("file name is %s\n", *((uint32_t *)f->esp+1));
//printf("now file descriptor is %d\n", thread_current()->tid);


    else if(pagedir_get_page( thread_current()->pagedir, *((uint32_t *)f->esp+1) ) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = -1;
      sema_up(&main_waiting_exec);
      thread_exit();
    }
    else{
//////////// ???????????????, ????????? ???????????? struct thread ????????? file_descriptor_table??? 50?????? ???????????? ????????? sys_open ?????? ??????????????? ????????? tid??? 400??? ????????? ????????? ?????? ??? ?????????, file_descriptor_table??? 129?????? ????????? ?????? ???????????? ????????????///////////////

        if(strcmp(*((uint32_t *)(f->esp+4)),"/") == 0){ // "/"??? sys_open????????? ?????? ?????? ==> ??????????????? ????????? ?????? ??????
          struct file *file = malloc(sizeof(struct file));
          file->inode = dir_open_root();
          file->pos = 0;
          file->deny_write = 0;
          
          int i = 0;
          for(; i <5; i++)
            if(thread_current()->directory_table[i] == NULL)
              break;
          thread_current()->directory_table[i] = file;
          // ??????????????? fd??? 100+tid??? ????????? ??? ???????????? ?????? ??????
          thread_current()->array_of_dir_fd[i] = 100 + thread_current()->tid; 
          f->eax = thread_current()->array_of_dir_fd[i];
          return;
          
        }

        static struct file *file_;
        file_ = filesys_open(*((uint32_t *)(f->esp+4)));
        struct thread *now = thread_current();
        int return_val = 2; // ?????? ????????? ??? file_descriptor_table??? index(0?????? ??????)

        
        if(file_ == NULL){
          f->eax = -1;
          //sema_up(&main_waiting_exec);
        }
        
         
        else if(memcmp(thread_current()->name, *(uint32_t *)(f->esp+4), strlen(front)) == 0){
         //???????????? ??? ????????? ?????????? --> ????????? ??? ????????? write????????? file_deny_write?????? ???????????? True??? ????????? ??????
         //printf("file name is %s\n", *(uint32_t *)(f->esp+4));
         //printf("deny_write?  %d\n", file_->deny_write);
          if(file_ != NULL){
            file_deny_write(file_);
            f->eax = thread_current()->tid; // return file descriptor
            now->file_descriptor_table[return_val] = file_;
            now->array_of_fd[return_val] = thread_current()->tid;
            return_val = return_val + 1; //  ???????????? file_descritor_table??? ?????????  index>??? file* ??? ??????
          }
          
        }

        else{
          if((struct file *)file_->inode->open_cnt == 1){ //open-normal ?????? ????????? ????????????

            //assert(now->file_descriptor_table[return_val] == NULL); //?????? ??? thread??? ??? ????????? ?????? ?????? ????????????, thread??? ???????????? ?????? ????????? ????????? ?????????

//printf("open_cnt is 1 and my tid is %d\n", thread_current()->tid);            

            f->eax = thread_current()->tid; // return file descriptor 
            now->file_descriptor_table[return_val] = file_;
              //?????? file_descritor_table ????????? ?????? struct file pointer??? fd??? array_of_fd??? ??????
            now->array_of_fd[return_val] = thread_current()->tid;
            return_val = return_val + 1; //  ???????????? file_descritor_table??? ?????????  index>??? file* ??? ?????? ??????


          }
          else{ // open-twice ?????? ?????? ??????????????? ?????? ????????? 2??? ????????? ?????? ??????
            
            while(now->file_descriptor_table[return_val] != NULL){
              return_val = return_val + 1;
            }
            if(return_val == 10){
              f->eax = -1;
            }
            else{
              now->array_of_fd[return_val] = now->tid + return_val;
              now->file_descriptor_table[return_val] = file_;
              f->eax = now->array_of_fd[return_val];
              return_val = return_val + 1;
            }
          }
       }

      //printf("open counter: %d\n",file_->inode->open_cnt);
      }
    
  //  }
      intr_set_level(old_level);
  }


  else if(*(uint32_t *)f->esp == 7){ // SYS_FILESIZE
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
    int16_t fd = 0;
    while(thread_current()->array_of_fd[fd] != *(uint32_t *)(f->esp+4)){
      fd = fd + 1;
    }
    struct file *file_ = thread_current()->file_descriptor_table[fd];
    f->eax = file_length(file_);
  }


  else if(*(uint32_t *)f->esp == 8){ // SYS_READ
    enum intr_level old_level = intr_enable();
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
    //printf("fd is %d\n", *(uint32_t *)(f->esp+20));
    if(*(uint32_t *)(f->esp+20) <= 0 || *(uint32_t *)(f->esp+20) == 1 || *(uint32_t *)(f->esp+20) == 2 || *(uint32_t *)(f->esp+20) > 128 ){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = -1;
      sema_up(&main_waiting_exec);
      thread_exit();
    }
    if( *(uint32_t *)(f->esp+24) > 0xc0000000){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = -1;
      sema_up(&main_waiting_exec);
      thread_exit();
    }

    else{
      int16_t fd = 0;
      while(thread_current()->array_of_fd[fd] != *(uint32_t *)(f->esp+20)){
        fd = fd + 1;
      }

      struct file *file_ = thread_current()->file_descriptor_table[fd];
    //printf("file descriptor struct address in read : %p\n", file_); 
    //int ss = *(uint32_t *)(f->esp+28);
      off_t ret_val = file_read(file_, *(uint32_t *)(f->esp+24), *(uint32_t *)(f->esp+28));
      f->eax = ret_val;
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
    //file_->pos = file_->pos + *(uint32_t *)(f->esp+28);
    }
    intr_set_level(old_level);
  }



  else if(*(uint32_t *)f->esp == 9){ // SYS_WRITE 

    enum intr_level old_level = intr_enable();

//printf("sys write and tid is %d\n",thread_current()->tid);
//printf("first arg is %d\n", *(uint32_t *)(f->esp+20));
//printf("second arg is %s\n", *(uint32_t *)(f->esp+24));
//printf("third arg is %d\n", *(uint32_t *)(f->esp+28));


    if(*(uint32_t *)(f->esp+24) > 0xc0000000 || *(uint32_t *)(f->esp+24) < 0x8048000 || *(uint32_t *)(f->esp+24) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      f->eax = 0;
      thread_exit();
    }
    if(pagedir_get_page( thread_current()->pagedir, *(uint32_t *)(f->esp+24) ) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      f->eax = 0;
      thread_exit();
    }
    if(*(uint32_t *)(f->esp+20) == 0){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      f->eax = 0;
      thread_exit();
    }
    if(*(uint32_t *)(f->esp+20) < 0 || *(uint32_t *)(f->esp+20) > 128){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      f->eax = 0;
      thread_exit();
    }

    else if(*(uint32_t *)(f->esp+20) == 0x1){
      //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
      //bad_write case??? ????????? ?????? ????????? ??????
      putbuf(*(uint32_t *)(f->esp+24), *(uint32_t *)(f->esp+28));
      //sema_up(&main_waiting_exec);
      f->eax = *(uint32_t *)(f->esp+28);
    }
    else{
      int16_t index = 0;
      while(thread_current()->array_of_fd[index] != *(uint32_t *)(f->esp+20)){
        index = index + 1;
      }
    
      if(thread_current()->file_descriptor_table[index]->deny_write == true){  //rox-simple.c -> ?????? rox-simple ????????? ???????????? ??????????????????, rox-simple ??????????????? ?????? ????????? ?????? ?????? return 0
        f->eax = 0;
      }
      else{
        struct thread *cur = thread_current();
        struct file *file_ = cur->file_descriptor_table[index];
        int ret_val = file_write(file_, *((uint32_t *)(f->esp+24)), *(uint32_t *)(f->esp+28));


        f->eax = ret_val;
      }
    }
    intr_set_level(old_level);
  }



  else if(*(uint32_t *)f->esp == 10){ // SYS_SEEK
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
    
    int16_t index = 0;
    while(thread_current()->array_of_fd[index] != *(uint32_t *)(f->esp+16)){
      index = index + 1;
    }
    struct file *file_ = thread_current()->file_descriptor_table[index];
    file_->pos = *(uint32_t *)(f->esp+20);
    
  }



  else if(*(uint32_t *)f->esp == 11){ // SYS_TELL
    int i = 0;
    for(; i <10; i++){
      if(thread_current()->array_of_fd[i] == *((uint32_t *)(f->esp+4)))
        break;
    }
    struct file *file_ = thread_current()->file_descriptor_table[i];
    f->eax = file_->pos;
  }



  else if(*(uint32_t *)f->esp == 12){ //SYS_CLOSE
//printf("sys close and argument is %d\n", *((uint32_t *)(f->esp+4)));
  
    enum intr_level old_level = intr_enable();

    if( *((uint32_t *)(f->esp+4)) == 0 || *((uint32_t *)(f->esp+4)) == 1 || *((uint32_t *)(f->esp+4)) == 2){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      thread_exit();
    }

    if((*((uint32_t *)(f->esp+4)) > 128) && (pagedir_get_page( thread_current()->pagedir, *((uint32_t *)(f->esp+4)))) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      sema_up(&main_waiting_exec);
      thread_exit(); 
    }
    struct thread *current = thread_current();
    //printf("fd is %d\n", *((uint32_t *)(f->esp+4)));
    ////printf("open counter is %d\n", current->file_descriptor_table[*((uint32_t *)(f->esp+4))]->inode->open_cnt);
    int16_t current_fd = *((uint32_t *)(f->esp+4));
    int16_t index_of_close_file = 0;

// fd??? ???????????? array_of_fd?????? ?????? close????????? fd??? ????????? ?????? ???????????? index??????
    while(current->array_of_fd[index_of_close_file] != current_fd){
      index_of_close_file = index_of_close_file + 1; 
      if(index_of_close_file == 10)
        break;
    }

    if(index_of_close_file == 10){
 // ?????? while??? ????????? ?????? ????????? fd??? ???????????? ???????????? ???????????? ?????? close????????? ?????? fd??? ?????? => ??? close-twice.c ?????? ?????? ????????? => terminate with exit code
 //
      if(current->tid == 3)
        sema_up(&main_waiting_exec);
      else if(current->tid == 4)
        sema_up(&exec_waiting_child_simple);
//////test case????????? tid=1??? main_waiting_exec????????? sema??? down????????????, tid=4??? ????????? terminate?????? case??? ????????? ?????? sema_up??? ?????? 2????????? ??????////////////////
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }


    else if(current->file_descriptor_table[index_of_close_file] != NULL && current->array_of_fd[index_of_close_file] == current_fd){
      //file_close(current->file_descriptor_table[index_of_close_file]); 
      //flil_close??? ?????? disk????????? ??????????????? ?????? ??????????
      current->file_descriptor_table[index_of_close_file] = NULL;
      current->array_of_fd[index_of_close_file] = 0;
    }
    else{
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }
    intr_set_level(old_level); 
   }



  else if(*(uint32_t *)f->esp == 13){ //SYS_MMAP

   enum intr_level my_level = intr_enable();

   //printf("stack pointer in sys_mmap is %p\n", f->esp);
   //printf("thread stack pointer is %p\n", thread_current()->esp);
   //printf("first argument is %d\n", *(int32_t *)(f->esp+16));
   //printf("second argument is %p\n", *(int32_t *)(f->esp+20));
   //printf("code segment is %p\n", f->cs);
   //printf("stack segment is %p\n", f->ss);
   //printf("data segment is %p\n", f->ds);

   //hex_dump(f->esp, f->esp, 200, 1);

   if((lookup_page(thread_current()->pagedir, *(int32_t *)(f->esp+20), false) != NULL)){
     if(!(  *(int32_t *)(f->esp+20) >= thread_current()->data_segment_base && *(int32_t *)(f->esp+20) < thread_current()->data_segment_base + (ROUND_UP(thread_current()->data_segment_size, PGSIZE)))  ){ //mmap-overlap
       if(thread_current()->my_parent == 1){
         //sema_up(&main_waiting_exec);
         f->eax = -1;
         return;
       }
       if(thread_current()->my_parent == 3){
         sema_up(&exec_waiting_child_simple);
         f->eax = -1;
         return;
       }
     }
   }   
  
   uint32_t sum_of_file_length = 0;
   int k = 0;
   for(; k<10; k++){
     if(thread_current()->file_descriptor_table[k] != 0){
       sum_of_file_length += thread_current()->file_descriptor_table[k]->inode->data.length;
     }
   }

// code, bss, initialized data ????????? base??? limit??? ????????? ???????
// gdt.c, gdt.h??? ??????????????? static?????? ?????????????????????
 
    if(*(int32_t *)(f->esp+20) >= 0x8048000 && *(int32_t *)(f->esp+20) < 0x8048000+ (ROUND_UP(thread_current()->code_segment_size, PGSIZE))){ //mmap-over-code 
      if(thread_current()->my_parent == 1){
        sema_up(&main_waiting_exec);
        f->eax = -1;
        return;
      }
      if(thread_current()->my_parent == 3){
        sema_up(&exec_waiting_child_simple);
        f->eax = -1;
        return;
      }
    }


/* ?????? ??? mmap-over-data??? sys exit ????????? ?????????
 * mmap-over-code??? sys exit ??????????????? abnormally terminate??? */
    if(*(int32_t *)(f->esp+20) >= thread_current()->data_segment_base && *(int32_t *)(f->esp+20) < thread_current()->data_segment_base + (ROUND_UP(thread_current()->data_segment_size, PGSIZE))){ //mmap-over-data
      if(thread_current()->my_parent == 1){
        //sema_up(&main_waiting_exec);
        f->eax = -1;
        return;
      }
      if(thread_current()->my_parent == 3){
        //sema_up(&exec_waiting_child_simple);
        f->eax = -1;
        return;
      }
   }

   uint32_t thread_sp = thread_current()->esp; 
   if(*(int32_t *)(f->esp+20) <= PHYS_BASE && *(int32_t *)(f->esp+20) >= (thread_sp & 0xfffff000)){ //mmap-over-stk
      if(thread_current()->my_parent == 1){
        sema_up(&main_waiting_exec);
        f->eax = -1;
        return;
      }
      if(thread_current()->my_parent == 3){
        sema_up(&exec_waiting_child_simple);
        f->eax = -1;
        return;
      }
   }
   

    int i = 0;
    bool valid_fd = false;
    for( ; i < 10; i++){
      if(thread_current()->array_of_fd[i] == *(uint32_t *)(f->esp+16)){
        valid_fd = true;
        break;
      }
    }
    if(valid_fd == false){ //mmap-bad-fd
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

   
    if(*(int32_t *)(f->esp+20) == 0){ //mmap-null
      if(thread_current()->my_parent == 1){
        f->eax = -1;
        sema_up(&main_waiting_exec);
        return;
      }
      if(thread_current()->my_parent == 3){
        sema_up(&exec_waiting_child_simple);
        thread_exit ();
        return;
      }
    }

    uint32_t not_page_aligned = *(int32_t *)(f->esp+20) & 0xfff;
    if(not_page_aligned != 0){ //mmap-misalign
      if(thread_current()->my_parent == 1){
        f->eax = -1;
        sema_up(&main_waiting_exec);
        return;
      }
      if(thread_current()->my_parent == 3){
        sema_up(&exec_waiting_child_simple);
        thread_exit ();
        return;
      }
    }

/////////////// ??????????????? mmap ?????? case ////////////////

    i = 0;
    uint8_t my_fd = 0;
    for(; i < 10; i++){
      my_fd = thread_current()->array_of_fd[i];
      if(my_fd == *(int32_t *)(f->esp+16))
        break;
    }
    //printf("fd is %d\n", my_fd);
    struct file *file = thread_current()->file_descriptor_table[i];
    uint8_t required_pages = DIV_ROUND_UP((file->inode->data.length),PGSIZE);
    uint32_t file_size = file->inode->data.length;

    /* mmap-zero?????? ????????? file????????? 0????????? ?????? 1??? page??? pool??? ?????????????????? */
    if(required_pages == 0) 
      required_pages = 1; 

//printf("*******bit mask result : %0x*********\n", bit_mask(3));

    uint32_t *user_pool_page = palloc_get_multiple(PAL_USER, required_pages);
    //printf("file length is %d\n", file->inode->data.length);


    /* install_page??? pte??? ????????? ???????????? 
     * ????????? install_page??? file->pos ??????????????? ?????????
     * ????????? pos ?????? ???????????? ?????????  */

    uint16_t original_pos = file->pos;

    if (!install_page (*(int32_t *)(f->esp+20), user_pool_page, 1)){ 
      palloc_free_page (user_pool_page);
    } 

    if(file_read(file, user_pool_page, file->inode->data.length) == 0){
      if(thread_current()->my_parent == 1){
        palloc_free_page (user_pool_page);
        sema_up(&main_waiting_exec);
        printf("%s: exit(%d)\n", thread_current()->name, -1);
        thread_exit ();
      }
      if(thread_current()->my_parent == 3){
         palloc_free_page (user_pool_page);
         printf("%s: exit(%d)\n", thread_current()->name, -1);
         sema_up(&exec_waiting_child_simple);
         thread_exit ();
      }
    }

   file->pos = original_pos; 

    int p = 0;
    for(; p<10; p++){
      if(thread_current()->mapping[p] == NULL)
        break;
    }
    thread_current()->mapping[p] = *(int32_t *)(f->esp+20);
    f->eax = *(int32_t *)(f->esp+20);
    
    intr_set_level(my_level);

  }

  else if(*(uint32_t *)f->esp == 14){ //SYS_UNMAP

    enum intr_level my_level = intr_disable();

    //hex_dump(f->esp, f->esp, 200, 1);
    uint32_t *pte = lookup_page (thread_current()->pagedir, *(int32_t *)(f->esp+4), 0);
    //*pte = *pte & 0xfffffffe; // present bit??? 0??????
    uint32_t p = ptov(*pte&0xfffff000);
    uint32_t user_pool_base = user_pool.base;
    uint32_t user_pool_idx = ((p-user_pool_base)/PGSIZE);
    
    palloc_free_multiple((void *)p,1);


    pagedir_clear_page(thread_current()->pagedir, *(int32_t *)(f->esp+4));

   
   intr_set_level(my_level); 

  }
  else if(*(uint32_t *)f->esp == 0xf){ //SYS_CHDIR(15)
    printf("******************syscall chdir ****************************\n");
    ASSERT(1 != 1);
  }

  else if(*(uint32_t *)f->esp == 0x10){ // SYS_MKDIR(16)
    printf("******************syscall mkdir ****************************\n");
    ASSERT(1 != 1);
  }
  
  else if(*(uint32_t *)f->esp == 0x11){ //SYS_READDIR (17)
    //printf("sysread_dir first argument is %d and second is %s\n", *((uint32_t *)(f->esp+16)) , *((uint32_t *)(f->esp+20)));
    bool exist = false;
    int i = 0;
    for(; i< 5; i++){
      if(thread_current()->array_of_dir_fd[i] == *((uint32_t *)(f->esp+16))){
        exist = true;
      }
    }
    struct dir_entry e;
    size_t ofs;
    for (ofs = 0; inode_read_at (thread_current()->directory_table[i]->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e){
    }
    f->eax = exist;
  }


  else if(*(uint32_t *)f->esp == 0x12){ //SYS_ISDIR (18)

    bool dir = false;
    int i = 0;
    for(; i<5; i++){
      if(thread_current()->array_of_dir_fd[i] == *((uint32_t *)(f->esp+4))){
        dir = true;
        f->eax = dir;
        return;
      }
    }
    f->eax = dir;

  }


  else if(*(uint32_t *)f->esp == 0x13){ //SYS_INUMBER (19)
//printf("sys_inumber argument is %d\n", *((uint32_t *)(f->esp+4)));
    bool dir = false;
    bool file = false;
    int i = 0;
    for(; i < 5; i++){
      if(thread_current()->array_of_dir_fd[i] == *((uint32_t *)(f->esp+4))){
        dir = true;
        break;
      }
    }
    if(dir){
      struct file *file_ = thread_current()->directory_table[i];
      f->eax = file_->inode->sector;
    }
    else{
      i = 0;
      for(; i <10; i++){
        if(thread_current()->array_of_fd[i] == *((uint32_t *)(f->esp+4))){
          file = true;
          break;
        }
      }
      struct file *file_ = thread_current()->file_descriptor_table[i];
      f->eax = file_->inode->sector;
    }
   
  }
  
  else{ 
    printf("%s: exit(%d)\n", front, -1);
    thread_exit();
  }
   


intr_set_level(my_level);
}



