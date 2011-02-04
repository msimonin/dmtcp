/****************************************************************************
 *   Copyright (C) 2006-2010 by Jason Ansel, Kapil Arya, and Gene Cooperman *
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

#include "constants.h"
#include "../jalib/jassert.h"
#include  "../jalib/jfilesystem.h"
#include "ptracewrapper.h"
#include "virtualpidtable.h"
#include "syscallwrappers.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <stdarg.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <list>

#ifdef PTRACE
/* ptrace cannot work without pid virtualization.  If we're not using
 * pid virtualization, then disable this wrapper around ptrace, and
 * let the application call ptrace from libc. */
#ifndef PID_VIRTUALIZATION
#error "PTRACE can not be used without enabling PID-Virtualization"
#endif

static pthread_mutex_t ptrace_info_list_mutex = PTHREAD_MUTEX_INITIALIZER;

dmtcp::list<struct ptrace_info> ptrace_info_list;

#define GETTID() (int)syscall(SYS_gettid)

extern "C" struct ptrace_info get_next_ptrace_info(int index) {
  if (index >= ptrace_info_list.size()) return EMPTY_PTRACE_INFO;

  dmtcp::list<struct ptrace_info>::iterator it;
  int local_index = 0;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (local_index == index) return *it;
    local_index++;
  }
  return EMPTY_PTRACE_INFO;
}

int open_ptrace_related_file (int file_option) {
  char file[256];
  memset(file, 0, 256);
  dmtcp::string tmpdir = dmtcp::UniquePid::getTmpDir();

  switch (file_option) {
    case PTRACE_SHARED_FILE_OPTION:
      sprintf(file, "%s/ptrace_shared.txt", tmpdir.c_str());
      break;
    case PTRACE_SETOPTIONS_FILE_OPTION:
      sprintf(file, "%s/ptrace_setoptions.txt", tmpdir.c_str());
      break;
    case PTRACE_CHECKPOINT_THREADS_FILE_OPTION:
      sprintf(file, "%s/ptrace_ckpthreads.txt", tmpdir.c_str());
      break;
    default:
      printf("open_ptrace_related_file: unknown file_option, %d\n",
              file_option);
      return -1;
  }
  return open(file, O_CREAT|O_APPEND|O_WRONLY|O_FSYNC, 0644); 
}

void write_ptrace_pair_to_given_file (int file, pid_t superior, pid_t inferior)
{
  int fd;
  struct flock lock;

  if ((fd = open_ptrace_related_file(file)) == -1) {
    printf("write_ptrace_pair_to_given_file: Error opening file\n: %s\n",
            strerror(errno));
    abort();
  }

  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_CUR;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = getpid();

  if (fcntl(fd, F_GETLK, &lock ) == -1) {
    printf("write_ptrace_pair_to_given_file: Error acquiring lock: %s\n",
            strerror(errno));
    abort();
  }

  if (write(fd, &superior, sizeof(pid_t)) == -1) {
    printf("write_ptrace_pair_to_given_file: Error writing to file: %s\n",
            strerror(errno));
    abort();
  }
  if (write(fd, &inferior, sizeof(pid_t)) == -1) {
    printf("write_ptrace_pair_to_given_file: Error writing to file: %s\n",
            strerror(errno));
    abort();
  }

  lock.l_type = F_UNLCK;
  lock.l_whence = SEEK_CUR;
  lock.l_start = 0;
  lock.l_len = 0;

  if (fcntl(fd, F_SETLK, &lock) == -1) {
    printf("write_ptrace_pair_to_given_file: Error releasing lock: %s\n",
            strerror(errno));
    abort();
  }
  if (close(fd) != 0) {
    printf("write_ptrace_pair_to_given_file: Error closing file: %s\n",
            strerror(errno));
    abort();
  }
}

void ptrace_info_list_update_inferior_st (pid_t superior, pid_t inferior,
                                          char inferior_st) {
  dmtcp::list<struct ptrace_info>::iterator it;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (it->superior == superior && it->inferior == inferior) {
      it->inferior_st = inferior_st;
      break;
    }
  }
}

