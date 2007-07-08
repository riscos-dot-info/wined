/* drag.h */

/* Initiates dragging of icon outlines */

/* Smallest allowable size (OS units) of an icon */
#define SMALLEST 4

/* Threshold for near edge detection */
#define NEAREDGE 24

#include <limits.h>
#include <string.h>

#include "DeskLib:Coord.h"
#include "DeskLib:Event.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Kbd.h"
#include "DeskLib:KeyCodes.h"
#include "DeskLib:SWI.h"
#include "DeskLib:WimpSWIs.h"
#include "DeskLib:Window.h"

#include "browser.h"
#include "common.h"
#include "browtools.h"
#include "choices.h"
#include "drag.h"
#include "globals.h"
#include "icnedit.h"
#include "viewer.h"
#include "viewtools.h"

#ifndef SWI_OS_Module
#define SWI_OS_Module 0x1e
#endif

#define dragext_dragbox(b) Wimp_DragBox(b)

static drag_ref drag_selallref;

wimp_point drag_startpoint;
wimp_rect  drag_startdims;
wimp_rect  drag_sidemove; /* used for boolean flags for if a side is moving */

void drag_moveicon(browser_winentry *winentry,icon_handle icon, BOOL choriz, BOOL cvert)
{
  window_state wstate;
  drag_block block;
  wimp_point origin;
  wimp_rect *workarearect;
  mouse_block ptr_info;
  convert_block con;
  char ptr_name[] = "ptr_dts";

  /* The monitor-update routine is different while dragging */
  monitor_dragging = TRUE;

  /* Find position of icon */
  Wimp_GetWindowState(winentry->handle,&wstate);
  workarearect = &winentry->window->icon[icon].workarearect;
  origin.x = wstate.openblock.screenrect.min.x - wstate.openblock.scroll.x;
  origin.y = wstate.openblock.screenrect.max.y - wstate.openblock.scroll.y;
  block.screenrect.min.x = workarearect->min.x + origin.x;
  block.screenrect.min.y = workarearect->min.y + origin.y;
  block.screenrect.max.x = workarearect->max.x + origin.x;
  block.screenrect.max.y = workarearect->max.y + origin.y;

  Window_GetCoords(winentry->handle, &con);
  Wimp_GetPointerInfo(&ptr_info);
  drag_startpoint.x = Coord_XToWorkArea(ptr_info.pos.x, &con);
  drag_startpoint.y = Coord_XToWorkArea(ptr_info.pos.y, &con);
  drag_startdims.min = workarearect->min;
  drag_startdims.max = workarearect->max;

  /* All sides are moving */
  drag_sidemove.min.x = TRUE;
  drag_sidemove.max.x = TRUE;
  drag_sidemove.min.y = TRUE;
  drag_sidemove.max.y = TRUE;

  block.type = drag_FIXEDBOX;

  if (choriz)
  {
    block.parent.min.x = block.screenrect.min.x;
    block.parent.max.x = block.screenrect.max.x;
  }
  else
  {
    block.parent.min.x = INT_MIN;
    block.parent.max.x = INT_MAX;
  }
  if (cvert)
  {
    block.parent.min.y = block.screenrect.min.y;
    block.parent.max.y = block.screenrect.max.y;
  }
  else
  {
    block.parent.min.y = INT_MIN;
    block.parent.max.y = INT_MAX;
  }

  dragext_dragbox(&block);

  /* Change pointer shape */
  Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
}

