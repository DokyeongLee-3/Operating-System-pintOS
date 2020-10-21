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





/* ARGUMENT PASSING

1. 구현해야 할 부분
  process_execute ()는 file_name을 받아서 새로운 process를 만드는 일을 한다.
  kernel은 여기에서 user stack에 argument를 미리 넣어줘야 함.

2. skeleton 분석
  2-1. process_execute ()가 호출되는 과정
    threads/init.c 의 main에서 run_actions ()을 호출
    -> run_actions ()가 run_task ()를 호출 (command가 'run' 이므로)
    -> run_task ()가 process_execute () 호출 (USERPROG이기 때문)
  2-2. process_execute ()가 argument passing에 관여하는 과정
    process_execute ()의 thread_create ()를 호출하며 start_process를 넘겨줌
    -> thread_create ()에서 start_process ()가 실행됨
    -> start_process ()의 load ()에서 user program을 불러옴 (filesys_open () 이용)
       => load ()에서 argument passing을 해줘야 함
    -> load ()에서 setup_stack ()을 호출. 이름부터 stack과 관련있음!!!!!!!!!!!!!!!!!!
    -> setup_stack ()에서 argument passing 해주면 됨

3. 구현 방법
  3-1. load () 수정
    load ()에서 user program file을 열기 위해 file_name을 넘겨줘야 한다
    => filesys_open ()을 할 때 file_name의 첫번째 argument를 넘겨줌으로써 해결 (strtok_r () 이용)
  3-2. setup_stack () 수정
    setup_stack ()에서 argument들을 stack에 push
    => 다섯 단계로 나누어서 구현
       3-2-1. strtok_r ()을 이용해서 argument 분리
       3-2-2. stack에 arguments 기록
       3-2-3. stack에 word-align 기록
       3-2-4. stack에 argments에 대한 ptr 기록
       3-2-5. stack에 argv address, argc, return address 기록

END */





// threads/thread.c 에 선언되어 있는 all_list. 모든 프로세스가 이 리스트에 들어있다.
extern struct list all_list;

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);




/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
/* process_execute ()는 threads/init.c에 있는 main () -> run_actions() -> run_task () -> process_execute () 이 순서로 호출됨.
   file_name은 original command_line의 특정 word의 시작을 가리킴. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;


  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  /* file_name(original command_line)을 PGSIZE만큼 fn_copy에 복사함.
     fn_copy가 사용할 공간(page)을 할당 받음. (이 부분은 정확하게 모르겠음..)
     threads/palloc.c에 있는 palloc_get_page ()를 이용해서 page 할당.
     parameter로 0을 넘겨주면 user_pool의 page를 사용함.
     또한 이 함수에서 palloc_get_multiple ()을 호출하고, 이때 기본적으로 한 페이지가 할당되기에 argument의 size는 한 페이지(4KB)를 넘기지 않음. */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);



  /* Create a new thread to execute FILE_NAME. */
  /* threads/thread.c에 있는 thread_create ()를 이용해서 thread를 만들게 됨.
     => tid_t thread_create (const char *name, int priority, thread_func *function, void *aux)
        init_thread ()에는 file_name 사용되고 stack frame 설정할 땐 fn_copy 사용됨.
        그리고 start_process ()의 ptr도 stack frame 설정할 때 사용되는 듯 (이 파트는 잘 모르겠다..) */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
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




  // file_name을 copy해둔 char*로 parse를 진행해보자 QQQQQQQQQQQQQQQQQQQQQ
  /* memset(void *_Dst, int _Val, size_t _Size)
     dst라는 시작 위치부터(포인터로 주어짐) size byte만큼 value로 초기화 하는 함수. */
  char my_copy[100];
  memset(my_copy, 0, sizeof file_name);
  strlcpy(my_copy, file_name, 10);



  /* user process를 불러오는 부분
     load () 함수를 살펴보면 load에서 file을 열어서 userprog을 불러오는 것을 볼 수 있음 */
  success = load (file_name, &if_.eip, &if_.esp);
  //hex_dump(if_.esp, if_.esp, PHYS_BASE - if_.esp, true);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) 
    thread_exit ();


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
  int i = 0;
  while(true){
    i = i + 1;
    if(i > 1000000000)
      break;
  }
  return -1;
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
  
  

  
  /* my_copy로 file_name을 복사한 후 argument를 parsing.
     복사를 하는 이유는 원래 command_line을 건드리지 않기 위해서. */

  //printf("file_name is ? %s\n", file_name);
  //printf("file_name size is %d\n", sizeof file_name);
  char my_copy[100];
  memcpy(my_copy, file_name, sizeof my_copy);
  //printf("original file name(my_copy): %s\n", my_copy);
  char *save_ptr;
  char *front = strtok_r(file_name, " ", &save_ptr);
  
  

  /* Open executable file. */
  file = filesys_open (front);
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
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
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
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

static bool install_page (void *upage, void *kpage, bool writable);

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
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
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

  /* minimal stack을 위해 page를 할당 받음.
     kpage의 페이지 할당 성공 -> esp(stack pointer)를 page 시작부분으로 mapping (install_page는 잘 모르겠음..)
     kpage의 페이지 할당 실패 -> success = false 설정하고 page 할당 해제함. */
  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        *esp = PHYS_BASE;
      else
        palloc_free_page (kpage);
    }



///////////// argument passing /////////////

