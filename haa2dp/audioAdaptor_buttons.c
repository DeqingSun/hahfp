/*
    This file was autogenerated by buttonparse
*/

#include "audioAdaptor_buttons.h"

#include <pio.h>
#include <app/message/system_message.h>
#include <panic.h>
#include <ps.h>
#include <stdlib.h>
#include <charger.h>

enum {internal_pio_timer_message = 0, double_pio_press_timer, pio_external_changed, internal_ext_timer_message, double_ext_press_timer};

static InternalState pio_encode(uint16 pressed)
{
	switch(pressed)
	{
		case (1UL<<0) : return sMFB;
		default : return Unknown;
	}
}

static InternalState external_encode(uint32 pressed)
{
	switch(pressed)
	{
		case (1UL<<17) : return sVREG;
		default : return Unknown;
	}
}

typedef struct
{
	unsigned release:1;
	unsigned double_tap:1;
	unsigned timeout:15;
	unsigned external:1;
	MessageId id;
} EnterMessage;

/* Messages sent on state entry */
static const EnterMessage enter_messages_sMFB[] = { { 1, 0, 0, 0, BUTTON_DEVICE_CONNECT_REQ },
												{ 0, 1, 800, 0, BUTTON_CONNECT_SECOND_DEVICE_REQ } };

static const EnterMessage enter_messages_sVREG[] = { { 1, 0, 0, 1, BUTTON_PWR_RELEASE } };

static const struct
{
	uint16 count; const EnterMessage *send;
} enter_messages[] = {
	{ 2, enter_messages_sMFB },
	{ 1, enter_messages_sVREG }
};

static void send_pio_enter_messages(PioState *pioState, InternalState state)
{
	uint16 init_count = enter_messages[state].count;
	uint16 count = init_count;
	const EnterMessage *p = enter_messages[state].send;
	while(count--)
	{
		if (p->external)
		{
			p++;
			continue;
		}
		if (p->double_tap)
		{
			if (pioState->pio_states.double_press == state)
			{
				if (MessageCancelAll(&pioState->task, double_pio_press_timer))
				{
					pioState->pio_states.store_held = Unknown;
					MessageSend(pioState->client, p->id, 0);
				}
				else
					MessageSendLater(&pioState->task, double_pio_press_timer, 0, p->timeout);
			}
			else
			{
				pioState->pio_states.double_press = state;
				(void) MessageCancelAll(&pioState->task, double_pio_press_timer);
				MessageSendLater(&pioState->task, double_pio_press_timer, 0, p->timeout);
			}
		}
		else
		if (!p->release)
			MessageSend(pioState->client, p->id, 0);
		else
		{
			pioState->pio_states.store_held = state;
			pioState->pio_states.store_count = init_count - (count + 1);
		}
		p++;
	}
}

static void send_ext_enter_messages(PioState *pioState, InternalState state)
{
	uint16 init_count = enter_messages[state].count;
	uint16 count = init_count;
	const EnterMessage *p = enter_messages[state].send;
	while(count--)
	{
		if (!p->external)
		{
			p++;
			continue;
		}
		if (p->double_tap)
		{
			if (pioState->ext_states.double_press == state)
			{
				if (MessageCancelAll(&pioState->task, double_ext_press_timer))
				{
					pioState->ext_states.store_held = Unknown;
					MessageSend(pioState->client, p->id, 0);
				}
				else
					MessageSendLater(&pioState->task, double_ext_press_timer, 0, p->timeout);
			}
			else
			{
				pioState->ext_states.double_press = state;
				(void) MessageCancelAll(&pioState->task, double_ext_press_timer);
				MessageSendLater(&pioState->task, double_ext_press_timer, 0, p->timeout);
			}
		}
		else
		if (!p->release)
			MessageSend(pioState->client, p->id, 0);
		else
		{
			pioState->ext_states.store_held = state;
			pioState->ext_states.store_count = init_count - (count + 1);
		}
		p++;
	}
}

/* Timed messages to be sent to the client */
typedef struct
{
	unsigned repeat:1;
	unsigned msec:15; /* Limit to 32767ms. Sounds reasonable. */
	unsigned msecRepeat:15;
	unsigned release:1;
	unsigned external:1;
	MessageId id;
} TimedMessage;

static const TimedMessage timed_messages_sMFB[] =
{
	{ 0, 2000, 0, 1, 0, BUTTON_DEVICE_DISCOVER_REQ },
	{ 0, 10000, 0, 0, 0, BUTTONS_CLEAR_PDL_REQ }
};
static const TimedMessage timed_messages_sVREG[] =
{
	{ 0, 4000, 0, 0, 1, BUTTON_PWR_OFF_REQ }
};

static const struct
{
	uint16 count;
	const TimedMessage *send;
} timed_messages[] =
{
	{ 2, timed_messages_sMFB },
	{ 1, timed_messages_sVREG }
};