void drag_getselectionbounds(browser_winentry *winentry,
     			     wimp_rect *workarearect,wimp_rect *screenrect)
{
  window_info winfo;
  icon_handle icon;
  wimp_rect *iconrect;
  wimp_point origin;

  Window_GetInfo3(winentry->handle,&winfo);

  /* Find bounding box of all selected icons; have to find logical bounds
     in case of dummy R6 */
  workarearect->min.x = INT_MAX;
  workarearect->min.y = INT_MAX;
  workarearect->max.x = INT_MIN;
  workarearect->max.y = INT_MIN;
  for (icon = 0;icon < winfo.block.numicons;icon++)
  {
    if (Icon_GetSelect(winentry->handle,icon))
    {
      iconrect = &winentry->window->icon[icon].workarearect;
      if (iconrect->min.x < workarearect->min.x)
        workarearect->min.x = iconrect->min.x;
      if (iconrect->min.y < workarearect->min.y)
        workarearect->min.y = iconrect->min.y;
      if (iconrect->max.x > workarearect->max.x)
        workarearect->max.x = iconrect->max.x;
      if (iconrect->max.y > workarearect->max.y)
        workarearect->max.y = iconrect->max.y;
    }
  }

  /* Convert bounds to screen coords */
  origin.x = winfo.block.screenrect.min.x - winfo.block.scroll.x;
  origin.y = winfo.block.screenrect.max.y - winfo.block.scroll.y;
  *screenrect = *workarearect;
  screenrect->min.x += origin.x;
  screenrect->min.y += origin.y;
  screenrect->max.x += origin.x;
  screenrect->max.y += origin.y;
}

void drag_moveselection(browser_winentry *winentry,wimp_rect *bounds,
	BOOL choriz, BOOL cvert)
{
  drag_block block;
  window_state wstate;
  wimp_rect *workarearect;
  convert_block con;
  mouse_block ptr_info;
  char ptr_name[] = "ptr_dts";

  /* The monitor-update routine is different while dragging */
  monitor_dragging = TRUE;

  /* Find position of icon */
  Wimp_GetWindowState(winentry->handle,&wstate);
  Wimp_GetPointerInfo(&ptr_info);
  workarearect = &winentry->window->icon[ptr_info.icon].workarearect;
  Window_GetCoords(winentry->handle, &con);
  drag_startpoint.x = Coord_XToWorkArea(ptr_info.pos.x, &con);
  drag_startpoint.y = Coord_XToWorkArea(ptr_info.pos.y, &con);
  drag_startdims.min = workarearect->min;
  drag_startdims.max = workarearect->max;

  /* All sides are moving */
  drag_sidemove.min.x = TRUE;
  drag_sidemove.max.x = TRUE;
  drag_sidemove.min.y = TRUE;
  drag_sidemove.max.y = TRUE;

  block.type = drag_FIXEDBOX;

  drag_getselectionbounds(winentry,bounds,&block.screenrect);

  if (choriz)
  {
    block.parent.min.x = block.screenrect.min.x;
    block.parent.max.x = block.screenrect.max.x;
  }
  else
  {
    block.parent.min.x = INT_MIN;
    block.parent.max.x = INT_MAX;
  }
  if (cvert)
  {
    block.parent.min.y = block.screenrect.min.y;
    block.parent.max.y = block.screenrect.max.y;
  }
  else
  {
    block.parent.min.y = INT_MIN;
    block.parent.max.y = INT_MAX;
  }

  dragext_dragbox(&block);

  /* Change pointer shape */
  Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
}

static void drag_dovert(drag_block *block, int bottom, int top)
{
  if (bottom <= top)
  {
    block->parent.min.y = block->screenrect.min.y;
    block->parent.max.y = block->screenrect.min.y;
  }
  else
  {
    block->parent.min.y = block->screenrect.max.y;
    block->parent.max.y = block->screenrect.max.y;
  }
}

static void drag_dohoriz(drag_block *block, int left, int right)
{
  if (left < right)
  {
    block->parent.min.x = block->screenrect.min.x;
    block->parent.max.x = block->screenrect.min.x;
  }
  else
  {
    block->parent.min.x = block->screenrect.max.x;
    block->parent.max.x = block->screenrect.max.x;
  }
}