/*

ARGUMENT PASSING 구현

1. strtok_r ()을 이용해서 argument 분리
2. stack에 arguments 기록
3. stack에 word-align 기록
4. stack에 argments에 대한 ptr 기록
5. stack에 argv address, argc, return address 기록

*/



  /* 변수 선언 */

  // argument의 갯수
  int arg_count = 0;
  // argument들의 address를 담은 배열
  char *front[25];
  // 각 argument의 size (null 까지 포함한 size) - stack에 기록할 때 null까지 함께 기록하니까!
  int contain_null_size[25] = {0, };
  // backup을 위한 ptr
  char *save_ptr;



  if(success){

    /* 1. strtok_r ()을 이용해서 argument 분리
        - arg_count를 추적하며 front[] 배열에 argument의 address를 기록함
        - argument number와 front, arg_count 사이의 관계
          - nth argument in front[n-1]
          - arg_count = n  */

    front[arg_count] = strtok_r(my_copy, " " , &save_ptr);
    arg_count = arg_count+1;
    while((front[arg_count] = strtok_r(NULL, " ", &save_ptr)) != NULL){
      arg_count = arg_count+1;
    }


    
    /* 2. stack에 arguments 기록
        - stack을 사용하면 esp가 감소함
        - argNum이 낮은 argument를 낮은 주소에 기록하려면, argNum이 높은 argument부터 기록해야 함
          tmp_count = arg_count를 1씩 깎으며 front[]에 접근, stack에 argNum이 높은 애부터 차례로 기록
        - argument를 기록할 때도 string을 역순으로 기록해야 함 (Big-Endian, 상위 비트를 낮은 주소에 배정)
          contain_null_size[]를 이용해서 null을 포함한 argument의 size 기록
          low byte부터 차례대로 high address에 기록 (bit에는 endian 구분 없음)
        - argument를 기록할 때의 순서는 관계없지만, 여기서는 4단계의 편의를 위해 이 convention을 따름 */

    int tmp_count = arg_count;

    while(tmp_count != 0){

      // argNum이 높은 argument부터 차례로 기록, nth argument는 contain_null_size[n-1]에 기록됨
      contain_null_size[tmp_count-1] = strlen(front[tmp_count-1]) + 1;   
      int temp_size = contain_null_size[tmp_count-1];

      // argument의 low byte부터 차례로 기록. Big-endian을 따름
      for( ; temp_size > 0; ){
        *esp = *esp - 1;
        *((char *)*esp) = front[tmp_count - 1][temp_size - 1];
        //printf(" **(char**)esp is %c\n", **(char**)esp);
        //printf("hex_dump %c\n", ((uint8_t*)(*esp))[contain_null_size-1]);
        temp_size = temp_size - 1;
      }

      //printf("*esp is %p and esp[] is %02hhx\n", *esp, ((uint8_t *)(*esp))[0]);
      tmp_count = tmp_count-1;
    }



    /* 3. stack에 word-align 기록
        - argument를 기록할 때 byte 단위로 기록했기에 word-align을 맞춰줘야 함
          - word size가 4 bytes이므로 이에 맞춰서 align할 것 */

    // word-align
    while((int)*esp % 4 != 0){
      *esp = *esp-1;
      **(char**)esp = '\0';
    }



    /* 4. stack에 argments에 대한 ptr 기록
        - nth argument의 stack address를 argNum이 큰 것부터 순차적으로 기록
          (2번처럼 argNum이 큰 argument의 address부터 차례로 기록)
        - address[kth_argument] = (stack_top_address) - (sum_of_argument_size_1_to_k)
        - address[(n+1)th_argument]도 기록해준다. null로 채움  */

    // argv[n+1] = 모두 null로 채움
    int i = 0;
    for( ; i < 4 ; i = i + 1){
      *esp = *esp - 1;
      **(char**)esp = '\0';
    }

    // argument n to 1 기록하기 위한 변수 선언
    tmp_count = arg_count;  // argNum이 큰 argument부터 기록
    int cumulative = 0;     // cumulative = (sum_of_argument_size_1_to_k)
    //printf("contain_null_size[tmp_count-1] is %d\n", contain_null_size[tmp_count-1]);
    
    // argument의 address 기록
    for( ; tmp_count > 0; tmp_count = tmp_count - 1){
      *esp= *esp - 4;   // address가 4 bytes이므로 4 bytes만큼 깎은 후 기록
      //printf(" PHYS_BASE-contain_null_size[tmp_count-1] is %p\n", PHYS_BASE-contain_null_size[tmp_count-1]);
      **(int **)esp = PHYS_BASE-contain_null_size[tmp_count-1] - cumulative;    // esp에 address[kth_argument] 기록
      cumulative = cumulative + contain_null_size[tmp_count-1];
    }




    /* 5. stack에 argv address, argc, return address 기록
        - argv = address[argv[0]] = esp + 4 (현재 stack값에서 4만큼 더한 값)
        - argc = arg_count
        - return address = null (4 bytes를 모두 null로 채울 것. fake return address)  */

    // argv 기록
    *esp = *esp - 4;        // esp update
    char* tmp = *esp + 4;   // argv address 계산, 0x bf ff ff ec
    **(int **)esp = tmp;    // push argv

    // argc 기록
    *esp = *esp - 4;            // esp update
    **(int**)esp = arg_count;   // push argc

    // return address 기록
    i = 0;
    for( ; i < 4; i = i + 1){
      *esp = *esp - 1;
      **(char **)esp = '\0';    // push fake return address
    }
  }
  //hex_dump(*esp, *esp, 300, 1);

///////////// END OF argument passing /////////////

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
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
