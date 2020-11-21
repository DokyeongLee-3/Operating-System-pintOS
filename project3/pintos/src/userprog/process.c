#include "vm/frame.h"
#include "vm/page.h"
#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

extern struct list all_list;
extern struct pool kernel_pool, user_pool;
extern uint32_t *frame_table;
extern uint8_t *which_process;

struct semaphore main_waiting_exec;
struct semaphore exec_waiting_child_simple;
struct semaphore multichild[45];

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

struct pool
  {
    struct lock lock;
    struct bitmap *used_map;
    uint8_t *base;
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



extern struct pool kernel_pool, user_pool;


/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */


tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
    if(thread_current()->tid == 1){
      sema_init(&main_waiting_exec, 0);
    }
    else if (thread_current()->tid == 3){
      sema_init(&exec_waiting_child_simple, 0);
    }
    else if(thread_current()->tid %44 == 4){
      sema_init(&multichild[0], 0);
    }
    else if(thread_current()->tid %44 == 5){
      sema_init(&multichild[1], 0);
    } 
    else if(thread_current()->tid %44 == 6){
      sema_init(&multichild[2], 0);
    }
    else if(thread_current()->tid %44 == 7){
      sema_init(&multichild[3], 0);
    }
    else if(thread_current()->tid %44 == 8){
      sema_init(&multichild[4], 0);
    }
    else if(thread_current()->tid %44 == 9){
      sema_init(&multichild[5], 0);
    }
    else if(thread_current()->tid %44 == 10){
      sema_init(&multichild[6], 0);
    }
    else if(thread_current()->tid %44 == 11){
      sema_init(&multichild[7], 0);
    }
    else if(thread_current()->tid %44 == 12){
      sema_init(&multichild[8], 0);
    }
    else if(thread_current()->tid %44 == 13){
      sema_init(&multichild[9], 0);
    }
    else if(thread_current()->tid %44 == 14){
      sema_init(&multichild[10], 0);
    }
    else if(thread_current()->tid %44 == 15){
      sema_init(&multichild[11], 0);
    }
    else if(thread_current()->tid %44 == 16){
      sema_init(&multichild[12], 0);
    }
    else if(thread_current()->tid %44 == 17){
      sema_init(&multichild[13], 0);
    }
    else if(thread_current()->tid %44 == 18){
      sema_init(&multichild[14], 0);
    }
    else if(thread_current()->tid %44 == 19){
      sema_init(&multichild[15], 0);
    }
    else if(thread_current()->tid %44 == 20){
      sema_init(&multichild[16], 0);
    }
    else if(thread_current()->tid %44 == 21){
      sema_init(&multichild[17], 0);
    }
    else if(thread_current()->tid %44 == 22){
      sema_init(&multichild[18], 0);
    }
    else if(thread_current()->tid %44 == 23){
      sema_init(&multichild[19], 0);
    }
    else if(thread_current()->tid %44 == 24){
      sema_init(&multichild[20], 0);
    }
    else if(thread_current()->tid %44 == 25){
      sema_init(&multichild[21], 0);
    }
    else if(thread_current()->tid %44 == 26){
      sema_init(&multichild[22], 0);
    }
    else if(thread_current()->tid %44 == 27){
      sema_init(&multichild[23], 0);
    }
    else if(thread_current()->tid %44 == 28){
      sema_init(&multichild[24], 0);
    }
    else if(thread_current()->tid %44 == 29){
      sema_init(&multichild[25], 0);
    }
    else if(thread_current()->tid %44 == 30){
      sema_init(&multichild[26], 0);
    }
    else if(thread_current()->tid %44 == 31){
      sema_init(&multichild[27], 0);
    }
    else if(thread_current()->tid %44 == 32){
      sema_init(&multichild[28], 0);
    }
    else if(thread_current()->tid %44 == 33){
      sema_init(&multichild[29], 0);
    }
    else if(thread_current()->tid %44 == 34){
      sema_init(&multichild[30], 0);
    }
    else if(thread_current()->tid %44 == 35){
      sema_init(&multichild[31], 0);
    }
    else if(thread_current()->tid %44 == 36){
      sema_init(&multichild[32], 0);
    }
    else if(thread_current()->tid %44 == 37){
      sema_init(&multichild[33], 0);
    }
    else if(thread_current()->tid %44 == 38){
      sema_init(&multichild[34], 0);
    }
    else if(thread_current()->tid %44 == 39){
      sema_init(&multichild[35], 0);
    }
    else if(thread_current()->tid %44 == 40){
      sema_init(&multichild[36], 0);
    }
    else if(thread_current()->tid %44 == 41){
      sema_init(&multichild[37], 0);
    }
    else if(thread_current()->tid %44 == 42){
      sema_init(&multichild[38], 0);
    }
    else if(thread_current()->tid %44 == 43){
      sema_init(&multichild[39], 0);
    }
    else if(thread_current()->tid %44 == 0){
      sema_init(&multichild[40], 0);
    }
    else if(thread_current()->tid  > 44 && thread_current()->tid % 44 == 1){
      sema_init(&multichild[41], 0);
    }
/*
    else if(thread_current()->tid  > 44 && thread_current()->tid % 44 == 2){
      sema_init(&multichild[42], 0);
    }
    else if(thread_current()->tid  > 44 && thread_current()->tid % 44 == 3){
      sema_init(&multichild[43], 0);
    }
    else if(thread_current()->tid  > 44 && thread_current()->tid % 44 == 4){
      sema_init(&multichild[44], 0);
    }
*/    

  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);

  static struct thread *child;
  struct list_elem *e = list_front(&all_list);
  for(; e != list_end(&all_list); e = list_next(e)){
    child = list_entry(e, struct thread, allelem);
    if(child->tid == tid)
      break;
  }

  child->my_parent = thread_current()->tid;

  //printf("created %d and my tid is %d and parent is %d\n", tid, thread_current()->tid, thread_current()->my_parent);
  
  enum intr_level my_level = intr_disable();  

  if(thread_current()->tid == 1){
    //printf("sema down and my pid is 1\n");
    sema_down(&main_waiting_exec);
    //printf("escape from main_waiting_exec\n");
  }

  else if(thread_current()->tid == 3){
    //printf("sema down and my pid is %d\n", thread_current()->tid);
    
    sema_down(&exec_waiting_child_simple);
    //printf("escape from exec_waiting_child_simple and my tid is %d\n",thread_current()->tid);
  }
  
  else if(thread_current()->tid % 44 == 4){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[0]);
  }
  
  else if(thread_current()->tid % 44 == 5){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[1]);
  }
  
  else if(thread_current()->tid % 44 == 6){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[2]);
  }
  else if(thread_current()->tid %44 == 7){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[3]);
  }
  else if(thread_current()->tid %44 == 8){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[4]);
  }
  else if(thread_current()->tid %44 == 9){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[5]);
  }
  else if(thread_current()->tid %44 == 10){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[6]);
  }
  else if(thread_current()->tid %44 == 11){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[7]);
  }
  else if(thread_current()->tid %44 == 12){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[8]);
  }
  else if(thread_current()->tid %44 == 13){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[9]);
  }
  else if(thread_current()->tid %44 == 14){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[10]);
  }
  else if(thread_current()->tid %44 == 15){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[11]);
  }
  else if(thread_current()->tid %44 == 16){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[12]);
  }
  else if(thread_current()->tid %44 == 17){ // rox-multichild, multi-recurse에서 쓰임
    sema_down(&multichild[13]);
  }
  else if(thread_current()->tid %44 == 18){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
    sema_down(&multichild[14]);
  }


  else if(thread_current()->tid %44 == 19){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[15]);
    }
    else{

 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }


  else if(thread_current()->tid %44 == 20){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
    sema_down(&multichild[16]);
  }



  else if(thread_current()->tid %44 == 21){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[17]);
    }
    else{
    
 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    
    }
  }



  else if(thread_current()->tid %44 == 22){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[18]);
  }



  else if(thread_current()->tid %44 == 23){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      sema_down(&multichild[19]);
    }
    else{

   
 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }





  else if(thread_current()->tid %44 == 24){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[20]);
  }

  else if(thread_current()->tid %44 == 25){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[21]);
    }
    else{
    
 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }




  else if(thread_current()->tid %44 == 26){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
    //printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[22]);
  }




  else if(thread_current()->tid %44 == 27){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
   
    enum intr_level my_level = intr_disable();
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
  }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[23]);
      //printf("get out of sema and my tid is %d\n", thread_current()->tid);
    }
    else{

 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 //  // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      thread_current()->abnormal_child = tid; 
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
    intr_set_level(my_level);
  }
 
  else if(thread_current()->tid %44 == 28){ // rox-multichild, multi-recurse, multi-oom에서 쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[24]);
  }


  else if(thread_current()->tid %44 == 29){ // rox-multichild, multi-recurse, multi-oom에서 쓰임

//printf("command is ? %s\n", file_name);
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }


    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[25]);
    }
    else{

 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
 //printf("tid is %d and crash case \n", thread_current()->tid);
       thread_current()->abnormal_child = tid;
       thread_current()->exit_status_of_child[tid%44] = -1;
    }

  }

  else if(thread_current()->tid %44 == 30){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[26]);
  }



  else if(thread_current()->tid %44 == 31){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
  }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[27]);
    }
    else{


 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 //  // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
       //printf("tid is %d and crash case \n", thread_current()->tid);
       thread_current()->abnormal_child = tid;
       thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }



  else if(thread_current()->tid %44 == 32){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[28]);
  }



  else if(thread_current()->tid %44 == 33){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
  }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[29]);
    }
    else{

 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      //printf("tid is %d and crash case \n", thread_current()->tid);
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }




  else if(thread_current()->tid %44 == 34){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
       sema_down(&multichild[30]);
  }


  else if(thread_current()->tid %44 == 35){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임

    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[31]);
    }
    else{


 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 //  // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      //printf("tid is %d and crash case \n", thread_current()->tid);
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }



  else if(thread_current()->tid %44 == 36){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[32]);
  }



  else if(thread_current()->tid %44 == 37){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[33]);
    }
    else{


 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
   }

  }




  else if(thread_current()->tid %44 == 38){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[34]);
  }


  else if(thread_current()->tid %44 == 39){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임

    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[35]);
    }
    else{


 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 //  // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
     //printf("tid is %d and crash case \n", thread_current()->tid);
     thread_current()->abnormal_child = tid;
     thread_current()->exit_status_of_child[tid%44] = -1;
   }
 
  }



  else if(thread_current()->tid %44 == 40){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
    //printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[36]);
  }




  else if(thread_current()->tid %44 == 41){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임

    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[37]);
    }
    else{


 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
      //printf("tid is %d and crash case \n", thread_current()->tid);
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
   }




  else if(thread_current()->tid %44 == 42){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
    //printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[38]);
  }




  else if(thread_current()->tid %44 == 43){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[39]);
    }
    else{


 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
 
      //printf("tid is %d and crash case \n", thread_current()->tid);
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }



  else if(thread_current()->tid %44 == 0){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    sema_down(&multichild[40]);
  }




  else if(thread_current()->tid > 44 && thread_current()->tid %44 == 1){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임 tid=45, 89, 133 ...
    
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[41]);
    }
    else{

 // 만약에 위에서 thread_create로 만든 자식 프로세스가 crash하는거라면 exit_status_of_child에 그 >쓰레드가 sys_exit없이 abnormal하게 종료한다는걸 -1로 표시한다
 // 여기서 tid는 위에서 thread_create로 만든 자식 프로세스의 tid
 
      //printf("tid is %d and crash case \n", thread_current()->tid);
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }


