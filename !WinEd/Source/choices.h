/* choices.h */

/* User-editable options */

#ifndef __wined_choices_h
#define __wined_choices_h

#include "DeskLib:Core.h"
#include "browser.h"

typedef struct {
  BOOL monitor;
  BOOL picker;
  BOOL browtools;
  BOOL viewtools;
  BOOL hotkeys;
  BOOL hatchredraw;
  BOOL round; /* This option removed from UI from version ar0.1. Always true now */
  BOOL furniture;
  BOOL autosprites;
  BOOL editpanes;
  BOOL mouseless_move;
  BOOL confirm;
  BOOL borders;
  BOOL formed;
  BOOL strict_panes;
  BOOL safe_icons;
} choices_str;

extern choices_str *choices;

/* This function is called when choices change */
typedef void (*choices_responder)(choices_str *old_ch,choices_str *new_ch);

void choices_init(choices_responder responder);

void choices_open(void);

/* Sort icons into proper places in browser */
void browser_sorticons(browser_fileinfo *browser,BOOL force,
	BOOL reopen,BOOL keep_selection);

#endif
