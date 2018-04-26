#ifndef KEYCODES_H
#define KEYCODES_H

#define KC_TRANSPARENT  1

/* USB HID Keyboard/Keypad Usage(0x07) */
enum hid_keyboard_keypad_usage {
    KC_NO               = 0x00,
    KC_ROLL_OVER,
    KC_POST_FAIL,
    KC_UNDEFINED,
    KC_A,
    KC_B,
    KC_C,
    KC_D,
    KC_E,
    KC_F,
    KC_G,
    KC_H,
    KC_I,
    KC_J,
    KC_K,
    KC_L,
    KC_M,               /* 0x10 */
    KC_N,
    KC_O,
    KC_P,
    KC_Q,
    KC_R,
    KC_S,
    KC_T,
    KC_U,
    KC_V,
    KC_W,
    KC_X,
    KC_Y,
    KC_Z,
    KC_1,
    KC_2,
    KC_3,               /* 0x20 */
    KC_4,
    KC_5,
    KC_6,
    KC_7,
    KC_8,
    KC_9,
    KC_0,
    KC_ENTER,
    KC_ESCAPE,
    KC_BSPACE,
    KC_TAB,
    KC_SPACE,
    KC_MINUS,
    KC_EQUAL,
    KC_LBRACKET,
    KC_RBRACKET,        /* 0x30 */
    KC_BSLASH,          /* \ (and |) */
    KC_NONUS_HASH,      /* Non-US # and ~ (Typically near the Enter key) */
    KC_SCOLON,          /* ; (and :) */
    KC_QUOTE,           /* ' and " */
    KC_GRAVE,           /* Grave accent and tilde */
    KC_COMMA,           /* , and < */
    KC_DOT,             /* . and > */
    KC_SLASH,           /* / and ? */
    KC_CAPSLOCK,
    KC_F1,
    KC_F2,
    KC_F3,
    KC_F4,
    KC_F5,
    KC_F6,
    KC_F7,              /* 0x40 */
    KC_F8,
    KC_F9,
    KC_F10,
    KC_F11,
    KC_F12,
    KC_PSCREEN,
    KC_SCROLLLOCK,
    KC_PAUSE,
    KC_INSERT,
    KC_HOME,
    KC_PGUP,
    KC_DELETE,
    KC_END,
    KC_PGDOWN,
    KC_RIGHT,
    KC_LEFT,            /* 0x50 */
    KC_DOWN,
    KC_UP,
    KC_NUMLOCK,
    KC_KP_SLASH,
    KC_KP_ASTERISK,
    KC_KP_MINUS,
    KC_KP_PLUS,
    KC_KP_ENTER,
    KC_KP_1,
    KC_KP_2,
    KC_KP_3,
    KC_KP_4,
    KC_KP_5,
    KC_KP_6,
    KC_KP_7,
    KC_KP_8,            /* 0x60 */
    KC_KP_9,
    KC_KP_0,
    KC_KP_DOT,
    KC_NONUS_BSLASH,    /* Non-US \ and | (Typically near the Left-Shift key) */
    KC_APPLICATION,
    KC_POWER,
    KC_KP_EQUAL,
    KC_F13,
    KC_F14,
    KC_F15,
    KC_F16,
    KC_F17,
    KC_F18,
    KC_F19,
    KC_F20,
    KC_F21,             /* 0x70 */
    KC_F22,
    KC_F23,
    KC_F24,
    KC_EXECUTE,
    KC_HELP,
    KC_MENU,
    KC_SELECT,
    KC_STOP,
    KC_AGAIN,
    KC_UNDO,
    KC_CUT,
    KC_COPY,
    KC_PASTE,
    KC_FIND,
    KC__MUTE,
    KC__VOLUP,          /* 0x80 */
    KC__VOLDOWN,
    KC_LOCKING_CAPS,    /* locking Caps Lock */
    KC_LOCKING_NUM,     /* locking Num Lock */
    KC_LOCKING_SCROLL,  /* locking Scroll Lock */
    KC_KP_COMMA,
    KC_KP_EQUAL_AS400,  /* equal sign on AS/400 */
    KC_INT1,
    KC_INT2,
    KC_INT3,
    KC_INT4,
    KC_INT5,
    KC_INT6,
    KC_INT7,
    KC_INT8,
    KC_INT9,
    KC_LANG1,           /* 0x90 */
    KC_LANG2,
    KC_LANG3,
    KC_LANG4,
    KC_LANG5,
    KC_LANG6,
    KC_LANG7,
    KC_LANG8,
    KC_LANG9,
    KC_ALT_ERASE,
    KC_SYSREQ,
    KC_CANCEL,
    KC_CLEAR,
    KC_PRIOR,
    KC_RETURN,
    KC_SEPARATOR,
    KC_OUT,             /* 0xA0 */
    KC_OPER,
    KC_CLEAR_AGAIN,
    KC_CRSEL,
    KC_EXSEL,           /* 0xA4 */