/*
  else if(thread_current()->tid > 44 && thread_current()->tid %44 == 2){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
//printf("sema_down, my tid is %d\n", thread_current()->tid);
    ////////////도대체 어디 sema에 걸려있는건가//////////
    sema_down(&multichild[42]);
  }




  else if(thread_current()->tid > 44 && thread_current()->tid %44 == 3){ // rox-multichild, multi-recurse, multi-oom에서 >쓰임
  
    char *token, *save_ptr;
    bool crash = false;
    for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)){
      if(memcmp(token, "-k", strlen(token)) == 0)
        crash = true;
    }
    if(crash == false){
      //printf("sema_down, my tid is %d\n", thread_current()->tid);
      sema_down(&multichild[43]);
       
    }
    else{
      //printf("tid is %d and crash case \n", thread_current()->tid);
      thread_current()->abnormal_child = tid;
      thread_current()->exit_status_of_child[tid%44] = -1;
    }
  }
*/

//printf("end of %d process_execute\n", thread_current()->tid);

  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;


  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;


  // file_name을 copy해둔 char*로 parse를 진행해보자
  /*
  char my_copy[100];
  memset(my_copy, 0, sizeof file_name);
  strlcpy(my_copy, file_name, 20);
  */

    success = load (file_name, &if_.eip, &if_.esp);
