#include "keymap.h"
#include "keycodes.h"

#include "zlog.h"
#include "zmap.h"

#define USE_NLOHMANN_JSON 1

#if USE_NLOHMANN_JSON
    #include <nlohmann/json.hpp>
#else
    #include "zjson.h"
#endif

// only include in this file
#include "gen_keymaps.h"

#define MIN(A, B) (A < B ? A : B)
#define MAX(A, B) (A > B ? A : B)

#define LAYOUT_SP 0x80
#define LAYOUT_MASK 0x3F

const ZArray<Keymap::Keycode> keycodes = {
    { KC_NO,                    "KC_NO",                    "",         "None" },
    { KC_TRANSPARENT,           "KC_TRNS",                  "",         "Transparent (keycode from previous layer)" },
    { KC_A,                     "KC_A",                     "A",        "A" },
    { KC_B,                     "KC_B",                     "B",        "B" },
    { KC_C,                     "KC_C",                     "C",        "C" },
    { KC_D,                     "KC_D",                     "D",        "D" },
    { KC_E,                     "KC_E",                     "E",        "E" },
    { KC_F,                     "KC_F",                     "F",        "F" },
    { KC_G,                     "KC_G",                     "G",        "G" },
    { KC_H,                     "KC_H",                     "H",        "H" },
    { KC_I,                     "KC_I",                     "I",        "I" },
    { KC_J,                     "KC_J",                     "J",        "J" },
    { KC_K,                     "KC_K",                     "K",        "K" },
    { KC_L,                     "KC_L",                     "L",        "L" },
    { KC_M,                     "KC_M",                     "M",        "M" },
    { KC_N,                     "KC_N",                     "N",        "N" },
    { KC_O,                     "KC_O",                     "O",        "O" },
    { KC_P,                     "KC_P",                     "P",        "P" },
    { KC_Q,                     "KC_Q",                     "Q",        "Q" },
    { KC_R,                     "KC_R",                     "R",        "R" },
    { KC_S,                     "KC_S",                     "S",        "S" },
    { KC_T,                     "KC_T",                     "T",        "T" },
    { KC_U,                     "KC_U",                     "U",        "U" },
    { KC_V,                     "KC_V",                     "V",        "V" },
    { KC_W,                     "KC_W",                     "W",        "W" },
    { KC_X,                     "KC_X",                     "X",        "X" },
    { KC_Y,                     "KC_Y",                     "Y",        "Y" },
    { KC_Z,                     "KC_Z",                     "Z",        "Z" },
    { KC_1,                     "KC_1",                     "1",        "1" },
    { KC_2,                     "KC_2",                     "2",        "2" },
    { KC_3,                     "KC_3",                     "3",        "3" },
    { KC_4,                     "KC_4",                     "4",        "4" },
    { KC_5,                     "KC_5",                     "5",        "5" },
    { KC_6,                     "KC_6",                     "6",        "6" },
    { KC_7,                     "KC_7",                     "7",        "7" },
    { KC_8,                     "KC_8",                     "8",        "8" },
    { KC_9,                     "KC_9",                     "9",        "9" },
    { KC_0,                     "KC_0",                     "0",        "0" },
    { KC_ENTER,                 "KC_ENTER",                 "ENTER",    "Enter" },
    { KC_ESCAPE,                "KC_ESCAPE",                "ESC",      "Esc" },
    { KC_BSPACE,                "KC_BSPACE",                "BSPACE",   "Backspace" },
    { KC_TAB,                   "KC_TAB",                   "TAB",      "Tab" },
    { KC_SPACE,                 "KC_SPACE",                 "SPACE",    "Space" },
    { KC_MINUS,                 "KC_MINUS",                 "-",        "-" },
    { KC_EQUAL,                 "KC_EQUAL",                 "=",        "=" },
    { KC_LBRACKET,              "KC_LBRACKET",              "[",        "[" },
    { KC_RBRACKET,              "KC_RBRACKET",              "]",        "]" },
    { KC_BSLASH,                "KC_BSLASH",                "\\",       "\\" },
    { KC_NONUS_HASH,            "KC_NONUS_HASH",            "",         "# (Non-US)" },
    { KC_SCOLON,                "KC_SCOLON",                ";",        ";" },
    { KC_QUOTE,                 "KC_QUOTE",                 "'",        "'" },
    { KC_GRAVE,                 "KC_GRAVE",                 "`",        "`" },
    { KC_COMMA,                 "KC_COMMA",                 ",",        "," },
    { KC_DOT,                   "KC_DOT",                   ".",        "." },
    { KC_SLASH,                 "KC_SLASH",                 "/",        "/" },
    { KC_CAPSLOCK,              "KC_CAPSLOCK",              "CAPS",     "Caps Lock" },
    { KC_F1,                    "KC_F1",                    "F1",       "F1" },
    { KC_F2,                    "KC_F2",                    "F2",       "F2" },
    { KC_F3,                    "KC_F3",                    "F3",       "F3" },
    { KC_F4,                    "KC_F4",                    "F4",       "F4" },
    { KC_F5,                    "KC_F5",                    "F5",       "F5" },
    { KC_F6,                    "KC_F6",                    "F6",       "F6" },
    { KC_F7,                    "KC_F7",                    "F7",       "F7" },
    { KC_F8,                    "KC_F8",                    "F8",       "F8" },
    { KC_F9,                    "KC_F9",                    "F9",       "F9" },
    { KC_F10,                   "KC_F10",                   "F10",      "F10" },
    { KC_F11,                   "KC_F11",                   "F11",      "F11" },
    { KC_F12,                   "KC_F12",                   "F12",      "F12" },
    { KC_PSCREEN,               "KC_PSCREEN",               "PRSC",     "Print Screen" },
    { KC_SCROLLLOCK,            "KC_SCROLLLOCK",            "SCLK",     "Scroll Lock" },
    { KC_PAUSE,                 "KC_PAUSE",                 "PAUS",     "Pause" },
    { KC_INSERT,                "KC_INSERT",                "INS",      "Insert" },
    { KC_HOME,                  "KC_HOME",                  "HOME",     "Home" },
    { KC_PGUP,                  "KC_PGUP",                  "PGUP",     "Page Up" },
    { KC_DELETE,                "KC_DELETE",                "DEL",      "Delete" },
    { KC_END,                   "KC_END",                   "END",      "End" },
    { KC_PGDOWN,                "KC_PGDOWN",                "PGDN",     "Page Down" },
    { KC_RIGHT,                 "KC_RIGHT",                 "RIGHT",    "Right Arrow" },
    { KC_LEFT,                  "KC_LEFT",                  "LEFT",     "Left Arrow" },
    { KC_DOWN,                  "KC_DOWN",                  "DOWN",     "Down Arrow" },
    { KC_UP,                    "KC_UP",                    "UP",       "Up Arrow" },
    { KC_NUMLOCK,               "KC_NUMLOCK",               "NUMLK",    "Number Lock" },
    { KC_KP_SLASH,              "KC_KP_SLASH",              "/",        "Keypad /" },
    { KC_KP_ASTERISK,           "KC_KP_ASTERISK",           "*",        "Keypad *" },
    { KC_KP_MINUS,              "KC_KP_MINUS",              "-",        "Keypad -" },
    { KC_KP_PLUS,               "KC_KP_PLUS",               "+",        "Keypad +" },
    { KC_KP_ENTER,              "KC_KP_ENTER",              "ENTER",    "Keypad Enter" },
    { KC_KP_1,                  "KC_KP_1",                  "1",        "Keypad 1" },
    { KC_KP_2,                  "KC_KP_2",                  "2",        "Keypad 2" },
    { KC_KP_3,                  "KC_KP_3",                  "3",        "Keypad 3" },
    { KC_KP_4,                  "KC_KP_4",                  "4",        "Keypad 4" },
    { KC_KP_5,                  "KC_KP_5",                  "5",        "Keypad 5" },
    { KC_KP_6,                  "KC_KP_6",                  "6",        "Keypad 6" },
    { KC_KP_7,                  "KC_KP_7",                  "7",        "Keypad 7" },
    { KC_KP_8,                  "KC_KP_8",                  "8",        "Keypad 8" },
    { KC_KP_9,                  "KC_KP_9",                  "9",        "Keypad 9" },
    { KC_KP_0,                  "KC_KP_0",                  "0",        "Keypad 0" },
    { KC_KP_DOT,                "KC_KP_DOT",                ".",        "Keypad ." },
    { KC_NONUS_BSLASH,          "KC_NONUS_BSLASH",          "\\",       "\\ (Non-US)" },
    { KC_APPLICATION,           "KC_APPLICATION",           "APP",      "Application Menu" },
//  { KC_POWER,                 "KC_POWER",                 "POWER",    "Power" },
    { KC_KP_EQUAL,              "KC_KP_EQUAL",              "=",        "Keypad =" },
    { KC_F13,                   "KC_F13",                   "F13",      "F13" },
    { KC_F14,                   "KC_F14",                   "F14",      "F14" },
    { KC_F15,                   "KC_F15",                   "F15",      "F15" },
    { KC_F16,                   "KC_F16",                   "F16",      "F16" },
    { KC_F17,                   "KC_F17",                   "F17",      "F17" },
    { KC_F18,                   "KC_F18",                   "F18",      "F18" },
    { KC_F19,                   "KC_F19",                   "F19",      "F19" },
    { KC_F20,                   "KC_F20",                   "F20",      "F20" },
    { KC_F21,                   "KC_F21",                   "F21",      "F21" },
    { KC_F22,                   "KC_F22",                   "F22",      "F22" },
    { KC_F23,                   "KC_F23",                   "F23",      "F23" },
    { KC_F24,                   "KC_F24",                   "F24",      "F24" },
    { KC_EXECUTE,               "KC_EXECUTE",               "EXEC",     "Execute" },
    { KC_HELP,                  "KC_HELP",                  "HELP",     "Help" },
    { KC_MENU,                  "KC_MENU",                  "MENU",     "Menu" },
    { KC_SELECT,                "KC_SELECT",                "SELECT",   "Select" },
    { KC_STOP,                  "KC_STOP",                  "STOP",     "Stop" },
    { KC_AGAIN,                 "KC_AGAIN",                 "AGAIN",    "Again" },
    { KC_UNDO,                  "KC_UNDO",                  "UNDO",     "Undo" },
    { KC_CUT,                   "KC_CUT",                   "CUT",      "Cut" },
    { KC_COPY,                  "KC_COPY",                  "COPY",     "Copy" },
    { KC_PASTE,                 "KC_PASTE",                 "PASTE",    "Paste" },
    { KC_FIND,                  "KC_FIND",                  "FIND",     "Find" },
    // not sure about these ys compared to media keys
//  { KC__MUTE,                 "KC__MUTE",                 "MUTE",     "Mute" },
//  { KC__VOLUP,                "KC__VOLUP",                "VOLUP",    "Volume Up" },
//  { KC__VOLDOWN,              "KC__VOLDOWN",              "VOLDN",    "Volume Down" },
    // these require specialandling on keyboard and OS
//  { KC_LOCKING_CAPS,          "KC_LOCKING_CAPS",          "LKCAPS",   "Locking Caps Lock" },
//  { KC_LOCKING_NUM,           "KC_LOCKING_NUM",           "LKNUM",    "Locking Num Lock" },
//  { KC_LOCKING_SCROLL,        "KC_LOCKING_SCROLL",        "LKSCLK",   "Locking Scroll Lock" },
    { KC_KP_COMMA,              "KC_KP_COMMA",              ",",        "Keypad ," },
//  { KC_KP_EQUAL_AS400,        "KC_KP_EQUAL_AS400",        "=",        "Keypad AS400 Equal" },
    { KC_INT1,                  "KC_INT1",                  "INT1",     "International 1" },
    { KC_INT2,                  "KC_INT2",                  "INT2",     "International 2" },
    { KC_INT3,                  "KC_INT3",                  "INT3",     "International 3" },
    { KC_INT4,                  "KC_INT4",                  "INT4",     "International 4" },
    { KC_INT5,                  "KC_INT5",                  "INT5",     "International 5" },
    { KC_INT6,                  "KC_INT6",                  "INT6",     "International 6" },
    { KC_INT7,                  "KC_INT7",                  "INT7",     "International 7" },
    { KC_INT8,                  "KC_INT8",                  "INT8",     "International 8" },
    { KC_INT9,                  "KC_INT9",                  "INT9",     "International 9" },
    { KC_LANG1,                 "KC_LANG1",                 "LANG1",    "Language Specific 1" },
    { KC_LANG2,                 "KC_LANG2",                 "LANG2",    "Language Specific 2" },
    { KC_LANG3,                 "KC_LANG3",                 "LANG3",    "Language Specific 3" },
    { KC_LANG4,                 "KC_LANG4",                 "LANG4",    "Language Specific 4" },
    { KC_LANG5,                 "KC_LANG5",                 "LANG5",    "Language Specific 5" },
    { KC_LANG6,                 "KC_LANG6",                 "LANG6",    "Language Specific 6" },
    { KC_LANG7,                 "KC_LANG7",                 "LANG7",    "Language Specific 7" },
    { KC_LANG8,                 "KC_LANG8",                 "LANG8",    "Language Specific 8" },
    { KC_LANG9,                 "KC_LANG9",                 "LANG9",    "Language Specific 9" },
    { KC_ALT_ERASE,             "KC_ALT_ERASE",             "ALTERA",   "Alternate Erase" },
    { KC_SYSREQ,                "KC_SYSREQ",                "SYSREQ",   "SysReq" },
    { KC_CANCEL,                "KC_CANCEL",                "CANCEL",   "Cancel" },
    { KC_CLEAR,                 "KC_CLEAR",                 "CLEAR",    "Clear" },
    { KC_PRIOR,                 "KC_PRIOR",                 "PRIOR",    "Prior" },
    { KC_RETURN,                "KC_RETURN",                "RETURN",   "Return" },
    { KC_SEPARATOR,             "KC_SEPARATOR",             "SEP",      "Separator" },
    { KC_OUT,                   "KC_OUT",                   "OUT",      "Out" },
    { KC_OPER,                  "KC_OPER",                  "OPER",     "Oper" },
    { KC_CLEAR_AGAIN,           "KC_CLEAR_AGAIN",           "CLRAG",    "Clear Again" },
    { KC_CRSEL,                 "KC_CRSEL",                 "CRSEL",    "CrSel" },
    { KC_EXSEL,                 "KC_EXSEL",                 "EXSEL",    "ExSel" },

    // Keypad codes (0xB0-0xDD) not used

    { KC_LCTRL,                 "KC_LCTRL",                 "LCTRL",    "Left Control" },
    { KC_LSHIFT,                "KC_LSHIFT",                "LSHIFT",   "Left Shift" },
    { KC_LALT,                  "KC_LALT",                  "LALT",     "Left Alt" },
    { KC_LGUI,                  "KC_LGUI",                  "LGUI",     "Left GUI" },
    { KC_RCTRL,                 "KC_RCTRL",                 "RCTRL",    "Right Control" },
    { KC_RSHIFT,                "KC_RSHIFT",                "RSHIFT",   "Right Shift" },
    { KC_RALT,                  "KC_RALT",                  "RALT",     "Right Alt" },
    { KC_RGUI,                  "KC_RGUI",                  "RGUI",     "Right GUI" },

    { KC_SYSTEM_POWER,          "KC_SYSTEM_POWER",          "POWER",    "System Power" },
    { KC_SYSTEM_SLEEP,          "KC_SYSTEM_SLEEP",          "SLEEP",    "System Sleep" },
    { KC_SYSTEM_WAKE,           "KC_SYSTEM_WAKE",           "WAKE",     "System Wake" },
    { KC_AUDIO_MUTE,            "KC_AUDIO_MUTE",            "MUTE",     "Audio Mute" },
    { KC_AUDIO_VOL_UP,          "KC_AUDIO_VOL_UP",          "VOLUP",    "Audio Volume Up" },
    { KC_AUDIO_VOL_DOWN,        "KC_AUDIO_VOL_DOWN",        "VOLDN",    "Audio Volume Down" },
    { KC_MEDIA_NEXT_TRACK,      "KC_MEDIA_NEXT_TRACK",      "MNEXT",    "Media Next Track" },
    { KC_MEDIA_PREV_TRACK,      "KC_MEDIA_PREV_TRACK",      "MPREV",    "Media Previous Track" },
    { KC_MEDIA_STOP,            "KC_MEDIA_STOP",            "MSTOP",    "Media Stop" },
    { KC_MEDIA_PLAY_PAUSE,      "KC_MEDIA_PLAY_PAUSE",      "MPLAY",    "Media Play/Pause" },
    { KC_MEDIA_SELECT,          "KC_MEDIA_SELECT",          "MSEL",     "Media Select" },
    { KC_MEDIA_EJECT,           "KC_MEDIA_EJECT",           "MEJECT",   "Media Eject" },
    { KC_MAIL,                  "KC_MAIL",                  "MAIL",     "Mail" },
    { KC_CALCULATOR,            "KC_CALCULATOR",            "CALC",     "Calculator" },
    { KC_MY_COMPUTER,           "KC_MY_COMPUTER",           "COMP",     "My Computer" },
    { KC_WWW_SEARCH,            "KC_WWW_SEARCH",            "SEARCH",   "WWW Search" },
    { KC_WWW_HOME,              "KC_WWW_HOME",              "WHOME",    "WWW Home" },
    { KC_WWW_BACK,              "KC_WWW_BACK",              "WBACK",    "WWW Back" },
    { KC_WWW_FORWARD,           "KC_WWW_FORWARD",           "WFOR",     "WWW Forward" },
    { KC_WWW_STOP,              "KC_WWW_STOP",              "WSTOP",    "WWW Stop" },
    { KC_WWW_REFRESH,           "KC_WWW_REFRESH",           "WREFR",    "WWW Refresh" },
    { KC_WWW_FAVORITES,         "KC_WWW_FAVORITES",         "WFAV",     "WWW Favorites" },
    { KC_MEDIA_FAST_FORWARD,    "KC_MEDIA_FAST_FORWARD",    "MFASTF",   "Media Fast Forward" },
    { KC_MEDIA_REWIND,          "KC_MEDIA_REWIND",          "MREW",     "Media Rewind" },

    { KC_MS_UP,                 "KC_MS_UP",                 "MSUP",     "Mouse Up" },
    { KC_MS_DOWN,               "KC_MS_DOWN",               "MSDOWN",   "Mouse Down" },
    { KC_MS_LEFT,               "KC_MS_LEFT",               "MSLEFT",   "Mouse Left" },
    { KC_MS_RIGHT,              "KC_MS_RIGHT",              "MSRGHT",   "Mouse Right" },
    { KC_MS_BTN1,               "KC_MS_BTN1",               "MSBTN1",   "Mouse Button 1" },
    { KC_MS_BTN2,               "KC_MS_BTN2",               "MSBTN2",   "Mouse Button 2" },
    { KC_MS_BTN3,               "KC_MS_BTN3",               "MSBTN3",   "Mouse Button 3" },
    { KC_MS_BTN4,               "KC_MS_BTN4",               "MSBTN4",   "Mouse Button 4" },
    { KC_MS_BTN5,               "KC_MS_BTN5",               "MSBTN5",   "Mouse Button 5" },
    { KC_MS_WH_UP,              "KC_MS_WH_UP",              "WHUP",     "Mouse Wheel Up" },
    { KC_MS_WH_DOWN,            "KC_MS_WH_DOWN",            "WHDOWN",   "Mouse Wheel Down" },
    { KC_MS_WH_LEFT,            "KC_MS_WH_LEFT",            "WHLEFT",   "Mouse Wheel Left" },
    { KC_MS_WH_RIGHT,           "KC_MS_WH_RIGHT",           "WHRGHT",   "Mouse Wheel Right" },
    { KC_MS_ACCEL0,             "KC_MS_ACCEL0",             "MSACC0",   "Mouse Acceleration 0" },
    { KC_MS_ACCEL1,             "KC_MS_ACCEL1",             "MSACC1",   "Mouse Acceleration 1" },
    { KC_MS_ACCEL2,             "KC_MS_ACCEL2",             "MSACC2",   "Mouse Acceleration 2" },

    { MO(0),                    "MO(0)",                    "MO(0)",    "Momentary Layer 0" },
    { MO(1),                    "MO(1)",                    "MO(1)",    "Momentary Layer 1" },
    { MO(2),                    "MO(2)",                    "MO(2)",    "Momentary Layer 2" },
    { MO(3),                    "MO(3)",                    "MO(3)",    "Momentary Layer 3" },
    { MO(4),                    "MO(4)",                    "MO(4)",    "Momentary Layer 4" },
    { MO(5),                    "MO(5)",                    "MO(5)",    "Momentary Layer 5" },
    { MO(6),                    "MO(6)",                    "MO(6)",    "Momentary Layer 6" },
    { MO(7),                    "MO(7)",                    "MO(7)",    "Momentary Layer 7" },

    { TG(0),                    "TG(0)",                    "TG(0)",    "Toggle Layer 0" },
    { TG(1),                    "TG(1)",                    "TG(1)",    "Toggle Layer 1" },
    { TG(2),                    "TG(2)",                    "TG(2)",    "Toggle Layer 2" },
    { TG(3),                    "TG(3)",                    "TG(3)",    "Toggle Layer 3" },
    { TG(4),                    "TG(4)",                    "TG(4)",    "Toggle Layer 4" },
    { TG(5),                    "TG(5)",                    "TG(5)",    "Toggle Layer 5" },
    { TG(6),                    "TG(6)",                    "TG(6)",    "Toggle Layer 6" },
    { TG(7),                    "TG(7)",                    "TG(7)",    "Toggle Layer 7" },

    { TT(0),                    "TT(0)",                    "TT(0)",    "Tap Toggle Layer 0" },
    { TT(1),                    "TT(1)",                    "TT(1)",    "Tap Toggle Layer 1" },
    { TT(2),                    "TT(2)",                    "TT(2)",    "Tap Toggle Layer 2" },
    { TT(3),                    "TT(3)",                    "TT(3)",    "Tap Toggle Layer 3" },
    { TT(4),                    "TT(4)",                    "TT(4)",    "Tap Toggle Layer 4" },
    { TT(5),                    "TT(5)",                    "TT(5)",    "Tap Toggle Layer 5" },
    { TT(6),                    "TT(6)",                    "TT(6)",    "Tap Toggle Layer 6" },
    { TT(7),                    "TT(7)",                    "TT(7)",    "Tap Toggle Layer 7" },

    { DF(0),                    "DF(0)",                    "DF(0)",    "Set Default Layer 0" },
    { DF(1),                    "DF(1)",                    "DF(1)",    "Set Default Layer 1" },
    { DF(2),                    "DF(2)",                    "DF(2)",    "Set Default Layer 2" },
    { DF(3),                    "DF(3)",                    "DF(3)",    "Set Default Layer 3" },
    { DF(4),                    "DF(4)",                    "DF(4)",    "Set Default Layer 4" },
    { DF(5),                    "DF(5)",                    "DF(5)",    "Set Default Layer 5" },
    { DF(6),                    "DF(6)",                    "DF(6)",    "Set Default Layer 6" },
    { DF(7),                    "DF(7)",                    "DF(7)",    "Set Default Layer 7" },

    { RESET,                    "RESET",                    "RESET",    "Magic Reset" },
    { DEBUG,                    "DEBUG",                    "DEBUG",    "Magic Debug" },

//  { MAGIC_SWAP_CONTROL_CAPSLOCK,      "MAGIC_SWAP_CONTROL_CAPSLOCK",      "", "" },
//  { MAGIC_CAPSLOCK_TO_CONTROL,        "MAGIC_CAPSLOCK_TO_CONTROL",        "", "" },
//  { MAGIC_SWAP_LALT_LGUI,             "MAGIC_SWAP_LALT_LGUI",             "", "" },
//  { MAGIC_SWAP_RALT_RGUI,             "MAGIC_SWAP_RALT_RGUI",             "", "" },
//  { MAGIC_NO_GUI,                     "MAGIC_NO_GUI",                     "", "" },
//  { MAGIC_SWAP_GRAVE_ESC,             "MAGIC_SWAP_GRAVE_ESC",             "", "" },
//  { MAGIC_SWAP_BACKSLASH_BACKSPACE,   "MAGIC_SWAP_BACKSLASH_BACKSPACE",   "", "" },
//  { MAGIC_HOST_NKRO,                  "MAGIC_HOST_NKRO",                  "", "" },
//  { MAGIC_SWAP_ALT_GUI,               "MAGIC_SWAP_ALT_GUI",               "", "" },
//  { MAGIC_UNSWAP_CONTROL_CAPSLOCK,    "MAGIC_UNSWAP_CONTROL_CAPSLOCK",    "", "" },
//  { MAGIC_UNCAPSLOCK_TO_CONTROL,      "MAGIC_UNCAPSLOCK_TO_CONTROL",      "", "" },
//  { MAGIC_UNSWAP_LALT_LGUI,           "MAGIC_UNSWAP_LALT_LGUI",           "", "" },
//  { MAGIC_UNSWAP_RALT_RGUI,           "MAGIC_UNSWAP_RALT_RGUI",           "", "" },
//  { MAGIC_UNNO_GUI,                   "MAGIC_UNNO_GUI",                   "", "" },
//  { MAGIC_UNSWAP_GRAVE_ESC,           "MAGIC_UNSWAP_GRAVE_ESC",           "", "" },
//  { MAGIC_UNSWAP_BACKSLASH_BACKSPACE, "MAGIC_UNSWAP_BACKSLASH_BACKSPACE", "", "" },
//  { MAGIC_UNHOST_NKRO,                "MAGIC_UNHOST_NKRO",                "", "" },
//  { MAGIC_UNSWAP_ALT_GUI,             "MAGIC_UNSWAP_ALT_GUI",             "", "" },
//  { MAGIC_TOGGLE_NKRO,                "MAGIC_TOGGLE_NKRO",                "", "" },
//  { GRAVE_ESC,                        "GRAVE_ESC",                        "", "" },

    { BL_ON,                    "BL_ON",                    "BLON",     "Backlight On" },
    { BL_OFF,                   "BL_OFF",                   "BLOFF",    "Backlight Off" },
    { BL_DEC,                   "BL_DEC",                   "BLDEC",    "Backlight Decrease" },
    { BL_INC,                   "BL_INC",                   "BLINC",    "Backlight Increase" },
    { BL_TOGG,                  "BL_TOGG",                  "BLTOGG",   "Backlight Toggle" },
    { BL_STEP,                  "BL_STEP",                  "BLSTEP",   "Backlight Step" },
    { BL_BRTG,                  "BL_BRTG",                  "BLBRTG",   "Backlight " },

    { RGB_TOG,                  "RGB_TOG",                  "RGBTOG",   "RGB Toggle" },
    { RGB_MODE_FORWARD,         "RGB_MODE_FORWARD",         "RGBMDF",   "RGB Mode Forward" },
    { RGB_MODE_REVERSE,         "RGB_MODE_REVERSE",         "RGBMDR",   "RGB Mode Reverse" },
    { RGB_HUI,                  "RGB_HUI",                  "RGBHUI",   "RGB Hue Increase" },
    { RGB_HUD,                  "RGB_HUD",                  "RGBHUD",   "RGB Hue Decrease" },
    { RGB_SAI,                  "RGB_SAI",                  "RGBSAI",   "RGB Saturation Increase" },
    { RGB_SAD,                  "RGB_SAD",                  "RGBSAD",   "RGB Saturation Decrease" },
    { RGB_VAI,                  "RGB_VAI",                  "RGBVAI",   "RGB Value Increase" },
    { RGB_VAD,                  "RGB_VAD",                  "RGBVAD",   "RGB Value Decrease" },
    { RGB_MODE_PLAIN,           "RGB_MODE_PLAIN",           "RGBMP",    "RGB Mode Plain" },
    { RGB_MODE_BREATHE,         "RGB_MODE_BREATHE",         "RGBMB",    "RGB Mode Breathe" },
    { RGB_MODE_RAINBOW,         "RGB_MODE_RAINBOW",         "RGBMR",    "RGB Mode Rainbow" },
    { RGB_MODE_SWIRL,           "RGB_MODE_SWIRL",           "RGBMSW",   "RGB Mode Swirl" },
    { RGB_MODE_SNAKE,           "RGB_MODE_SNAKE",           "RGBMSN",   "RGB Mode Snake" },
    { RGB_MODE_KNIGHT,          "RGB_MODE_KNIGHT",          "RGBMK",    "RGB Mode Knight" },
    { RGB_MODE_XMAS,            "RGB_MODE_XMAS",            "RGBMX",    "RGB Mode XMas" },
    { RGB_MODE_GRADIENT,        "RGB_MODE_GRADIENT",        "RGBMG",    "RGB Mode Gradient" },

    { SAFE_RANGE+0,             "CUSTOM0",                  "CUST0",    "Custom 0" },
    { SAFE_RANGE+1,             "CUSTOM1",                  "CUST1",    "Custom 1" },
    { SAFE_RANGE+2,             "CUSTOM2",                  "CUST2",    "Custom 2" },
    { SAFE_RANGE+3,             "CUSTOM3",                  "CUST3",    "Custom 3" },
    { SAFE_RANGE+4,             "CUSTOM4",                  "CUST4",    "Custom 4" },
    { SAFE_RANGE+5,             "CUSTOM5",                  "CUST5",    "Custom 5" },
    { SAFE_RANGE+6,             "CUSTOM6",                  "CUST6",    "Custom 6" },
    { SAFE_RANGE+7,             "CUSTOM7",                  "CUST7",    "Custom 7" },

};

