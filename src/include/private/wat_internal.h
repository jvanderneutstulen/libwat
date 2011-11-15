/*
 * libwat: Wireless AT commands library
 *
 * David Yat Sin <dyatsin@sangoma.com>
 * Copyright (C) 2011, Sangoma Technologies.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *
 */

#ifndef _WAT_INTERNAL_H
#define _WAT_INTERNAL_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "wat_config.h"
#include "wat_mutex.h"
#include "wat_queue.h"
#include "wat_buffer.h"
#include "wat_sched.h"

#define WAT_EVENT_QUEUE_SZ				20
#define WAT_CMD_QUEUE_SZ				100
//#define WAT_SMS_QUEUE_SZ				50
#define WAT_SMS_QUEUE_SZ				5
#define WAT_BUFFER_SZ					500
#define WAT_TOKENS_SZ					20
#define WAT_TIMEOUTS_SZ					30
#define WAT_MAX_NOTIFYS_PER_SPAN		100

#define WAT_SCHEDULE_NEXT_DEFAULT_TIME	1000 /* TODO: May not need this */

#define wat_log_span(span, level, a, ...) if (g_interface.wat_log_span) g_interface.wat_log_span(span->id, level,a, ##__VA_ARGS__)

#define wat_log(level,a,...) if (g_interface.wat_log) g_interface.wat_log(level, a, ##__VA_ARGS__)

#define wat_assert(msg) if (g_interface.wat_assert) g_interface.wat_assert(msg)

#define wat_assert_return(assertion, retval, msg) \
									if (!(assertion)) { \
										wat_assert(msg); \
										return retval; \
									}

#define wat_assert_return_void(assertion, msg) \
									if (!(assertion)) { \
										wat_assert(msg); \
										return; \
}
#define wat_safe_free(ptr) if (ptr) wat_free(ptr)

#define wat_test_flag(obj, flag)	((obj)->flags & (1 << flag))
#define wat_set_flag(obj, flag) 	((obj)->flags |= (1 << flag))
#define wat_clear_flag(obj, flag)	((obj)->flags &= ~(1 << flag))

#define ARRAY_LEN(_array) sizeof(_array)/sizeof(_array[0])
#define SWAP_NIBBLE(byte) ((((byte) >> 4) & 0x0F) | (((byte) << 4) & 0xF0))

extern wat_interface_t g_interface;
extern uint32_t	g_debug;

typedef struct wat_span wat_span_t;
typedef struct wat_module wat_module_t;

typedef enum {
	WAT_NET_NOT_REGISTERED = 0,             /* Initial state */
	WAT_NET_REGISTERED_HOME,                /* Registered to home network */
	WAT_NET_NOT_REGISTERED_SEARCHING,       /* Not Registered, searching for an operator */
	WAT_NET_REGISTRATION_DENIED,            /* Registration denied */
	WAT_NET_UNKNOWN,                        /* Unknown */
	WAT_NET_REGISTERED_ROAMING,             /* Registered, roaming */
	WAT_NET_INVALID,
} wat_net_stat_t;

#define WAT_NET_STAT_STRINGS "Not Registered", "Registered Home", "Not Registered, Searching", "Registration Denied", "Unknown", "Registered Roaming", "Invalid"
WAT_STR2ENUM_P(wat_str2wat_net_stat, wat_net_stat2str, wat_net_stat_t);

typedef enum {
	WAT_CSQ_BER_0,
	WAT_CSQ_BER_1,
	WAT_CSQ_BER_2,
	WAT_CSQ_BER_3,
	WAT_CSQ_BER_4,
	WAT_CSQ_BER_5,
	WAT_CSQ_BER_6,
	WAT_CSQ_BER_7,
	WAT_CSQ_BER_NOT_DETECTABLE,
} wat_csq_ber_t;

#define WAT_CSQ_BER_STRINGS "less than 0.2%", "0.2 to 0.4%", "0.4 to 0.8%", "0.8 to 1.6%", "1.6 to 3.2%", "3.2 to 6.4%", "6.4 to 12.8%", "more than 12.8%", "not detectable"
WAT_STR2ENUM_P(wat_str2wat_csq_ber, wat_csq_ber2str, wat_csq_ber_t);