//hex_dump(if_.esp, if_.esp, PHYS_BASE - if_.esp, true);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) { 
// memory 부족으로 load 실패하는 경우도 부모의 exit_status_of_child에 -1 쓰고
// 부모는 sys_exec에서 자식 pid의 index에 해당하는 exit_stauts_of_child가 -1이면 
// return -1(FAQ에 나와있음)

   /////////FAQ에 나와있듯이 자식이 load 실패하면 부모 sema_up해주고 부모의 exit_status_o
    struct list_elem *e = list_front(&all_list);
    struct thread *t; // t는 부모 프로세스
    for(; e != list_end(&all_list); e = list_next(e)){
      t = list_entry(e, struct thread, allelem);
      if(t->tid == thread_current()->my_parent){
        break;
      }
    }

    //if(thread_current()->tid <= 44)
    t->exit_status_of_child[(thread_current()->tid)%44] = -1;
    //else if(thread_current()->tid > 44){
    //  t->exit_status_of_child[(
    //}

    if(thread_current()->my_parent == 1){ // 내가 pid가 3이면 나의 부모는 pid가 1
      sema_up(&main_waiting_exec);
    }
    else if(thread_current()->my_parent %44 == 3){ 
      sema_up(&exec_waiting_child_simple);
    }
    else if(thread_current()->my_parent %44 == 4){
      sema_up(&multichild[0]);
    }
    else if(thread_current()->my_parent %44 == 5){
      sema_up(&multichild[1]);
    }
    else if(thread_current()->my_parent %44 == 6){
      sema_up(&multichild[2]);
    }
    else if(thread_current()->my_parent %44 == 7){
      sema_up(&multichild[3]);
    }
    else if(thread_current()->my_parent %44 == 8){
      sema_up(&multichild[4]);
    }
    else if(thread_current()->my_parent %44 == 9){
      sema_up(&multichild[5]);
    }
    else if(thread_current()->my_parent %44 == 10){
      sema_up(&multichild[6]);
    }
    else if(thread_current()->my_parent %44 == 11){
      sema_up(&multichild[7]);
    }
    else if(thread_current()->my_parent %44 == 12){
      sema_up(&multichild[8]);
    }
    else if(thread_current()->my_parent %44 == 13){
      sema_up(&multichild[9]);
    }
    else if(thread_current()->my_parent %44 == 14){
      sema_up(&multichild[10]);
    }
    else if(thread_current()->my_parent %44 == 15){
      sema_up(&multichild[11]);
    }
    else if(thread_current()->my_parent %44 == 16){
      sema_up(&multichild[12]);
    }
    else if(thread_current()->my_parent %44 == 17){
      sema_up(&multichild[13]);
    }
    else if(thread_current()->my_parent %44 == 18){
      sema_up(&multichild[14]);
    }
    else if(thread_current()->my_parent %44 == 19){
      sema_up(&multichild[15]);
    }
    else if(thread_current()->my_parent %44 == 20){
      sema_up(&multichild[16]);
    }
    else if(thread_current()->my_parent %44 == 21){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[17]);
    }
    else if(thread_current()->my_parent %44 == 22){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[18]);
    }
    else if(thread_current()->my_parent %44 == 23){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[19]);
    }
    else if(thread_current()->my_parent %44 == 24){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[20]);
    }
    else if(thread_current()->my_parent %44 == 25){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[21]);
    }
    else if(thread_current()->my_parent %44 == 26){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[22]);
    }
    else if(thread_current()->my_parent %44 == 27){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[23]);
    }
    else if(thread_current()->my_parent %44 == 28){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[24]);
    }
    else if(thread_current()->my_parent %44 == 29){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[25]);
    }
    else if(thread_current()->my_parent %44 == 30){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[26]);
    }
    else if(thread_current()->my_parent %44 == 31){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[27]);
    }
    else if(thread_current()->my_parent %44 == 32){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[28]);
    }
    else if(thread_current()->my_parent %44 == 33){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[29]);
    }
    else if(thread_current()->my_parent %44 == 34){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[30]);
    }
    else if(thread_current()->my_parent %44 == 35){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[31]);
    }
    else if(thread_current()->my_parent %44 == 36){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[32]);
    }
    else if(thread_current()->my_parent %44 == 37){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[33]);
    }
    else if(thread_current()->my_parent %44 == 38){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[34]);
    }
    else if(thread_current()->my_parent %44 == 39){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[35]);
    }
    else if(thread_current()->my_parent %44 == 40){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[36]);
    }
    else if(thread_current()->my_parent %44 == 41){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[37]);
    }
    else if(thread_current()->my_parent %44 == 42){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[38]);
    }
    else if(thread_current()->my_parent %44 == 43){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[39]);
    }
    else if(thread_current()->my_parent %44 == 0){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[40]);
    }
    else if(thread_current()->my_parent > 44 && thread_current()->my_parent % 44 == 1){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[41]);
    }
    else if(thread_current()->my_parent > 44 && thread_current()->my_parent % 44 == 2){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[42]);
    }
    else if(thread_current()->my_parent > 44 && thread_current()->my_parent % 44 == 3){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[43]);
    }
    else if(thread_current()->my_parent > 44 && thread_current()->my_parent % 44 == 4){
//printf("sema up, my tid is %d\n", thread_current()->tid);
      sema_up(&multichild[44]);
    }
    

    thread_exit ();
  }
  else{
    //printf("load success....%s and i am %d\n", my_copy, thread_current()->tid);
    thread_current()->load_success = true;  
  }

