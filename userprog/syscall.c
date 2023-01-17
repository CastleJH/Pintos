#include "userprog/syscall.h"

static void syscall_handler (struct intr_frame *);
struct semaphore s_file;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init(&s_file, 1);
}

void 
valid_address (void *addr) {
  if (!is_user_vaddr(addr) || !pagedir_get_page(thread_current()->pagedir, addr)) exit(-1);
}

void 
valid_string (void *addr) {
  while (1) {
    valid_address(addr);
    if (*(char *)addr == '\0') return;
    addr++;
  }
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  /*************************** 20191104 start (11) ***************************/
  //printf("syscall: %d\n", *(int *)(f->esp));
  //printf("f->esp: %x\n", f->esp);
  //hex_dump(f->esp, f->esp, 100, 1);
  
  valid_address(f->esp);
  switch (*(int *)(f->esp)) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      valid_address(f->esp + 4);
      int status = (int)*(uint32_t *)(f->esp + 4);
      exit(status);
      break;
    case SYS_EXEC:
      valid_address(f->esp + 4);
      char *exec_file = (char *)*(uint32_t *)(f->esp + 4);
      valid_string(exec_file);
      f->eax = exec((const char*)exec_file);
      break;
    case SYS_WAIT:
      valid_address(f->esp + 4);
      tid_t tid = (tid_t)*(uint32_t *)(f->esp + 4);
      f->eax = wait(tid);
      break;
    case SYS_READ:
      valid_address(f->esp + 4);
      valid_address(f->esp + 8);
      valid_address(f->esp + 12);
      int read_fd = (int)*(uint32_t *)(f->esp + 4);
      void *read_buf = *(uint32_t *)(f->esp + 8);
      unsigned read_size = (unsigned)*(uint32_t *)(f->esp + 12);
      f->eax = read(read_fd, read_buf, read_size);
      valid_string(read_buf);
      break;
    case SYS_WRITE:
      valid_address(f->esp + 4);
      valid_address(f->esp + 8);
      valid_address(f->esp + 12);
      int write_fd = (int)*(uint32_t *)(f->esp + 4);
      void *write_buf = *(uint32_t *)(f->esp + 8);
      valid_string(write_buf);
      unsigned write_size = (unsigned)*(uint32_t *)(f->esp + 12);
      f->eax = write(write_fd, (const void *)write_buf, write_size);
      break;
    case SYS_FIBO:
      valid_address(f->esp + 4);
      int n = (int)*(uint32_t *)(f->esp + 4);
      f->eax = fibonacci(n);
      break;
    case SYS_MAXFOUR:
      valid_address(f->esp + 4);
      valid_address(f->esp + 8);
      valid_address(f->esp + 12);
      valid_address(f->esp + 16);
      int a = (int)*(uint32_t *)(f->esp + 4);
      int b = (int)*(uint32_t *)(f->esp + 8);
      int c = (int)*(uint32_t *)(f->esp + 12);
      int d = (int)*(uint32_t *)(f->esp + 16);
      f->eax = max_of_four_int(a, b, c, d);      
      break;
    case SYS_CREATE:
      valid_address(f->esp + 4);
      valid_address(f->esp + 8);
      char *create_file = (char *)*(uint32_t *)(f->esp + 4);
      valid_string(create_file);
      unsigned initial_size = (unsigned)*(uint32_t *)(f->esp + 8);
      f->eax = create(create_file, initial_size);
      break;
    case SYS_REMOVE:
      valid_address(f->esp + 4);
      char *remove_file = (char *)*(uint32_t *)(f->esp + 4);
      valid_string(remove_file);
      f->eax = remove(remove_file);
      break;
    case SYS_OPEN:
      valid_address(f->esp + 4);
      char *open_file = (char *)*(uint32_t *)(f->esp + 4);
      valid_string(open_file);
      f->eax = open(open_file);
      break;
    case SYS_CLOSE:
      valid_address(f->esp + 4);
      int close_fd = (int)*(uint32_t *)(f->esp + 4);
      close(close_fd);
      break;
    case SYS_FILESIZE:
      valid_address(f->esp + 4);
      int filesize_fd = (int)*(uint32_t *)(f->esp + 4);
      f->eax = filesize(filesize_fd);
      break;
    case SYS_SEEK:
      valid_address(f->esp + 4);
      valid_address(f->esp + 8);
      int seek_fd = (int)*(uint32_t *)(f->esp + 4);
      unsigned position = (unsigned)*(uint32_t *)(f->esp + 8);
      seek(seek_fd, position);
      break;
    case SYS_TELL:
      valid_address(f->esp + 4);
      int tell_fd = (int)*(uint32_t *)(f->esp + 4);
      f->eax = tell(tell_fd);
      break;
    default: 
      exit(-1);
      //printf("syscall!\n");
      break;
  }
  /*************************** 20191104 done  (11) ***************************/
}