char *wat_decode_csq_rssi(char *in, unsigned rssi);

typedef struct {
	wat_net_stat_t stat;
	uint8_t lac;	/* Local Area Code for the currently registered on cell */
	uint8_t ci;		/* Cell Id for currently registered on cell */

	uint8_t rssi;
	uint8_t ber;
} wat_net_info_t;

typedef enum {
	WAT_TIMEOUT_CLIP,
	WAT_PROGRESS_MONITOR,
} wat_call_timeout_id_t;

typedef wat_status_t (wat_module_start_func)(wat_span_t *span);
typedef wat_status_t (wat_module_restart_func)(wat_span_t *span);
typedef wat_status_t (wat_module_shutdown_func)(wat_span_t *span);

struct wat_module {
	wat_module_start_func *start;
	wat_module_restart_func *restart;
	wat_module_shutdown_func *shutdown;
};

wat_status_t wat_module_register(wat_span_t *, wat_module_t *module);

typedef enum {
	WAT_EVENT_CON_REQ,
	WAT_EVENT_CON_CFM,
	WAT_EVENT_REL_REQ,
	WAT_EVENT_REL_CFM,
	WAT_EVENT_SMS_REQ,
	WAT_EVENT_INVALID,
} wat_event_id_t;

#define WAT_EVENT_STRINGS "Con Req", "Con Cfm", "Rel Req", "Rel Cfm", "Sms Req", "invalid"
WAT_STR2ENUM_P(wat_str2wat_event, wat_event2str, wat_event_id_t);

typedef enum {
	WAT_CALL_STATE_INIT,		/* Initial state */
	WAT_CALL_STATE_DIALING,		/* We just received a CRING/RING */
	WAT_CALL_STATE_DIALED,		/* We notified the user of the incoming call */
	WAT_CALL_STATE_RINGING,		/* On outgoing call, remote side is ringing */
	WAT_CALL_STATE_ANSWERED,	/* Incoming Call has been answered by user */
	WAT_CALL_STATE_UP,			/* Call is up */
	WAT_CALL_STATE_TERMINATING,		/* Call has been hung-up on the remote side */
	WAT_CALL_STATE_TERMINATING_CMPL, /* User has acknowledged remote hang-up */
	WAT_CALL_STATE_HANGUP,		/* Call has been hung-up on local side */
	WAT_CALL_STATE_HANGUP_CMPL,	/* GSM Chip has acknowledged local hang-up */
	WAT_CALL_STATE_INVALID,
} wat_call_state_t;

#define WAT_CALL_STATE_STRINGS "init", "dialing", "dialed", "ringing", "answered", "up", "terminating", "terminating cmpl", "hangup", "hangup cmpl", "invalid"
WAT_STR2ENUM_P(wat_str2wat_call_state, wat_call_state2str, wat_call_state_t);

typedef enum {
	WAT_SMS_STATE_INIT,
	WAT_SMS_STATE_QUEUED,
	WAT_SMS_STATE_START,
	WAT_SMS_STATE_SEND_HEADER,
	WAT_SMS_STATE_SEND_BODY,
	WAT_SMS_STATE_SEND_TERMINATOR,
	WAT_SMS_STATE_COMPLETE,
	WAT_SMS_STATE_INVALID,
} wat_sms_state_t;

#define WAT_SMS_STATE_STRINGS "init", "queued", "start", "send header", "send body", "send terminator", "complete", "invalid"
WAT_STR2ENUM_P(wat_str2wat_sms_state, wat_sms_state2str, wat_sms_state_t);

typedef enum {
	WAT_CALL_FLAG_RCV_CLIP,		/* We already received a CLIP event for this call */

} wat_call_flag_t;

typedef enum {
	WAT_DIRECTION_OUTGOING,
	WAT_DIRECTION_INCOMING,
	WAT_DIRECTION_INVALID,
} wat_direction_t;

