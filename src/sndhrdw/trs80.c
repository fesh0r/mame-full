#include "driver.h"

int     trs80_sh_sound_init(const char * gamename)
{
        if (sound_start())
                return 1;
        return 0;
}