static void send_pio_timed_message(PioState *pioState, const TimedMessage *p, int hold_repeat)
{
	const TimedMessage **m = (const TimedMessage **) PanicNull(malloc(sizeof(const TimedMessage *)));
	*m = p;
	if (hold_repeat)
		MessageSendLater(&pioState->task, internal_pio_timer_message, m, p->msecRepeat);
	else
		MessageSendLater(&pioState->task, internal_pio_timer_message, m, p->msec);
}

static void send_pio_timed_messages(PioState *pioState, InternalState state)
{
	uint16 count = timed_messages[state].count;
	const TimedMessage *p = timed_messages[state].send;
	while(count--)
	{
		if (!p->external)
			send_pio_timed_message(pioState, p, 0);
		p++;
	}
}

static void send_ext_timed_message(PioState *pioState, const TimedMessage *p, int hold_repeat)
{
	const TimedMessage **m = (const TimedMessage **) PanicNull(malloc(sizeof(const TimedMessage *)));
	*m = p;
	if (hold_repeat)
		MessageSendLater(&pioState->task, internal_ext_timer_message, m, p->msecRepeat);
	else
		MessageSendLater(&pioState->task, internal_ext_timer_message, m, p->msec);
}

static void send_ext_timed_messages(PioState *pioState, InternalState state)
{
	uint16 count = timed_messages[state].count;
	const TimedMessage *p = timed_messages[state].send;
	while(count--)
	{
		if (p->external)
			send_ext_timed_message(pioState, p, 0);
		p++;
	}
}

static void pioChanged(Task task, PioState *pioState, uint16 pio_bits)
{
	InternalState next;
	uint16 raw_bits = 0;
	uint16 mState = pio_bits;
	bool pio_changed = TRUE;

	mState &= ~ 0;
	raw_bits = (pio_bits & 0);
	if (raw_bits != pioState->pio_states.pio_raw_bits)
	{
		PIO_RAW_T *message = malloc(sizeof(PIO_RAW_T));
		message->pio = raw_bits;
		MessageSend(pioState->client, PIO_RAW, message);
		pioState->pio_states.pio_raw_bits = raw_bits;
	}

	next = pio_encode(mState);

	if (mState == pioState->pio_states.store_bits)
		pio_changed = FALSE;

	if (pio_changed)
	{
		if ((pioState->pio_states.store_held != Unknown) && (next != pioState->pio_states.store_held))
		{
			uint16 changed = mState ^ pioState->pio_states.store_bits;
			uint16 released = changed & pioState->pio_states.store_bits;
			if (released == pioState->pio_states.store_bits)
			{
				uint16 count, i;
				const EnterMessage *p = enter_messages[pioState->pio_states.store_held].send;
				count = enter_messages[pioState->pio_states.store_held].count;
				for (i = count; i > 0; i--)
				{
					if (p[i-1].release)
						MessageSend(pioState->client, p[i-1].id, 0);
				}
			}
			pioState->pio_states.store_held = Unknown;
		}

		if (pioState->pio_states.timed_id)
		{
			MessageSend(pioState->client, pioState->pio_states.timed_id, 0);
			pioState->pio_states.timed_id = 0;
		}

		(void) MessageCancelAll(task, internal_pio_timer_message);
		if(next != Unknown)
		{
			send_pio_enter_messages(pioState, next);
			send_pio_timed_messages(pioState, next);
		}
		pioState->pio_states.store_bits = mState;
	}
}

static void externalChanged(Task task, PioState *pioState, uint32 external_bits)
{
	InternalState next;
	uint32 raw_bits = 0;
	uint32 mState = external_bits;

	mState &= ~ (1UL<<16);
	raw_bits = (external_bits & (1UL<<16));
	if (raw_bits != pioState->ext_states.pio_raw_bits)
	{
		CHARGER_RAW_T *message = malloc(sizeof(CHARGER_RAW_T));
		message->chg = (raw_bits>>16) & 0x1;
		message->vreg = (raw_bits>>17) & 0x1;
		MessageSend(pioState->client, CHARGER_RAW, message);
		pioState->ext_states.pio_raw_bits = raw_bits;
	}

	next = external_encode(mState);

	if ((pioState->ext_states.store_held != Unknown) && (next != pioState->ext_states.store_held))
	{
		uint32 changed = mState ^ pioState->ext_states.store_bits;
		uint32 released = changed & pioState->ext_states.store_bits;
		if (released == pioState->ext_states.store_bits)
		{
			uint16 count, i;
			const EnterMessage *p = enter_messages[pioState->ext_states.store_held].send;
			count = enter_messages[pioState->ext_states.store_held].count;
			for (i = count; i > 0; i--)
			{
				if (p[i-1].release)
					MessageSend(pioState->client, p[i-1].id, 0);
			}
		}
		pioState->ext_states.store_held = Unknown;
	}

	if (pioState->ext_states.timed_id)
	{
		MessageSend(pioState->client, pioState->ext_states.timed_id, 0);
		pioState->ext_states.timed_id = 0;
	}

	(void) MessageCancelAll(task, internal_ext_timer_message);
	if(next != Unknown)
	{
		send_ext_enter_messages(pioState, next);
		send_ext_timed_messages(pioState, next);
	}
	pioState->ext_states.store_bits = mState;
}

