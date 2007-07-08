/* pane.c */

#include "DeskLib:Error.h"
#include "DeskLib:WimpSWIs.h"

#include "pane.h"

void pane_findoffset(window_handle parent,window_handle pane,
     		     wimp_point *offset)
{
  window_state wstate;

  Error_Check(Wimp_GetWindowState(pane,&wstate));
  offset->x = wstate.openblock.screenrect.min.x;
  offset->y = wstate.openblock.screenrect.max.y;
  Error_Check(Wimp_GetWindowState(parent,&wstate));
  offset->x -= wstate.openblock.screenrect.min.x;
  offset->y -= wstate.openblock.screenrect.max.y;
}

void pane_open(window_handle parent,window_handle pane,wimp_point *offset,
     	       window_openblock *openblock,BOOL resetscroll)
{
  window_state wstate;
  wimp_point size;
  wimp_point corner;

  Wimp_GetWindowState(pane,&wstate);
  size.x = wstate.openblock.screenrect.max.x -
  	   wstate.openblock.screenrect.min.x;
  size.y = wstate.openblock.screenrect.max.y -
  	   wstate.openblock.screenrect.min.y;
  corner.x = openblock->screenrect.min.x + offset->x;
  corner.y = openblock->screenrect.max.y + offset->y;
  wstate.openblock.screenrect.min.x = corner.x;
  wstate.openblock.screenrect.max.y = corner.y;
  wstate.openblock.screenrect.max.x = corner.x + size.x;
  wstate.openblock.screenrect.min.y = corner.y - size.y;
  wstate.openblock.behind = openblock->behind;
  if (resetscroll)
  {
    wstate.openblock.scroll.x = 0;
    wstate.openblock.scroll.y = 0;
  }
  Wimp_OpenWindow(&wstate.openblock);
  openblock->behind = pane;
  Wimp_OpenWindow(openblock);

  /* Re-open pane if parent has been forced on screen */
  if (openblock->screenrect.min.x + offset->x != corner.x ||
      openblock->screenrect.max.y + offset->y != corner.y)
  {
    corner.x = openblock->screenrect.min.x + offset->x;
    corner.y = openblock->screenrect.max.y + offset->y;
    wstate.openblock.screenrect.min.x = corner.x;
    wstate.openblock.screenrect.max.y = corner.y;
    wstate.openblock.screenrect.max.x = corner.x + size.x;
    wstate.openblock.screenrect.min.y = corner.y - size.y;
    Wimp_OpenWindow(&wstate.openblock);
  }
}
