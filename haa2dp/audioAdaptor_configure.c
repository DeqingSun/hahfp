/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2009
Part of Audio-Adaptor-SDK 2009.R1

DESCRIPTION
    Support code for reading and writing the Persistent Store, and buffering of device data.
*/


#include "audioAdaptor_private.h"
#include "audioAdaptor_configure.h"
#include "audioAdaptor_powermanager.h"
#include "audioAdaptor_dev_instance.h"

#include <ps.h>
#include <bdaddr.h>
#include <panic.h>
#include <string.h>
#include <stdlib.h>


#define DEVICE_DATA_SIZE 4
#define DEVICE_BDADDR_OFFSET 0
#define DEVICE_INFO_OFFSET   3
#define DEVICE_PROFILES_BITPOS  12
#define DEVICE_PROFILES_BITMASK 0xF
#define DEVICE_PINIDX_BITPOS  4
#define DEVICE_PINIDX_BITMASK 0xF
#define DEVICE_ORDERING_BITPOS  0
#define DEVICE_ORDERING_BITMASK 0xF


static mvdPsKeyData *s_ps_key_cache;


/****************************************************************************
  LOCAL FUNCTIONS
*/
      
/****************************************************************************
NAME
    erasePsKey

DESCRIPTION
    Erases the specified PS Key.

*/
static bool erasePsKey(uint16 key)
{
    if (!PsStore(key, 0, 0))
    {
        return TRUE;
    }

    return FALSE;
}


/****************************************************************************
NAME
    writePsKey

DESCRIPTION
    Writes the specified value to the PS Key.

*/
static bool writePsKey(uint16 key, const void *buffer, uint16 length)
{
    if (PsStore(key, buffer, length) == length)
    {
        return TRUE;
    }

    return FALSE;
}


/****************************************************************************
NAME
    readPsKey

DESCRIPTION
    Reads the value from the PS Key.

*/
static bool readPsKey(uint16 key, void *buffer, uint16 length)
{
    if (PsRetrieve(key, buffer, length) == length)
    {
        return TRUE;
    }

    return FALSE;
}


/****************************************************************************
NAME
    expandBdaddr

DESCRIPTION
    Convert a Bluetooth address from an array of uint16 values into a bdaddr structure.

*/
static void expandBdaddr (bdaddr *bd_addr, uint16 *buffer)
{
    bd_addr->nap = *buffer;
    bd_addr->uap = (*(buffer+1) >> 8) & 0xff;
    bd_addr->lap = (uint32)(*(buffer+1) & 0xff) << 16;
    bd_addr->lap |= (uint32)*(buffer+2);
}



/****************************************************************************
NAME
    contractBdaddr

DESCRIPTION
    Convert a Bluetooth address from a bdaddr structure into an array of uint16 values.

*/
static void contractBdaddr (bdaddr *bd_addr, uint16 *buffer)
{
    *buffer = bd_addr->nap;
    *(buffer+1) = (bd_addr->uap & 0xff) << 8;
    *(buffer+1) |= (uint16)((bd_addr->lap >> 16) & 0xff);
    *(buffer+2) = (uint16)(bd_addr->lap & 0xffff);
}



/****************************************************************************
NAME
    cacheCreate

DESCRIPTION
    Memory allocation for holding PS Key data.

*/
static void cacheCreate (void)
{
    if (s_ps_key_cache == NULL)
    {
        s_ps_key_cache = (mvdPsKeyData *)malloc(sizeof(mvdPsKeyData));
        PanicNull(s_ps_key_cache);
        
        memset(s_ps_key_cache, 0, sizeof(mvdPsKeyData));
    }
}


/****************************************************************************
NAME
    cacheLoad

DESCRIPTION
    Load PS Key data into memory.

*/
static void cacheLoad (void)
{
    if (s_ps_key_cache == NULL)
    {
        uint16 size;
        
        cacheCreate();
        
        size = PsRetrieve(PSKEY_REMOTE_DEVICE_LIST, (void *)s_ps_key_cache->remote_device_list.buffer, PS_REMOTE_DEVICE_LIST_MAX_SIZE);
        s_ps_key_cache->remote_device_list.count = size/PS_REMOTE_DEVICE_LIST_ELEMENT_SIZE;
    }
}


