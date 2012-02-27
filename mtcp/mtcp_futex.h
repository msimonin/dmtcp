#ifndef _MTCP_FUTEX_H
#define _MTCP_FUTEX_H

#include <linux/futex.h>
#include <sys/time.h>
#include "mtcp_sys.h"  /* for mtcp_sys_kernel.h */

/* FIXME:  The call to mtcp_sys_kernel_futex() should replace
	 the inline assembly;  See comment near bottom. */
/* Glibc does not provide a futex routine, so provide one here... */

static inline int mtcp_futex (int *uaddr, int op, int val,
                              const struct timespec *timeout)
{
#if defined(__x86_64__)
  int rc;

  register long int a1 asm ("rdi") = (long int)uaddr;
  register long int a2 asm ("rsi") = (long int)op;
  register long int a3 asm ("rdx") = (long int)val;
  register long int a4 asm ("r10") = (long int)timeout;
  register long int a5 asm ("r8")  = (long int)0;

  asm volatile ("syscall"
                : "=a" (rc)
                : "0" (__NR_futex), 
                  "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5)
                : "memory", "cc", "r11", "cx");
  return (rc);
#elif defined(__i386__)
  int rc;

  asm volatile ("int $0x80"
                : "=a" (rc)
                : "0" (__NR_futex), 
                  "b" (uaddr), "c" (op), "d" (val), "S" (timeout), "D" (0)
                : "memory", "cc");
  return (rc);
#elif defined(__arm__)
  int rc;

  register int r0 asm("r0") = (int)uaddr;
  register int r1 asm("r1") = (int)op;
  register int r2 asm("r2") = (int)val;
  register int r3 asm("r3") = (unsigned int)timeout;
  // Use of "r7" requires gcc -fomit-frame-pointer
  // Maybe could restrict to non-thumb mode here, and gcc might not complain.
  asm volatile ("mov r7, %1\n\t swi 0x0"
                : "=r" (rc)
		: "I" __NR_futex,
                  "r" (r0), "r" (r1), "r" (r2), "r" (r3)
                : "memory", "cc", "r7");
  return (rc);
#else
/* BUG on ARM (& others?):  returns -1 for error, while assembly above calls
 *   return -errno (negative of errno).  mtcp_state.c is confused by this.
 * Probably, we should test if returning -1, and return -mtcp_sys_errno below.
 */

/* mtcp_internal.h defines the macros associated with futex(). */
  int uaddr2=0, val3=0; /* These last two args of futex not used by MTCP. */
  return mtcp_sys_kernel_futex(uaddr, op, val, timeout, &uaddr2, val3);
#endif
}

#endif
