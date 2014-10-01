#include "device.hh"

#include <iostream>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

using namespace molly;
using namespace std;

int main()
{
  if (setlogmask(LOG_UPTO(LOG_INFO)) < 0)
  {
    cerr << "Error setting log mask" << endl;
    exit(1);
  }

  openlog("mollyd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

  syslog(LOG_INFO, "Starting mollyd");

  Device device;
  device.open("/dev/big_red_button");

  pid_t pid = fork();

  if (pid < 0)
  {
    // TODO syslog this error too
    cerr << "Unable for fork child process" << endl;
    exit(1);
  }

  if (pid > 0)
  {
    // We successfully created the child process and received it's PID
    // The child receives a PID of zero and falls through
    // As we're the parent, we should now exit
    cout << "mollyd started with PID " << pid << endl;
    exit(EXIT_SUCCESS);
  }

  // Make this child process the session leader
  if (setsid() < 0)
  {
    syslog(LOG_ERR, "Unable to set session ID for process");
    exit(EXIT_FAILURE);
  }

  // Ignore SIGCHLD and SIGHUP signals
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGKILL, [](int signo)
  {
      syslog(LOG_INFO, "Received %s -- shutting down", strsignal(signo));
      shutdown = true;
  });

  // Change the file mask
  umask(0);

  // Set the process's working directory to something that is guaranteed to exist
  if (chdir("/") < 0)
  {
    syslog(LOG_ERR, "Unable to set working directory.");
    exit(1);
  }

  // Close standard in/out/err file descriptors for this process
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  DeviceState lastState = DeviceState::Unknown;

  while (true)
  {
    DeviceState state = device.sample();

    switch (state)
    {
      case DeviceState::ButtonPressed:
        if (lastState != DeviceState::ButtonPressed)
          syslog(LOG_INFO, "PRESS!!!");
        break;
      case DeviceState::LidOpen:
        if (lastState != DeviceState::LidOpen && lastState != DeviceState::ButtonPressed)
          syslog(LOG_INFO, "OPEN...");
        break;
      case DeviceState::LidClosed:
        if (lastState != DeviceState::LidClosed)
          syslog(LOG_INFO, "Closed");
        break;
      default:
        continue;
    }

    lastState = state;

    usleep(20000);
  }
}