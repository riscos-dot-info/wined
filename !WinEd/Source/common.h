/* common.h */

/* Saves having loads of includes in every file */

#ifndef __wined_common_h
#define __wined_common_h

#ifndef __stdio_h
#include <stdio.h>
#endif
#ifndef __stdlib_h
#include <stdlib.h>
#endif
#ifndef __string_h
#include <string.h>
#endif

#ifndef __dl_error_h
#include "DeskLib:Error.h"
#endif
#ifndef __dl_event_h
#include "DeskLib:Event.h"
#endif
#ifndef __dl_eventMsg_h
#include "DeskLib:EventMsg.h"
#endif
#ifndef __dl_file_h
#include "DeskLib:File.h"
#endif
#ifndef __dl_handler_h
#include "DeskLib:Handler.h"
#endif
#ifndef __dl_icon_h
#include "DeskLib:Icon.h"
#endif
#ifndef __dl_linkList_h
#include "DeskLib:LinkList.h"
#endif
#ifndef __wined_mem_h
#include "Mem.h"
#endif
#ifndef __dl_menu_h
#include "DeskLib:Menu.h"
#endif
#ifndef __dl_msgtrans_h
#include "DeskLib:MsgTrans.h"
#endif
#ifndef __dl_screen_h
#include "DeskLib:Screen.h"
#endif
#ifndef __dl_sprite_h
#include "DeskLib:Sprite.h"
#endif
#ifndef __dl_str_h
#include "DeskLib:Str.h"
#endif
#ifndef __dl_wimpswis_h
#include "DeskLib:WimpSWIs.h"
#endif
#ifndef __dl_window_h
#include "DeskLib:Window.h"
#endif

#include "DeskLib:Debug.h"

#ifndef __wined_globals_h
#include "globals.h"
#endif
#ifndef __wined_help_h
#include "help.h"
#endif
#ifndef __wined_tempdefs_h
#include "tempdefs.h"
#endif
#ifndef __wined_templates_h
#include "templates.h"
#endif

#endif
