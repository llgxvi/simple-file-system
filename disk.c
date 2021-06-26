#include <stdio.h>  // printf
#include <unistd.h> // read, write, close, lseek
#include <fcntl.h>  // O_CREAT
#include <string.h> // strcmp, memset


// 📂
// 4096 * 8192
// 33,554,432  Bytes
// 32,768      KB
// 32          MB
#define DISK_BLOCKS 8192
#define BLOCK_SIZE  4096 // Bytes


// 📂 Function prototypes
void pe(char *one, const char *f);
int make_disk(char *name);
int open_disk(char *name);
int close_disk();
int block_write(int block, char *buf, int buf_size);
int block_read( int block, char *buf, int buf_size);
int check(int block, char *buf, int buf_size, const char *f);


// 📂 Private variables
static int active = 0;
static int handle = -1;


// 📂
// f: __func__
void pe(char *one, const char *f) {
  if (strcmp(f, "") == 0)
    printf("⚠️ %s\n", one);
  else
    printf("⚠️ %s() %s\n", f, one);
}


// 📂
int make_disk(char *name)
{ 
  int f;
  char buf[BLOCK_SIZE] = "";

  f = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  for (int i=0; i<DISK_BLOCKS; i++)
    write(f, buf, BLOCK_SIZE);

  close(f);

  return 0;
}


int open_disk(char *name)
{
  int f;
  
  if (active) {
    pe("disk already open", __func__);
    return -1;
  }
  
  f = open(name, O_RDWR, 0644);

  handle = f;
  active = 1;

  return 0;
}


int close_disk()
{
  if (!active) {
    pe("disk already closed", __func__);
    return -1;
  }
  
  close(handle);
  active = 0;
  handle = -1;

  return 0;
}


int block_write(int block, char *buf, int buf_size)
{
  if (check(block, buf, buf_size, __func__) == -1)
    return -1;

  lseek(handle, block*BLOCK_SIZE, SEEK_SET);

  write(handle, buf, buf_size);

  return 0;
}


int block_read(int block, char *buf, int buf_size)
{
  if (check(block, buf, buf_size, __func__) == -1)
    return -1;

  lseek(handle, block*BLOCK_SIZE, SEEK_SET);

  read(handle, buf, buf_size);

  return 0;
}


// 📂
// f: __func__
int check(int block, char *buf, int buf_size, const char *f)
{
  if (!active) {
    pe("disk not open", f);
    return -1;
  }

  if ((block<0) || (block>=DISK_BLOCKS)) {
    pe("block out of range", f);
    return -1;
  }

  if (buf_size<0 || buf_size>BLOCK_SIZE) {
    pe("buf_size out of range", f);
    return -1;
  }

  return 0;
}


// 📂 Test
int main() {
  char buf1[10];
  char buf2[20];

  memset(buf1, 100, sizeof(buf1));

  make_disk("disk.test");
  open_disk("disk.test");
  block_write(3, buf1, sizeof(buf1));
  block_read( 3, buf2, sizeof(buf2));
  close_disk();

  for (int i=0; i<sizeof(buf2); i++) {
    printf("%d %d\n", i, buf2[i]);
  }

  return 0;
}
