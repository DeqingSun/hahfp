/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    connection_private.h
    
DESCRIPTION

*/

#ifndef    CONNECTION_BLUESTACK_HANDLER_H_
#define    CONNECTION_BLUESTACK_HANDLER_H_


/****************************************************************************
NAME    
    connectionBluestackHandler    

DESCRIPTION
    Connection task handler for incoming Bluestack primitives

RETURNS
    void    
*/
void connectionBluestackHandler(Task task, MessageId id, Message message);


#endif    /* CONNECTION_BLUESTACK_HANDLER_H_ */
