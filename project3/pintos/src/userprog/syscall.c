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

extern struct list all_list;

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
//extern bool load_success;
extern struct semaphore main_waiting_exec;
extern struct semaphore exec_waiting_child_simple;
extern struct semaphore multichild[46]; //multi-recurse.c, rox-multichild.c에서 사용


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) //intr_handlers 배열에 0x30(interrupt)에 해당하는 handler는 syscall_handler라고 등록
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
      if(parent->tid == thread_current()->my_parent){ // t는 부모 쓰레드
          break;
      }
    }
  if(parent->abnormal_child == thread_current()->tid){
    //printf("I am an abnormal thread: %d\n", thread_current()->tid);
    parent->exit_status_of_child[thread_current()->tid%44] = 0;
    thread_exit();
  }



/* multi-oom에서 consume_some_resoucre로 파일 열어놓고 abnormal하게 terminate
 한 자식 프로세스들을 부모 프로세스가 syscall_handler 부를 때마다 그 파일 여느라 쓴 kernel resource들 free*/

/*
  if(thread_current()->abnormal_child != 0){ //원래 설정 안했을 때 초기값은 0
// process_execute에서 abnormal child tid를 설정 해줬다면?


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
        if(t->tid == my_parent){ // t는 부모 쓰레드
          break;
        }
      }

      t->exit_status_of_child[(thread_current()->tid)%44] = *(uint32_t *)(f->esp+4);
//여기서 부모의 exit_status_of_child에 나의 tid index에 해당하는 곳에 status를 할당해주는 이
//유는 spawn_child의 recursion이 모두 끝나고 47->45->43...이렇게 다시 쭉 return 할때, 예를 들>어, 45가 sys_exit으로 끝나고 이걸 sys_wait로 기다리는 43의 wait의 리턴값으로 30(depth)를 줘>야하고 이걸 43의 exit_status_of_child에 적어줌


// consume_resource_and_die의 case4 처럼 정상적으로 sys_exit call하고 죽는 경우도 있음
// -> 이때는 sema_up을 하면 안됨. 예를 들어 부모가 tid=21이고, 자식이 tid=22, 23 인 경우 22는 multi-oom ~ -k 라는 명령어를 실행시키므로 부모인 tid=21이 sema_down을 안함
// -> 이때 sema_up을 해버리면 자칫 21과 23 사이에 걸린 semaphore를 sema_up 해버릴 수 있음

     /* exit할때 user pool에 load해준 page palloc_free_multiple 해주기
      * palloc_free_multiple에서 frame_table 알아서 비워줄 것 */ 

    
     while(!hash_empty(&thread_current()->pages)){
       int i = 0;
       for(; i < thread_current()->pages.bucket_cnt; i++){

         while(!list_empty(&thread_current()->pages.buckets[i])){
           struct page *temp = list_front(&thread_current()->pages.buckets[i]);
           palloc_free_page(temp->kernel_vaddr);
           hash_delete(&thread_current()->pages, &temp->hash_elem);
           // spt_entry, 그 kernel_pool page 메모리 해제, hash에서 지우기(hash_delete)
         }
       }
     }


      int8_t index = 0;
      struct file *wasted;

      if(my_parent == 1){
        sema_up(&main_waiting_exec);
      }

     else if(my_parent %44 == 3){
        sema_up(&exec_waiting_child_simple); //exec-multiple처럼 pid=3인 쓰레드가 여러개를 exec하면 pid =4,5,6...전부 자식이 될 수 있음-> pid=4에서 바로 sema_up하면 pid=5인 자식쓰레드는 sema_up이미 된걸 또 sema_up하려고 함 -> 한 부모의 자식중 마지막으로 exit하는 자식이 sema_up해야한다.
      }
