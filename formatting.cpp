#include "formatting.h"

#ifdef ANSI_FORMATTING
    #define SRG(n) "\x1b["#n"m"
#else
    #define SRG(n) ""
#endif

namespace formatting {
    const char
        *REVERT_ALL=SRG(0),
        *BOLD_WEIGHT=SRG(1),
        *LIGHT_WEIGHT=SRG(2),
        *ITALIC=SRG(3),
        *UNDERLINE=SRG(4),
        *REVERT_WEIGHT=SRG(22),
        *REVERT_ITALIC=SRG(23),
        *REVERT_UNDERLINE=SRG(24)
    ;

    namespace colors {
        namespace fg {
            const char
                *BLACK=SRG(30),
                *RED=SRG(31),
                *GREEN=SRG(32),
                *YELLOW=SRG(33),
                *BLUE=SRG(34),
                *MAGENTA=SRG(35),
                *CYAN=SRG(36),
                *WHITE=SRG(37),
                *BR_BLACK=SRG(90),
                *BR_RED=SRG(91),
                *BR_GREEN=SRG(92),
                *BR_YELLOW=SRG(93),
                *BR_BLUE=SRG(94),
                *BR_MAGENTA=SRG(95),
                *BR_CYAN=SRG(96),
                *BR_WHITE=SRG(97),
                *REVERT=SRG(39)
            ;
        }
        namespace bg {
            const char
                *BLACK=SRG(40),
                *RED=SRG(41),
                *GREEN=SRG(42),
                *YELLOW=SRG(43),
                *BLUE=SRG(44),
                *MAGENTA=SRG(45),
                *CYAN=SRG(46),
                *WHITE=SRG(47),
                *BR_BLACK=SRG(100),
                *BR_RED=SRG(101),
                *BR_GREEN=SRG(102),
                *BR_YELLOW=SRG(103),
                *BR_BLUE=SRG(104),
                *BR_MAGENTA=SRG(105),
                *BR_CYAN=SRG(106),
                *BR_WHITE=SRG(107),
                *REVERT=SRG(49)
            ;
        }
    }
}
#undef SRG