/****************************************************************************
NAME
    cacheStore

DESCRIPTION
    Write PS Key data from memory back into PS.

*/
static void cacheStore (void)
{
    if (s_ps_key_cache == NULL)
    {
        cacheCreate();
    }
    
    /* Don't store PIO config, pin code list or locally supported profiles - these are read only */
    if (s_ps_key_cache->remote_device_list.count)
    {
        writePsKey(PSKEY_REMOTE_DEVICE_LIST, (const void *)&s_ps_key_cache->remote_device_list.buffer, s_ps_key_cache->remote_device_list.count*PS_REMOTE_DEVICE_LIST_ELEMENT_SIZE);
    }
    else
    {
        erasePsKey(PSKEY_REMOTE_DEVICE_LIST);
    }
    
}


/****************************************************************************
NAME
    getDeviceBdaddr

DESCRIPTION
    Retrieves the Bluetooth address from the specified index of the paired device list.

*/
static void getDeviceBdaddr(uint16 device_idx, bdaddr *bd_addr)
{
    expandBdaddr(bd_addr, &s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_BDADDR_OFFSET]);
}


/****************************************************************************
NAME
    getDeviceOrdering

DESCRIPTION
    Returns the ordering position for the paired device with the specified index.
    Lower ordering positions are the more recently used devices.

*/
static uint16 getDeviceOrdering(uint16 device_idx)
{
    return (uint16)(s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] >> DEVICE_ORDERING_BITPOS) & DEVICE_ORDERING_BITMASK;
}


/****************************************************************************
NAME
    getDevicePinIdx

DESCRIPTION
    Returns the PIN index for the paired device with the specified index.
    

*/
static uint16 getDevicePinIdx(uint16 device_idx)
{
    return (uint16)(s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] >> DEVICE_PINIDX_BITPOS) & DEVICE_PINIDX_BITMASK;
}


/****************************************************************************
NAME
    getDeviceProfiles

DESCRIPTION
    Returns the profiles supported for the paired device at the specified index.

*/
static uint16 getDeviceProfiles(uint16 device_idx)
{
    return (mvdProfiles)(s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] >> DEVICE_PROFILES_BITPOS) & DEVICE_PROFILES_BITMASK;
}


/****************************************************************************
NAME
    setDeviceBdaddr

DESCRIPTION
    Sets the Bluetooth address for the paired device at the specified index.

*/
static void setDeviceBdaddr(uint16 device_idx, bdaddr *bd_addr)
{
    contractBdaddr(bd_addr, &s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_BDADDR_OFFSET]);
}


/****************************************************************************
NAME
    setDeviceOrdering

DESCRIPTION
    Sets the ordering position for the paired device at the specified index.

*/
static void setDeviceOrdering(uint16 device_idx, uint16 ordering)
{
    s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] &= ~(DEVICE_ORDERING_BITMASK << DEVICE_ORDERING_BITPOS);
    s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] |= (ordering & DEVICE_ORDERING_BITMASK) << DEVICE_ORDERING_BITPOS;
}


/****************************************************************************
NAME
    setDevicePinIdx

DESCRIPTION
    Sets the PIN index for the paired device at the specified index.

*/
static void setDevicePinIdx(uint16 device_idx, uint16 pin_idx)
{
    s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] &= ~(DEVICE_PINIDX_BITMASK << DEVICE_PINIDX_BITPOS);
    s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] |= (pin_idx & DEVICE_PINIDX_BITMASK) << DEVICE_PINIDX_BITPOS;
}


/****************************************************************************
NAME
    setDeviceProfiles

DESCRIPTION
    Sets the supported profiles for the paired device at the specified index.

*/
static void setDeviceProfiles(uint16 device_idx, mvdProfiles profiles)
{
    s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] &= ~(DEVICE_PROFILES_BITMASK << DEVICE_PROFILES_BITPOS);
    s_ps_key_cache->remote_device_list.buffer[(device_idx*DEVICE_DATA_SIZE)+DEVICE_INFO_OFFSET] |= ((uint16)profiles & DEVICE_PROFILES_BITMASK) << DEVICE_PROFILES_BITPOS;
}


/****************************************************************************
NAME
    findDevice

DESCRIPTION
    Finds the device in the paired device list with the specified Bluetooth address.
    
RETURNS    
    The index of the found device in device_idx.
    Bool return value if the search was successful.

*/
static bool findDevice (uint16 *device_idx, const bdaddr *bd_addr)
{
    for (*device_idx=0; *device_idx<s_ps_key_cache->remote_device_list.count; (*device_idx)++)
    {
        bdaddr device_bdaddr;
        
        getDeviceBdaddr(*device_idx, &device_bdaddr);
        if ( BdaddrIsSame(bd_addr, &device_bdaddr) )
        {
            return TRUE;
        }
    }

    return FALSE;
}


