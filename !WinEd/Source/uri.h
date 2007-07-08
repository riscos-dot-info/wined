/* URI handling */
#ifndef __wined_uri_h
#define __wined_uri_h

#ifndef __dl_wimp_h
#include "DeskLib:Wimp.h"
#endif

#define message_URI ((message_action) 0x4af80)

extern void uri_init(void);

extern void uri_send(const char *uri);

#endif
