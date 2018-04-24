#include "keycodes.h"

#include "zmap.h"

struct Keycode {
    ZString name;
    ZString desc;
};

const ZMap<zu16, Keycode> keycodes = {
    { KC_NO,            { "KC_NO",      "None" } },
    { KC_TRANSPARENT,   { "KC_TRNS",    "Transparent" } },
    { KC_A,             { "KC_A",       "A" } },
    { KC_B,             { "KC_B",       "B" } },
    { KC_C,             { "KC_C",       "C" } },
    { KC_D,             { "KC_D",       "D" } },
    { KC_E,             { "KC_E",       "E" } },
    { KC_F,             { "KC_F",       "F" } },
    { KC_G,             { "KC_G",       "G" } },
    { KC_H,             { "KC_H",       "H" } },
    { KC_I,             { "KC_I",       "I" } },
    { KC_J,             { "KC_J",       "J" } },
    { KC_K,             { "KC_K",       "K" } },
    { KC_L,             { "KC_L",       "L" } },
    { KC_M,             { "KC_M",       "M" } },
    { KC_N,             { "KC_N",       "N" } },
    { KC_O,             { "KC_O",       "O" } },
    { KC_P,             { "KC_P",       "P" } },
    { KC_Q,             { "KC_Q",       "Q" } },
    { KC_R,             { "KC_R",       "R" } },
    { KC_S,             { "KC_S",       "S" } },
    { KC_T,             { "KC_T",       "T" } },
    { KC_U,             { "KC_U",       "U" } },
    { KC_V,             { "KC_V",       "V" } },
    { KC_W,             { "KC_W",       "W" } },
    { KC_X,             { "KC_X",       "X" } },
    { KC_Y,             { "KC_Y",       "Y" } },
    { KC_Z,             { "KC_Z",       "Z" } },
    { KC_1,             { "KC_1",       "1" } },
    { KC_2,             { "KC_2",       "2" } },
    { KC_3,             { "KC_3",       "3" } },
    { KC_4,             { "KC_4",       "4" } },
    { KC_5,             { "KC_5",       "5" } },
    { KC_6,             { "KC_6",       "6" } },
    { KC_7,             { "KC_7",       "7" } },
    { KC_8,             { "KC_8",       "8" } },
    { KC_9,             { "KC_9",       "9" } },
    { KC_0,             { "KC_0",       "0" } },
};

ZString keycodeName(zu16 kc){
    if(keycodes.contains(kc)){
        return keycodes[kc].name;
    } else {
        return "0x" + ZString::ItoS(kc, 16, 4);
    }
}
