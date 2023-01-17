#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

void syscall_init (void);

void valid_address (void *);
void valid_string (void *);

void halt(void);
void exit(int);
tid_t exec(const char *);
int wait(tid_t);
int read(int, void *, unsigned);
int write(int, const void*, unsigned);

int fibonacci(int);
int max_of_four_int(int, int, int, int);

/******proj2******/
bool create(const char*, unsigned);
bool remove(const char*);
int open(const char*);
void close(int);
int filesize(int);
void seek(int, unsigned);
unsigned tell(int);

#endif /* userprog/syscall.h */