ZMap<Keymap::keycode, Keymap::Keycode> kcode_map;
ZMap<ZString, Keymap::keycode> kname_map;

Keymap::Keymap(zu8 _rows, zu8 _cols) : rows(_rows), cols(_cols), nkeys(0){
    // make keycode map
    if(!kcode_map.size()){
        for(zsize i = 0; i < keycodes.size(); ++i){
            kcode_map[keycodes[i].keycode] = keycodes[i];
            if(kname_map.contains(keycodes[i].name))
                ELOG("Duplicate keycode name");
            kname_map[keycodes[i].name] = keycodes[i].keycode;
        }
    }
}

void Keymap::loadLayout(ZString lname, ZBinary layout){
    layout_name = lname;

    const int knum = rows * cols;
    zassert(layout.size() == knum, "Bad layout map size!");

    matrix2layout.resize(knum);
    layout2matrix.resize(knum);
    for(zsize i = 0; i < knum; ++i){
        matrix2layout[i] = 0;
        layout2matrix[i] = 0;
    }

    // search known layouts
    bool found = false;
    zu16 lkeys = 0;
    for(zu64 i = 0; i < keymaps_json_size / sizeof(unsigned); ++i){
        zu64 len = keymaps_json_sizes[i];
        ZString str(keymaps_json[i], len);

    #if USE_NLOHMANN_JSON
        auto json = nlohmann::json::parse(str.str());
        ZString name = json["name"].get<std::string>();
        if(name == layout_name){
            auto j1 = json["layout"];
            for(auto it = j1.cbegin(); it != j1.cend(); ++it){
                auto j2 = *it;
                for(auto jt = j2.cbegin(); jt != j2.cend(); ++jt){
                    int width = *jt;
                    Key k;
                    k.width = width & LAYOUT_MASK;
                    k.space = width & LAYOUT_SP;
                    if(!k.space){
                        lkeys++;
                        lkmap[lkeys] = wlayout.size();
                    }
                    wlayout.push(k);
                }
                wlayout[wlayout.size() - 1].newrow = true;
            }
            found = true;
            break;
        }
    #else
        ZJSON json;
        zassert(json.decode(str), "layout json decode");
        zassert(json.type() == ZJSON::OBJECT, "layout json object");
        zassert(json.object().contains("name"), "layout json name");
        zassert(json["name"].type() == ZJSON::STRING, "layout json name type");
        ZString name = json["name"].string();
        if(name == layout_name){
            zassert(json.object().contains("layout"), "layout json layout");
            zassert(json["layout"].type() == ZJSON::ARRAY, "layout json layout type");
            for(auto it = json["layout"].array().begin(); it.more(); ++it){
                zassert(it.get().type() == ZJSON::ARRAY, "bad layout row type");
                zassert(it.get().array().size(), "empty layout row");
                for(auto jt = it.get().array().begin(); jt.more(); ++jt){
                    zassert(jt.get().type() == ZJSON::NUMBER, "bad layout key type");
                    int width = jt.get().number();
                    Key k;
                    k.width = width & LAYOUT_MASK;
                    k.space = width & LAYOUT_SP;
                    if(!k.space){
                        lkeys++;
                        lkmap[lkeys] = wlayout.size();
                    }
                    wlayout.push(k);
                }
                wlayout[wlayout.size() - 1].newrow = true;
            }
            found = true;
            break;
        }
#endif
    }
    zassert(found, "layout not known");

    // layout is a matrix, with each key given an index
    // numbered left to right, wrapping rows
    nkeys = 0;
    for(zsize i = 0, r = 0; r < rows; ++r){
        for(zsize c = 0; c < cols; ++c, ++i){
            const zu8 mpos = i;
            const zu8 lpos = layout.readu8();
            if(lpos){
                zassert(matrix2layout[mpos] == 0, "duplicate matrix index");
                zassert(layout2matrix[lpos - 1] == 0, "duplicate layout index");
                wlayout[lkmap[lpos]].row = r;
                wlayout[lkmap[lpos]].col = c;
                matrix2layout[mpos] = lpos;
                layout2matrix[lpos - 1] = mpos;
                nkeys++;
            }
        }
    }
    zassert(nkeys == lkeys, "key count mismatch");
    wlayout.resize(nkeys);
    wlayout[nkeys - 1].newrow = true;

    zu16 rwmax = 0;
    int rwidth = 0;
    for(zsize i = 0; i < nkeys; ++i){
        const zu8 keyw = wlayout[i].width;
        rwidth += keyw;
        if(wlayout[i].newrow){
            rwmax = MAX(rwmax, rwidth);
            rwidth = 0;
        }
    }
    mwidth = rwmax;
}

