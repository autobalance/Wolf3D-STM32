/*
    Adapted from STM32H7 Cube.
    Partial implementation of system calls for use with 'newlib', primarily
    to use printf to output to UART and interface with an SD card via FatFS.

    TODO: The calls to read/write to the SD card are essentially double translated
          and the overhead is significant (~770us via syscalls vs ~375us via direct
          call to FatFS for reading 4096 bytes). The overhead results in an actually
          perceivable performance deficit. Mitigating the overhead can be done through
          changing MAX_PAGES_IN_MEMORY in 'id_pm.h' from 8 to 32 (32K to 128K !!!). So,
          maybe just implement the ID Page Manager ('id_pm.h', 'id_pm.c') via direct
          calls to FatFS, and have the rest of the game interface through syscalls,
          e.g. for game saves.
*/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include <ff.h>

#undef errno
extern int errno;

// arbitrary limit of 32 files open at any time
// first 3 are reserved as usual: stdin, stdout, stderr
#define MAX_FILNOS 32
int filnos[MAX_FILNOS] = {1, 1, 1, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0};

FIL fils[MAX_FILNOS];

#define MAX_STACK_SIZE 0x20000

extern int __io_putchar(int ch) __attribute__((weak));
extern int __io_getchar(void) __attribute__((weak));

char *_eheap = (char *) 0x24080000;

caddr_t _sbrk(int incr) {
  extern char end asm("end");
  static char* heap_end;
  char* prev_heap_end, *min_stack_ptr;

  if (heap_end == 0) {
    heap_end = &end;
  }

  prev_heap_end = heap_end;

  if (heap_end + incr > _eheap)
  {
    errno = ENOMEM;
    return (caddr_t) - 1;
  }

  heap_end += incr;

  return (caddr_t) prev_heap_end;
}


int _gettimeofday (struct timeval* tp, struct timezone* tzp) {
  if (tzp) {
    tzp->tz_minuteswest = 0;
    tzp->tz_dsttime = 0;
  }

  return 0;
}

void initialise_monitor_handles() {
}

int _getpid(void) {
  return 1;
}

int _kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}

void _exit (int status) {
  _kill(status, -1);
  while (1) {}
}

int _write(int file, char* ptr, int len) {
  if (file == STDOUT_FILENO || file == STDERR_FILENO)
  {
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
      __io_putchar( *ptr++ );
    }

    return len;
  }

  if (file >= MAX_FILNOS || filnos[file] == 0) {
    errno = EBADF;
    return -1;
  }

  UINT bw;
  FRESULT fr = f_write(&fils[file], (void *) ptr, (UINT) len, &bw);

  if (fr != FR_OK) {
    errno = 0;
    if (fr == FR_DISK_ERR || fr == FR_INT_ERR) errno |= EIO;
    if (fr == FR_INVALID_OBJECT) errno |= EINVAL;
    if (fr == FR_DENIED) errno |= EACCES;
    if (fr == FR_TIMEOUT) errno |= ETIME;

    return -1;
  }

  if (bw < (UINT) len) {
    errno = ENOSPC;
    return -1;
  }

  return bw;
}

int _close(int file) {
  if (file < 3 || file >= MAX_FILNOS || filnos[file] == 0) {
    errno = EBADF;
    return -1;
  }

  FRESULT fr = f_close(&fils[file]);

  if (fr != FR_OK) {
    errno = 0;
    if (fr == FR_DISK_ERR || fr == FR_INT_ERR) errno |= EIO;
    if (fr == FR_INVALID_OBJECT) errno |= EINVAL;
    if (fr == FR_TIMEOUT) errno |= ETIME;

    return -1;
  }

  filnos[file] = 0;

  return file;
}

int _fstat(int file, struct stat* st) {
  if (file < 0 || file >= MAX_FILNOS || filnos[file] == 0) {
    errno = EBADF;
    return -1;
  }

  if (file < 3) {
    st->st_mode = S_IFCHR;
  }
  else {
    st->st_mode = S_IFREG;
  }

  return 0;
}

int _isatty(int file) {
  if (file >= 0 && file < 3) {
    return 1;
  }

  if (file < MAX_FILNOS) {
    return 0;
  }

  errno = EBADF;
  return -1;
}

int _lseek(int file, int ptr, int dir) {
  if (file < 3 || file >= MAX_FILNOS || filnos[file] == 0) {
    errno = EBADF;
    return -1;
  }

  if (dir & 1) {
    ptr += f_tell(&fils[file]);
  }
  else if (dir & 2) {
    ptr += f_size(&fils[file]);
  }

  FRESULT fr = f_lseek(&fils[file], (FSIZE_t) ptr);

  if (fr != FR_OK) {
    errno = 0;
    if (fr == FR_DISK_ERR || fr == FR_INT_ERR) errno |= EIO;
    if (fr == FR_INVALID_OBJECT) errno |= EINVAL;
    if (fr == FR_TIMEOUT) errno |= ETIME;

    return -1;
  }

  FSIZE_t curptr = f_tell(&fils[file]);

  return curptr;
}