#define WAT_DIRECTION_STRINGS "outgoing", "incoming", "invalid"
WAT_STR2ENUM_P(wat_str2wat_direction, wat_direction2str, wat_direction_t);

typedef struct {
	uint8_t id;		/* ID used by libwat*/
	uint32_t modid;		/* ID used internally by module */
	wat_call_type_t type;
	wat_number_t calling_num;
	wat_number_t called_num;
	wat_call_state_t state;

	uint32_t flags;
	wat_direction_t dir; /* Inbound or outbound */

	wat_span_t *span; /* Span on which this call exists */
	wat_timer_id_t timeouts[WAT_TIMEOUTS_SZ];
} wat_call_t;

typedef struct {
	uint32_t id;					/* ID used by libwat*/	
	wat_sms_state_t state;
	wat_sms_cause_t cause;			/* Cause for failure if any */
	wat_direction_t dir;			/* Inbound or outbound */ /* TODO: Do I even need this ? */
	wat_sms_type_t type;			/* PDU or Plain Text */
	
	wat_number_t called_num;
	wat_span_t *span; /* Span on which this sms exists */

	char message[WAT_MAX_SMS_SZ + 4];	/* Message, need 4 more bytes to store terminator */
	uint32_t len;					/* Length of message */
	uint32_t wrote;					/* Number of bytes written */
	char *error;					/* Error code reported by chip */
} wat_sms_t;

typedef struct {
	wat_event_id_t id;

	uint16_t call_id;
	uint16_t sms_id;
	
	/* event specific info here */
	union {
		wat_con_event_t con_event;
		wat_sms_event_t sms_event;
	} data;
} wat_event_t;

#define WAT_EVENT_ARGS (wat_span_t *span, wat_event_t *event)
#define WAT_EVENT_FUNC(name) void (name) WAT_EVENT_ARGS
typedef void (wat_event_func) WAT_EVENT_ARGS;

typedef struct wat_event_handler {
	wat_event_id_t event_id;
	wat_event_func *func;
} wat_event_handler_t;

void *wat_calloc(wat_size_t nmemb, wat_size_t size);
void *wat_malloc(wat_size_t size);
void wat_free(void *ptr);
void wat_free_tokens(char *tokens[]);
char *wat_strdup(const char *str);

char* format_at_data(char *dest, void *indata, wat_size_t len);

wat_status_t wat_event_enqueue(wat_span_t *span, wat_event_t *event);
wat_event_t *wat_event_dequeue(wat_span_t *span);
wat_status_t wat_event_process(wat_span_t *span, wat_event_t *event);

#define WAT_RESPONSE_ARGS (wat_span_t *span, char *tokens[], wat_bool_t success, void *obj, char *error)
#define WAT_RESPONSE_FUNC(name) int (name)  WAT_RESPONSE_ARGS
typedef int (wat_cmd_response_func) WAT_RESPONSE_ARGS;

#define WAT_NOTIFY_ARGS (wat_span_t *span, char *tokens[])
#define WAT_NOTIFY_FUNC(name) int (name) WAT_NOTIFY_ARGS
typedef int (wat_cmd_notify_func) WAT_NOTIFY_ARGS;

#define WAT_SCHEDULED_FUNC(name) void (name) (void *data)

wat_status_t wat_cmd_register(wat_span_t *span, const char *prefix, wat_cmd_notify_func *func);
wat_status_t wat_cmd_enqueue(wat_span_t *span, const char *cmd, wat_cmd_response_func *cb, void *obj);

typedef struct wat_cmd {
	char *cmd;
	wat_cmd_response_func *cb;
	void *obj;
} wat_cmd_t;

typedef struct wat_notify {
	char *prefix;
	wat_cmd_notify_func *func;
} wat_notify_t;

typedef struct {
	uint8_t busy:1;
} wat_channel_t;

struct wat_span {
	uint8_t id;					/* User Id */
	uint8_t configured:1;		/* Span has been configured */
	uint8_t running:1;			/* Span was started */