void Keymap::loadLayerMap(ZBinary layer){
    const int knum = rows * cols;
    zassert(layer.size() == (knum * 2), "Bad layer map size!");
    ZArray<keycode> keymap;
    keymap.resize(nkeys);

    for(zsize i = 0; i < knum; ++i){
        keycode kc = layer.readleu16();
        zu8 kp = matrix2layout[i];
        if(kp){
            keymap[kp - 1] = kc;
        }
    }
    layers.push(keymap);
}

ZArray<ZArray<Keymap::keycode> > Keymap::getKeycodeLayout(zu8 l) const {
    ZArray<ZArray<keycode>> keymap;
    ZArray<keycode> layer;
    for(zsize i = 0; i < nkeys; ++i){
        layer.push(layers[l][i]);
        if(wlayout[i].newrow){
            keymap.push(layer);
            layer.clear();
        }
    }
    return keymap;
}

ZBinary Keymap::toMatrix() const {
    ZBinary matrices;

    for(zsize l = 0; l < layers.size(); ++l){
        zu16 matrix[rows][cols];
        memset(matrix, 0, sizeof(matrix));

        // Put keycodes in matrix
        for(zsize i = 0; i < nkeys; ++i){
            const zu16 kc = layers[l][i];
            matrix[wlayout[i].row][wlayout[i].col] = kc;
        }

        // Serialize matrix
        for(zsize r = 0; r < rows; ++r){
            for(zsize c = 0; c < cols; ++c){
                matrices.writeleu16(matrix[r][c]);
            }
        }
    }

    return matrices;
}

