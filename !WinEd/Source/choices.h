/* choices.h */

/* User-editable options */

#ifndef __wined_choices_h
#define __wined_choices_h

#include "DeskLib:Core.h"
#include "browser.h"

/**
 * The structure holding the user choices.
 */ 
typedef struct {
  BOOL monitor;         /**< Automatically display the monitor.                               */
  BOOL picker;          /**< Automatically display the icon picker.                           */
  BOOL browtools;       /**< Show a toolbar in the template browser.                          */
  BOOL viewtools;       /**< Show a toolbar on editable windows.                              */
  BOOL hotkeys;         /**< Provide keyborad shortcuts.                                      */
  BOOL hatchredraw;     /**< Hatch the work area of user redrawn windows.                     */
  BOOL browser_sort;    /**< Sort the browser contents into alphabetical order.               */
  BOOL furniture;       /**< Always show furniture on editable windows.                       */
  BOOL autosprites;     /**< Automatically load sprites files.                                */
  BOOL editpanes;       /**< Use panes for action buttons in window and icon edit dialogues.  */
  BOOL mouseless_move;  /**< Move selected icons with just the cursor keys.                   */
  BOOL confirm;         /**< Confirm the deletions of any windows or icons.                   */
  BOOL borders;         /**< Always plot borders on editable icons.                           */
  BOOL formed;          /**< Use FormEd style string terminators (ie. \n and not \0).         */
  BOOL strict_panes;    /**< Limit flag options on pane windows.                              */
  BOOL safe_icons;      /**< Make "Don't Move" the default in icon edit dialogue box.         */

  /* The values above this point form the structure of the legacy binary config file, and     *
   * should not be changed, moved or deleted. If no longer required, leave in-situ and mark   *
   * as obsolete. Below this point, values have only appeared in the textual config files,    *
   * and may be added, removed or changed as required.                                        */

  BOOL round_coords;    /**< Round all window and icon coordinates to pixel boundaries.       */
} choices_str;

/**
 * Public pointer to the choices data.
 */
extern choices_str *choices;

/* This function is called when choices change */
typedef void (*choices_responder)(choices_str *old_ch, choices_str *new_ch);

/* Initialise the choices. */
void choices_init(choices_responder responder);

/* Open the Choices dialogue. */
void choices_open(void);

#endif