    /* NOTE: 0xA5-DF are used for internal special purpose */

#if 0
    /* NOTE: Following codes(0xB0-DD) are not used. Leave them for reference. */
    KC_KP_00            = 0xB0,
    KC_KP_000,
    KC_THOUSANDS_SEPARATOR,
    KC_DECIMAL_SEPARATOR,
    KC_CURRENCY_UNIT,
    KC_CURRENCY_SUB_UNIT,
    KC_KP_LPAREN,
    KC_KP_RPAREN,
    KC_KP_LCBRACKET,    /* { */
    KC_KP_RCBRACKET,    /* } */
    KC_KP_TAB,
    KC_KP_BSPACE,
    KC_KP_A,
    KC_KP_B,
    KC_KP_C,
    KC_KP_D,
    KC_KP_E,            /* 0xC0 */
    KC_KP_F,
    KC_KP_XOR,
    KC_KP_HAT,
    KC_KP_PERC,
    KC_KP_LT,
    KC_KP_GT,
    KC_KP_AND,
    KC_KP_LAZYAND,
    KC_KP_OR,
    KC_KP_LAZYOR,
    KC_KP_COLON,
    KC_KP_HASH,
    KC_KP_SPACE,
    KC_KP_ATMARK,
    KC_KP_EXCLAMATION,
    KC_KP_MEM_STORE,    /* 0xD0 */
    KC_KP_MEM_RECALL,
    KC_KP_MEM_CLEAR,
    KC_KP_MEM_ADD,
    KC_KP_MEM_SUB,
    KC_KP_MEM_MUL,
    KC_KP_MEM_DIV,
    KC_KP_PLUS_MINUS,
    KC_KP_CLEAR,
    KC_KP_CLEAR_ENTRY,
    KC_KP_BINARY,
    KC_KP_OCTAL,
    KC_KP_DECIMAL,
    KC_KP_HEXADECIMAL,  /* 0xDD */
#endif

    /* Modifiers */
    KC_LCTRL            = 0xE0,
    KC_LSHIFT,
    KC_LALT,
    KC_LGUI,
    KC_RCTRL,
    KC_RSHIFT,
    KC_RALT,
    KC_RGUI,

    /* NOTE: 0xE8-FF are used for internal special purpose */
};

/* Special keycodes */
/* NOTE: 0xA5-DF and 0xE8-FF are used for internal special purpose */
enum internal_special_keycodes {
    /* System Control */
    KC_SYSTEM_POWER     = 0xA5,
    KC_SYSTEM_SLEEP,
    KC_SYSTEM_WAKE,

    /* Media Control */
    KC_AUDIO_MUTE,
    KC_AUDIO_VOL_UP,
    KC_AUDIO_VOL_DOWN,
    KC_MEDIA_NEXT_TRACK,
    KC_MEDIA_PREV_TRACK,
    KC_MEDIA_STOP,
    KC_MEDIA_PLAY_PAUSE,
    KC_MEDIA_SELECT,
    KC_MEDIA_EJECT,
    KC_MAIL,
    KC_CALCULATOR,
    KC_MY_COMPUTER,
    KC_WWW_SEARCH,
    KC_WWW_HOME,
    KC_WWW_BACK,
    KC_WWW_FORWARD,
    KC_WWW_STOP,
    KC_WWW_REFRESH,
    KC_WWW_FAVORITES,
    KC_MEDIA_FAST_FORWARD,
    KC_MEDIA_REWIND,    /* 0xBC */