static ptrace_info ptrace_info_list_has_pair (pid_t superior, pid_t inferior) {
  dmtcp::list<struct ptrace_info>::iterator it;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (it->superior == superior && it->inferior == inferior)
      return *it;
  }
  return EMPTY_PTRACE_INFO;
}

void ptrace_info_list_remove_pair (pid_t superior, pid_t inferior) {
  struct ptrace_info pt_info = ptrace_info_list_has_pair(superior, inferior);
  if (pt_info == EMPTY_PTRACE_INFO) return;
  pthread_mutex_lock(&ptrace_info_list_mutex);
  ptrace_info_list.remove(pt_info);
  pthread_mutex_unlock(&ptrace_info_list_mutex);
}

void ptrace_info_update_last_command (pid_t superior, pid_t inferior,
  int last_command) {
  dmtcp::list<struct ptrace_info>::iterator it;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (it->superior == superior && it->inferior == inferior) {
      it->last_command = last_command;
      if (last_command == PTRACE_SINGLESTEP_COMMAND)
        it->singlestep_waited_on = FALSE;
      break;
    }
  }
}

extern "C" long ptrace (enum __ptrace_request request, ...)
{
  va_list ap;
  pid_t pid;
  void *addr;
  void *data;

  pid_t superior;
  pid_t inferior;

  long ptrace_ret;

  va_start(ap, request);
  pid = va_arg(ap, pid_t);
  addr = va_arg(ap, void *);
  data = va_arg(ap, void *);
  va_end(ap);

  superior = syscall(SYS_gettid);
  inferior = pid;
  struct ptrace_waitpid_info pwi = mtcp_get_ptrace_waitpid_info();

  switch (request) {
    case PTRACE_ATTACH: {
      if (!pwi.is_ptrace_local) {
        struct cmd_info cmd = {PTRACE_INFO_LIST_INSERT, superior, inferior,
                               PTRACE_UNSPECIFIED_COMMAND, FALSE, 'u',
                               PTRACE_SHARED_FILE_OPTION};
        ptrace_info_list_command(cmd);
      }
      break;
    }
    case PTRACE_TRACEME: {
      superior = getppid();
      inferior = syscall(SYS_gettid);
      struct cmd_info cmd = {PTRACE_INFO_LIST_INSERT, superior, inferior,
                             PTRACE_UNSPECIFIED_COMMAND, FALSE, 'u',
                             PTRACE_SHARED_FILE_OPTION};
      ptrace_info_list_command(cmd);
      break;
    }
    case PTRACE_DETACH: {
     if (!pwi.is_ptrace_local)
       ptrace_info_list_remove_pair(superior, inferior);
     break;
    }
    case PTRACE_CONT: {
     if (!pwi.is_ptrace_local) {
       ptrace_info_update_last_command(superior, inferior,
                                       PTRACE_CONTINUE_COMMAND);
       /* The ptrace_info pair was already recorded. The superior is just
        * issuing commands. */
       struct cmd_info cmd = {PTRACE_INFO_LIST_INSERT, superior, inferior,
                              PTRACE_CONTINUE_COMMAND, FALSE, 'u',
                              PTRACE_NO_FILE_OPTION};
       ptrace_info_list_command(cmd);
     }
     break;
    }
    case PTRACE_SINGLESTEP: {
     pid = dmtcp::VirtualPidTable::instance().originalToCurrentPid(pid);
     if (!pwi.is_ptrace_local) {
       if (_real_pthread_sigmask (SIG_BLOCK, &signals_set, NULL) != 0) {
         perror ("waitpid wrapper");
         exit(-1);
       }
       ptrace_info_update_last_command(superior, inferior,
                                       PTRACE_SINGLESTEP_COMMAND);
       /* The ptrace_info pair was already recorded. The superior is just
        * issuing commands. */
       struct cmd_info cmd = {PTRACE_INFO_LIST_INSERT, superior, inferior,
                              PTRACE_SINGLESTEP_COMMAND, FALSE, 'u',
                              PTRACE_NO_FILE_OPTION};
       ptrace_info_list_command(cmd);
       ptrace_ret =  _real_ptrace (request, pid, addr, data);
       if (_real_pthread_sigmask (SIG_UNBLOCK, &signals_set, NULL) != 0) {
         perror ("waitpid wrapper");
         exit(-1);
       }
     }
     else ptrace_ret = _real_ptrace(request, pid, addr, data);
     break;
    }
    case PTRACE_SETOPTIONS: {
      write_ptrace_pair_to_given_file(PTRACE_SETOPTIONS_FILE_OPTION,
                                      superior, inferior);
      break;
    }
    default: {
      break;
    }
  }

  /* TODO: We might want to check the return value in certain cases */

  if (request != PTRACE_SINGLESTEP) {
    pid = dmtcp::VirtualPidTable::instance().originalToCurrentPid(pid);
    ptrace_ret =  _real_ptrace(request, pid, addr, data);
  }

  return ptrace_ret;
}

