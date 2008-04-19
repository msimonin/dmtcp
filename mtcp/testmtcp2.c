//+++2006-01-17
//    Copyright (C) 2006  Mike Rieker, Beverly, MA USA
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//---2006-01-17

/********************************************************************************************************************************/
/*																*/
/*  Simple multi-threaded test program												*/
/*  Checkpoint is written to testmtcp2.mtcp every 10 seconds									*/
/*  It uses the mtcp_wrapper_clone routine to create the threads								*/
/*																*/
/*  Four threads are created													*/
/*  First thread prints 1 11 21 31 41 51 61 ...											*/
/*  Second thread prints 2 12 22 32 42 52 62 ...										*/
/*  Third thread prints 3 13 23 33 43 53 63 ...											*/
/*  Fourth thread prints 4 14 24 34 44 54 64 ...										*/
/*  When checkpoint is restored, the counts should resume where they left off							*/
/*																*/
/********************************************************************************************************************************/

#define u32 unsigned int

#include <asm/unistd.h>
#include <errno.h>

#ifndef __user
// this is needed to compile futex.h on LXR/Suse10.2
#  define __user
#endif

#include <linux/futex.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mtcp.h"
#include "mtcp_futex.h"

#define STACKSIZE 1024*1024
#define THREADFLAGS (CLONE_FS | CLONE_FILES | CLONE_VM)

static int printfutex = 0;

/* Set *loc to newval iff *loc equal to oldval */
/* Return 0 if failed, 1 if succeeded          */

static inline int atomic_setif_int (volatile int *loc, int newval, int oldval)

{
  char rc;

  asm volatile ("lock\n\t"
                "cmpxchgl %2,%3\n\t"
                "sete     %%al"
                : "=a" (rc)
                :  "a" (oldval), "r" (newval), "m" (*loc)
                : "cc", "memory");

  return (rc);
}

static void lockstdout (void)

{
  int rc;

  while (!atomic_setif_int (&printfutex, 1, 0)) {
    rc = mtcp_futex (&printfutex, FUTEX_WAIT, 0, NULL);
    if ((rc < 0) && (rc != -EAGAIN) && (rc != -EWOULDBLOCK) && (rc != -EINTR)) {
      fprintf (stderr, "testmtcp2: FUTEX_WAIT error %d\n", rc);
      abort ();
    }
  }
}

static void unlkstdout (void)

{
  int rc;

  printfutex = 0;
  rc = mtcp_futex (&printfutex, FUTEX_WAKE, 1, NULL);
  if (rc < 0) {
    fprintf (stderr, "testmtcp2: FUTEX_WAKE error %d\n", rc);
    abort ();
  }
}

static int thread1_func (void *dummy);

int main ()

{
  char *thread1_stack;
  int i, thread1_tid;

  mtcp_init ("testmtcp2.mtcp", 10, 0);

  for (i = 0; i < 3; i ++) {
    thread1_stack = malloc (STACKSIZE);
    thread1_tid = clone (thread1_func, thread1_stack + STACKSIZE, THREADFLAGS, (void *)(long)(i + 2));
    if (thread1_tid < 0) {
      fprintf (stderr, "error creating thread1: %s\n", strerror (errno));
      return (-1);
    }
  }

  thread1_func ((void *)1);
  return (0);
}

static int thread1_func (void *dummy)

{
  int count, delay;

  mtcp_ok ();

  count = (long)dummy;

  while (1) {
    for (delay = 100; -- delay >= 0;) usleep (10);
    lockstdout ();
    printf (" %d", count);
    fflush (stdout);
    unlkstdout ();
    count += 10;
  }
}
