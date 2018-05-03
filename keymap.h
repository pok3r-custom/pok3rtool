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

    ZArray<ZArray<keycode>> getKeycodeLayout(zu8 layer) const;
    ZBinary toMatrix() const;

    void printLayers() const;
    void printMatrix() const;

    keycode get(zu8 l, zu16 k) const { return layers[l][k]; }
    void set(zu8 l, zu16 k, keycode kc){ layers[l][k] = kc; }

    zu16 rowCount(zu8 row) const;
    zu16 layoutRC2K(zu8 r, zu8 c) const;

    zu16 keyOffset(zu8 l, zu16 k) const;

    keycode toKeycode(ZString name) const;
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
