#pragma once

#include <string>
#include <iosfwd>

namespace molly
{
  enum class DeviceState
  {
    Unknown = -2,
    Unavailable = -1,
    LidClosed = 21,
    ButtonPressed = 22,
    LidOpen = 23
  };

  std::ostream& operator<<(std::ostream& os, DeviceState const& state);

  class MollyError : public std::exception
  {
  public:
    MollyError(std::string message)
    : _message(message)
    {}

    const char* what() const _GLIBCXX_USE_NOEXCEPT override
    {
      return _message.c_str();
    }

  private:
    std::string _message;
  };

  class Device
  {
  public:
    Device();
    ~Device();

    void open(std::string devicePath);
    void close();
    DeviceState sample();

  private:
    int _fd;
  };
}