	wat_sigstatus_t sigstatus;

	char manufacturer_name [50];
	char manufacturer_id [20];
	char revision_id [20];
	char serial_number [50];
	char imsi[50];				/* International Mobile Subscriber Identify */
	char subscriber_number[50];		/* Subscriber Number */
	wat_bool_t clip;

	wat_net_info_t net_info;	/* Network Registration Report */

	wat_span_config_t config;	/* Configuration parameters */

	wat_buffer_t *buffer;		/* Buffer for reads */
	wat_queue_t	*event_queue;
	wat_sched_t *sched;			/* Scheduler for timeouts */

	wat_module_t module;		/* Module interface */

	uint8_t	cmd_busy:1;			/* If currently executing a command */
	wat_cmd_t *cmd;				/* Current command being executed */
	wat_queue_t *cmd_queue;		/* Commands waiting to be executed */

	wat_channel_t *channel;

	wat_call_t *calls[WAT_MAX_CALLS_PER_SPAN];
	unsigned last_call_id;

	wat_notify_t *notifys[WAT_MAX_NOTIFYS_PER_SPAN];
	unsigned notify_count;

	uint8_t sms_write;			/* We are currently writing an SMS, cannot process anything else */
	wat_sms_type_t sms_mode;	/* Current mode for SMS */
	
	wat_queue_t *sms_queue;		/* Queue for pending outgoing SMS */
	wat_sms_t *sms;				/* Current SMS being executed */
};


void wat_span_run_events(wat_span_t *span);
void wat_span_run_cmds(wat_span_t *span);
void wat_span_run_smss(wat_span_t *span);
void wat_span_run_sched(wat_span_t *span);
wat_status_t wat_cmd_process(wat_span_t *span);
wat_status_t wat_sms_process(wat_sms_t *sms);
wat_status_t wat_sms_send_body(wat_sms_t *sms);
wat_status_t wat_handle_incoming_sms_pdu(uint8_t *data, wat_size_t len);
wat_status_t wat_event_process(wat_span_t *span, wat_event_t *event);
void wat_span_run_timeouts(wat_span_t *span);
wat_status_t wat_span_update_sig_status(wat_span_t *span, wat_bool_t up);
wat_bool_t wat_sig_status_up(wat_net_stat_t stat);
wat_status_t wat_span_update_net_status(wat_span_t *span, unsigned stat);
int wat_span_write(wat_span_t *span, void *data, uint32_t len);

WAT_RESPONSE_FUNC(wat_response_atz);
WAT_RESPONSE_FUNC(wat_response_ate);
WAT_RESPONSE_FUNC(wat_response_cgmm);
WAT_RESPONSE_FUNC(wat_response_cgmi);
WAT_RESPONSE_FUNC(wat_response_cgmr);
WAT_RESPONSE_FUNC(wat_response_cgsn);
WAT_RESPONSE_FUNC(wat_response_cimi);
WAT_RESPONSE_FUNC(wat_response_clip);
WAT_RESPONSE_FUNC(wat_response_creg);
WAT_RESPONSE_FUNC(wat_response_cnmi);
WAT_RESPONSE_FUNC(wat_response_cops);
WAT_RESPONSE_FUNC(wat_response_cnum);
WAT_RESPONSE_FUNC(wat_response_csq);
WAT_RESPONSE_FUNC(wat_response_clcc);
WAT_RESPONSE_FUNC(wat_response_ata);
WAT_RESPONSE_FUNC(wat_response_ath);
WAT_RESPONSE_FUNC(wat_response_atd);
WAT_RESPONSE_FUNC(wat_response_cmgs_start);
WAT_RESPONSE_FUNC(wat_response_cmgs_end);
WAT_RESPONSE_FUNC(wat_response_cmgf);

WAT_NOTIFY_FUNC(wat_notify_cring);
WAT_NOTIFY_FUNC(wat_notify_cmt);
WAT_NOTIFY_FUNC(wat_notify_clip);
WAT_NOTIFY_FUNC(wat_notify_creg);

