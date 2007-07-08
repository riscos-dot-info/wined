/* help.h */

/* Interactive help message handlers */

#ifndef __wined_help_h
#define __wined_help_h

#include "DeskLib:EventMsg.h"
#include "DeskLib:Wimp.h"

/* These are standard event handlers.

   void *reference is cast to char *tag which is used to generate a
   MessageTrans tag of the form:

   H_<tag>_[dd][G]

   G is added if the icon is shaded; if message not found it is retried
   without G.
   dd is the icon under the pointer (these are 'strung' for menus) or is not
   present for the work area. If an icon-specific message is not found it is
   retried without _dd.
 */

BOOL help_on_window(event_pollblock *event,void *reference);

BOOL help_on_menu(event_pollblock *event,void *reference);

/* Macros to claim & release help events */
#define help_claim_window(window,tag) \
  EventMsg_Claim(message_HELPREQUEST,(window),help_on_window,(void *) (tag))

#define help_release_window(window) \
  EventMsg_Release(message_HELPREQUEST,(window),help_on_window)

#define help_claim_menu(tag) \
  EventMsg_Claim(message_HELPREQUEST,event_ANY,help_on_menu,(void *) (tag))

#define help_release_menu() \
  EventMsg_Release(message_HELPREQUEST,event_ANY,help_on_menu)

#endif
