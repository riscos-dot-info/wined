/* unsaved.h */
/* Warns user of attempts to quit/close with unsaved data */

#ifndef __wined_unsaved_h
#define __wined_unsaved_h

#include "DeskLib:Event.h"

#include "datatrans.h"
#include "tempdefs.h"

/* Calls handler if Discard chosen, whether to shutdown worked out from
   event. If event == NULL no reply given */
void unsaved_quit(event_pollblock *event);

typedef void (*unsaved_closehandler)(browser_fileinfo *browser);

/* Calls closehandler if Discard chosen, savehandler if Save chosen */
void unsaved_close(unsaved_closehandler closehandler,datatrans_saver saver,
                   browser_fileinfo *browser);

/* Load dialogue boxes etc */
void unsaved_init(void);

#endif
