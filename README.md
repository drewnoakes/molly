# molly

Linux tools for the "Big Red Button" USB device manufactured by Dream Cheeky.

Allows running arbitrary shell scripts in response to open, close and button press events.

## Build

The project uses CMake.

    cmake .
    make

## Configure

Set up a `udev` rule to mount the USB device under a known path and with read/write permissions for all users.

    $ sudo vim /etc/udev/rules.d/99-big-red-button.rules

Add the following text:

    ACTION=="add", ENV{ID_MODEL}=="DL100B_Dream_Cheeky_Generic_Controller", SYMLINK+="big_red_button", MODE="0666"

This rule causes a device having `ID_MODEL` of `DL100B_Dream_Cheeky_Generic_Controller` to be mounted at `/dev/big_red_button` with permissions `666` (read/write for owner,group,other).

Reload the udev rules:

    $ sudo udevadm control --reload-rules

## Test

Plug in the button (if already plugged in, unplug then replug it) and check that `/dev/big_red_button` exists.

Test your button is working via the `molly-test` command. It will log all state transitions.

    $ ./molly-test
    Closed from Unknown
    OPEN...
    PRESS!!!
    Closed from LidOpen

## Daemon

The project includes `mollyd`, a Linux daemon process that runs silently in the background.

> Currently this daemon's behaviour is hard coded. Future changes will make this configurable, and hence more broadly useful.

## TODO

Currently the basic functionality is there but some settings are hard coded. Some work is needed to make this utility broadly useful.

It would also be good to have this daemon start and stop in response to the USB device being added and removed, avoiding the need to poll.