void 
halt(void)
{
  shutdown_power_off();
}

void
exit (int status)
{
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", thread_current()->name, status);
  //for (int i = 0; i < 128; i++) file_close(thread_current()->fd[i]);
  thread_exit();
}

tid_t
exec (const char *file) 
{
  //printf("%s\n", file);
  tid_t tid = -1;
  struct thread *child;
  sema_down(&s_file);
  tid = process_execute(file);
  sema_up(&s_file);
  return tid;
}

int
wait (tid_t tid) 
{
  return process_wait(tid);
}

int
read (int fd, void *buffer, unsigned size)
{
  //printf("try read\n");
  int i, r = size;
  if (fd == 0) {
    for (i = 0; i < size; i++) {
      valid_address(buffer);
      *((char *)buffer++) = input_getc();
    }
  }
  else {
    if (0 > fd || fd >= 128) exit(-1);
    struct thread *c = thread_current();
    if (c->fd[fd] == NULL) exit(-1);
    sema_down(&s_file);
    valid_address(buffer);
    r = file_read(c->fd[fd], buffer, size);
    sema_up(&s_file);
  }
  return r;
}

int
write(int fd, const void *buffer, unsigned size)
{
  //printf("try write\n");
  int r = size;
  if (fd == 1) {
    putbuf(buffer, size);
  }
  else {
    if (0 > fd || fd >= 128) exit(-1);
    struct thread *c = thread_current();
    if (c->fd[fd] == NULL) exit(-1);
    sema_down(&s_file);
    r = file_write(c->fd[fd], buffer, size);
    sema_up(&s_file);
  }
  return r;
}

int
fibonacci(int n)
{
  int ret = 1, prev = 0, tmp, j;
  for (j = 1; j < n; j++) {
    tmp = ret;
    ret += prev;
    prev = tmp;
  }
  return ret;
}

int
max_of_four_int (int a, int b, int c, int d) 
{
  int maxnum = a;
  if (maxnum < b) maxnum = b;
  if (maxnum < c) maxnum = c;
  if (maxnum < d) maxnum = d;
  return maxnum;
}

bool
create(const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

bool
remove(const char *file)
{
  return filesys_remove(file);
}

int
open(const char* file)
{
  sema_down(&s_file);
  struct file *f = filesys_open(file);
  if (f == NULL) {
    sema_up(&s_file);
    return -1;
  }
  struct thread *t = thread_current();
  for (int i = 2; i < 128; i++) {
    if (t->fd[i] == NULL) {
      if (!strcmp(t->name, file)) file_deny_write(f);
      t->fd[i] = f;
      sema_up(&s_file);
      return i;
    }
  }
  sema_up(&s_file);
  return -1;
}

void
close(int fd)
{
  if (0 > fd || fd >= 128) exit(-1);
  struct thread *c = thread_current();
  if (c->fd[fd] == NULL) exit(-1);
  file_close(c->fd[fd]);
  c->fd[fd] = NULL;
}

int
filesize(int fd)
{
  if (0 > fd || fd >= 128) exit(-1);
  struct thread *c = thread_current();
  if (c->fd[fd] == NULL) exit(-1);
  return file_length(c->fd[fd]);
}

void
seek(int fd, unsigned position)
{
  if (0 > fd || fd >= 128) exit(-1);
  struct thread *c = thread_current();
  if (c->fd[fd] == NULL) exit(-1);
  file_seek(c->fd[fd], position);
}

unsigned
tell(int fd)
{
  if (0 > fd || fd >= 128) exit(-1);
  struct thread *c = thread_current();
  if (c->fd[fd] == NULL) exit(-1);
  return file_tell(c->fd[fd]);
}
