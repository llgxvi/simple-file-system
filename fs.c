/*
block size 4096        (4K)
block num  8192
disk size  4096 * 8192 (32M) 33,554,432

Data Structure | Block
——————————————————————
super block    | 0
file info      | 1-500
file data      | 501-

*/


// 📂
#define FI_START = 1   // file info
#define FD_START = 501 // file data

#define FN_MAX_LEN 256 // file name
#define FD_MAX_NUM 32  // file descriptor
#define F_MAX_NUM  7500


// 📂
#include <stdio.h>
#include <string.h>


// 📂
#define pr printf
#define br block_read
#define bw block_write


// 📁
typedef enum { false, true } bool;


typedef struct 
{
  int f_num;
  int fi_start; // file info
  int fd_start; // file data
} super_block;


// 4096 / 272 = 15 
// 15   * 500 = 7500
// file_info.head holds 1st block num of file data
// 1st int of every file data block holds block num of next file data block
// 1st int of the last file data block is -1
typedef struct
{
  bool used;             // 4
  char name[FN_MAX_LEN]; // 256
  int  size;             // 4
  int  head;             // 4
  char fd_count;         // 1
} file_info;             // 269 + 3


typedef struct 
{
  bool used;
  int  fi; // file_info index
  int  offset;
} file_descriptor;


// 📂
super_block     *SP;
file_info       FI[F_MAX_NUM];
file_descriptor FD[FD_MAX_NUM];


// 📂
// ✅✅
int make_fs(char *disk_name)
{
  make_disk(disk_name);
  open_disk(disk_name);

  SP = (super_block*)malloc(sizeof(super_block));
  SP->f_num = 0;
  SP->fi_start = FI_START;
  SP->fd_start = FD_START;

  // write super block to disk block 0
  int s = sizeof(*SP);
  char buf[s] = "";
  memcpy(buf, SP, s);
  bw(0, buf, s);

  free(SP);
  close_disk();

  pr("✅ file system [%s] created\n", disk_name);
  return 0;
}


// ✅✅
int mount_fs(char *disk_name)
{
  open_disk(disk_name);

  // read super block
  int s = sizeof(super_block);
  char buf[s] = "";
  br(0, buf, s);
  memcpy(SP, buf, s);

  // read file info
  s = sizeof(file_info) * SP->f_num;
  char buf2[s] = "";
  br(SP->fi_start, buf2, s);
  memcpy(FI, buf2, s);

  pr("✅ file system [%s] mounted\n", disk_name);
  return 0;
}


// ✅✅
int umount_fs(char *disk_name)
{
  // write super block
  int s = sizeof(super_block);
  char buf[s] = "";
  memcpy(buf, SP, s);
  bw(0, buf, s);

  // write file info
  s = sizeof(file_info) * SP->f_num;
  char buf2[s] = "";
  char *p = buf2;
  for (int i=0; i<F_MAX_NUM; i++) {
    if (FI[i].used) {
      memcpy(p, &FI[i], sizeof(file_info));
      p += sizeof(file_info);
    }
  }
  bw(SP->fi_start, buf2, s);

  close_disk();

  pr("✅ file system [%s] umounted\n", disk_name);
  return 0;
}


// ✅✅
int fs_create(char *name)
{
  if (!is_fn_valid(name)) {
    pe("invalid file name");
    return -1;
  }

  if (is_f_exists(name)) {
    pe("file exists");
    return -1;
  }

  int fi;

  fi = get_new_fi(name);
  if (fi < 0) {
    pe("exceed file number limit");
    return -1;
  }

  FI[fi].used = true;
  strcpy(FI[fi].name, name);
  FI[fi].size = 0;
  FI[fi].head = 0;
  FI[fi].fd_count = 0;

  SP->f_num++;

  pr("✅ file [%s] created\n", name);
  return 0;
}


// ✅✅
int fs_delete(char *name)
{
  if (!is_fn_valid(name)) {
    pe("invalid file name");
    return -1;
  }

  if (!is_f_exists(name)) {
    pe("file not exists");
    return -1;
  }

  if (is_f_in_use(name)) {
    pe("file in use");
    return -1;
  }

  int fi = get_fi_by_name(name);

  FI[fi].used = false;
  SP->f_num--;

  // del file data
  int block = FI[fi].head;
  while (block != -1) {
    set_block_addr(block, 0);
    block = get_next_block(block);
  }

  pr("✅ file [%s] deleted\n", name);
  return 0;
}