    /* Fn key */
    KC_FN0              = 0xC0,
    KC_FN1,
    KC_FN2,
    KC_FN3,
    KC_FN4,
    KC_FN5,
    KC_FN6,
    KC_FN7,
    KC_FN8,
    KC_FN9,
    KC_FN10,
    KC_FN11,
    KC_FN12,
    KC_FN13,
    KC_FN14,
    KC_FN15,

    KC_FN16             = 0xD0,
    KC_FN17,
    KC_FN18,
    KC_FN19,
    KC_FN20,
    KC_FN21,
    KC_FN22,
    KC_FN23,
    KC_FN24,
    KC_FN25,
    KC_FN26,
    KC_FN27,
    KC_FN28,
    KC_FN29,
    KC_FN30,
    KC_FN31,            /* 0xDF */

    /**************************************/
    /* 0xE0-E7 for Modifiers. DO NOT USE. */
    /**************************************/

    /* Mousekey */
    KC_MS_UP            = 0xF0,
    KC_MS_DOWN,
    KC_MS_LEFT,
    KC_MS_RIGHT,
    KC_MS_BTN1,
    KC_MS_BTN2,
    KC_MS_BTN3,
    KC_MS_BTN4,
    KC_MS_BTN5,         /* 0xF8 */
    /* Mousekey wheel */
    KC_MS_WH_UP,
    KC_MS_WH_DOWN,
    KC_MS_WH_LEFT,
    KC_MS_WH_RIGHT,     /* 0xFC */
    /* Mousekey accel */
    KC_MS_ACCEL0,
    KC_MS_ACCEL1,
    KC_MS_ACCEL2        /* 0xFF */
};

enum quantum_keycodes {
    // Ranges used in shortucuts - not to be used directly
    QK_TMK                = 0x0000,
    QK_TMK_MAX            = 0x00FF,
    QK_MODS               = 0x0100,
    QK_LCTL               = 0x0100,
    QK_LSFT               = 0x0200,
    QK_LALT               = 0x0400,
    QK_LGUI               = 0x0800,
    QK_RMODS_MIN          = 0x1000,
    QK_RCTL               = 0x1100,
    QK_RSFT               = 0x1200,
    QK_RALT               = 0x1400,
    QK_RGUI               = 0x1800,
    QK_MODS_MAX           = 0x1FFF,
    QK_FUNCTION           = 0x2000,
    QK_FUNCTION_MAX       = 0x2FFF,
    QK_MACRO              = 0x3000,
    QK_MACRO_MAX          = 0x3FFF,
    QK_LAYER_TAP          = 0x4000,
    QK_LAYER_TAP_MAX      = 0x4FFF,
    QK_TO                 = 0x5000,
    QK_TO_MAX             = 0x50FF,
    QK_MOMENTARY          = 0x5100,
    QK_MOMENTARY_MAX      = 0x51FF,
    QK_DEF_LAYER          = 0x5200,
    QK_DEF_LAYER_MAX      = 0x52FF,
    QK_TOGGLE_LAYER       = 0x5300,
    QK_TOGGLE_LAYER_MAX   = 0x53FF,
    QK_ONE_SHOT_LAYER     = 0x5400,
    QK_ONE_SHOT_LAYER_MAX = 0x54FF,
    QK_ONE_SHOT_MOD       = 0x5500,
    QK_ONE_SHOT_MOD_MAX   = 0x55FF,
#ifndef DISABLE_CHORDING
    QK_CHORDING           = 0x5600,
    QK_CHORDING_MAX       = 0x56FF,
#endif
    QK_TAP_DANCE          = 0x5700,
    QK_TAP_DANCE_MAX      = 0x57FF,
    QK_LAYER_TAP_TOGGLE   = 0x5800,
    QK_LAYER_TAP_TOGGLE_MAX = 0x58FF,
    QK_LAYER_MOD          = 0x5900,
    QK_LAYER_MOD_MAX      = 0x59FF,
#ifdef STENO_ENABLE
    QK_STENO              = 0x5A00,
    QK_STENO_BOLT         = 0x5A30,
    QK_STENO_GEMINI       = 0x5A31,
    QK_STENO_MAX          = 0x5A3F,
#endif
#ifdef SWAP_HANDS_ENABLE
    QK_SWAP_HANDS         = 0x5B00,
    QK_SWAP_HANDS_MAX     = 0x5BFF,
#endif
    QK_MOD_TAP            = 0x6000,
    QK_MOD_TAP_MAX        = 0x7FFF,
#if defined(UNICODEMAP_ENABLE) && defined(UNICODE_ENABLE)
    #error "Cannot enable both UNICODEMAP && UNICODE"
#endif
#ifdef UNICODE_ENABLE
    QK_UNICODE            = 0x8000,
    QK_UNICODE_MAX        = 0xFFFF,
#endif
#ifdef UNICODEMAP_ENABLE
    QK_UNICODE_MAP        = 0x8000,
    QK_UNICODE_MAP_MAX    = 0x83FF,
#endif