void Keymap::printLayers() const{
    for(zsize l = 0; l < layers.size(); ++l){
        bool blank = true;
        for(zsize i = 0; i < nkeys; ++i){
            if(layers[l][i] != KC_NO){
                blank = false;
                break;
            }
        }

        if(blank){
            LOG("Layer " << l << ": BLANK");
        } else {
            LOG("Layer " << l << ":");
            ZString lstr;
            if(mwidth){
                lstr += ZString('=', (mwidth*2)+1);
                lstr += "\n|";
            }
            for(zsize i = 0; i < nkeys; ++i){
                const zu16 kc = layers[l][i];
                const zu8 width = wlayout[i].width;
                ZString kstr;
                if(width){
                    ZString str = keycodeAbbrev(kc);
                    kstr += ZString(str).substr(0, str.size()/2).lpad(' ', width-1);
                    kstr += ZString(str).substr(str.size()/2).rpad(' ', width+1);
                    kstr.last() = '|';
                } else {
                    ZString str = keycodeName(kc);
                    kstr += (str + ", ");
                }
                lstr += kstr;
                if(wlayout[i].newrow){
                    if(mwidth){
                        lstr += "\n";
                        lstr += ZString('=', (mwidth*2)+1);
                        if(i < (nkeys-1))
                            lstr += "\n|";
                    }
                }
            }
            RLOG(lstr << ZLog::NEWLN);
        }
    }
}