WAT_SCHEDULED_FUNC(wat_scheduled_clcc);

wat_status_t wat_call_create(wat_span_t *span, wat_call_t **call, wat_direction_t dir);
void wat_call_destroy(wat_call_t **call);

wat_status_t wat_span_sms_create(wat_span_t *span, wat_sms_t **insms, uint8_t sms_id, wat_direction_t dir);
void wat_span_sms_destroy(wat_sms_t **insms);

wat_status_t _wat_sms_set_state(const char *func, int line, wat_sms_t *sms, wat_sms_state_t new_state);
#define wat_sms_set_state(sms, new_state) _wat_sms_set_state(__FUNCTION__, __LINE__, sms, new_state)

wat_status_t _wat_call_set_state(const char *func, int line, wat_call_t *call, wat_call_state_t new_state);
#define wat_call_set_state(call, new_state) _wat_call_set_state(__FUNCTION__, __LINE__, call, new_state)

typedef enum {
	WAT_ITERATOR_CALLS =1,
	WAT_ITERATOR_NOTIFYS,
} wat_iterator_type_t;

typedef struct wat_iterator {
	wat_iterator_type_t type;
	unsigned int allocated:1;
	uint32_t index;
	const wat_span_t *span;
} wat_iterator_t;

wat_iterator_t *wat_get_iterator(wat_iterator_type_t type, wat_iterator_t *iter);
void *wat_iterator_current(wat_iterator_t *iter);
wat_iterator_t *wat_iterator_next(wat_iterator_t *iter);
wat_status_t wat_iterator_free(wat_iterator_t *iter);

wat_iterator_t *wat_span_get_call_iterator(const wat_span_t *span, wat_iterator_t *iter);
wat_iterator_t *wat_span_get_notify_iterator(const wat_span_t *span, wat_iterator_t *iter);

wat_status_t wat_span_call_create(wat_span_t *span, wat_call_t **call, uint8_t id, wat_direction_t dir);
void wat_span_call_destroy(wat_call_t **incall);
wat_call_t *wat_span_get_call_by_state(wat_span_t *span, wat_call_state_t state);
wat_call_t *wat_span_get_call_by_id(wat_span_t *span, uint16_t id);

#ifdef WAT_FUNC_DEBUG
#define WAT_FUNC_DBG_START	wat_log(WAT_LOG_DEBUG, "Entering function %s\n", __FUNCTION__);
#define WAT_FUNC_DBG_END 	wat_log(WAT_LOG_DEBUG, "Leaving function %s:%d\n", __FUNCTION__, __LINE__);

#define WAT_PRINT_TOKENS(__tokens) \
		{ \
			int __i = 0; \
			while(__tokens[__i]) { \
				wat_log(WAT_LOG_DEBUG, "  token[%d]:%s\n", __i, __tokens[__i]); \
				__i++; \
			} \
		}

#define WAT_RESPONSE_FUNC_DBG_START \
			wat_log(WAT_LOG_DEBUG, "Entering function %s (success:%s)\n", __FUNCTION__, (success==WAT_TRUE)? "yes":"no"); \
			WAT_PRINT_TOKENS(tokens)

#define WAT_NOTIFY_FUNC_DBG_START \
			wat_log(WAT_LOG_DEBUG, "Entering function %s \n", __FUNCTION__); \
			WAT_PRINT_TOKENS(tokens)

#define WAT_SPAN_FUNC_DBG_START	wat_log_span(span, WAT_LOG_DEBUG, "Entering function %s\n", __FUNCTION__);

#else

#define WAT_FUNC_DBG_START
#define WAT_FUNC_DBG_END

#define WAT_SPAN_FUNC_DBG_START

#define WAT_RESPONSE_FUNC_DBG_START
#define WAT_NOTIFY_FUNC_DBG_START

#endif /* WAT_FUNC_DEBUG */

#endif /* _WAT_INTERNAL_H */

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */

/******************************************************************************/


