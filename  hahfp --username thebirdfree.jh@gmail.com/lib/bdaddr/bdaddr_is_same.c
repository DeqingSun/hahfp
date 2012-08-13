#include <bdaddr.h>

bool BdaddrIsSame(const bdaddr *a, const bdaddr *b)
{ return a->nap == b->nap && a->uap == b->uap && a->lap == b->lap; }