typedef struct { uint32 xor; uint32 and;} PioExternalMessage;

static void pioHandler(Task task, MessageId id, Message data)
{
	PioState *pioState = (PioState *) task;
	switch(id)
	{
	case internal_pio_timer_message:
	{
		const TimedMessage **m = (const TimedMessage **) data;
		const TimedMessage *p = *m;
		uint16 temp_timed_id = pioState->pio_states.timed_id;

		if (p->release)
			pioState->pio_states.timed_id = p->id;
		else
		{
			MessageSend(pioState->client, p->id, 0);
			pioState->pio_states.timed_id = 0;
		}
		pioState->pio_states.store_held = Unknown;
		if(p->repeat)
		{
			if(p->msecRepeat)
			{
				pioState->pio_states.timed_id = temp_timed_id;
				send_pio_timed_message(pioState, p, 1);
			}
			else
			{
				send_pio_timed_message(pioState, p, 0);
			}
		}
	}
	break;
	case internal_ext_timer_message:
	{
		const TimedMessage **m = (const TimedMessage **) data;
		const TimedMessage *p = *m;
		if (p->release)
			pioState->ext_states.timed_id = p->id;
		else
		{
			MessageSend(pioState->client, p->id, 0);
			pioState->ext_states.timed_id = 0;
		}
		pioState->ext_states.store_held = Unknown;
		if(p->repeat)
		{
			if(p->msecRepeat)
				send_ext_timed_message(pioState, p, 1);
			else
				send_ext_timed_message(pioState, p, 0);
		}
	}
	break;
	case MESSAGE_PIO_CHANGED:
	{
		const MessagePioChanged *m = (const MessagePioChanged *)data;
		pioChanged(task, pioState, m->state^pioState->pio_states.pskey_wakeup);
	}
	break;
	case pio_external_changed:
	{
		const PioExternalMessage *m = (const PioExternalMessage *)data;
		externalChanged(task, pioState, m->xor ^ (pioState->ext_states.store_bits & ~m->and));
	}
	break;
	case MESSAGE_CHARGER_CHANGED:
	{
		const MessageChargerChanged *m = (const MessageChargerChanged *)data;
		uint32 m_and = (1UL<<16) | (1UL<<17);
		uint32 m_or = ((uint32)m->charger_connected<<16) | ((uint32)m->vreg_en_high<<17);
		externalChanged(task, pioState, m_or ^ (pioState->ext_states.store_bits & ~m_and));
	}
	break;
	case double_pio_press_timer:
	case double_ext_press_timer:
	default:
		break;
	}
}

void pioExternal(PioState *pioState, uint32 external_and, uint32 external_xor)
{
	PioExternalMessage *m = malloc(sizeof(PioExternalMessage));
	m->xor = external_xor;
	m->and = external_and;
	MessageSend(&pioState->task, pio_external_changed, m);
}

void pioInit(PioState *pioState, Task client)
{
	MessagePioChanged *m = malloc(sizeof(MessagePioChanged));
	MessageChargerChanged *n = malloc(sizeof(MessageChargerChanged));
	bool vreg_en = PioGetVregEn();
	bool chg_en = (ChargerStatus()==NOT_CHARGING) ? FALSE : TRUE;
	uint16 pio_get = PioGet();
	uint16 pskey_wakeup = 0xFFFF;

	pioState->task.handler = pioHandler;
	pioState->client       = client;

	PsFullRetrieve(PSKEY_PIO_WAKEUP_STATE, &pskey_wakeup, sizeof(pskey_wakeup));

	pskey_wakeup = ~pskey_wakeup;

	pioState->pio_states.store_held = Unknown;
	pioState->pio_states.double_press = Unknown;
	pioState->pio_states.store_count = 0;
	pioState->pio_states.store_bits = 0;
	pioState->pio_states.timed_id = 0;
	pioState->pio_states.pskey_wakeup = pskey_wakeup;
	pioState->pio_states.pio_raw_bits = ~(pio_get^pskey_wakeup) & (0);

	pioState->ext_states.store_held = Unknown;
	pioState->ext_states.double_press = Unknown;
	pioState->ext_states.store_count = 0;
	pioState->ext_states.store_bits = 0;
	pioState->ext_states.timed_id = 0;
	pioState->ext_states.pio_raw_bits = 0;

	(void) MessagePioTask(&pioState->task);
	PioDebounce((1UL<<0), 2, 20);

	(void) MessageChargerTask(&pioState->task);
	ChargerDebounce(CHARGER_CONNECT_EVENT|CHARGER_VREG_EVENT, 4, 15);

	m->state = pio_get & ((1UL<<0));
	m->time  = 0;
	MessageSend(&pioState->task, MESSAGE_PIO_CHANGED, m);
	n->charger_connected = chg_en;
	n->vreg_en_high = vreg_en;
	MessageSend(&pioState->task, MESSAGE_CHARGER_CHANGED, n);
}