void Keymap::printMatrix() const {
    LOG("Matrix Dump: " << rows << "/" << cols << " r/c, " << layers.size() << " layers");

    for(zsize l = 0; l < layers.size(); ++l){
        zu16 matrix[rows][cols];
        memset(matrix, 0, sizeof(matrix));

        for(zsize i = 0; i < nkeys; ++i){
            const zu16 kc = layers[l][i];
            matrix[wlayout[i].row][wlayout[i].col] = kc;
        }

        LOG("Layer " << l << ":");

        RLOG("{" << ZLog::NEWLN);
        for(zsize r = 0; r < rows; ++r){
            ZString line = "  { ";
            for(zsize c = 0; c < cols; ++c){
                line += (ZString(keycodeName(matrix[r][c]) + ",").pad(' ', 20) + " ");
            }
            line += "},";
            RLOG(line << ZLog::NEWLN);
        }
        RLOG("}" << ZLog::NEWLN);
    }
}

ZArray<int> Keymap::getLayer(zu8 layer) const {
    ZArray<int> keymap;
    for(zsize i = 0; i < layers[layer].size(); ++i){
        keymap.push(layers[layer][i]);
    }
    return keymap;
}

ZArray<ZString> Keymap::getLayerAbbrev(zu8 layer) const {
    ZArray<ZString> keymap;
    for(zsize i = 0; i < layers[layer].size(); ++i){
        keymap.push(keycodeAbbrev(layers[layer][i]));
    }
    return keymap;
}