//printf("start user program %s\n", my_copy);
  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
//The "memory" clobber tells the compiler that the assembly code performs memory reads or writes
// &if를 esp에게 전달하고 intr_exit으로 점프
//asm volatile ( : : : "memory") -> compiler가 memory access order 최적화를 못하게함
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED) 
{

//printf("process_wait and my tid is %d\n", thread_current()->tid);

  if(child_tid == TID_ERROR){
    return -1;
  }
  if(thread_current()->load_success == false){
    return -1;
  }
  int exit_status_of_child = thread_current()->exit_status_of_child[child_tid%44];
  thread_current()->exit_status_of_child[child_tid] = -1; // return value만 복사해놓고 다시 이렇게 초기값 -1로 돌려놓으면 만약 나의 child가 sys_exit에서 exit_status_of_child를 자신의 exit status로 update 해주지 않으면 계속 -1 => child가 execute안하고 wait하면 -1을 return
  return exit_status_of_child;
  
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp, char *my_copy);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{

  hash_init(&(thread_current()->pages), page_hash, page_less, NULL);


  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();
  /* Open executable file. */
//printf("file_name is ? %s\n", file_name);
//printf("file_name size is %d\n", sizeof file_name);
char my_copy[100];
memcpy(my_copy, file_name, sizeof my_copy);
//printf("original file name(my_copy): %s\n", my_copy);
char *save_ptr;
char *front = strtok_r(file_name, " ", &save_ptr);

  // multi-oom같은 경우 filesys_open이 NULL을 줄때까지 시도하다보면
  // memory 공간이 생길때 NULL이 아닌 struct file* 를 return
  // 시도 횟수 제한을 둔 이유는 exec-missing같은건 진짜 없는 것을 실행하려 하기에
  // 이때 일정 횟수 넘게 실패하면 진짜 없는 파일이라 간주하고 retrun NULL
  int k = 0; 
  for(; i< 100 ; i++){
    file = filesys_open (front);
    if(file != NULL)
      break;
  }


  if (file == NULL) 
    {
       printf ("load: %s: open failed\n", file_name);
       thread_current()->load_success = false;

      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }
  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
//PGMASK = 0x00000fff
//struct *file은 file_open으로 열때 calloc으로 kernel_pool의 메모리를
//할당해주므로 커널 영역의 vm을 가지고 있음
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
//printf("address of file is %p\n", file);
//printf("offset of segment %d\n", phdr.p_offset);
//printf("virtual address of segment %p\n", phdr.p_vaddr);
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
//printf("mem_page is %x\n", mem_page);
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  /* ROUND_UP은 2번째 인자의 배수에 가까운 수로 만들어줌
                   * 예를 들어 ROUND_up(19,5)는 20 */ 
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }


  /* Set up stack. */
  if (!setup_stack (esp, my_copy))
    goto done;


  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;
  success = true;
 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);

  return success;
}
/* load() helpers. */

bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
//printf("@@@@@@@@@@@@@@@ load segment start @@@@@@@@@@@@@@@@\n");
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  uint8_t required_pages = DIV_ROUND_UP(read_bytes,PGSIZE);

/* kernel pool에서 필요한 만큼페이지 할당해서 여기 파일의 segment 전부를 적어둔다 */
  file_seek (file, ofs);
  /* 매뉴얼에서 이 while loop 고치라고 함 (4.3.2 Paging)*/


  int cnt = 0;
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      struct page *spt_entry = malloc(sizeof(struct page));
//printf("****************this time read bytes: %d / zero_bytes: %d *******************\n", read_bytes, zero_bytes);   
      // kernel_pool에서 frame 할당 받아서 여기에 file 읽어두고
      // 나중에 page fault뜨면 여기서 file 읽어와서 user frame에 load해주자
      uint32_t *backup_kpage = palloc_get_page(PAL_ZERO);

      int k = 0;      

//printf("In load segment, user process address is %p\n", upage); 
//printf("********address of spt is %p********\n", spt_entry);

      spt_entry->kernel_vaddr = backup_kpage;
 
      spt_entry->user_vaddr = upage;
      spt_entry->read_size = page_read_bytes;
      spt_entry->writable = writable;

      if (file_read (file, backup_kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (backup_kpage);
          return false;
        }
      memset (backup_kpage + page_read_bytes, 0, page_zero_bytes);

//printf("$$$$$$$$$In load_segment, size is %p$$$$$$$$$$\n", spt_entry->size);
//printf("$$$$$$$$$In load_segment, file inode is %p$$$$$$$$$$\n", file->inode);

      hash_insert(&(thread_current()->pages), &(spt_entry->hash_elem));


//printf(">>>>>>>>>In load segment, size is %p <<<<<<<<<<\n", page_read_bytes);

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
      cnt += 1;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp, char *my_copy) 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        *esp = PHYS_BASE;
      else
        palloc_free_page (kpage);
    }