    // Loose keycodes - to be used directly

    RESET = 0x5C00,
    DEBUG,
    MAGIC_SWAP_CONTROL_CAPSLOCK,
    MAGIC_CAPSLOCK_TO_CONTROL,
    MAGIC_SWAP_LALT_LGUI,
    MAGIC_SWAP_RALT_RGUI,
    MAGIC_NO_GUI,
    MAGIC_SWAP_GRAVE_ESC,
    MAGIC_SWAP_BACKSLASH_BACKSPACE,
    MAGIC_HOST_NKRO,
    MAGIC_SWAP_ALT_GUI,
    MAGIC_UNSWAP_CONTROL_CAPSLOCK,
    MAGIC_UNCAPSLOCK_TO_CONTROL,
    MAGIC_UNSWAP_LALT_LGUI,
    MAGIC_UNSWAP_RALT_RGUI,
    MAGIC_UNNO_GUI,
    MAGIC_UNSWAP_GRAVE_ESC,
    MAGIC_UNSWAP_BACKSLASH_BACKSPACE,
    MAGIC_UNHOST_NKRO,
    MAGIC_UNSWAP_ALT_GUI,
    MAGIC_TOGGLE_NKRO,
    GRAVE_ESC,

    // Leader key
#ifndef DISABLE_LEADER
    KC_LEAD,
#endif

    // Auto Shift setup
    KC_ASUP,
    KC_ASDN,
    KC_ASRP,
    KC_ASTG,
    KC_ASON,
    KC_ASOFF,

    // Audio on/off/toggle
    AU_ON,
    AU_OFF,
    AU_TOG,

#ifdef FAUXCLICKY_ENABLE
    // Faux clicky
    FC_ON,
    FC_OFF,
    FC_TOG,
#endif

    // Music mode on/off/toggle
    MU_ON,
    MU_OFF,
    MU_TOG,

    // Music mode cycle
    MU_MOD,

    // Music voice iterate
    MUV_IN,
    MUV_DE,

