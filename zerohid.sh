#!/bin/bash
die() { echo "$*" >&2; exit 1; }

# Run by systemd via zerohid.service to initialize USB HID and start the
# zerohid listener. hid.sh and the zerohid binary must be executable and in the
# same directory as this script.

# Config options:

  # If "ascii", convert plain ASCII characters to HID scan codes.
  # If "xkb", convert X key codes to HID scan codes (for use with vnccon).
  mode=ascii

  # If "yes", write debug info to the serial port.
  debug=yes

  # If "yes", also support 3-button mouse in xkb mode
  mouse=no

# Devices of interest
serial=/dev/ttyS0
hidk=/dev/hidg0
hidm=/dev/hidg1

[[ -e $serial ]] || die "No device $serial"
stty 115200 cs8 -cstopb -parenb -ixon < $serial

cmd=${0%/*}/hid.sh
[[ $mouse == yes ]] && cmd+=" -m"
echo "Running '$cmd'"
eval $cmd || die "HID initialization failed"
[[ -e $hidk ]] || die "No device $hidk"
[[ $mouse != yes ]] || [[ -e $hidm ]] || die "No device $hidm"

cmd="${0%/*}/zerohid"
[[ $mode == ascii ]] && cmd+=" -a"
[[ $mode == xkb ]] && cmd+=" -x"
[[ $debug == yes ]] && cmd+=" -d"
cmd+=" $hidk"
[[ $mouse == yes ]] && cmd+=" $hidm"
cmd+=" <$serial"
[[ $debug == yes ]] && cmd+=" >$serial"
echo "Running '$cmd'"
eval exec $cmd

die "Exec failed"
