#include "device.hh"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

using namespace molly;
using namespace std;

#define INVALID_FD (-1)

Device::Device()
: _fd(INVALID_FD)
{}

Device::~Device()
{
  if (_fd != INVALID_FD)
  {
    int res = ::close(_fd);
    if (res != 0)
    {
      // Failed to close the file descriptor, however we're in a destructor so cannot safely throw
      // nor do we know where to log this to. So we just drop it on the floor.
    }
  }
}

void Device::open(string devicePath)
{
  if (_fd != INVALID_FD)
    throw MollyError("Device already open");

  struct stat buffer;
  if (stat(devicePath.c_str(), &buffer) == -1)
  {
    stringstream msg;
    msg << "Device does not exist: " << strerror(errno);
    throw MollyError(msg.str());
  }

  int fd = ::open(devicePath.c_str(), O_RDWR|O_NONBLOCK);

  if (fd < 0)
  {
    _fd = INVALID_FD;
    stringstream msg;
    msg << "Error opening device: " << strerror(errno);
    throw MollyError(msg.str());
  }

  _fd = fd;
}

void Device::close()
{
  if (_fd == INVALID_FD)
    throw MollyError("Device not open");

  int res = ::close(_fd);

  if (res != 0)
    throw MollyError("Error closing device");
}

DeviceState Device::sample()
{
  if (_fd == INVALID_FD)
    throw MollyError("Device not yet open");

  char buf[8] = {0x08, 0, 0, 0, 0, 0, 0, 0x02 };

  ssize_t res = ::write(_fd, buf, 8);

  if (res < 0)
  {
    stringstream msg;
    msg << "Error writing to device: " << strerror(errno);
    throw MollyError(msg.str());
  }

  unsigned char code;
  res = read(_fd, &code, 1);

  if (res != 1)
  {
    if (errno == EAGAIN)
      return DeviceState::Unavailable;

    stringstream msg;
    msg << "Error reading from device: " << strerror(errno);
    throw MollyError(msg.str());
  }

  switch (code)
  {
    case (unsigned char)DeviceState::ButtonPressed:
      return DeviceState::ButtonPressed;
    case (unsigned char)DeviceState::LidClosed:
      return DeviceState::LidClosed;
    case (unsigned char)DeviceState::LidOpen:
      return DeviceState::LidOpen;
    default:
      stringstream msg;
      msg << "Unexpected response: " << (int)code;
      throw MollyError(msg.str());
  }
}


bool Device::isOpen() const
{
  // TODO do we have to check whether "errno == EBADF" here if fcntl returns -1?
  return _fd != INVALID_FD && fcntl(_fd, F_GETFD) != -1;
}

std::ostream& molly::operator<<(std::ostream& os, DeviceState const& state)
{
  switch (state)
  {
    case DeviceState::Unknown:       os << "Unknown";       break;
    case DeviceState::Unavailable:   os << "Unavailable";   break;
    case DeviceState::LidClosed:     os << "LidClosed";     break;
    case DeviceState::ButtonPressed: os << "ButtonPressed"; break;
    case DeviceState::LidOpen:       os << "LidOpen";       break;
  }
  return os;
}