void drag_resizeicon(browser_winentry *winentry,event_pollblock *event)
{
  window_state wstate;
  drag_block block;
  wimp_point origin;
  int left,bottom,right,top;
  wimp_rect *workarearect;
  convert_block con;
  mouse_block ptr_info;
  BOOL leftedge = FALSE;
  BOOL rightedge = FALSE;
  BOOL topedge = FALSE;
  BOOL bottomedge = FALSE;
  char ptr_name[13];

  /* The monitor-update routine is different while dragging */
  monitor_dragging = TRUE;

  /* Parent box will be limited to window */

  /* Find position of icon */
  Wimp_GetWindowState(event->data.mouse.window,&wstate);
  workarearect=&winentry->window->icon[event->data.mouse.icon].workarearect;
  origin.x = wstate.openblock.screenrect.min.x - wstate.openblock.scroll.x;
  origin.y = wstate.openblock.screenrect.max.y - wstate.openblock.scroll.y;
  block.screenrect.min.x = workarearect->min.x + origin.x;
  block.screenrect.min.y = workarearect->min.y + origin.y;
  block.screenrect.max.x = workarearect->max.x + origin.x;
  block.screenrect.max.y = workarearect->max.y + origin.y;

  Window_GetCoords(winentry->handle, &con);
  Wimp_GetPointerInfo(&ptr_info);
  drag_startpoint.x = Coord_XToWorkArea(ptr_info.pos.x, &con);
  drag_startpoint.y = Coord_XToWorkArea(ptr_info.pos.y, &con);
  drag_startdims.min = workarearect->min;
  drag_startdims.max = workarearect->max;

  block.type = drag_RUBBERBOX;

  /* Find which edge of icon is nearest pointer */
  left = event->data.mouse.pos.x - block.screenrect.min.x;
  bottom = event->data.mouse.pos.y - block.screenrect.min.y;
  right = block.screenrect.max.x - event->data.mouse.pos.x;
  top = block.screenrect.max.y - event->data.mouse.pos.y;

  if (!Kbd_KeyDown(inkey_SHIFT))
  /* Only allow one edge to be moved */
  {
    switch (drag_nearestedge(&block.screenrect, event->data.mouse.pos.x, event->data.mouse.pos.y))
    {
      case drag_LeftEdge:
        block.parent.min.x = wstate.openblock.screenrect.min.x;
        block.parent.max.x = block.screenrect.max.x - SMALLEST;
        drag_dovert(&block, bottom, top);
        drag_sidemove.min.x = TRUE;
        leftedge = TRUE;
        break;
      case drag_BottomEdge:
        block.parent.min.y = wstate.openblock.screenrect.min.y;
        block.parent.max.y = block.screenrect.max.y - SMALLEST - 4;
        drag_dohoriz(&block, left, right);
        drag_sidemove.max.y = TRUE;
        bottomedge = TRUE;
        break;
      case drag_RightEdge:
        block.parent.min.x = block.screenrect.min.x + SMALLEST + 4;
        block.parent.max.x = wstate.openblock.screenrect.max.x;
        drag_dovert(&block, bottom, top);
        drag_sidemove.max.x = TRUE;
        rightedge = TRUE;
        break;
      case drag_TopEdge:
        block.parent.min.y = block.screenrect.min.y + SMALLEST;
        block.parent.max.y = wstate.openblock.screenrect.max.y;
        drag_dohoriz(&block, left, right);
        drag_sidemove.min.y = TRUE;
        topedge = TRUE;
        break;
    }
  }
  else
  /* Move nearest corner */
  /* Limit edges to min possible size of icon or edge of window */
  {
    if (left < right)
    {
      block.parent.min.x = wstate.openblock.screenrect.min.x;
      block.parent.max.x = block.screenrect.max.x - SMALLEST;
      drag_sidemove.min.x = TRUE;
      leftedge = TRUE;
    }
    else
    {
      block.parent.min.x = block.screenrect.min.x + SMALLEST + 4;
      block.parent.max.x = wstate.openblock.screenrect.max.x;
      drag_sidemove.max.x = TRUE;
      rightedge = TRUE;
    }
    if (bottom <= top)
    {
      block.parent.min.y = wstate.openblock.screenrect.min.y;
      block.parent.max.y = block.screenrect.max.y - SMALLEST - 4;
      drag_sidemove.max.y = TRUE;
      bottomedge = TRUE;
    }
    else
    {
      block.parent.min.y = block.screenrect.min.y + SMALLEST;
      block.parent.max.y = wstate.openblock.screenrect.max.y;
      drag_sidemove.min.y = TRUE;
      topedge = TRUE;
    }
  }

  /* Change which edge is moveable if appropriate */
  if (left < right)
  {
    block.screenrect.min.x ^= block.screenrect.max.x;
    block.screenrect.max.x ^= block.screenrect.min.x;
    block.screenrect.min.x ^= block.screenrect.max.x;
  }
  if (top < bottom)
  {
    block.screenrect.min.y ^= block.screenrect.max.y;
    block.screenrect.max.y ^= block.screenrect.min.y;
    block.screenrect.min.y ^= block.screenrect.max.y;
  }

  /* Bodge (from FormEd!) */
  block.parent.min.x += block.screenrect.min.x -
  			      block.screenrect.max.x;
  block.parent.max.y += block.screenrect.max.y -
  			      block.screenrect.min.y;

  dragext_dragbox(&block);

  /* Find which edge(s) are moving to set correct pointer shape */
  if (topedge)
  {
    if (leftedge)
    {
      /* top left */
      strncpy(ptr_name, "ptr_autoscrd", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
    else if (rightedge)
    {
      /* top right */
      strncpy(ptr_name, "ptr_autoscre", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
    else
    {
      /* top only */
      strncpy(ptr_name, "ptr_autoscrv", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
  }
  else if (bottomedge)
  {
    if (leftedge)
    {
      /* bottom left */
      strncpy(ptr_name, "ptr_autoscre", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
    else if (rightedge)
    {
      /* bottom right */
      strncpy(ptr_name, "ptr_autoscrd", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
    else
    {
      /* bottom only */
      strncpy(ptr_name, "ptr_autoscrv", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
  }
  else if (leftedge)
  {
    if (!topedge && !bottomedge) /* otherwise should have been caught above */
    {
      /* left only */
      strncpy(ptr_name, "ptr_autoscrh", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
  }
  else if (rightedge)
  {
    if (!topedge && !bottomedge) /* otherwise should have been caught above */
    {
      /* right only */
      strncpy(ptr_name, "ptr_autoscrh", 12);
      Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 2, 0, 0, 0, 0));
    }
  }
}

static BOOL drag_esccancel(event_pollblock *event,void *reference)
{
  if (event->data.key.code == keycode_ESCAPE)
  {
    Wimp_DragBox(NULL);
    Event_Release(event_USERDRAG,event_ANY,event_ANY,
                (event_handler) drag_selectcontents,reference);
    Event_Release(event_KEY,event_ANY,event_ANY,drag_esccancel,reference);
    return TRUE;
  }
  return FALSE;
}

void drag_rubberbox(event_pollblock *event,void *broworview,BOOL viewer)
{
  drag_block drag;
  window_state wstate;

  drag.type = drag_RUBBERBOX;
  drag.screenrect.min.x = event->data.mouse.pos.x - 4;
  drag.screenrect.min.y = event->data.mouse.pos.y - 4;
  drag.screenrect.max.x = event->data.mouse.pos.x + 4;
  drag.screenrect.max.y = event->data.mouse.pos.y + 4;

  /* Confine drag to window */
  Wimp_GetWindowState(event->data.mouse.window,&wstate);
  drag.parent = wstate.openblock.screenrect;
  /* Bodge */
  drag.parent.min.x -= 8;
  drag.parent.max.y += 8;

  drag_selallref.window = event->data.mouse.window;
  drag_selallref.adjust = event->data.mouse.button.data.dragadjust;
  drag_selallref.broworview.winentry = broworview;
  drag_selallref.viewer = viewer;
  Event_Claim(event_USERDRAG,event_ANY,event_ANY,
              (event_handler) drag_selectcontents,&drag_selallref);
  Event_Claim(event_KEY,event_ANY,event_ANY,drag_esccancel,&drag_selallref);

  Wimp_DragBox(&drag);
}

BOOL drag_selectcontents(event_pollblock *event,drag_ref *reference)
{
  window_info winfo;
  window_state wstate;
  wimp_box dragbox;
  int icon;

  /* Coordinates may be wrong way round */
  dragbox = event->data.screenrect;
  if (dragbox.max.x < dragbox.min.x)
  {
    dragbox.min.x = event->data.screenrect.max.x;
    dragbox.max.x = event->data.screenrect.min.x;
  }
  if (dragbox.max.y < dragbox.min.y)
  {
    dragbox.min.y = event->data.screenrect.max.y;
    dragbox.max.y = event->data.screenrect.min.y;
  }
  /* Convert screen coords of drag to work area */
  Wimp_GetWindowState(reference->window,&wstate);
  Coord_RectToWorkArea(&dragbox,
  		       (convert_block *) &wstate.openblock.screenrect);

  /* Find number of icons in window */
  Window_GetInfo3(reference->window,&winfo);
  /* Find all icons enclosed by drag and select them */
  for (icon = 0; icon < winfo.block.numicons; icon++)
  {
    icon_block istate;

    Wimp_GetIconState(reference->window,icon,&istate);
    if (!istate.flags.data.deleted &&
    	Coord_RectsOverlap(&dragbox,&istate.workarearect))
    {
      if (reference->viewer)
        viewer_setselect(reference->broworview.winentry,icon,
          	reference->adjust ? !istate.flags.data.selected : TRUE);
      else
        Icon_SetSelect(reference->window,icon,
        	reference->adjust ? !istate.flags.data.selected : TRUE);
    }
  }

  if (count_selections(reference->window))
  {
    if (reference->viewer)
    {
      selection_viewer = reference->broworview.winentry;
      viewer_selection_withmenu = FALSE;
    }
    else
    {
      selection_browser = reference->broworview.browser;
      browser_selection_withmenu = FALSE;
    }
  }
  else
  {
    if (reference->viewer)
    {
      selection_viewer = NULL;
      viewer_selection_withmenu = TRUE;
    }
    else
    {
      selection_browser = NULL;
      browser_selection_withmenu = TRUE;
    }
  }

  /* Affect tool pane */
  if (reference->viewer && choices->viewtools)
    viewtools_shadeapp(reference->broworview.winentry);
  else if (!reference->viewer && choices->browtools)
    browtools_shadeapp(reference->broworview.browser);

  /* Release self */
  Event_Release(event_USERDRAG,event_ANY,event_ANY,
  		(event_handler) drag_selectcontents,reference);
  Event_Release(event_KEY,event_ANY,event_ANY,drag_esccancel,reference);
  return TRUE;
}

drag_edgetype drag_nearestedge(const wimp_rect *bounds, int x, int y)
{
  int left, bottom, right, top;
  int horiz, vert;
  drag_edgetype result = drag_LeftEdge;

  left = x - bounds->min.x;
  bottom = y - bounds->min.y;
  right = bounds->max.x - x;
  top = bounds->max.y - y;

  horiz = left < right ? left : right;
  vert = bottom < top ? bottom : top;

  /* At first stage result is only concerned with horiz/vert */
  if (horiz < NEAREDGE && vert < NEAREDGE)
  {
    float hratio = (float) horiz /
    	((float) bounds->max.x - (float) bounds->min.x);
    float vratio = (float) vert /
    	((float) bounds->max.y - (float) bounds->min.y);

    if (hratio < vratio)
      result = drag_LeftEdge;
    else
      result = drag_BottomEdge;
  }
  else
  {
    if (horiz < vert)
      result = drag_LeftEdge;
    else
      result = drag_BottomEdge;
  }

  /* Now fine tune */
  if (result == drag_LeftEdge)
  {
    if (right < left)
      result = drag_RightEdge;
  }
  else
  {
    if (top < bottom)
      result = drag_TopEdge;
  }

  return result;
}
