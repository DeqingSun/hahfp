#include <bdaddr.h>

bool BdaddrIsZero(const bdaddr *b)
{ return !b->nap && !b->uap && !b->lap; }
