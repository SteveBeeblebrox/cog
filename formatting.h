#ifndef FORMATTING_H
#define FORMATTING_H

namespace formatting {
    extern const char
        *REVERT_ALL,
        *BOLD_WEIGHT,
        *LIGHT_WEIGHT,
        *ITALIC,
        *UNDERLINE,
        *REVERT_WEIGHT,
        *REVERT_ITALIC,
        *REVERT_UNDERLINE
    ;

    namespace colors {
        namespace fg {
            extern const char
                *BLACK,
                *RED,
                *GREEN,
                *YELLOW,
                *BLUE,
                *MAGENTA,
                *CYAN,
                *WHITE,
                *BR_BLACK,
                *BR_RED,
                *BR_GREEN,
                *BR_YELLOW,
                *BR_BLUE,
                *BR_MAGENTA,
                *BR_CYAN,
                *BR_WHITE,
                *REVERT
            ;
        }
        namespace bg {
            extern const char
                *BLACK,
                *RED,
                *GREEN,
                *YELLOW,
                *BLUE,
                *MAGENTA,
                *CYAN,
                *WHITE,
                *BR_BLACK,
                *BR_RED,
                *BR_GREEN,
                *BR_YELLOW,
                *BR_BLUE,
                *BR_MAGENTA,
                *BR_CYAN,
                *BR_WHITE,
                *REVERT
            ;
        }
    }
    #undef SRG
}
#endif