#ifndef POK3R_RGB_H
#define POK3R_RGB_H

#include "libusb.h"
#include "usbdevice.h"

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

#define HOLTEK_VID          0x04d9
#define POK3R_RGB_PID       0x0167

#define RECV_EP             3
#define SEND_EP             4
#define PKT_LEN             64

#define POK3R_RGB_FW_ADDR   0x3400

class Pok3rRGB : public USBDevice {
public:
    enum pok3r_rgb_cmd {
        CMD_16      = 0x10,
        RESET_CMD   = 0x11,
        CMD_18      = 0x12,
    };

public:
    //! Find a Pok3r RGB keyboard.
    bool findPok3rRGB();
};

#endif // POK3R_RGB_H
