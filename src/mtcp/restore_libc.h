/****************************************************************************
 *   Copyright (C) 2006-2012 by Jason Ansel, Kapil Arya, and Gene Cooperman *
 *   jansel@csail.mit.edu, kapil@ccs.neu.edu, gene@ccs.neu.edu              *
 *                                                                          *
 *   This file is part of the dmtcp/src module of DMTCP (DMTCP:dmtcp/src).  *
 *                                                                          *
 *  DMTCP:dmtcp/src is free software: you can redistribute it and/or        *
 *  modify it under the terms of the GNU Lesser General Public License as   *
 *  published by the Free Software Foundation, either version 3 of the      *
 *  License, or (at your option) any later version.                         *
 *                                                                          *
 *  DMTCP:dmtcp/src is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU Lesser General Public License for more details.                     *
 *                                                                          *
 *  You should have received a copy of the GNU Lesser General Public        *
 *  License along with DMTCP:dmtcp/src.  If not, see                        *
 *  <http://www.gnu.org/licenses/>.                                         *
 ****************************************************************************/

#ifndef TLSINFO_H
#define TLSINFO_H

#include "ldt.h"
#include "protectedfds.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __x86_64__
# define eax rax
# define ebx rbx
# define ecx rcx
# define edx rax
# define ebp rbp
# define esi rsi
# define edi rdi
# define esp rsp
# define CLEAN_FOR_64_BIT(args...) CLEAN_FOR_64_BIT_HELPER(args)
# define CLEAN_FOR_64_BIT_HELPER(args...) #args
#elif __i386__
# define CLEAN_FOR_64_BIT(args...) #args
#else
# define CLEAN_FOR_64_BIT(args...) "CLEAN_FOR_64_BIT_undefined"
#endif

#define PRINTF(fmt, ...) \
  do { \
    /* In some cases, the user stack may be very small (less than 10KB). */ \
    /* We will overrun the buffer with just two extra stack frames. */ \
    char buf[256]; \
    int c = snprintf(buf, sizeof(buf) - 1, "[%d] %s:%d in %s; REASON= " fmt, \
                 getpid(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    if (c == sizeof(buf) - 1) buf[c] = '\n'; \
    /* assign to rc in order to avoid 'unused result' compiler warnings */ \
    ssize_t rc = write(PROTECTED_STDERR_FD, buf, c + 1); \
  } while (0);

#ifdef DEBUG
# define DPRINTF PRINTF
#else
# define DPRINTF(args...) // debug printing
#endif

#define ASSERT(condition) \
  do { \
    if (! (condition)) { \
      PRINTF("Assertion failed: %s\n", #condition); \
      _exit(0); \
    } \
  } while (0);

#define ASSERT_NOT_REACHED() \
  do { \
    PRINTF("NOT_REACHED Assertion failed.\n"); \
    _exit(0); \
  } while (0);


#ifdef __i386__
typedef unsigned short segreg_t;
#elif __x86_64__
typedef unsigned int segreg_t;
#elif __arm__
typedef unsigned int segreg_t;
#endif

/* TLS segment registers used differently in i386 and x86_64. - Gene */
#ifdef __i386__
# define TLSSEGREG gs
#elif __x86_64__
# define TLSSEGREG fs
#elif __arm__
/* FIXME: fs IS NOT AN arm REGISTER.  BUT THIS IS USED ONLY AS A FIELD NAME.
 *   ARM uses a register in coprocessor 15 as the thread-pointer (TLS Register)
 */
# define TLSSEGREG fs
#endif

typedef struct _ThreadTLSInfo {
  segreg_t fs, gs;  // thread local storage pointers
  struct user_desc gdtentrytls[1];
} ThreadTLSInfo;

void TLSInfo_PostRestart();
void TLSInfo_VerifyPidTid(pid_t pid, pid_t tid);
void TLSInfo_UpdatePid();
void TLSInfo_SaveTLSState (ThreadTLSInfo *tlsInfo);
void TLSInfo_RestoreTLSState(ThreadTLSInfo *tlsInfo);
void TLSInfo_SetThreadSysinfo(void *sysinfo);
void *TLSInfo_GetThreadSysinfo();
int  TLSInfo_HaveThreadSysinfoOffset();

void Thread_RestoreAllThreads(void);

#ifdef __cplusplus
}
#endif

#endif
