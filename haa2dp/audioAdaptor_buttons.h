/*
    This file was autogenerated by buttonparse
*/

#ifndef _AUDIOADAPTOR_BUTTONS_H
#define _AUDIOADAPTOR_BUTTONS_H

#include <message.h>

/* messages sent to the client */
enum
{
	BUTTON_DEVICE_DISCOVER_REQ = 1000 /* base value */,
	BUTTON_PWR_ON_REQ,
	BUTTON_PWR_OFF_REQ,
	VOLUME_UP,
	VOLUME_DN,
	CHARGER_RAW,
	PIO_RAW
};

typedef enum
{
	sMFB,
	sVREG,
	sVUP,
	sVDN,
	Unknown
} InternalState;

typedef struct
{
	uint16 pio;
} PIO_RAW_T;

typedef struct
{
	bool chg;
	bool vreg;
} CHARGER_RAW_T;

typedef struct
{
	InternalState store_held;
	InternalState double_press;
	uint16 pio_raw_bits;
	uint16 pskey_wakeup;
	uint16 store_bits;
	uint16 store_count;
	uint16 timed_id;
} PioStoredState;

typedef struct
{
	InternalState store_held;
	InternalState double_press;
	uint32 pio_raw_bits;
	uint32 store_bits;
	uint16 store_count;
	uint16 timed_id;
} ExtStoredState;

typedef struct
{
	TaskData task;
	Task client;
	PioStoredState pio_states;
	ExtStoredState ext_states;
} PioState;

void pioInit(PioState *state, Task client);

void pioExternal(PioState *pioState, uint32 external_and, uint32 external_xor);

#endif