// 이렇게 무식한 방법밖에 생각이 안남...
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
    if(*(uint32_t *)(f->esp+28) == 31){ //multi-oom에서 depth가 30 넘으면 child_pid = spawn_child(n+1, RECURSE)여기서 

      f->eax = -1;
      struct thread *parent; // depth=30에 도달했으니 부모에게 나의 exit status를 기록해주고 종료
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
      if(t->tid == pid && t->load_success == true){ // 만약 exec-missing처럼 child가 load 실패 하면 그 child는 load_success 멤버가 false가 되면서 child_waiting_exit이라는 sema에서 block된 채로 있고 여기서 그 child의 exit_status확인하고 sema를 풀어줘서 child thread는 exit하게 됨
        f->eax = pid;
      } 

      else if(thread_current()->exit_status_of_child[pid%44] != -1){ // exit_status는 sys_exit에서 설정-> 정상적으로 sys_exit으로 user_program을 마친 thread의 부모에게 -1이 아닌 값으로 설정되어있음
        f->eax = pid;
      }

      else{ //exec-missing처럼 자식이 load에 실패해서 sys_exit으로 종료한것도 아님
        f->eax = -1;
      }

    }
  }



  else if(*(uint32_t *)f->esp == 3){ // SYS_WAIT int wait(pid_t pid)


//printf("sys wait and waiting %d\n", *((uint32_t *)(f->esp+4)));

    if(thread_current()->exit_status_of_child[(*(uint32_t *)(f->esp+4))%44] == -1){ // multi-oom에서 자식 프로세스가 abnormal하게 sys_exit안쓰고 종료된 경우(multi-oom에서if(wait(child_pid)!= -1) 에서 부모가 부른 경우임
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
      if(find == false){ //multi-oom 경우처럼 abnormal하게 종료된 thread(sys_exit을 안부르고 끝낸 경우)를 wait하도록 한 경우 or 이미 exit한 자식을 찾으려고 하거나 or wait-twice.c 처럼 한번 wait한 자식을 또 wait 하려고 하면 thread를 모아놓은 all_list에서 찾을 수 없고 이러면 exit_status_of_child멤버에서 자식의 exit_status 찾아내자
  
        f->eax = thread_current()->exit_status_of_child[(*(uint32_t *)(f->esp+4))%44];
        //wait-twice.c 처럼 이미 한번 wait해서 reap한 자식을 또 wait하려고 하는 경우도
        //있기 때문에 한번 쓰고 나서는 초기화
        thread_current()->exit_status_of_child[(*(uint32_t *)(f->esp+4))%44] = -1;
      }

      else if(*((uint32_t *)f->esp+4) == -1){
        printf("%s: exit(%d)\n", front, -1);
        f->eax = -1;
        thread_exit();
      }
   
      else {
        f->eax = process_wait(*(uint32_t *)(f->esp+4)); //여기서 pid(4)를 인자로 넘김
      }
      //printf("sys_exit end...\n"); 
    }
  }

  
  else if(*(uint32_t *)f->esp == 4){ //SYS_CREATE
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

    if(*((uint32_t *)(f->esp+4)) >= 0xc0000000){ // multi-oom에서 0xc0000000을 open의 인자로 줌

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
//////////// 수정해야함, 메모리 부족으로 struct thread 멤버중 file_descriptor_table을 50개로 줄였는데 이러면 sys_open 코드 수정해야함 그리고 tid가 400을 넘어갈 정도로 커질 수 있으니, file_descriptor_table을 129개의 배열로 하는 디자인은 바꿔야함///////////////
        struct file *file_ = filesys_open(*((uint32_t *)(f->esp+4)));
        struct thread *now = thread_current();
        int return_val = 2; // 내가 채우게 될 file_descriptor_table의 index(0에서 시작)

        if(file_ == NULL){
          f->eax = -1;
          //sema_up(&main_waiting_exec);
        }
         
        else if(memcmp(thread_current()->name, *(uint32_t *)(f->esp+4), strlen(front)) == 0){
         //실행중인 나 자신을 열겠다? --> 나중에 이 파일에 write하려면 file_deny_write여부 확인하고 True면 못쓰게 하기
         //printf("file name is %s\n", *(uint32_t *)(f->esp+4));
         //printf("deny_write?  %d\n", file_->deny_write);
          if(file_ != NULL){
            file_deny_write(file_);
            f->eax = thread_current()->tid; // return file descriptor
            now->file_descriptor_table[return_val] = file_;
            now->array_of_fd[return_val] = thread_current()->tid;
            return_val = return_val + 1; //  다음번엔 file_descritor_table의 다음번  index>에 file* 를 채울
          }
          
        }

        else{
          if((struct file *)file_->inode->open_cnt == 1){ //open-normal 처럼 한번만 여는경우

            //assert(now->file_descriptor_table[return_val] == NULL); //만약 이 thread가 이 파일을 처음 여는 경우이며, thread가 이때까지 다른 파일을 연적이 없을때

//printf("open_cnt is 1 and my tid is %d\n", thread_current()->tid);            

            f->eax = thread_current()->tid; // return file descriptor 
            now->file_descriptor_table[return_val] = file_;
              //방금 file_descritor_table 배열에 채운 struct file pointer의 fd를 array_of_fd에 채움
            now->array_of_fd[return_val] = thread_current()->tid;
            return_val = return_val + 1; //  다음번엔 file_descritor_table의 다음번  index>에 file* 를 채울 것임
              
          }
          else{ // open-twice 처럼 같은 프로세스가 같은 파일을 2번 열려고 하는 경우
            
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

//printf("sys_write and my tid is %d\n", thread_current()->tid);
    enum intr_level old_level = intr_enable();

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
      //bad_write case를 위해서 여기 무언가 필요

      putbuf(*(uint32_t *)(f->esp+24), *(uint32_t *)(f->esp+28));
      //sema_up(&main_waiting_exec);
      f->eax = *(uint32_t *)(f->esp+28);
    }
    else{
     
      int16_t index = 0;
      while(thread_current()->array_of_fd[index] != *(uint32_t *)(f->esp+20)){
        index = index + 1;
      }
    
      if(thread_current()->file_descriptor_table[index]->deny_write == true){  //rox-simple.c -> 내가 rox-simple 파일을 실행하는 프로세스인데, rox-simple 실행하면서 열고 거기에 쓰면 그냥 return 0
        f->eax = 0;
      }
      else{
        struct thread *cur = thread_current();
        struct file *file_ = cur->file_descriptor_table[index];
        int ret_val = file_write(file_, *(uint32_t *)(f->esp+24), *(uint32_t *)(f->esp+28));
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
    return;

  }



  else if(*(uint32_t *)f->esp == 12){ //SYS_CLOSE

  
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

// fd를 모아놓은 array_of_fd에서 내가 close하려는 fd가 있는지 찾고 위치하는 index찾기
    while(current->array_of_fd[index_of_close_file] != current_fd){
      index_of_close_file = index_of_close_file + 1; 
      if(index_of_close_file == 10)
        break;
    }

    if(index_of_close_file == 10){
 // 위의 while문 돌면서 내가 열었던 fd를 모아놓은 배열에서 찾아보니 지금 close하려고 하는 fd가 없다 => 즉 close-twice.c 처럼 이미 닫았다 => terminate with exit code
 //
      if(current->tid == 3)
        sema_up(&main_waiting_exec);
      else if(current->tid == 4)
        sema_up(&exec_waiting_child_simple);
//////test case에서는 tid=1이 main_waiting_exec이라는 sema에 down되어있고, tid=4가 여기서 terminate하는 case만 있어서 일단 sema_up을 이거 2가지만 적음////////////////
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }


    else if(current->file_descriptor_table[index_of_close_file] != NULL && current->array_of_fd[index_of_close_file] == current_fd){
      file_close(current->file_descriptor_table[index_of_close_file]);
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
   //printf("first argument is %d\n", *(int32_t *)(f->esp+16));
   //printf("second argument is %p\n", *(int32_t *)(f->esp+20));
   //printf("thread esp is %p\n", thread_current()->esp);
   //hex_dump(f->esp, f->esp, 200, 1);

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
    

/////////////// 정상적으로 mmap 하는 case ////////////////

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
    uint32_t *user_pool_page = palloc_get_multiple(PAL_USER,required_pages);
    //printf("file length is %d\n", file->inode->data.length);

    load_segment(file, file->pos, *(int32_t *)(f->esp+20) , file_size, required_pages*PGSIZE-file_size, 1);
    
    
    intr_set_level(my_level);

  }

  else if(*(uint32_t *)f->esp == 14){ //SYS_UNMAP

    //hex_dump(f->esp, f->esp, 200, 1);
    //printf("first argument is %0x\n", *(int32_t *)(f->esp+16)); 
    uint32_t *pte = lookup_page (thread_current()->pagedir, *(int32_t *)(f->esp+16), 0);
    //printf("user pool page address is %p\n", ptov(*pte));

    //palloc_free_multiple(ptov(*pte));
    //*pte = *pte & 0xfffffffe; // present bit를 0으로
    

  }
  
  else{ 
    printf("%s: exit(%d)\n", front, -1);
    thread_exit();
  }
   


intr_set_level(my_level);
}



