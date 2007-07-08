/* drag.h */

/* Initiates dragging of icon outlines */

#ifndef __wined_drag_h
#define __wined_drag_h

#include "DeskLib:Wimp.h"

#include "tempdefs.h"

typedef struct {
  window_handle window;
  BOOL adjust;
  union {
    browser_fileinfo *browser;
    browser_winentry *winentry;
  } broworview;
  BOOL viewer;
} drag_ref;

/* Find bounds (work area and screen) of all selected icons */
void drag_getselectionbounds(browser_winentry *winentry,
     			     wimp_rect *workarearect,wimp_rect *screenrect);

/* Move a single icon (physical) */
void drag_moveicon(browser_winentry *winentry,icon_handle icon,
	BOOL choriz, BOOL cvert);
/* choriz, cvert mean drag is constrained horizontally/vertically */

/* Move a selection of icons; bounds updated to work area rect surrounding
   selection */
void drag_moveselection(browser_winentry *winentry,wimp_rect *bounds,
	BOOL choriz, BOOL cvert);
/* choriz, cvert mean drag is constrained horizontally/vertically */

/* Resize an icon (given by (click) event data) */
void drag_resizeicon(browser_winentry *winentry,event_pollblock *event);

/* Drag a general rubber box around pointer, register drag_selectcontents;
   last two params are to allow tool pane shading */
void drag_rubberbox(event_pollblock *event,void *broworview,BOOL viewer);

/* Use as an event_handler (needs casting) to a drag of a rubber bounding
   box. All "window's" icons enclosed by drag are selected.
   The function then releases itself */
BOOL drag_selectcontents(event_pollblock *event,drag_ref *reference);

/* Works out which edge of a bounding box a position is logically nearest to */
typedef enum {
  drag_LeftEdge, drag_BottomEdge, drag_RightEdge, drag_TopEdge
} drag_edgetype;

drag_edgetype drag_nearestedge(const wimp_rect *bounds, int x, int y);

extern BOOL monitor_dragging;

#endif
