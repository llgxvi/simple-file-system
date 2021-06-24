// array size as function parameter
// https://stackoverflow.com/a/609284

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// 4096 X 8192
// 33,554,432 Bytes
// 32,768     KB
// 32         MB
#define DISK_BLOCKS 8192
#define BLOCK_SIZE  4096

// Function prototypes
int make_disk(char *name);
int open_disk(char *name);
int close_disk();
int block_write(int block, char *buf, int bufSize);
int block_read( int block, char *buf, int bufSize);

// Private variables
static int active = 0;
static int handle; 

//
int make_disk(char *name)
{ 
  int f;
  char buf[BLOCK_SIZE];

  f = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  memset(buf, 0, BLOCK_SIZE);

  for (int i=0; i<DISK_BLOCKS; i++)
    write(f, buf, BLOCK_SIZE);

  close(f);

  return 0;
}

//
int open_disk(char *name)
{
  int f;
  
  if (active) {
    perror("open_disk: disk already open\n");
    return -1;
  }
  
  f = open(name, O_RDWR, 0644);

  handle = f;
  active = 1;

  return 0;
}

//
int close_disk()
{
  if (!active) {
    perror("close_disk: disk already closed\n");
    return -1;
  }
  
  close(handle);
  active = handle = 0;

  return 0;
}

//
int block_write(int block, char *buf, int bufSize)
{
  if (!active) {
    perror("block_write: disk not open\n");
    return -1;
  }

  if ((block<0) || (block>=DISK_BLOCKS)) {
    perror("block_write: block index out of range\n");
    return -1;
  }

  if (bufSize != BLOCK_SIZE) {
    perror("block_write: buf size must equal block size\n");
    return -1;
  }

  lseek(handle, block * BLOCK_SIZE, SEEK_SET);

  write(handle, buf, BLOCK_SIZE);

  return 0;
}

//
int block_read(int block, char *buf, int bufSize)
{
  if (!active) {
    perror("block_read: disk not open\n");
    return -1;
  }

  if ((block<0) || (block>=DISK_BLOCKS)) {
    perror("block_read: block index out of range\n");
    return -1;
  }

  if (bufSize != BLOCK_SIZE) {
    perror("block_read: buf size must equal block size\n");
    return -1;
  }

  lseek(handle, block * BLOCK_SIZE, SEEK_SET);

  read(handle, buf, BLOCK_SIZE);

  return 0;
}

// Test
int main() {
  char buf1[BLOCK_SIZE];
  char buf2[BLOCK_SIZE];

  memset(buf1, 100, BLOCK_SIZE);

  make_disk("disk.test");
  open_disk("disk.test");
  block_write(3, buf1, sizeof buf1);
  block_read( 3, buf2, sizeof buf2);
  close_disk();

  for (int i=0; i<sizeof(buf2); i++) {
    printf("%d ", buf2[i]);
  }

  return 0;
}
