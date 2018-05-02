#ifndef KEYMAP_H
#define KEYMAP_H

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

class Keymap {
public:
    typedef zu16 keycode;
    struct Key {
        Key() : row(0xFF), col(0xFF), width(0){}
        zu8 row, col;
        zu8 width;
        bool newrow;
    };

public:
    Keymap(zu8 rows, zu8 cols);

    void loadLayout(ZBinary layout);
    void loadLayerMap(ZBinary layer);

    void dump();

    keycode get(zu8 l, zu8 k){ return layers[l][k]; }
    void set(zu8 l, zu8 k, keycode kc){ layers[l][k] = kc; }

    ZString keycodeName(keycode kc) const;
    ZString keycodeAbbrev(keycode kc) const;
    ZString keycodeDesc(keycode kc) const;

private:
    const zu8 rows;
    const zu8 cols;
    zu16 nkeys;
    ZArray<Key> wlayout;
    zu16 mwidth;
    ZArray<zu8> matrix2layout;
    ZArray<zu8> layout2matrix;
    ZArray<ZArray<keycode>> layers;
};

#endif // KEYMAP_H