/****************************************************************************
NAME
    findLeastRecentDevice

DESCRIPTION
    Finds the least recently used device in the paired device list.
    
RETURNS    
    The index of the found device in device_idx.
    Bool return value if the search was successful.

*/
static bool findLeastRecentDevice (uint16 *device_idx)
{
    for (*device_idx=0; *device_idx<s_ps_key_cache->remote_device_list.count; (*device_idx)++)
    {
        if ( getDeviceOrdering(*device_idx)==(MIN_DEVICE_ORDERING + s_ps_key_cache->remote_device_list.count-1) )
        {
            return TRUE;
        }
    }
    
    return FALSE;
}


/****************************************************************************
NAME
    getPairedDevice

DESCRIPTION
    Finds the paired device at the specified index.
    
RETURNS
    The Bluetooth address in bd_addr.
    The PIN index in pin_idx.
    The supported profiles in profiles;
    Bool return value if the search was successful.

*/
static bool getPairedDevice (uint16 device_idx, bdaddr *bd_addr, uint16 *pin_idx, mvdProfiles *profiles)
{    
    if ( device_idx < s_ps_key_cache->remote_device_list.count )
    {
        getDeviceBdaddr(device_idx, bd_addr);
        *pin_idx = getDevicePinIdx(device_idx);
        *profiles = getDeviceProfiles(device_idx);
        
        return TRUE;
    }
    else
    {
        BdaddrSetZero(bd_addr);
        *pin_idx = 0;
        *profiles = ProfileNone;
        
        return FALSE;
    }
}