// ✅✅
int fs_open(char *name)
{
  if (!is_fn_valid(name)) {
    pe("invalid file name");
    return -1;
  }

  if (!is_f_exists(name)) {
    pe("file not exists");
    return -1;
  }

  int fd = get_new_fd();

  if (fd < 0) {
    pe("no fd available");
    return -1;
  }

  int fi = get_fi_by_name(name);

  FI[fi].fd_count++;
  FD[fd].used = true;
  FD[fd].fi = fi;
  FD[fd].offset = 0;

  pr("✅ file [%s] opened\n", name);
  return fd;
}


// ✅✅
int fs_close(int fd)
{
  if (!is_fd_valid(fd)) {
    pe("invalid fd");
    return -1;
  }

  int fi = FD[fd].fi;

  FI[fi].fd_count--;
  FD[fd].used = false;

  pr("✅ file [%s] closed\n", FI[fi].name);
  return 0;
}


// ✅✅
int fs_read(int fd, char *buf, int count)
{
  if (!is_fd_valid(fd)) {
    pe("invalid fd");
    return -1;
  }

  if (count <= 0) {
    pe("count should be > 0");
    return -1;
  }

  int fi = FD[fd].fi;
  int s1 = FI[fi].size;
  int offset = FD[fd].offset;
 
  if (offset >= s1) {
    //
  }

  if (count + offset > s1)
    count = s1 - offset;

  int s;
  int s2 = sizeof(int);
  int s3 = BLOCK_SIZE - sizeof(int);

  char *p = buf;
  int c = count;

  //
  int block = get_block_by_size(offset);
  s = s3 - offset % s3;
  if (c <= s) {
    char b[c] = "";
    br(block+s2+offset%s3, b, c);
    memcpy(p, b, c);
    return c;
  }

  //
  p += s;
  c -= s;

  while (c > 0) {
    block = get_next_block(block);

    if (c >= s3) {
      char b[s3] = "";
      br(block+s2, b, s3);
      memcpy(p, b, s3);
      p += s3;
      c -= s3;
      continue;
    }

    char b[c] = "";
    br(block+s2, b, c);
    memcpy(p, b, c);
    return count;
  }
}


// ✅✅
int fs_write(int fd, char *buf, int count)
{
  if (!is_fd_valid(fd)) {
    pe("invalid fd");
    return -1;
  }

  if (count <= 0) {
    pe("count should be > 0");
    return -1;
  }

  int fi = FD[fd].fi;
  int offset = FD[fd].offset;

  int s;
  int s1 = FI[fi].size;
  int s2 = sizeof(int);
  int s3 = BLOCK_SIZE - sizeof(int);

  char *p = buf;
  int c = count;

  //
  int block = get_block_by_size(offset);
  s = s3 - offset % s3;
  if (c < s) {
    char b[s] = "";
    memcpy(b, p, c);
    b[c] = -1;
    bw(block+s2+offset%s3, b, c+1);
    return count;
  }

  //
  p += s;
  c -= s;

  int curr = block;

  while (c >= 0) {
    int next = get_next_or_free_block(curr);
    if (next == -1) {
      pe("no more space");
      return -1;
    }
    block_set_addr(curr, next);
    curr = next;

    if (c >= s3) {
      char b[s3] = "";
      memcpy(b, p, c);
      bw(next+s2, b, c);
      p += s3;
      c -= s3;
      continue;
    }

    char b[s3] = "";
    memcpy(b, p, c);
    b[c] = -1;
    bw(next+s2, b, c+1);
    return count;
  }
}


// ✅✅
// https://man7.org/linux/man-pages/man2/lseek.2.html
int fs_lseek(int fd, int offset, int whence=0)
{
  if (!is_fd_valid(fd)) {
    pe("invalid fd");
    return -1;
  }

  int fi = FD[fd].fi;
  int s = FI[fi].size;

  if (offset<0 || offset>=s) {
    pe("offset beyond range");
    return -1;
  }

  if (whence<0 || whence>=s) {
    pe("whence beyond range");
    return -1;
  }

  if ((whence+offset) >= s) {
    pe("whence+offset beyond range");
    return -1;
  }

  FD[fd].offset = whence + offset;

  pr("✅ %s\n", __func__);
  return 0;
}


