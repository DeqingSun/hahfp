#include <bdaddr.h>

void BdaddrSetZero(bdaddr *b)
{
    b->nap = 0;
    b->uap = 0;
    b->lap = 0;
}
