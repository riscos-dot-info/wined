/* viewcom.h */

/****************************************************************************
 * Viewer commands - issued from menu/keys/tool pane
 ***************************************************************************/

#ifndef __wined_viewcom_h
#define __wined_viewcom_h

#include "alndiag.h"
#include "bordiag.h"
#include "coorddiag.h"
#include "icndiag.h"
#include "rszdiag.h"
#include "spcdiag.h"
#include "tempdefs.h"
#include "viewer.h"
#include "visarea.h"
#include "wadiag.h"
#include "windiag.h"

/* Direction to copy in */
typedef enum {
  copy_LEFT,
  copy_RIGHT,
  copy_UP,
  copy_DOWN,
  copy_ANY = -1
} viewer_copydirection;

/* How far away to make copy if no direction given */
#define copy_DISTANCE 32

/* Space to leave if a direction is given */
#define copy_SPACE 8

/* Copy current selection in given direction */
void viewcom_copy(browser_winentry *winentry,viewer_copydirection direction);

/* Delete current selection */
void viewcom_delete(browser_winentry *winentry);

/* Open renumber dbox */
void viewcom_renum(BOOL submenu, int x, int y, browser_winentry *winentry);

/* Open goto dbox */
void viewcom_goto(BOOL submenu, int x, int y, browser_winentry *winentry);

/* Renumber with current value */
void viewcom_quickrenum(browser_winentry *winentry);

/* Goto specified icon */
void viewcom_quickgoto(browser_winentry *winentry);

/* Select all icons */
void viewcom_selectall(browser_winentry *winentry);

/* Clear selection */
#ifndef __browser_h
extern void browcom_clearsel(void);
#endif
#define viewcom_clearselection browcom_clearsel

/* Close window */
#define viewcom_closewindow viewer_close

/* Open window editing dialogue */
#define viewcom_editwindow windiag_open

/* Open work area editing dialogue */
#define viewcom_editworkarea wadiag_open

/* Open visible area editing dialogue */
#define viewcom_editvisarea visarea_open

/* Open icon editing dialogue box */
void viewcom_editicon(browser_winentry *winentry);

/* Open icon editing dialogue box for title */
#define viewcom_edittitle(winentry) icndiag_open((winentry),-1)

/* Open alignment dialogue box */
#define viewcom_align(winentry) alndiag_open(winentry)

/* Open space out dialogue box */
#define viewcom_spaceout(winentry) spcdiag_open(winentry)

/* Open resize dialogue box */
#define viewcom_resize(winentry) rszdiag_open(winentry)

/* Open coordinates dialogue box */
#define viewcom_coords(winentry) coorddiag_open(winentry)

/* Open border dialogue box */
#define viewcom_border(winentry) bordiag_open(winentry)

#endif