    // Midi
#if !MIDI_ENABLE_STRICT || (defined(MIDI_ENABLE) && defined(MIDI_BASIC))
    MI_ON,
    MI_OFF,
    MI_TOG,
#endif

#if !MIDI_ENABLE_STRICT || (defined(MIDI_ENABLE) && defined(MIDI_ADVANCED))
    MIDI_TONE_MIN,

#if !MIDI_ENABLE_STRICT || MIDI_TONE_KEYCODE_OCTAVES > 0
    MI_C = MIDI_TONE_MIN,
    MI_Cs,
    MI_Db = MI_Cs,
    MI_D,
    MI_Ds,
    MI_Eb = MI_Ds,
    MI_E,
    MI_F,
    MI_Fs,
    MI_Gb = MI_Fs,
    MI_G,
    MI_Gs,
    MI_Ab = MI_Gs,
    MI_A,
    MI_As,
    MI_Bb = MI_As,
    MI_B,
#endif

#if !MIDI_ENABLE_STRICT || MIDI_TONE_KEYCODE_OCTAVES > 1
    MI_C_1,
    MI_Cs_1,
    MI_Db_1 = MI_Cs_1,
    MI_D_1,
    MI_Ds_1,
    MI_Eb_1 = MI_Ds_1,
    MI_E_1,
    MI_F_1,
    MI_Fs_1,
    MI_Gb_1 = MI_Fs_1,
    MI_G_1,
    MI_Gs_1,
    MI_Ab_1 = MI_Gs_1,
    MI_A_1,
    MI_As_1,
    MI_Bb_1 = MI_As_1,
    MI_B_1,
#endif

#if !MIDI_ENABLE_STRICT || MIDI_TONE_KEYCODE_OCTAVES > 2
    MI_C_2,
    MI_Cs_2,
    MI_Db_2 = MI_Cs_2,
    MI_D_2,
    MI_Ds_2,
    MI_Eb_2 = MI_Ds_2,
    MI_E_2,
    MI_F_2,
    MI_Fs_2,
    MI_Gb_2 = MI_Fs_2,
    MI_G_2,
    MI_Gs_2,
    MI_Ab_2 = MI_Gs_2,
    MI_A_2,
    MI_As_2,
    MI_Bb_2 = MI_As_2,
    MI_B_2,
#endif

#if !MIDI_ENABLE_STRICT || MIDI_TONE_KEYCODE_OCTAVES > 3
    MI_C_3,
    MI_Cs_3,
    MI_Db_3 = MI_Cs_3,
    MI_D_3,
    MI_Ds_3,
    MI_Eb_3 = MI_Ds_3,
    MI_E_3,
    MI_F_3,
    MI_Fs_3,
    MI_Gb_3 = MI_Fs_3,
    MI_G_3,
    MI_Gs_3,
    MI_Ab_3 = MI_Gs_3,
    MI_A_3,
    MI_As_3,
    MI_Bb_3 = MI_As_3,
    MI_B_3,
#endif

#if !MIDI_ENABLE_STRICT || MIDI_TONE_KEYCODE_OCTAVES > 4
    MI_C_4,
    MI_Cs_4,
    MI_Db_4 = MI_Cs_4,
    MI_D_4,
    MI_Ds_4,
    MI_Eb_4 = MI_Ds_4,
    MI_E_4,
    MI_F_4,
    MI_Fs_4,
    MI_Gb_4 = MI_Fs_4,
    MI_G_4,
    MI_Gs_4,
    MI_Ab_4 = MI_Gs_4,
    MI_A_4,
    MI_As_4,
    MI_Bb_4 = MI_As_4,
    MI_B_4,
#endif

#if !MIDI_ENABLE_STRICT || MIDI_TONE_KEYCODE_OCTAVES > 5
    MI_C_5,
    MI_Cs_5,
    MI_Db_5 = MI_Cs_5,
    MI_D_5,
    MI_Ds_5,
    MI_Eb_5 = MI_Ds_5,
    MI_E_5,
    MI_F_5,
    MI_Fs_5,
    MI_Gb_5 = MI_Fs_5,
    MI_G_5,
    MI_Gs_5,
    MI_Ab_5 = MI_Gs_5,
    MI_A_5,
    MI_As_5,
    MI_Bb_5 = MI_As_5,
    MI_B_5,
#endif

#if !MIDI_ENABLE_STRICT || MIDI_TONE_KEYCODE_OCTAVES > 5
    MIDI_TONE_MAX = MI_B_5,
#elif MIDI_TONE_KEYCODE_OCTAVES > 4
    MIDI_TONE_MAX = MI_B_4,
#elif MIDI_TONE_KEYCODE_OCTAVES > 3
    MIDI_TONE_MAX = MI_B_3,
#elif MIDI_TONE_KEYCODE_OCTAVES > 2
    MIDI_TONE_MAX = MI_B_2,
#elif MIDI_TONE_KEYCODE_OCTAVES > 1
    MIDI_TONE_MAX = MI_B_1,
#elif MIDI_TONE_KEYCODE_OCTAVES > 0
    MIDI_TONE_MAX = MI_B,
#endif

