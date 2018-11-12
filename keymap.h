#ifndef KEYMAP_H
#define KEYMAP_H

#include "zstring.h"
#include "zbinary.h"
#include "zmap.h"
using namespace LibChaos;

class Keymap {
public:
    typedef zu16 keycode;
    struct Key {
        Key() : row(0xFF), col(0xFF), width(0), space(false), newrow(false), raw(0){}
        zu8 row, col;
        zu8 width;
        bool space;
        bool newrow;
        int raw;
    };

    struct Keycode {
        Keymap::keycode keycode;
        ZString name;
        ZString abbrev;
        ZString desc;
    };

public:
    Keymap(zu8 rows, zu8 cols);

    void loadLayout(ZString name, ZBinary layout);
    void loadLayerMap(ZBinary layer);

    ZArray<ZArray<keycode>> getKeycodeLayout(zu8 layer) const;
    //! Get layer matrices binary for firmware.
    ZBinary toMatrix() const;

    //! Pretty-print layout layers and keycodes.
    void printLayers() const;
    //! Print layer matrices and keycodes.
    void printMatrix() const;

    keycode get(zu8 l, zu16 k) const { return layers[l][k]; }
    void set(zu8 l, zu16 k, keycode kc){ layers[l][k] = kc; }

    ZArray<int> getLayer(zu8 layer) const;
    ZArray<ZString> getLayerAbbrev(zu8 layer) const;

    //! Get number of keys in layout.
    zu16 numKeys() const { return nkeys; }
    //! Get number of layers.
    zsize numLayers() const { return layers.size(); }
    //! Get number of rows in layout.
    zu16 rowCount(zu8 row) const;

    //! Get name of current layout.
    ZString layoutName() const { return layout_name; }
    ZArray<int> getLayout() const;
    //! Covert layout key row and column to key number.
    zu16 layoutRC2K(zu8 r, zu8 c) const;

    zu16 keyOffset(zu8 l, zu16 k) const;

    keycode toKeycode(ZString name) const;
    ZString keycodeName(keycode kc) const;
    ZString keycodeAbbrev(keycode kc) const;
    ZString keycodeDesc(keycode kc) const;

    static const ZArray<Keycode> &getAllKeycodes();
    static ZArray<ZString> getKnownLayouts();

private:
    //! Matrix rows, columns.
    const zu8 rows, cols;
    zu16 nkeys;
    ZMap<zu16, zu16> lkmap;
    ZArray<Key> wlayout;
    zu16 mwidth;
    ZArray<zu8> matrix2layout;
    ZArray<zu8> layout2matrix;
    ZArray<ZArray<keycode>> layers;

    ZString layout_name;
};

#endif // KEYMAP_H
