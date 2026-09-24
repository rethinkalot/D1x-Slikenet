/* Console */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "pstypes.h"

/* Priority levels */
#define CON_CRITICAL -3
#define CON_URGENT   -2
#define CON_HUD      -1
#define CON_NORMAL    0
#define CON_VERBOSE   1
#define CON_DEBUG     2
#define CON_NET_TX    -4
#define CON_NET_RX    -5
#define CON_NET_STATUS -6
#define CON_NET_DEALLOC -7

/* Network telemetry priorities. The console renders these with their own
 * colours (see con_draw).
 *
 * CON_NET_STATUS / CON_NET_DEALLOC are connection level notices - the host
 * address, the port in use and channel teardown. They are emitted at every
 * verbosity level so that a normal launch still records them in gamelog.txt;
 * the Windows console window simply stays hidden unless -debug is given.
 *
 * CON_NET_TX / CON_NET_RX carry one line per packet. That is far too noisy
 * and too expensive for normal play, so they additionally require both -debug
 * and -verbose; see con_printf. */
#define CON_NET_PACKET_LOWEST  CON_NET_RX
#define CON_NET_PACKET_HIGHEST CON_NET_TX
#define CON_IS_NET_PACKET_PRIORITY(p) \
	((p) >= CON_NET_PACKET_LOWEST && (p) <= CON_NET_PACKET_HIGHEST)


#define CON_LINES_ONSCREEN 18
#define CON_SCROLL_OFFSET  (CON_LINES_ONSCREEN - 3)
#define CON_LINES_MAX      128
#define CON_LINE_LENGTH    2048

#define CON_STATE_OPEN 2
#define CON_STATE_OPENING 1
#define CON_STATE_CLOSING -1
#define CON_STATE_CLOSED -2

typedef struct console_buffer
{
	char line[CON_LINE_LENGTH];
	int priority;
} __pack__ console_buffer;

void con_init(void);
void con_printf(int level, const char *fmt, ...);
void con_showup(void);
void con_switch_log(const char* filename);

#endif /* _CONSOLE_H_ */