void ptrace_info_list_update_is_inferior_ckpthread(pid_t pid, pid_t tid) {
  dmtcp::list<struct ptrace_info>::iterator it;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (tid == it->inferior) {
      it->inferior_is_ckpthread = 1;
      break;
    }
  }
}

bool ptrace_info_compare (ptrace_info left, ptrace_info right) {
  if (left.superior < right.superior) return true;
  else if (left.superior == right.superior) return
    left.inferior < right.inferior;
  return false;
}

/* This function does three things:
 * 1) Moves all ckpt threads to the end of ptrace_info_list.
 * 2) Sorts UTs by superior and then by inferior, if there's a tie on superior.
 * 3) Sorts CTs by superior and then by inferior, if there's a tie on superior.
 * It's important to have the checkpoint threads unattached for as long as
 * possible. */
void ptrace_info_list_sort () {
  dmtcp::list<struct ptrace_info> tmp_ckpths_list;
  dmtcp::list<struct ptrace_info>::iterator it;

  /* Temporarily remove checkpoint threads from ptrace_info_list. */
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (it->inferior_is_ckpthread) {
      tmp_ckpths_list.push_back(*it);
      ptrace_info_list.remove(*it);
      it--;
    }
  }

  /* Sort the two lists: first by superior and if there's a tie on superior,
   * then sort by inferior. */
  ptrace_info_list.sort(ptrace_info_compare);
  tmp_ckpths_list.sort(ptrace_info_compare);

  /* Add the temporary list of ckpt threads at the end of ptrace_info_list. */
  for (it = tmp_ckpths_list.begin(); it != tmp_ckpths_list.end(); it++) { 
    ptrace_info_list.push_back(*it);
  }
}

void ptrace_info_list_remove_pairs_with_dead_tids () {
  dmtcp::list<ptrace_info>::iterator it;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (!procfs_state(it->inferior)) {
      ptrace_info_list.remove(*it);
      it--;
    }
  }
}

void ptrace_info_list_save_threads_state () {
  dmtcp::list<struct ptrace_info>::iterator it;
  for(it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
      it->inferior_st = procfs_state(it->inferior);
  }
}

void ptrace_info_list_print () {
  dmtcp::list<struct ptrace_info>::iterator it;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    fprintf(stdout, "GETTID = %d superior = %d inferior = %d state =  %c "
            "inferior_is_ckpthread = %d\n",
            GETTID(), it->superior, it->inferior, it->inferior_st,
            it->inferior_is_ckpthread);
  }
}