// ✅✅
// https://man7.org/linux/man-pages/man2/ftruncate.2.html
int fs_truncate(int fd, int length)
{
  if (!is_fd_valid(fd)) {
    pe("invalid fd");
    return -1;
  }

  if (length < 0) {
    pe("length must be >= 0");
    return -1;
  }

  int fi = FD[fd].fi;
  int s = FI[fi].size;
  int s2 = sizeof(int);
  int s3 = BLOCK_SIZE - sizeof(int);
  int s4 = s3 - s % s3

  if (length < s) {
    int block = get_block_by_size(fi, length);

    // update EOF
    char buf3[s3] = "";
    br(block+s2, buf3, s3);
    buf3[length % s3] = -1;
    bw(block+s2, buf3, s3);

    int next = get_next_block(block);
    set_block_addr(block, -1);

    // free blocks
    while (next != -1) {
      set_block_addr(next, 0);
      next = get_next_block(next);
    }
  }

  if (length > s) {
    int block = get_block_by_size(fi, s);

    // replace EOF with zero padding
    char buf4[s4] = "";
    bw(block+s2+s%s3, buf4, s4);

    // (length-1)/s3 + 1
    // -
    // (s-1)/s3 + 1
    int c = (length-s)/s3;
    while (c > 0) {
      int next = get_free_block();
      clean_block(next);
      update_block_addr(block, next);
      block = next;
      c-=1;
    }

    // set new EOF
    char buf3[s3] = "";
    buf3[length % s3] = -1;
    bw(block+s2, buf3, s3);
    set_block_addr(block, -1);
  }

  pr("✅ %s", __func__);
  return 0;
}


// 📂
// ✅✅
// addr: 1st int of block
void set_block_addr(int block, int addr) {
  int s = sizeof(int);
  char buf[s] = "";
  memcpy(buf, &addr, s);
  bw(block, buf, s);
}


// ✅✅
int get_free_block() {
  int block;
  int s = sizeof(int);
  char buf[s] = "";

  for (int i=SP->fd_start; i<DISK_BLOCKS; i++) {
    br(i, buf, s);
    memcpy(&block, buf, s);
    if (block == 0)
      return block;
  }

  return -1;
}


// ✅✅
int clean_block(int block) {
  char buf[BLOCK_SIZE] = "";
  bw(block, buf, BLOCK_SIZE);
}


// ✅✅
int get_next_block(int curr) {
  int next;
  int s = sizeof(int);
  char buf[s] = "";

  br(curr, buf, s);
  memcpy(&next, buf, s);

  if (next == 0)
    return -1;

  return next;
}


// ✅✅
int get_next_or_free_block(int curr) {
  int block;

  block = get_next_block(curr);
  if (block != -1)
    return block;

  block = get_free_block();
  if (block != -1)
    return block;

  return -1;
}


// ✅✅
int get_block_by_size(int fi, int size) {
  if (size > FI[fi].size) {
    return -1;
  }

  int block = FI[fi].head;
  int blocks = (size-1)/(BLOCK_SIZE-sizeof(int)) + 1;

  while (blocks > 1) {
    block = get_next_block(block);
    blocks-=1;
  }

  return block;
}


// 📂 Helper functions
// ✅✅
bool is_fn_valid(char *name)
{
  int s = sizeof(name);

  if (s<=0 || s>FN_MAX_LEN)
    return false;

  char c = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_.";

  for (int i=0; i<s; i++)
    if(strchr(c, name[i]) == NULL)
      return false;

  return true;
}


// ✅✅
bool is_f_exists(char *name)
{
  for (int i=0; i<F_MAX_NUM; i++)
    if (FI[i].used)
      if (strcmp(FI[i].name, name) == 0)
        return true;

  return false;
}


// ✅✅
int get_new_fi() {
  for (int i=0; i<F_MAX_NUM; i++)
    if (!FI[i].used)
      return i;

  return -1;
}


// ✅✅
// fi: file index
int get_fi_by_name(char *name) {
  for (int i=0; i<F_MAX_NUM; i++)
    if (strcmp(FI[i].name, name) == 0)
      return i;

  return -1;
}


// ✅✅
bool is_f_in_use(char *name)
{
  for (int i=0; i<F_MAX_NUM; i++)
    if (strcmp(FI[i].name, name) == 0)
      if(FI[i].fd_count > 0)
        return true;
      else
        return false;
}


// ✅✅
bool is_fd_valid(int fd)
{
  if (fd>=0 && fd<FD_MAX_NUM && FD[fd].used)
    return true;

  return false;
}


// ✅✅
int get_new_fd()
{
  for (int i=0; i<FD_MAX_NUM; i++)
    if (!FD[i].used)
      return i;

  return -1;
}