    MIDI_OCTAVE_MIN,
    MI_OCT_N2 = MIDI_OCTAVE_MIN,
    MI_OCT_N1,
    MI_OCT_0,
    MI_OCT_1,
    MI_OCT_2,
    MI_OCT_3,
    MI_OCT_4,
    MI_OCT_5,
    MI_OCT_6,
    MI_OCT_7,
    MIDI_OCTAVE_MAX = MI_OCT_7,
    MI_OCTD, // octave down
    MI_OCTU, // octave up

    MIDI_TRANSPOSE_MIN,
    MI_TRNS_N6 = MIDI_TRANSPOSE_MIN,
    MI_TRNS_N5,
    MI_TRNS_N4,
    MI_TRNS_N3,
    MI_TRNS_N2,
    MI_TRNS_N1,
    MI_TRNS_0,
    MI_TRNS_1,
    MI_TRNS_2,
    MI_TRNS_3,
    MI_TRNS_4,
    MI_TRNS_5,
    MI_TRNS_6,
    MIDI_TRANSPOSE_MAX = MI_TRNS_6,
    MI_TRNSD, // transpose down
    MI_TRNSU, // transpose up

    MIDI_VELOCITY_MIN,
    MI_VEL_1 = MIDI_VELOCITY_MIN,
    MI_VEL_2,
    MI_VEL_3,
    MI_VEL_4,
    MI_VEL_5,
    MI_VEL_6,
    MI_VEL_7,
    MI_VEL_8,
    MI_VEL_9,
    MI_VEL_10,
    MIDI_VELOCITY_MAX = MI_VEL_10,
    MI_VELD, // velocity down
    MI_VELU, // velocity up

    MIDI_CHANNEL_MIN,
    MI_CH1 = MIDI_CHANNEL_MIN,
    MI_CH2,
    MI_CH3,
    MI_CH4,
    MI_CH5,
    MI_CH6,
    MI_CH7,
    MI_CH8,
    MI_CH9,
    MI_CH10,
    MI_CH11,
    MI_CH12,
    MI_CH13,
    MI_CH14,
    MI_CH15,
    MI_CH16,
    MIDI_CHANNEL_MAX = MI_CH16,
    MI_CHD, // previous channel
    MI_CHU, // next channel

    MI_ALLOFF, // all notes off

    MI_SUS, // sustain
    MI_PORT, // portamento
    MI_SOST, // sostenuto
    MI_SOFT, // soft pedal
    MI_LEG,  // legato

    MI_MOD, // modulation
    MI_MODSD, // decrease modulation speed
    MI_MODSU, // increase modulation speed
#endif // MIDI_ADVANCED

    // Backlight functionality
    BL_ON,
    BL_OFF,
    BL_DEC,
    BL_INC,
    BL_TOGG,
    BL_STEP,
    BL_BRTG,

    // RGB functionality
    RGB_TOG,
    RGB_MODE_FORWARD,
    RGB_MODE_REVERSE,
    RGB_HUI,
    RGB_HUD,
    RGB_SAI,
    RGB_SAD,
    RGB_VAI,
    RGB_VAD,
    RGB_MODE_PLAIN,
    RGB_MODE_BREATHE,
    RGB_MODE_RAINBOW,
    RGB_MODE_SWIRL,
    RGB_MODE_SNAKE,
    RGB_MODE_KNIGHT,
    RGB_MODE_XMAS,
    RGB_MODE_GRADIENT,

    // Left shift, open paren
    KC_LSPO,

    // Right shift, close paren
    KC_RSPC,

    // Shift, Enter
    KC_SFTENT,

    // Printing
    PRINT_ON,
    PRINT_OFF,

    // output selection
    OUT_AUTO,
    OUT_USB,
#ifdef BLUETOOTH_ENABLE
    OUT_BT,
#endif

#ifdef KEY_LOCK_ENABLE
    KC_LOCK,
#endif

#ifdef TERMINAL_ENABLE
    TERM_ON,
    TERM_OFF,
#endif

    // always leave at the end
    SAFE_RANGE
};

#endif // KEYCODES_H
