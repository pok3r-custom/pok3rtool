#ifndef KEYMAP_H
#define KEYMAP_H

#include "zstring.h"
using namespace LibChaos;

#include "keycodes.h"

class Keymap {
public:
    Keymap();

    zu16 get(zu8 i){ return keymap[i]; }
    void set(zu8 i, zu16 kc){ keymap[i] = kc; }

    zu8 size(){ return keymap.size(); }

public:
    static ZString keycodeName(zu16 kc);
    static ZString keycodeAbbrev(zu16 kc);
    static ZString keycodeDesc(zu16 kc);

private:
    ZArray<zu16> keymap;
};

#endif // KEYMAP_H