/****************************************************************************
NAME
    setPairedDevice

DESCRIPTION
    Sets the Bluetooth address, PIN index, and supported profiles of the paired device
    at the specified index.
    
RETURNS
    Bool return value if the setting of the values was successful.

*/
static bool setPairedDevice (uint16 device_idx, bdaddr *bd_addr, uint16 pin_idx, mvdProfiles profiles)
{
    if ( device_idx < s_ps_key_cache->remote_device_list.count )
    {
        setDeviceBdaddr(device_idx, bd_addr);
        setDevicePinIdx(device_idx, pin_idx);
        setDeviceProfiles(device_idx, profiles);
            
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/****************************************************************************
NAME
    setMostRecentDevice

DESCRIPTION
    Sets the paired device at the specified index, to be marked as the most recently used device.
    
RETURNS
    Bool return value if the the device was set to be the most recent successfully.

*/
static bool setMostRecentDevice (uint16 device_idx)
{
    if ( device_idx < s_ps_key_cache->remote_device_list.count )
    {
        uint16 idx;
        uint16 device_ordering = getDeviceOrdering(device_idx);
        
        for (idx=0; idx<s_ps_key_cache->remote_device_list.count; idx++)
        {
            if ( idx!=device_idx )
            {
                uint16 ordering = getDeviceOrdering(idx);
                
                if ( !device_ordering || (ordering < device_ordering) )
                {
                    setDeviceOrdering(idx, ordering+1);
                }
            }
        }
        
        setDeviceOrdering(device_idx, MIN_DEVICE_ORDERING);

        return TRUE;
    }
    
    return FALSE;
}


/****************************************************************************
  MAIN FUNCTIONS
*/

/****************************************************************************
NAME
    configureGetPinCode

DESCRIPTION
    Retrives the next 4 digit PIN code to use for the specified remote device.
    
*/
char *configureGetPinCode(devInstanceTaskData *inst)
{
    static char pin_code[] = "0000";      
    
    return pin_code;
}


/****************************************************************************
NAME
    configureFindMostRecentDevice

DESCRIPTION
    Returns if the most recently used has been found. The index of the device
    is returned in device_idx.
    
*/
bool configureFindMostRecentDevice (uint16 *device_idx)
{
    for (*device_idx=0; *device_idx < s_ps_key_cache->remote_device_list.count; (*device_idx)++)
    {
        if ( getDeviceOrdering(*device_idx)==MIN_DEVICE_ORDERING )
        {
            return TRUE;
        }
    }
    
    return FALSE;
}


/****************************************************************************
NAME
    configureGetPowerManagerConfig

DESCRIPTION
    Reads the battery monitoring configuration from PS.
    Returns if successful or not.
    
*/
bool configureGetPowerManagerConfig(void)
{
    power_config_type config;
 
    memset(&config, 0, sizeof(power_config_type));
    
    /* Read in the battery monitoring configuration */
    if (readPsKey(PSKEY_BATTERY_CONFIG, (void *)(&(config.battery)), PS_BATTERY_CONFIG_SIZE))
    {
        /* Setup the power manager */
        powerManagerConfig(&config);  
        
        return TRUE;
    }
    
    return FALSE;

}

    
/****************************************************************************
NAME
    configureGetConfigCache

DESCRIPTION
    Reads the PS configuration into memory.
    
*/
void configureGetConfigCache (void)
{
    cacheLoad();
}


/****************************************************************************
NAME
    configureResetPairedDeviceListBuffer

DESCRIPTION
    Clears the Paired device list in memory and PS.
    
*/
bool configureResetPairedDeviceListBuffer(void)
{
    PsStore(PSKEY_REMOTE_DEVICE_LIST, 0, 0);
    memset(s_ps_key_cache->remote_device_list.buffer, 0, PS_REMOTE_DEVICE_LIST_MAX_SIZE);
    s_ps_key_cache->remote_device_list.count = 0;
    
    return TRUE;
}


/****************************************************************************
NAME
    configureGetSupportedProfiles

DESCRIPTION
    Sets the locally supported profiles..
    Returns if the supported profiles were read successfully.
    
*/
bool configureGetSupportedProfiles (void)
{
	/* Always a2dp only */
	the_app->supported_profiles = ProfileA2dp;

    DEBUG_CONFIG(("PS_LOCAL_PROFILES = 0x%X\n", (uint16)the_app->supported_profiles));
    return TRUE;
}

    
/****************************************************************************
NAME
    configureGetMostRecentDevice

DESCRIPTION
    Returns the device instance of the most recently used device. 
    Returns NULL if not found.
    
*/
devInstanceTaskData *configureGetMostRecentDevice (void)
{
    uint16 device_idx;
    uint16 pin_idx;
    mvdProfiles remote_profiles;
    devInstanceTaskData *inst;
    bdaddr bd_addr;
    
    if ( configureFindMostRecentDevice(&device_idx) )
    {
        getPairedDevice(device_idx, &bd_addr, &pin_idx, &remote_profiles);
        inst = devInstanceFindFromBddr(&bd_addr, TRUE);
        
        if (inst == NULL)
            return NULL;
        
        inst->pin_idx = (pin_idx > 0) ? pin_idx : 1;
        inst->remote_profiles = remote_profiles;
        inst->pin_authorised = (inst->pin_idx > 0) ? TRUE : FALSE;
    
        return inst;
    }
    
    return NULL;
}


/****************************************************************************
NAME
    configureGetBdaddrMostRecentDevice

DESCRIPTION
    Returns the Bluetooth address of the most recently used device in bd_addr. 
    Returns if successfully found or not.
    
*/
bool configureGetBdaddrMostRecentDevice (bdaddr *bd_addr)
{
    uint16 device_idx;
    uint16 pin_idx;
    mvdProfiles remote_profiles;
     
    if ( configureFindMostRecentDevice(&device_idx) )
    {
        getPairedDevice(device_idx, bd_addr, &pin_idx, &remote_profiles);
        
        return TRUE;
    }
    
    return FALSE;
}


/****************************************************************************
NAME
    configureGetBdaddrNextRecentDevice

DESCRIPTION
    Returns Bluetooth address of the next most recently used device after the one
    specified by the Bluetooth address.
    Returns if successfully found or not.
    
*/
bool configureGetBdaddrNextRecentDevice (bdaddr *bd_addr)
{
    uint16 device_idx;
    uint16 pin_idx;
    mvdProfiles remote_profiles;
    
    if ( !BdaddrIsZero(bd_addr) && findDevice(&device_idx, bd_addr) )
    {
        uint16 device_ordering = getDeviceOrdering(device_idx);
        
        if ( device_ordering < (MIN_DEVICE_ORDERING + s_ps_key_cache->remote_device_list.count-1) )
        {
            uint16 idx;
            
            for (idx=0; idx<s_ps_key_cache->remote_device_list.count; idx++)
            {
                if ( idx!=device_idx )
                {
                    uint16 ordering = getDeviceOrdering(idx);
                    
                    if ( ordering == device_ordering+1 )
                    {
                        getPairedDevice(idx, bd_addr, &pin_idx, &remote_profiles);
                        
                        return TRUE;
                    }
                }
            }
        }

        return FALSE;
        
    }
    
    return configureGetBdaddrMostRecentDevice(bd_addr);
}


/****************************************************************************
NAME
    configureGetBdaddrNextDisconnectedDevice

DESCRIPTION
    Returns Bluetooth address of the most recently used disconnected device,
    that is not the one specifed by the passed in Bluetooth address. 
    Returns if successfully found or not.
    
*/
bool configureGetBdaddrNextDisconnectedDevice(bdaddr *bd_addr)
{
    bdaddr next_addr;
    
    if (configureGetBdaddrMostRecentDevice(&next_addr))
    {
        uint16 i;
        bool next_device = FALSE;
        
        while (1)
        {
            next_device = FALSE;
            for (i = 0; i < MAX_NUM_DEV_CONNECTIONS; i++)
            {
                if (the_app->dev_inst[i] != NULL)
                {           
                    if (BdaddrIsSame(&the_app->dev_inst[i]->bd_addr, &next_addr))
                    {
                        if (configureGetBdaddrNextRecentDevice(&next_addr))
                        {
                            next_device = TRUE;
                            break;
                        }
                        return FALSE;
                    }
                    *bd_addr = next_addr;
                    return TRUE;
                }                
             }
             if (!next_device)
             {
                *bd_addr = next_addr;
                return TRUE;
             }
        }
    }
    
    return FALSE;
}


/****************************************************************************
NAME
    configureGetDeviceInfo

DESCRIPTION
    Returns paired device information for the device with the specified Bluetooth address.
    
*/
bool configureGetDeviceInfo (const bdaddr *bd_addr, uint16 *pin_idx, mvdProfiles *remote_profiles, bool *pin_authorised)
{
    uint16 device_idx;
    bdaddr addr;
    
    *pin_idx = 0;
    *remote_profiles = ProfileNone;
    *pin_authorised = FALSE;
   
    if ( findDevice(&device_idx, bd_addr) )
    {
        getPairedDevice(device_idx, &addr, pin_idx, remote_profiles);
        *pin_authorised = (*pin_idx > 0) ? TRUE : FALSE;
        *pin_idx = (*pin_idx > 0) ? *pin_idx : 1;
       
        return TRUE;
    }    
    
    return FALSE;
}


/****************************************************************************
NAME
    configureClearRecentDeviceList

DESCRIPTION
    Clears the Paired device list in PS.
    
*/
void configureClearRecentDeviceList (void)
{
    s_ps_key_cache->remote_device_list.count = 0;
    cacheStore();
}


/****************************************************************************
NAME
    configureStoreCurrentPairedDevice

DESCRIPTION
    Store the paired device and associated data. This will become the most recent device.
    
*/
bool configureStoreCurrentPairedDevice (devInstanceTaskData *inst)
{
    uint16 device_idx;
    
    if ( !findDevice(&device_idx, &inst->bd_addr) )
    {    /* Can't find an existing entry for this bdaddr */
        if (s_ps_key_cache->remote_device_list.count < PS_REMOTE_DEVICE_LIST_MAX_ENTRIES)
        {    /* Create a new entry */
            device_idx = s_ps_key_cache->remote_device_list.count;
            s_ps_key_cache->remote_device_list.count++;
        }
        else
        {    /* Overwrite the oldest entry */
            findLeastRecentDevice(&device_idx);
        }
    }
    
    setPairedDevice(device_idx, &inst->bd_addr, inst->pin_idx, inst->remote_profiles);
    setMostRecentDevice(device_idx);
    
    cacheStore();
    
    return TRUE;
}
    

/****************************************************************************
NAME
    configureIsPinRequested

DESCRIPTION
    Returns a PIN code is currently being requested for the specified remote device.
    
*/
bool configureIsPinRequested(devInstanceTaskData *inst)
{
    if (inst != NULL)
        return inst->pin_requested;
    else
        return FALSE;
}


/****************************************************************************
NAME
    configureSetPinAuthorised

DESCRIPTION
    Sets the paired device with the specified Bluetooth address as being
    authorised or un-authorised.
    
*/
bool configureSetPinAuthorised (bdaddr *bd_addr, bool authorised)
{
    devInstanceTaskData *inst = devInstanceFindFromBddr(bd_addr, FALSE);
    
    if (inst != NULL)
    {    /* Same device as last time */
        inst->pin_requested = FALSE;
        inst->pin_authorised = authorised;
        
        return TRUE;
    }
    
    return FALSE;
}


/****************************************************************************
NAME
    configureIsPinListExhausted

DESCRIPTION
    Returns if all PIN codes have been tried for the specified remote device.
    
*/
bool configureIsPinListExhausted(devInstanceTaskData *inst)
{
    if (inst == NULL)
        return FALSE;
        
    return inst->pin_list_exhausted;
}