zu16 Keymap::rowCount(zu8 row) const{
    zu8 r = 0;
    zu16 width = 0;
    for(zsize i = 0; i < nkeys; ++i){
        width += wlayout[i].width;
        if(wlayout[i].newrow){
            if(r == row)
                return width;
            width = 0;
            r++;
        }
    }
    return width;
}

ZArray<int> Keymap::getLayout() const {
    ZArray<int> layout;
    for(zsize i = 0; i < wlayout.size(); ++i){
        layout.push(wlayout[i].width | (wlayout[i].space ? LAYOUT_SP : 0));
        if(wlayout[i].newrow)
            layout.push(-1);
    }
    return layout;
}

zu16 Keymap::layoutRC2K(zu8 r, zu8 c) const {
    zu8 row = 0;
    zu8 col = 0;
    for(zsize i = 0; i < nkeys; ++i){
        if(row == r && col == c)
            return i;
        col++;
        if(wlayout[i].newrow){
            row++;
            col = 0;
        }
    }
    return 0xFFFF;
}

zu16 Keymap::keyOffset(zu8 l, zu16 k) const {
    const zu8 row = wlayout[k].row;
    const zu8 col = wlayout[k].col;
    const zu16 lsize = rows * cols;
    return ((lsize * l) + ((row * cols) + col)) * sizeof(zu16);
}

