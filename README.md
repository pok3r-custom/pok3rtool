# Vortex Keyboard Firmware Tool

*Disclaimer: This project comes with no warranty, and may be used for free at your own risk.*

`pokr3tool` is a Python CLI app/library that implements the firmware update protocol over USB for the
POK3R, POK3R RGB, Vortex Core, and more.

**WARNING: THIS TOOL CAN VERY EASILY BRICK YOUR KEYBOARD IF USED INCORRECTLY, MAKING IT
UNUSABLE WITHOUT EXPENSIVE DEVELOPMENT TOOLS. READ THE DOCUMENTATION, POSSIBLY READ THE
CODE, AND PROCEED AT YOUR OWN RISK.**

## Install

```shell
pip install .
```

## Usage

See the `--help` for all commands:
```shell
pok3rtool --help
```

List connected supported keyboards:
```shell
pok3rtool list
```

### Flashing

**WARNING: BE SURE YOU HAVE THE RIGHT FIRMWARE FILE FOR YOUR KEYBOARD**

The bootloaders on these keyboards have no good way to know if the firmware is valid.
If broken firmware is uploaded, the bootloader may get stuck in a hard fault loop. 
There is no way to prevent the bootloader from starting the main firmware, so this effectively bricks the keyboard.

```shell
# pok3rtool flash VERSION FILENAME
pok3rtool flash 1.1.7 POK3R-61_Keys-1.1.7.bin
```

Reboot a keyboard into its bootloader:
```shell
pok3rtool reboot --bootloader
```

### Extracting Firmware

Extract firmware from a supported updater EXE:
```shell
pok3rtool extract maajonsn POK3R_V117.exe
```
See [test_extract.sh](test_extract.sh) for known working formats and filenames.
