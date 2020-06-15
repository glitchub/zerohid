#!/bin/bash -eu

die() { echo "$*" >&2; exit 1; }

usage() { die "\
Usage:

    hid.sh [options]

Install USB OTG HID keyboard support on Pi Zero, where:

    -m   - also install 3-button mouse support
    -n   - don't re-install if support already exists
    -u   - uninstall existing support

The keyboard device will be at /dev/hidg0, mouse at /dev/hidg1.

The host view of hid devices is at /sys/kernel/debug/hid/x:x:x:x/rdesc."
}

# Also see https://www.kernel.org/doc/Documentation/usb/gadget_configfs.txt

domouse=0

install=1 # 0=uninstall, 1=install, 2=conditional install

while getopts ":mnu" c; do case $c in
    m) domouse=1;;
    n) install=2;;
    u) install=0;;
    *) usage;;
esac; done


# Keyboard report descriptor, a report is 8 bytes:
#   Mod Zero K1 K2 K3 K4 K5 K6
# Where mod is 8 modifier key bits and Kx is a pressed HID key code (up to 6 at
# once)
keyboard=(
    05 01        # USAGE_PAGE (Generic Desktop)
    09 06        # USAGE (Keyboard)
    A1 01        # COLLECTION (Application)
    05 07        #     USAGE_PAGE (Keyboard)
    19 E0        #     USAGE_MINIMUM (Keyboard LeftControl)
    29 E7        #     USAGE_MAXIMUM (Keyboard Right GUI)
    15 00        #     LOGICAL_MINIMUM (0)
    25 01        #     LOGICAL_MAXIMUM (1)
    75 01        #     REPORT_SIZE (1)
    95 08        #     REPORT_COUNT (8)
    81 02        #     INPUT (Data,Var,Abs)
    95 01        #     REPORT_COUNT (1)
    75 08        #     REPORT_SIZE (8)
    81 03        #     INPUT (Cnst,Var,Abs)
#   95 05        #     REPORT_COUNT (5)
#   75 01        #     REPORT_SIZE (1)
#   05 08        #     USAGE_PAGE (LEDs)
#   19 01        #     USAGE_MINIMUM (Num Lock)
#   29 05        #     USAGE_MAXIMUM (Kana)
#   91 02        #     OUTPUT (Data,Var,Abs)
#   95 01        #     REPORT_COUNT (1)
#   75 03        #     REPORT_SIZE (3)
#   91 03        #     OUTPUT (Cnst,Var,Abs)
    95 06        #     REPORT_COUNT (6)
    75 08        #     REPORT_SIZE (8)
    15 00        #     LOGICAL_MINIMUM (0)
    25 65        #     LOGICAL_MAXIMUM (101)
    05 07        #     USAGE_PAGE (Keyboard)
    19 00        #     USAGE_MINIMUM (Reserved (no event indicated))
    29 65        #     USAGE_MAXIMUM (Keyboard Application)
    81 00        #     INPUT (Data,Ary,Abs)
    c0           # END_COLLECTION
)

# Mouse report descriptor
mouse=(
    05 01        # USAGE_PAGE (Generic Desktop)
    09 02        # USAGE (Mouse)
    a1 01        #   COLLECTION (Application)
    09 01        #   USAGE (Pointer)
    a1 00        #     COLLECTION (Physical)
    05 09        #     USAGE_PAGE (Button)
    19 01        #     USAGE_MINIMUM (Button 1)
    29 03        #     USAGE_MAXIMUM (Button 3)
    15 00        #     LOGICAL_MINIMUM (0)
    25 01        #     LOGICAL_MAXIMUM (1)
    95 03        #     REPORT_COUNT (3)
    75 01        #     REPORT_SIZE (1)
    81 02        #     INPUT (Data,Var,Abs)
    95 01        #     REPORT_COUNT (1)
    75 05        #     REPORT_SIZE (5)
    81 03        #     INPUT (Cnst,Var,Abs)
    05 01        #     USAGE_PAGE (Generic Desktop)
    09 30        #     USAGE (X)
    09 31        #     USAGE (Y)
    15 00        #     LOGICAL_MINIMUM (0)
    26 ff 7f     #     LOGICAL_MAXIMUM (32767)
    75 10        #     REPORT_SIZE (16)
    95 02        #     REPORT_COUNT (2)
    81 02        #     INPUT (Data,Var,Abs)
    09 38        #     USAGE (Wheel)
    15 81        #     LOGICAL_MINIMUM (-127)
    25 7f        #     LOGICAL_MAXIMUM (127)
    75 08        #     REPORT_SIZE (8)
    95 01        #     REPORT_COUNT (1)
    81 06        #     INPUT (Data,Var,Rel)
    c0           #   END_COLLECTION
    c0           # END_COLLECTION
)

(UID)) && die "Must be root!"

gadget=/sys/kernel/config/usb_gadget/zerohid
config=$gadget/configs/c.1
function="$gadget/functions/hid.usb"

if  [ -e $gadget ]; then
    ((install != 2)) || { echo "$gadget is already installed"; exit 0; }
    echo > $gadget/UDC || true
    rm $config/${function##*/}* || true
    rmdir $config/strings/0x409 || true
    rmdir $config || true
    rmdir $function* || true
    rmdir $gadget/strings/0x409 || true
    rmdir $gadget || true
    [ -e $gadget ] && die "Couldn't remove existing $gadget"
fi

if ((install == 0)); then
    modprobe -r usb_f_hid libcomposite
    echo "$gadget has been removed"
    exit 0
fi

# Maybe install module
[ -d ${gadget%/*} ] || modprobe libcomposite

# Create the gadget
mkdir -p $gadget
echo 0x0100 > $gadget/bcdDevice         # Version 1.0.0
echo 0x0200 > $gadget/bcdUSB            # USB 2.0
echo 0x08   > $gadget/bMaxPacketSize0   # EP0 max packet size
echo 0x0104 > $gadget/idProduct         # Multifunction Composite Gadget
echo 0x1d6b > $gadget/idVendor          # Linux Foundation
mkdir -p $gadget/strings/0x409
echo "zerohid" > $gadget/strings/0x409/manufacturer
echo "zerohid" > $gadget/strings/0x409/product
cat /etc/machine-id > $gadget/strings/0x409/serialnumber

# create the configuration
mkdir -p $config
echo 0x80 > $config/bmAttributes
echo 200 > $config/MaxPower # 200 mA
mkdir -p $config/strings/0x409
echo "zerohid" > $config/strings/0x409/configuration

# create device /dev/hidg0
mkdir -p ${function}0
echo 1 > ${function}0/subclass
echo 1 > ${function}0/protocol
echo 8 > ${function}0/report_length     # 8 byte reports
printf $(printf '\\x%s' ${keyboard[@]}) > ${function}0/report_desc
ln -s ${function}0 $config

if ((domouse)); then
    # create device /dev/hidg1
    mkdir -p ${function}1
    echo 1 > ${function}1/subclass
    echo 1 > ${function}1/protocol
    echo 6 > ${function}1/report_length # 6 byte reports
    printf $(printf '\\x%s' ${mouse[@]}) > ${function}1/report_desc
    ln -s ${function}1 $config
fi

# Enable gadget
ls /sys/class/udc > $gadget/UDC

echo "$gadget has been installed"
