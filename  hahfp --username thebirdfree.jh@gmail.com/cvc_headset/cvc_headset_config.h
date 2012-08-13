// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio plc 2007             http://www.csr.com
// %%version
//
// $Revision$ $Date$
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    CVC configuration file
//
//    This file is used to include/exclude CVC modules from the build.
//    To include a module define it as 1.  To exclude a module define it as 0.
//
// NOTE
//    A clean build must be executed after making modifications to this file.
//    (i.e. "Rebuild All")
//
// *****************************************************************************
.ifndef CVC_HEADSET_CONFIG_H
.define CVC_HEADSET_CONFIG_H


.define uses_DCBLOCK                        1

.define uses_RCV_PEQ                        1

.define uses_SND_PEQ                        1

.define uses_NSVOLUME                       1

.define uses_AEC                            1

.define uses_SND_NS                         1

.define uses_RCV_AGC                        1

.define uses_SND_AGC                        1

.define uses_RCV_NS                         1

.define uses_AEQ                            1

.define uses_PLC                            1


// Bit-mask flags to return in SPI status, which tells the Parameter Manager
// (Windows Realtime Tuning GUI) which modules are included in the build.  The
// mask is also written to the kap file so that the build configuration can be
// identified using a text editor.

.define CVC_HEADSET_CONFIG_FLAG\
                        uses_RCV_AGC                    *0x008000 + \
                        uses_DCBLOCK                    *0x004000 + \
                        uses_RCV_PEQ                    *0x002000 + \
                        uses_SND_PEQ                    *0x001000 + \
                        uses_NSVOLUME                   *0x000800 + \
                        uses_AEC                        *0x000400 + \
                        uses_SND_NS                     *0x000200 + \
                        uses_SND_AGC                    *0x000080 + \
                        uses_RCV_NS                     *0x000100 + \
                        uses_PLC                        *0x000040 + \
                        uses_AEQ                        *0x000020                        
                        
                        
.endif //headset_config_H