Keymap::keycode Keymap::toKeycode(ZString name) const {
    if(kname_map.contains(name)){
        return kname_map[name];
    } else {
        return KC_NO;
    }
}

ZString Keymap::keycodeName(keycode kc) const {
    if(kcode_map.contains(kc)){
        return kcode_map[kc].name;
    } else {
        return "0x" + ZString::ItoS(kc, 16, 4);
    }
}

ZString Keymap::keycodeAbbrev(keycode kc) const {
    if(kcode_map.contains(kc)){
        return kcode_map[kc].abbrev;
    } else {
        return "0x" + ZString::ItoS(kc, 16, 4);
    }
}

ZString Keymap::keycodeDesc(keycode kc) const {
    if(kcode_map.contains(kc)){
        return kcode_map[kc].desc;
    } else if(kc >= SAFE_RANGE){
        return "Custom keycode 0x" + ZString::ItoS(kc, 16, 4);
    } else {
        return "Unknown keycode 0x" + ZString::ItoS(kc, 16, 4);
    }
}

const ZArray<Keymap::Keycode> &Keymap::getAllKeycodes(){
    return keycodes;
}

ZArray<ZString> Keymap::getKnownLayouts()
{
    ZArray<ZString> list;
    zu64 n = keymaps_json_size / sizeof(unsigned);
    for(zu64 i = 0; i < n; ++i){
        zu64 len = keymaps_json_sizes[i];
        ZString str(keymaps_json[i], len);
        auto json = nlohmann::json::parse(str.str());
//        LOG(i << " " << len << " " << json["name"].get<std::string>());
        list.push(json["name"].get<std::string>());
    }
    return list;
}
