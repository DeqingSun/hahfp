/*!
  @file bdaddr.h
  
  @brief Helper routines for Bluetooth addresses
*/

#ifndef BDADDR_H_
#define BDADDR_H_

#include <bdaddr_.h>

/*!
    @brief Sets the Bluetooth address passed to zero.
*/
void BdaddrSetZero(bdaddr *);


/*!
    @brief Compares two Bluetooth addresses and returns TRUE if
    they are the same, else returns FALSE.
*/
bool BdaddrIsSame(const bdaddr *, const bdaddr *);

/*!
    @brief Returns TRUE if the Bluetooth address passed is zero, else
    returns FALSE.
*/
bool BdaddrIsZero(const bdaddr *);


#endif /* BDADDR_H_ */