void ptrace_info_list_insert (pid_t superior, pid_t inferior, int last_command,
                              int singlestep_waited_on, char inferior_st,
                              int file_option) {
  if (file_option != PTRACE_NO_FILE_OPTION) {
    write_ptrace_pair_to_given_file(file_option, superior, inferior);
    /* In this case, superior is the pid and inferior is the tid and also the
     * checkpoint thread. We're recording that for process pid, tid is the
     * checkpoint thread. */ 
    if (file_option == PTRACE_CHECKPOINT_THREADS_FILE_OPTION) return;
  }

  if (ptrace_info_list_has_pair(superior, inferior) != EMPTY_PTRACE_INFO) {
    ptrace_info_list_update_inferior_st(superior, inferior, inferior_st);
    return;
  }

  struct ptrace_info new_ptrace_info;
  new_ptrace_info.superior = superior;
  new_ptrace_info.inferior = inferior;
  new_ptrace_info.last_command = last_command;
  new_ptrace_info.singlestep_waited_on = singlestep_waited_on;
  new_ptrace_info.inferior_st = inferior_st;
  new_ptrace_info.inferior_is_ckpthread = 0;

  pthread_mutex_lock(&ptrace_info_list_mutex);
  ptrace_info_list.push_back(new_ptrace_info);
  pthread_mutex_unlock(&ptrace_info_list_mutex);
}

extern "C" void ptrace_info_list_update_info(pid_t superior, pid_t inferior,
                                  int singlestep_waited_on) {
  dmtcp::list<struct ptrace_info>::iterator it;
  for (it = ptrace_info_list.begin(); it != ptrace_info_list.end(); it++) {
    if (it->superior == superior && it->inferior == inferior) {
      if (it->last_command == PTRACE_SINGLESTEP_COMMAND)
        it->singlestep_waited_on = singlestep_waited_on;
      it->last_command = PTRACE_UNSPECIFIED_COMMAND;
      break;
    }
  }
}

extern "C" void ptrace_info_list_command(struct cmd_info cmd) {
  switch (cmd.option) {
    case PTRACE_INFO_LIST_UPDATE_IS_INFERIOR_CKPTHREAD:
      ptrace_info_list_update_is_inferior_ckpthread(cmd.superior, cmd.inferior);
      break;
    case PTRACE_INFO_LIST_SORT:
      ptrace_info_list_sort();
      break;
    case PTRACE_INFO_LIST_REMOVE_PAIRS_WITH_DEAD_TIDS:
      ptrace_info_list_remove_pairs_with_dead_tids();
      break;
    case PTRACE_INFO_LIST_SAVE_THREADS_STATE:
      ptrace_info_list_save_threads_state();
      break;
    case PTRACE_INFO_LIST_PRINT:
      ptrace_info_list_print();
      break;
    case PTRACE_INFO_LIST_INSERT:
      ptrace_info_list_insert(cmd.superior, cmd.inferior, cmd.last_command, 
                              cmd.singlestep_waited_on, cmd.inferior_st,
                              cmd.file_option);
      break;
    default:
      printf ("ptrace_info_list_command: unknown option %d\n", cmd.option);
  }
}

char procfs_state(int tid) {
  char name[64];
  sprintf(name, "/proc/%d/stat", tid);
  int fd = open(name, O_RDONLY, 0);
  if (fd < 0) {
    //JNOTE("procfs_state: cannot open")(name);
    return 0;
  }

  /* The format of /proc/pid/stat is: tid (name_of_executable) state.
   * We need to retrieve the state of tid. 255 is enough in this case. */
  char sbuf[256];
  size_t num_read = 0;
  ssize_t rc;
  /* Read at most 255 characters or until the end of the file. */
  while (num_read != 255) {
    rc = read(fd, sbuf + num_read, 255 - num_read);
    if (rc < 0) break;
    else if (rc == 0) break;
    num_read += rc;
  }
  if (close(fd) != 0) {
    JWARNING("procfs_state: error closing file")(strerror(errno));
    JASSERT(0);
  }
  if (num_read <= 0) return 0;

  sbuf[num_read] = '\0';
  char *state, *tmp;
  state = strchr(sbuf, '(') + 1;
  if (!state) return 'u';
  tmp = strrchr(state, ')');
  if (!tmp || (tmp + 2 - sbuf) > 255) return 'u';
  state = tmp + 2;

  return state[0];
}

#endif