int _read(int file, char* ptr, int len) {
  if (file == STDOUT_FILENO || file == STDERR_FILENO)
  {
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
      *ptr++ = __io_getchar();
    }

    return len;
  }

  if (file >= MAX_FILNOS || filnos[file] == 0) {
    errno = EBADF;
    return -1;
  }

  UINT br;
  FRESULT fr = f_read(&fils[file], (void *) ptr, (UINT) len, &br);

  if (fr != FR_OK) {
    errno = 0;
    if (fr == FR_DISK_ERR || fr == FR_INT_ERR) errno |= EIO;
    if (fr == FR_INVALID_OBJECT) errno |= EINVAL;
    if (fr == FR_DENIED) errno |= EACCES;
    if (fr == FR_TIMEOUT) errno |= ETIME;

    return -1;
  }

  if (br < (UINT) len) {
    //eof[file] = 1;
  }

  return br;
}

int _open(const char* name, int flags, ...) {
  int filno = 2;

  while (filnos[++filno] && filno < MAX_FILNOS);
  if (filno == MAX_FILNOS) {
    errno = ENFILE;
    return -1;
  }

  BYTE mode = 0;

  if ((flags & ~O_BINARY) == O_RDONLY) mode |= FA_READ;

  if (flags & O_WRONLY) mode |= FA_WRITE;
  else if (flags & O_RDWR) mode |= FA_READ | FA_WRITE;

  if (flags & O_CREAT) {
    if (flags & O_EXCL) mode |= FA_CREATE_NEW;
    else if (flags & O_TRUNC) mode |= FA_CREATE_ALWAYS;
    else if (flags & O_APPEND) mode |= FA_OPEN_APPEND;
  }
  else {
    mode |= FA_OPEN_EXISTING;
  }

  FRESULT fr = f_open(&fils[filno], name, mode);

  if (fr != FR_OK) {
    errno = 0;
    if (fr == FR_DISK_ERR || fr == FR_INT_ERR) errno |= EIO;
    if (fr == FR_NOT_READY) errno |= ENODEV;
    if (fr == FR_NO_FILE || fr == FR_NO_PATH) errno |= ENOENT;
    if (fr == FR_INVALID_NAME || fr == FR_INVALID_OBJECT) errno |= EINVAL;
    if (fr == FR_DENIED || fr == FR_INVALID_PARAMETER) errno |= EPERM;
    if (fr == FR_EXIST) errno |= EEXIST;
    if (fr == FR_WRITE_PROTECTED) errno |= EROFS;
    if (fr == FR_INVALID_DRIVE) errno |= EPERM;
    if (fr == FR_NOT_ENABLED || fr == FR_NO_FILESYSTEM) errno |= ENXIO;
    if (fr == FR_TIMEOUT) errno |= ETIME;
    if (fr == FR_LOCKED) errno |= EBUSY;
    if (fr == FR_NOT_ENOUGH_CORE) errno |= ENOMEM;
    if (fr == FR_TOO_MANY_OPEN_FILES) errno |= ENFILE;

    return -1;
  }

  filnos[filno] = 1;

  return filno;
}

int _wait(int* status) {
  errno = ECHILD;
  return -1;
}

int _unlink(char* name) {
  errno = ENOENT;
  return -1;
}

int _times(struct tms* buf) {
  return -1;
}

int _stat(char* file, struct stat* st) {
  FILINFO finfo;
  FRESULT fr = f_stat(file, &finfo);

  if (fr != FR_OK) {
    errno = 0;
    if (fr == FR_DISK_ERR || fr == FR_INT_ERR) errno |= EIO;
    if (fr == FR_NOT_READY) errno |= ENODEV;
    if (fr == FR_NO_FILE || fr == FR_NO_PATH) errno |= ENOENT;
    if (fr == FR_INVALID_NAME) errno |= EINVAL;
    if (fr == FR_INVALID_DRIVE) errno |= EPERM;
    if (fr == FR_NOT_ENABLED || fr == FR_NO_FILESYSTEM) errno |= ENXIO;
    if (fr == FR_TIMEOUT) errno |= ETIME;
    if (fr == FR_NOT_ENOUGH_CORE) errno |= ENOMEM;

    return -1;
  }

  st->st_mode = S_IFREG;

  return 0;
}

int _link(char* old, char* new) {
  errno = EMLINK;
  return -1;
}

int _fork(void) {
  errno = EAGAIN;
  return -1;
}

int _execve(char* name, char** argv, char** env) {
  errno = ENOMEM;
  return -1;
}

#pragma GCC diagnostic pop
