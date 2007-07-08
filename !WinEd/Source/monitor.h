/* monitor.h */

#ifndef __wined_monitor_h
#define __wined_monitor_h

#include "tempdefs.h"
#include "round.h"
#include "DeskLib:Kbd.h"

/* Load template */
void monitor_init(void);

/* Open monitor */
void monitor_open(void);

/* Inc/dec viewers known to monitor */
void monitor_activeinc(void);
void monitor_activedec(void);

/* Claim/release NULL events when ptr enter/leave events received */
BOOL monitor_activate(event_pollblock *event,void *reference);
BOOL monitor_deactivate(event_pollblock *event,void *reference);

/* Updates monitor on null events */
BOOL monitor_update(event_pollblock *event,void *reference);

/* Whether monitor is active */
extern BOOL monitor_isactive;

/* Whether selection has been moved by cursor keys */
extern BOOL monitor_moved;

/* winentry being monitored */
extern browser_winentry *monitor_winentry;

extern wimp_point drag_startpoint;
extern wimp_rect  drag_startdims;
extern wimp_rect drag_sidemove;

#endif
