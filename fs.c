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
#include <stdio.h>
#include <stdlib.h>


// 📂
#define FN_MAX_LEN 256 // file name
#define FD_MAX_NUM 32  // file descriptor
#define F_MAX_NUM  7500

#define FI_START = 1   // file info
#define FD_START = 501 // file data


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
// 1st int of the last file data block is 0
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
int make_fs(char *disk_name)
{
  make_disk(disk_name);
  open_disk(disk_name);

  SP = (super_block*)malloc(sizeof(super_block));
  SP->f_num = 0;
  SP->fi_start = FI_START;
  SP->fd_start = FD_START;

  // write super block to disk block 0
  char buf[BLOCK_SIZE] = "";
  memcpy(buf, SP, sizeof(*SP));
  bw(0, buf, sizeof(buf));

  free(SP);
  close_disk();

  pr("✅ file system [%s] created\n", disk_name);
  return 0;
}


int mount_fs(char *disk_name)
{
  open_disk(disk_name);

  // read super block
  char buf[BLOCK_SIZE] = "";
  br(0, buf, sizeof(buf));
  memcpy(SP, buf, sizeof(*SP));

  // read file info
  int n = SP->f_num;
  int s = sizeof(file_info) * n;
  char buf_1[s] = "";
  br(SP->fi_start, buf_1, s);
  memcpy(FI, buf_1, s);

  pr("✅ file system [%s] mounted\n", disk_name);
  return 0;
}


int umount_fs(char *disk_name)
{
  // write super block
  char buf[BLOCK_SIZE] = "";
  memcpy(buf, SP, sizeof(*SP));
  bw(0, buf, sizeof(buf));

  // write file info
  int n = SP->f_num;
  int s = sizeof(file_info) * n;
  char buf_1[s] = "";
  char *p = buf_1;
  for (int i=0; i<F_MAX_NUM; i++) {
    if (FI[i].used) {
      memcpy(p, &FI[i], sizeof(file_info));
      p += sizeof(file_info);
    }
  }
  bw(SP->fi_start, buf_1, s);

  close_disk();

  pr("✅ file system [%s] umounted\n", disk_name);
  return 0;
}


int fs_create(char *name)
{
  if (f_check_exists(name, __func__))
    return -1;

  int i = f_get_new_i(name, __func__);
  if (i < 0)
    return -1;

  FI[i].used = true;
  strcpy(FI[i].name, name);
  FI[i].size = 0;
  FI[i].head = 0;
  FI[i].fd_count = 0;

  SP->f_num++;

  pr("✅ file [%s] created\n", name);
  return 0;
}


int fs_delete(char *name)
{
  if (!f_check_exists(name, __func__))
    return -1;

  if (f_check_in_use(name, __func__))
    return -1;

  int i = f_get_i_by_name(name, __func__);
  if (i < 0)
    return -1;

  // del file data
  int s = sizeof(int);
  char buf[s] = "";
  int block = FI[i].head;
  while (block != 0) {
    bw(block, 0, s);
    br(block, buf, s);
    memcpy(&block, buf, s);
  }

  FI[i].used = false;
  SP->f_num--;

  pr("✅ file [%s] deleted\n", name);
  return 0;
}


int fs_open(char *name)
{
  if (!f_check_exists(name, __func__))
    return -1;

  int fd = fd_get_new_i(__func__);
  if (fd < 0)
    return -1;

  int i = f_get_i_by_name(name, __func__);
  if (i < 0)
    return -1;

  FI[i].fd_count++;
  FD[fd].used = true;
  FD[fd].fi = i;
  FD[fd].offset = 0;

  pr("✅ file [%s] opened\n", name);
  return j;
}


int fs_close(int fd)
{
  if (!fd_check_valid(fd, __func__))
    return -1;

  int i = FD[fd].fi;

  // write data
  XXX

  FI[i].size = XXX;
  FI[i].fd_count--;
  FD[fd].used = false;
 
  pr("%s ", __func__);
  pr("file [%s] closed\n", FI[i].name);
  return 0;
}


// https://man7.org/linux/man-pages/man2/read.2.html
int fs_read(int fd, void *buf, size_t count)
{
  if (!fd_check_valid(fd, __func__))
    return -1;

  if (count <= 0) {
    pe("invalid count", __func__);
    return -1;
  }

  int i = FD[fd].fi;

  int s = count;
  if (s > FI[i].size)
    s = FI[i].size;
  int s2 = sizeof(int);
  int s3 = BLOCK_SIZE - sizeof(int);

  char buf[s] = "";
  char *p = buf;

  int block = FI[i].head;
  char buf_block[s2] = "";

  int blocks = (s-1)/BLOCK_SIZE + 1;
  while (blocks > 0) {
    if (blocks != 1)
      br(block + s2, p, s3);
    else
      br(block + s2, p, s % s3);
    p += s3;

    br(block, buf_block, s2);
    memcpy(&block, buf_block, s2);

    blocks-=1;
  }
}


// https://man7.org/linux/man-pages/man2/write.2.html
int fs_write(int fd, void *buf, size_t count)
{
  if (!fd_check_valid(fd, __func__))
    return -1;

  if (count <= 0) {
    pe("invalid count", __func__);
    return -1;
  }

  Xxx
}


// https://man7.org/linux/man-pages/man2/lseek.2.html
int fs_lseek(int fd, int offset, int whence=0)
{
  if (!fd_check_valid(fd, __func__))
    return -1;

  if (offset<0 || offset>=FI[FD[fd].fi].size) {
    pr("%s offset beyond range\n", __func__);
    return -1;
  }

  if (whence<0 || whence>=FI[FD[fd].fi].size) {
    pr("%s whence beyond range\n", __func__);
    return -1;
  }

  FD[fd].offset = whence + offset;

  pr("✅ %s\n", __func__);
  return 0;
}


// https://man7.org/linux/man-pages/man2/ftruncate.2.html
int fs_truncate(int fd, int length)
{
  if (!fd_check_valid(fd, __func__))
    return -1;

  if (length <= 0) {
    pe("length must be > 0", __func__);
    return -1;
  }

  int i = FD[fd].fi;
  int s = FD[fd].size;
  int s2 = sizeof(int);
  int s3 = BLOCK_SIZE - sizeof(int);
  int s4 = s3 - s % s3

  if (length < s) {
    int block = get_block_num_by_size(i, length);

    // update EOF
    char buf3[s3] = "";
    br(block+s2, buf3, s3);
    buf3[length % s3 - 1] = -1;
    bw(block+s2, buf3, s3);

    // free blocks
    char buf2[s2] = "";
    while (block) {
      bw(block, buf2, s2);
      block = get_next_block_num(block);
      if (block == -1)
        break;
    }
  }

  // zero padding
  if (length > s) {
    int block = get_block_num_by_size(i, s);

    // update og last block
    char buf4[s4] = "";
    bw(block+s2+s%s3-1, buf4, s4);

    //
    int t = (length-s-1)/s3 + 1
    char buf3[s3] = "";
    while (t > 0) {
      int n = get_free_block_num();
      clean_block(n);
      update_block_addr(block, n);
      block = n;
      t-=1;
    }
  }

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








