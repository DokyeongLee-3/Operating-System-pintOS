#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"





/* SYSTEM CALLS

1. 구현해야 할 부분
  syscall_handler ()에서 syscall number에 대한 handler 구현
  
2. skeleton 분석
  2-1. syscall_init ()
    syscall_handler ()의 comment 참고
  2-2. syscall_handler ()
    system call이 발생하면 process를 종료

3. 구현
  3-1. user pointer가 제대로 된 address를 가리키는지 확인
  3-2. syscall number에 해당하는 syscall 구현

END */





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



/* syscall_init: intr_handlers 배열에 0x30(interrupt)에 해당하는 handler는 syscall_handler라고 등록
   1. threads/interrupt.c의 intr_register_int ()를 호출함
   2. register_handler () 호출
   3. 0x30(interrupt)에 해당하는 handler를 배정함
      - intr_handlers[0x30] = syscall_handler
      - intr_name[0x30] = "syscall"  */
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}





///////////////// SYSTEM CALLS /////////////////



/* syscall_handler: syscall number에 해당하는 handler 작성
  1. user pointer가 제대로 된 address를 가리키는지 확인
  2. syscall number에 해당하는 syscall 구현
    * user는 lib/user/syscall.c의 syscall0, syscall1, syscall2, syscall3를 이용해서 간접적으로 kernel의 syscall을 호출함  */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{

  /* syscall number, esp 출력 */
  // printf("syscall num : %d\n", *(uint32_t *)(f->esp));
  // printf("now esp is %p\n", (uint32_t *)(f->esp));





  /* 1. user pointer가 제대로 된 address를 가리키는지 확인
      - invalid user pointer일 경우 thread 종료
      - user stack에서 argument를 읽어와야 하는 경우도 있으므로 반드시 확인해야 함
      - user stack의 범위는 (0x8048000, 0xc0000000)
      - thread를 종료할 때 종료되는 thread의 name도 필요함
        => thread_current () 이용해서 name 가져올 것(다른 syscall에서도 사용됨)
      - threads/vaddr.h에서 is_user_vaddr ()을 사용해도 됨 (valid address이면 return true)   */

  // thread name을 갖고 옴
  char *front;
  char *next;
  front = strtok_r(thread_current()->name, " ", &next);  // QQQQQQQQQQQQQQQQQQQQ
  
  // user pointer validity 체크
  if(((uint32_t *)(f->esp) > 0xc0000000) || ((uint32_t *)(f->esp) < 0x8048000) || ((uint32_t *)(f->esp) == NULL)){
    //printf("invalid user given pointer!!!!\n");
    printf("%s: exit(%d)\n", front, -1);
    thread_exit();
  }

  /* current thread name과 user stack 출력 */
  // printf("current thread is %p\n", &thread_current()->name);
  // hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 200, 1);





  /* 2. syscall number에 해당하는 syscall 구현
    - syscall에 사용되는 argument의 갯수는 정해져 있음
    - lib/user/syscall.c에서 확인 가능 (syscall0 이면 argument가 0개...)
    - argument가 1개
      - arg[0] = *(esp + 1)
    - argument가 2개
      - arg[0] = *(esp + 4)
      - arg[1] = *(esp + 5)
    - argument가 3개
      - arg[0] = *(esp + 5)
      - arg[1] = *(esp + 6)
      - arg[2] = *(esp + 7)

    2-1. SYS_HALT(0): arguemnt 0
    2-2. SYS_EXIT(1): argument 1
    2-3. SYS_CREATE(4): argument 2
    2-4. SYS_OPEN(6): argument 1
    2-5. SYS_WRTIE(9): argument 3
    2-6. SYS_CLOSE(12): argument 1

    * syscall number
      handler로 제어가 넘어왔을 때, user's stack pointer(esp)에서부터 4 bytes를 읽어서 syscall number를 확인할 수 있음
      lib/syscall-nr.h에서 syscall의 이름을 확인할 수 있음
      - 0 :SYS_HALT,                   // Halt the operating system.
      - 1: SYS_EXIT,                   // Terminate this process.
      - 2: SYS_EXEC,                   // Start another process. 
      - 3: SYS_WAIT,                   // Wait for a child process to die. 
      - 4: SYS_CREATE,                 // Create a file. 
      - 5: SYS_REMOVE,                 // Delete a file. 
      - 6: SYS_OPEN,                   // Open a file. 
      - 7: SYS_FILESIZE,               // Obtain a file's size. 
      - 8: SYS_READ,                   // Read from a file. 
      - 9: SYS_WRITE,                  // Write to a file. 
      - 10: SYS_SEEK,                  // Change position in a file. 
      - 11: SYS_TELL,                  // Report current position in a file. 
      - 12: SYS_CLOSE,                 // Close a file.     */

  /* 2-1. SYS_HALT(0): arguemnt 0
      - shutdown_power_off () 호출하여 pintOS 종료 (devices/shutdown.h에 있음) */

  else if(*(uint32_t *)f->esp == 0){  //SYS_HALT
    shutdown_power_off();
  }



  /* 2-2. SYS_EXIT(1): argument 1
      - thread_exit ()로 thread 종료
        정상적으로 종료되면 return status
        문제가 있으면 return -1
      - argument가 1개
      - arg[1] = *(esp + 1) = status  */

  else if(*(uint32_t *)f->esp == 1){

    /* tid, eax 관련 코드 */
    // int exit_tid = thread_current()->tid;
    // (uint32_t *)(f->eax) = 0;

    // valid address 확인
    if( ((uint32_t *)f->esp + 4 > 0xc0000000) || ((uint32_t *)f->esp + 4 < 0x8048000) || ( (uint32_t *)(f->esp) == NULL)){   //QQQ (f->esp) or (f->esp+4)?
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }
    else{
      printf("%s: exit(%d)\n", front, *((uint32_t *)f->esp + 1));   // status = arg[1] = *(esp + 1)
      thread_exit();
    }
  }



  /* 2-3. SYS_CREATE(4): argument 2
      - filesys_create ()를 이용한다
        - arg[1] = file name
        - arg[2] = file size
      - 정상적으로 종료되면 eax = true, 아니면 eax = false
      - argument가 2개. */  

  else if(*(uint32_t *)f->esp == 4){
    
    // valid address 확인
    if(*((uint32_t *)f->esp + 4) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = false;   // return false
      thread_exit();
    }

    // user pointer in pagedir의 physical address를 반환
    if(pagedir_get_page(thread_current()->pagedir, *((uint32_t *)f->esp+4)) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = false;
      thread_exit();
    }
    
    // filesys_create하는 동안 interrupt 끄고, arg[1]과 arg[2]를 이용하여 create
    enum intr_level old_level = intr_enable();
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);
    f->eax = filesys_create (*((uint32_t *)f->esp+4), *((uint32_t *)f->esp+5));
    intr_set_level(old_level); 
  }



  /* 2-4. SYS_OPEN(6): argument 1
      - filesys_open ()이용하여 file을 연다
        => return value type이 struct file (in filesys/file.c) 이므로 struct file을 추가해줌
      - argument가 1개
        - arg[1] = file name
      - static int return_value = fd
        모든 process는 다른 fd 값을 가짐. 따라서 SYS_OPEN을 호출할 때마다 다른 return_value를 return 해야함.  */


  else if(*(uint32_t *)f->esp == 6){ //SYS_OPEN

    // old interrupt level을 저장해둔다
    enum intr_level old_level = intr_enable();
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);  

    // valid address 확인
    if(*((uint32_t *)(f->esp+4)) == NULL){
      f->eax = -1;
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    } 

    /* file name, file descriptor 확인 */
    //printf("file name is %s\n", *((uint32_t *)f->esp+1));
    //printf("now file descriptor is %d\n", thread_current()->tid);

    // user pointer in pagedir의 physical address를 반환
    if(pagedir_get_page( thread_current()->pagedir, *((uint32_t *)f->esp+1) ) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      f->eax = -1;
      thread_exit();
    }
    
    // filesys_open ()을 이용하여 file을 연다
    struct file *file_ = filesys_open(*((uint32_t *)(f->esp+4)));
    static int return_val;
    //printf("open : %s\n", *((uint32_t *)(f->esp+4)));

    // failure at file open
    if(file_ == NULL)
      f->eax = -1;

    // success at file open
    else{
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1);

      // file을 연 횟수가 1번
      if((struct file *)file_->inode->open_cnt == 1){
        f->eax = thread_current()->tid;   // return file descriptor 
        return_val = f->eax;
       //printf("return val at first is %d\n", return_val);
      }

      // file을 연 횟수가 2번 이상? => 같은 return_val을 return 하면 안 됨. 1 증가시켜서 return
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



  /* 2-5. SYS_WRTIE(9): argument 3
      - 부분적으로만 구현
      - argument가 3개
        - arg[1] = fd
        - arg[2] = buffer
        - arg[3] = size */

  else if(*(uint32_t *)f->esp == 9){ // SYS_WRITE 
    //int count = 0; // count length of an argument
    //int tmp = 72;
   
    //while(*((char *)f->esp+tmp) != ' '){
      //printf("if i die\n");
      //count = count+1; 
      //tmp = tmp+1;
    //}

    // arg[1] valid address 확인
    if((uint32_t *)(f->esp+20) > 0xc0000000 || (uint32_t *)(f->esp+20) < 0x8048000 || (uint32_t *)(f->esp+20) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }

    // arg[2] valid address 확인
    if(*(uint32_t *)(f->esp+24) > 0xc0000000 || *(uint32_t *)(f->esp+24) < 0x8048000 || *(uint32_t *)(f->esp+24) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }
    
    // arg[3] valid address 확인
    if((uint32_t *)(f->esp+28) > 0xc0000000 || (uint32_t *)(f->esp+28) < 0x8048000 || (uint32_t *)(f->esp+28) == NULL){
      printf("%s: exit(%d)\n", front, -1);
      thread_exit();
    }

    // fd == 1 경우에만 putbuf(buffer, size) 호출하고 return size
    if(*(uint32_t *)(f->esp+20) == 0x1)
      putbuf(*(uint32_t *)(f->esp+24), *(uint32_t *)(f->esp+28));
      f->eax = *(uint32_t *)(f->esp+28);
  }



  /* 2-6. SYS_CLOSE(12): argument 1 */

  else if(*(uint32_t *)f->esp == 12){ //SYS_CLOSE  why all pass without any code?
    //hex_dump((uint32_t *)(f->esp), (uint32_t *)(f->esp), 300, 1); 
    //printf("file descriptor is %p\n", ((uint32_t *)(f->esp+8)));
    //printf("tid is %d\n", thread_current()->tid);
    //printf("file descriptor is %p\n", *((uint32_t *)(f->esp+8)));
    //file_close((struct file *)*((uint32_t *)(f->esp+8))); fd를 가지고 struct file*를 끌어낼 수 있나
  }
}



///////////////// END OF SYSTEM CALLS /////////////////

//void my_exit(int status){
//  exit(1);
//  NOT_REACHED ();
//}



