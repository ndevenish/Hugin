// taken from licq
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
//#include "time-fix.h"

#ifdef HAVE_PATHS_H
#include <paths.h>
#else
#define _PATH_BSHELL "/bin/sh"
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif

#include "../utils.h"
#include "Process.h"

using namespace std;

//===========================================================================

Process::Process(bool redirect)
  : redirectOutput(redirect)
{
  fStdOut = fStdErr = NULL;
  pid = -1;
}

Process::~Process()
{
  if (running()) close();
}

bool Process::running()
{
  int pstat;
  int r;
  // See if the child is still there
  r = waitpid(pid, &pstat, WNOHANG);
  if (r == pid) {
    // process exited
    return false;
  }

  return true;
}

bool Process::open(const char *cmd)
{
  int pdes_out[2], pdes_err[2];

  if (redirectOutput) {
    if (pipe(pdes_out) < 0) return false;
    if (pipe(pdes_err) < 0) return false;
  }

  switch (pid = fork())
  {
    case -1:                        /* Error. */
    {
      if (redirectOutput) {
        ::close(pdes_out[0]);
        ::close(pdes_out[1]);
        ::close(pdes_err[0]);
        ::close(pdes_err[1]);
      }
      return false;
      /* NOTREACHED */
    }
    case 0:                         /* Child. */
    {
      if (redirectOutput) {
        if (pdes_out[1] != STDOUT_FILENO)
        {
          dup2(pdes_out[1], STDOUT_FILENO);
          ::close(pdes_out[1]);
        }
        ::close(pdes_out[0]);
        if (pdes_err[1] != STDERR_FILENO)
        {
          dup2(pdes_err[1], STDERR_FILENO);
          ::close(pdes_err[1]);
        }
        ::close(pdes_err[0]);
      }
      execl(_PATH_BSHELL, "sh", "-c", cmd, NULL);
      _exit(127);
      /* NOTREACHED */
    }
  }

  if (redirectOutput) {
    /* Parent; assume fdopen can't fail. */
    fStdOut = fdopen(pdes_out[0], "r");
    ::close(pdes_out[1]);
    fStdErr = fdopen(pdes_err[0], "r");
    ::close(pdes_err[1]);

  // Set both streams to line buffered
    setvbuf(fStdOut, (char*)NULL, _IOLBF, 0);
    setvbuf(fStdErr, (char*)NULL, _IOLBF, 0);
  } else {
    fStdOut = 0;
    fStdOut = 0;
  }
  return true;
}

void Process::wait()
{
  int pstat;
  int r;
  // See if the child is still there
  r = waitpid(pid, &pstat, WNOHANG);
  if (r == pid) {
    // process exited (not sure...)
    printf("wait(), waitpid returnd pid\n");
    return;
  } else if (r == -1 && errno == ECHILD) {
    // process exited
    return;
  } else if (r == -1) {
    perror("Process::wait() error during waitpid");
  } else {
    // process is still running, sleep longer
    waitpid(pid,&pstat,0);
  }


}

int Process::close()
{
   int r, pstat;
   struct timeval tv = { 0, 500000 };

   if (redirectOutput) {
     // Close the file descriptors
     fclose(fStdOut);
     fclose(fStdErr);
   }
   fStdOut = fStdErr = NULL;

   // See if the child is still there
   r = waitpid(pid, &pstat, WNOHANG);
   // Return if child has exited or there was an error
   if (r == pid || r == -1) goto pclose_leave;

   // Give the process another .5 seconds to die
   select(0, NULL, NULL, NULL, &tv);

   // Still there?
   r = waitpid(pid, &pstat, WNOHANG);
   if (r == pid || r == -1) goto pclose_leave;

   // Try and kill the process
   if (kill(pid, SIGTERM) == -1) return -1;

   // Give it 1 more second to die
   tv.tv_sec = 1;
   tv.tv_usec = 0;
   select(0, NULL, NULL, NULL, &tv);

   // See if the child is still there
   r = waitpid(pid, &pstat, WNOHANG);
   if (r == pid || r == -1) goto pclose_leave;

   // Kill the bastard
   kill(pid, SIGKILL);
   // Now he will die for sure
   r = waitpid(pid, &pstat, 0);

pclose_leave:

   if (r == -1 || !WIFEXITED(pstat))
     return -1;
   return WEXITSTATUS(pstat);

}