///////////// argument passing/////////////
  int arg_count = 0;
  char *front[25];
  int contain_null_size[25] = {0, };
  char *save_ptr;
  if(success){
    front[arg_count] = strtok_r(my_copy, " " , &save_ptr);
    arg_count = arg_count+1;
    while((front[arg_count] = strtok_r(NULL, " ", &save_ptr)) != NULL){
      arg_count = arg_count+1;
    }


    int tmp_count = arg_count;
    while(tmp_count != 0){
      contain_null_size[tmp_count-1] = strlen(front[tmp_count-1])+1; //here!
      int temp_size = contain_null_size[tmp_count-1];
      for(;temp_size > 0;){
        *esp = *esp - 1;
        *((char *)*esp) = front[tmp_count-1][temp_size-1];
        //printf(" **(char**)esp is %c\n", **(char**)esp);
        //printf("hex_dump %c\n", ((uint8_t*)(*esp))[contain_null_size-1]);
        temp_size = temp_size - 1;
      }
      //printf("*esp is %p and esp[] is %02hhx\n", *esp, ((uint8_t *)(*esp))[0]);
      tmp_count = tmp_count-1;
    }
    while((int)*esp % 4 != 0){ //word align
      *esp = *esp-1;
      **(char**)esp = '\0';
    }
    int i = 0;
    for( ;i < 4; i = i+1){
      *esp = *esp-1;
      **(char**)esp = '\0';
    }
    tmp_count = arg_count;
    int cumulative = 0;
    //printf("contain_null_size[tmp_count-1] is %d\n", contain_null_size[tmp_count-1]);
    for(; tmp_count > 0; tmp_count = tmp_count-1){
      *esp= *esp-4;
      //printf(" PHYS_BASE-contain_null_size[tmp_count-1] is %p\n", PHYS_BASE-contain_null_size[tmp_count-1]);
      **(int **)esp = PHYS_BASE-contain_null_size[tmp_count-1] - cumulative;
      cumulative = cumulative + contain_null_size[tmp_count-1];
    }
    *esp = *esp - 4;
    char* tmp = *esp + 4; //0x bf ff ff ec
    **(int **)esp = tmp;

    *esp = *esp - 4;
    **(int**)esp = arg_count; // push argc
    i = 0;
    for(; i < 4; i = i+1){
      *esp = *esp-1;
      **(char **)esp = '\0'; // push fake return address
    }
    
  }
  ///////////////////////////////////////////////
  //hex_dump(*esp-0x79b3e000, *esp, 0x79f8aadd, 1);
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */

/* manual보면 핀토스는 kernel memory를 physical memory에 directly mapping
 * 함으로서 첫번째 kernel virtual page가 첫번째 physical memory frame으로
 * 매핑된다고 한다 --> 이 말은 frame에 kernel virtual address로 접근할 수
 * 있다는 의미이다 --> 결국 아래 install_page함수는 겉보기엔 user virtual 
 * memory를 kernel virtual page로 매핑해주는 것 같지만 결과적으로는 user
 * virtual memory를 physical memory frame으로 매핑해주는 것이다 */
bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  ASSERT(kpage > &user_pool);
 
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
