
#include "common.h"

void gotoxy(int x, int y)
{
    printf("\033[%d;%dH", x, y);
}
