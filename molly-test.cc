#include "device.hh"

#include <iostream>
#include <unistd.h>

using namespace molly;
using namespace std;

int main()
{
  Device device;
  device.open("/dev/big_red_button");

  DeviceState lastState = DeviceState::Unknown;

  while (true)
  {
    DeviceState state = device.sample();

    switch (state)
    {
      case DeviceState::ButtonPressed:
        if (lastState != DeviceState::ButtonPressed)
          cout << "PRESS!!!" << endl;
        break;
      case DeviceState::LidOpen:
        if (lastState != DeviceState::LidOpen && lastState != DeviceState::ButtonPressed)
          cout << "OPEN..." << endl;
        break;
      case DeviceState::LidClosed:
        if (lastState != DeviceState::LidClosed)
          cout << "Closed from " << lastState << endl;
        break;
      default:
        continue;
    }

    lastState = state;

    usleep(20000);
  }
}