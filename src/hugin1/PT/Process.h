// taken from licq
#ifndef UTILITY_H
#define UTILITY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>


class Process
{
public:

  /** runs a program in a new process.
   *
   *  @param redirect make output available through pipes on stdOut(),
   *                  stdErr(), if false, do not create these pipes.
   */
  Process(bool redirect = true);
  ~Process();

  /** execute command
   *
   *  @param cmd to execute
   */
  bool open(const char *cmd);
  int close();

  bool running();

  /** wait until the process has finished */
  void wait();

  FILE *stdOut()  { return fStdOut; }
  FILE *stdErr()  { return fStdErr; }
protected:
  bool redirectOutput;
  int pid;
  FILE *fStdOut;
  FILE *fStdErr;
};

#endif
