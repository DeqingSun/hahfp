/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2010

FILE NAME
    headset_dut.c

DESCRIPTION
	Place the headset into Device Under Test (DUT) mode    

NOTES
		

*/


/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_dut.h"

#include <pio.h>
#include <test.h>


#include <ps.h>
#include <string.h>
#include "headset_configmanager.h"



#define TX_START_TEST_MODE_LO_FREQ  (2441)
#define TX_START_TEST_MODE_LEVEL    (63)
#define TX_START_TEST_MODE_MOD_FREQ (0)


/****************************************************************************
DESCRIPTION
  	This function is called to place the headset into DUT mode
*/
void enterDutMode(void)
{
	ConnectionEnterDutMode();
}

/****************************************************************************
DESCRIPTION
  	Entera continuous transmit test mode
*/
void enterTxContinuousTestMode ( void ) 
{
    TestTxStart (TX_START_TEST_MODE_LO_FREQ, 
                 TX_START_TEST_MODE_LEVEL, 
                 TX_START_TEST_MODE_MOD_FREQ) ;
}

/****************************************************************************
DESCRIPTION
  	This function is called to determine if the headset should enter DUT mode.
*/
bool checkForDUTModeEntry( void )
{
	if(theHeadset.conf->input_PIO.dut_pio != INPUT_PIO_UNASSIGNED)
	{
		/* Enable the DUT mode PIO line */
		PioSetDir32(1 << theHeadset.conf->input_PIO.dut_pio, 0);

		/* If it's high, enter DUT mode */
		if(PioGet32() & (1 << theHeadset.conf->input_PIO.dut_pio))
		{
			enterDutMode();
			return TRUE;
		}
	}	

	return FALSE;
}



/****************************************************************************
DESCRIPTION
  	Enter service mode - headset changers local name and enters discoverable 
	mode
*/
void enterServiceMode( void )
{
        /* Reset pair devices list */
    configManagerReset();
            
        /* Entered service mode */
    MessageSend(&theHeadset.task, EventServiceModeEntered, 0);

        /* Power on immediately */
    MessageSend(&theHeadset.task, EventPowerOn, 0);   
		/* Ensure pairing never times out */
	theHeadset.conf->timeouts.PairModeTimeout_s = 0;		
	theHeadset.conf->timeouts.PairModeTimeoutIfPDL_s = 0;
	
    MessageSend(&theHeadset.task, EventEnterPairing, 0); 
            
    ConnectionReadLocalAddr(&theHeadset.task);
}

/****************************************************************************
DESCRIPTION
  	convert decimal to ascii
*/
static char hex(int hex_dig)
{
    if (hex_dig < 10)
        return '0' + hex_dig;
    else
        return 'A' + hex_dig - 10;
}

/****************************************************************************
DESCRIPTION
  	handle a local bdaddr request and continue to enter service mode
*/
void DutHandleLocalAddr(CL_DM_LOCAL_BD_ADDR_CFM_T *cfm)
{
    char new_name[32];
    
    uint16 i = 0 ;
 
    new_name[i++] = hex((cfm->bd_addr.nap & 0xF000) >> 12);
    new_name[i++] = hex((cfm->bd_addr.nap & 0x0F00) >> 8);
    new_name[i++] = hex((cfm->bd_addr.nap & 0x00F0) >> 4);
    new_name[i++] = hex((cfm->bd_addr.nap & 0x000F) >> 0);
    
    new_name[i++] = hex((cfm->bd_addr.uap & 0x00F0) >> 4);
    new_name[i++] = hex((cfm->bd_addr.uap & 0x000F) >> 0);
    
    new_name[i++] = hex((cfm->bd_addr.lap & 0xF00000) >> 20);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x0F0000) >> 16);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x00F000) >> 12);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x000F00) >> 8);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x0000F0) >> 4);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x00000F) >> 0);


        /*terminate the string*/
    new_name[i] = 0;
   
 	ConnectionChangeLocalName(i, (uint8 *)new_name);
}
