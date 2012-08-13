/* Copyright (C) Cambridge Silicon Radio Limited 2010 */
/* Part of HeadsetSDK-Stereo R110.0 */
/* 
    Copyright (C) Cambridge Silicon Radio Limited 2010
    Part of HeadsetSDK-Stereo R110.0
    This is a generic header file for all BlueLab libraries.
*/

/*!
 @file library.h
 @brief Common header file for Bluelab libraries

This header file defines common data types and values required for
all Bluelab libraries.
*/
#ifndef _LIBRARY_H
#define _LIBRARY_H


/*!   Default RFCOMM Channels provided to RFCOMM based services.
*/


/*! @brief  Default SPP Service Channel */
#define     SPP_DEFAULT_CHANNEL   (0x01)

/*! @brief  Default DUN Service Channel */
#define     DUN_DEFAULT_CHANNEL   (0x02)

/*! @brief  Default FTP Service Channel */
#define     FTP_DEFAULT_CHANNEL   (0x03)

/*! @brief  Default OPP Service Channel */
#define     OPP_DEFAULT_CHANNEL   (0x04)

/*! @brief  Default PBAP Service Channel for PCE */
#define     PBAPC_DEFAULT_CHANNEL (0x05)

/*! @brief  Default PBAP Service Channel for PSE */
#define     PBAPS_DEFAULT_CHANNEL (0x06)

/*! @brief  Default MAP Service Channel for MNS */
#define     MAPC_DEFAULT_CHANNEL  (0x07)

/*! @brief  Default MAP Service Channel for MAS */
#define     MAPS_DEFAULT_CHANNEL  (0x08)

/*! @brief  Default HFP AG service channel */
#define     AGHFP_DEFAULT_CHANNEL (0x09)

/* @brief   Default HFP Device Service Channel */
#define     HFP_DEFAULT_CHANNEL   (0x0A)

/* @brief   Default HSP service channel */
#define     HSP_DEFAULT_CHANNEL   (0x0B)




/*!   Base values for library messages 
*/


/*! @brief  Message base for profile libraries */
#define     CL_MESSAGE_BASE         0x5000
#define     SPP_MESSAGE_BASE        0x5100
#define     DUN_MESSAGE_BASE        0x5200

#define     GOEP_MESSAGE_BASE       0x5300
#define     OBEX_MESSAGE_BASE       0x5350
#define     FTPC_MESSAGE_BASE	    0x5400
#define     FTPS_MESSAGE_BASE       0x5450
#define     OPPC_MESSAGE_BASE	    0x5500
#define     OPPS_MESSAGE_BASE	    0x5550
#define     PBAPC_MESSAGE_BASE	    0x5600
#define     PBAPS_MESSAGE_BASE	    0x5650
#define     MAPC_MESSAGE_BASE       0x5700
#define     MAPS_MESSAGE_BASE       0x5750

#define     HFP_MESSAGE_BASE	    0x5A00
#define     AGHFP_MESSAGE_BASE      0x5B00
#define     WBS_MESSAGE_BASE        0x5C00
#define     A2DP_MESSAGE_BASE	    0x5D00
#define     AVRCP_MESSAGE_BASE      0x5E00

#define     HDP_MESSAGE_BASE        0x6000

#define     HID_MESSAGE_BASE        0x6100
#define     HIDKP_MESSAGE_BASE	    0x6150



/*! @brief  Message base for non profile libraries */
#define     AUDIO_DOWNSTREAM_MESSAGE_BASE 0x7000
#define     BATTERY_MESSAGE_BASE          0x7100
#define     CODEC_MESSAGE_BASE            0x7200
#define     AUDIO_UPSTREAM_MESSAGE_BASE   0x7300


#endif
