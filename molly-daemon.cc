#include "device.hh"

#include <iostream>
#include <unistd.h>
#include <syslog.h>
#include <csignal>
#include <sys/stat.h>
#include <sstream>
#include <string.h>

using namespace molly;
using namespace std;

// TODO deal with device being unplugged and replugged
// TODO configuration from file or cmdline args
// TODO use exec instead of system?
// TODO system returning non-zero for some reason

void runCommand(std::string command)
{
  pid_t pid = fork();

  if (pid < 0)
  {
    syslog(LOG_ERR, "Error forking to invoke command: %s", command.c_str());
  }
  else if (pid == 0)
  {
    syslog(LOG_INFO, "Invoking command: %s", command.c_str());
    if (system(command.c_str()) != 0)
      syslog(LOG_ERR, "Error making system call with command: %s", command.c_str());
    exit(0);
  }
}

bool shutdown = false;

int main()
{
  if (setlogmask(LOG_UPTO(LOG_INFO)) < 0)
  {
    cerr << "Error setting log mask" << endl;
    exit(1);
  }

  openlog("mollyd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
  syslog(LOG_INFO, "Starting mollyd");

  pid_t pid = fork();

  if (pid < 0)
  {
    syslog(LOG_ERR, "Unable for fork child process");
    cerr << "Unable for fork child process" << endl;
    exit(1);
  }

  if (pid > 0)
  {
    // We successfully created the child process and received its PID.
    // The child receives a PID of zero and falls through.
    // As we're the parent, we should now exit.
    stringstream msg;
    msg << "mollyd started with PID " << pid;
    syslog(LOG_ERR, "%s", msg.str().c_str());
    cout << msg.str() << endl;
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

  // Close standard in/out/err file descriptors for this process.
  // Reopen them to /dev/null to avoid errors if code attempts to use them.
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  stdin = fopen("/dev/null", "r");
  stdout = fopen("/dev/null", "w+");
  stderr = fopen("/dev/null", "w+");

  DeviceState lastState = DeviceState::Unknown;
  Device device;

  while (!shutdown)
  {
    if (!device.isOpen())
    {
      try
      {
        syslog(LOG_INFO, "Opening device");
        device.open("/dev/big_red_button");
        syslog(LOG_INFO, "Device opened");
      }
      catch (MollyError err)
      {
        syslog(LOG_ERR, "Error trying to open device: %s", err.what());
        usleep(100 * 1000);
        continue;
      }
    }

    DeviceState state;
    try
    {
      state = device.sample();
    }
    catch (MollyError err)
    {
      syslog(LOG_ERR, "Error reading from device: %s", err.what());

      try
      {
        device.close();
        syslog(LOG_INFO, "Device closed");
      }
      catch (MollyError err)
      {
        syslog(LOG_ERR, "Error closing device: %s", err.what());
      }

      continue;
    }

    switch (state)
    {
      case DeviceState::ButtonPressed:
        if (lastState != DeviceState::ButtonPressed)
        {
          syslog(LOG_INFO, "STATE: Pressed");
          runCommand("espeak pressed");
        }
        break;
      case DeviceState::LidOpen:
        if (lastState != DeviceState::LidOpen && lastState != DeviceState::ButtonPressed)
        {
          syslog(LOG_INFO, "STATE: Open");
          runCommand("espeak open");
        }
        break;
      case DeviceState::LidClosed:
        if (lastState != DeviceState::LidClosed)
        {
          syslog(LOG_INFO, "STATE: Closed");
          runCommand("espeak closed");
        }
        break;
      default:
        continue;
    }

    lastState = state;

    usleep(20 * 000);
  }
}
