/* viewtools.c */

#include "common.h"

#include "viewcom.h"
#include "viewtools.h"

typedef enum {
  viewtools_SELALL,
  viewtools_CLEARSEL,
  viewtools_WORKAREA,
  viewtools_WINDOW,
  viewtools_TITLE,
  viewtools_DELETE,
  viewtools_RENUM,
  viewtools_ALIGN,
  viewtools_SPACE,
  viewtools_RESIZE,
  viewtools_FRAME,
  viewtools_VISAREA,
  viewtools_COORDS
} viewtools_icons;

/* Browser sprite area */
extern sprite_area browser_sprites;

/* Click event handlers */
BOOL viewtools_selall(event_pollblock *event,void *reference);
BOOL viewtools_workarea(event_pollblock *event,void *reference);
BOOL viewtools_visarea(event_pollblock *event,void *reference);
BOOL viewtools_window(event_pollblock *event,void *reference);
BOOL viewtools_title(event_pollblock *event,void *reference);
BOOL viewtools_delete(event_pollblock *event,void *reference);
BOOL viewtools_renum(event_pollblock *event,void *reference);
BOOL viewtools_align(event_pollblock *event,void *reference);
BOOL viewtools_space(event_pollblock *event,void *reference);
BOOL viewtools_resize(event_pollblock *event,void *reference);
BOOL viewtools_frame(event_pollblock *event,void *reference);
BOOL viewtools_coords(event_pollblock *event,void *reference);

static window_block *viewtools_template;

static int viewtools_paneheight,viewtools_panewidth;

void viewtools_init()
{
  viewtools_template = templates_load("WinTools",0,0,0,0);
  viewtools_template->spritearea = browser_sprites;
  viewtools_paneheight = viewtools_template->screenrect.max.y -
                         viewtools_template->screenrect.min.y;
  viewtools_panewidth = viewtools_template->screenrect.max.x -
                        viewtools_template->screenrect.min.x;
}

void viewtools_newpane(browser_winentry *winentry)
{
  Error_Check(Wimp_CreateWindow(viewtools_template,&winentry->pane));
  viewtools_shadeapp(winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_SELALL,
              viewtools_selall,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_WORKAREA,
              viewtools_workarea,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_VISAREA,
              viewtools_visarea,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_WINDOW,
              viewtools_window,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_TITLE,
              viewtools_title,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_DELETE,
              viewtools_delete,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_RENUM,
              viewtools_renum,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_ALIGN,
              viewtools_align,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_SPACE,
              viewtools_space,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_RESIZE,
              viewtools_resize,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_FRAME,
              viewtools_frame,winentry);
  Event_Claim(event_CLICK,winentry->pane,viewtools_COORDS,
              viewtools_coords,winentry);
  help_claim_window(winentry->pane,"VT");
}

void viewtools_open(window_openblock *openblock,window_handle pane)
{
  window_state panestate;
  int rqopen_x,rqopen_y;

  /* Remember corner of where parent is meant to be opened */
  rqopen_x = openblock->screenrect.min.x;
  rqopen_y = openblock->screenrect.max.y;

  /* Open pane relative to this */
  Wimp_GetWindowState(pane,&panestate);
  panestate.openblock.screenrect.max.x = openblock->screenrect.min.x - 1;
  panestate.openblock.screenrect.min.x = openblock->screenrect.min.x - 1 -
                                         viewtools_panewidth;
  panestate.openblock.screenrect.max.y = openblock->screenrect.max.y;
  panestate.openblock.screenrect.min.y = openblock->screenrect.max.y -
                                         viewtools_paneheight;
  if (panestate.openblock.screenrect.min.x < 0)
  {
    if (openblock->screenrect.min.x > 0)
    {
      panestate.openblock.screenrect.max.x = viewtools_panewidth;
      panestate.openblock.screenrect.min.x = 0;
    }
    else
    {
      panestate.openblock.screenrect.min.x = openblock->screenrect.min.x;
      panestate.openblock.screenrect.max.x =
      		panestate.openblock.screenrect.min.x + viewtools_panewidth;
    }
  }
  if (openblock->behind != pane)
  {
    panestate.openblock.behind = openblock->behind;
    openblock->behind = pane;
  }
  Wimp_OpenWindow(&panestate.openblock);

  /* Open parent behind pane */
  Wimp_OpenWindow(openblock);

  /* If top left corner isn't where expected reopen pane */
  if (rqopen_x != openblock->screenrect.min.x ||
      rqopen_y != openblock->screenrect.max.y)
  {
    panestate.openblock.screenrect.max.x = openblock->screenrect.min.x - 1;
    panestate.openblock.screenrect.min.x = openblock->screenrect.min.x - 1 -
                                           viewtools_panewidth;
    panestate.openblock.screenrect.max.y = openblock->screenrect.max.y;
    panestate.openblock.screenrect.min.y = openblock->screenrect.max.y -
                                           viewtools_paneheight;
    if (panestate.openblock.screenrect.min.x < 0)
    {
      if (openblock->screenrect.min.x > 0)
      {
        panestate.openblock.screenrect.max.x = viewtools_panewidth;
        panestate.openblock.screenrect.min.x = 0;
      }
      else
      {
        panestate.openblock.screenrect.min.x = openblock->screenrect.min.x;
        panestate.openblock.screenrect.max.x =
        	panestate.openblock.screenrect.min.x + viewtools_panewidth;
      }
    }
    Wimp_OpenWindow(&panestate.openblock);
  }
}

/* Close and delete tool pane */
void viewtools_deletepane(browser_winentry *winentry)
{
  EventMsg_ReleaseWindow(winentry->pane);
  Event_ReleaseWindow(winentry->pane);
  EventMsg_ReleaseWindow(winentry->pane);
  Wimp_CloseWindow(winentry->pane);
  Wimp_DeleteWindow(winentry->pane);
  winentry->pane = 0;
}

void viewtools_unshade(window_handle pane)
{
  icon_handle i;

  for (i = viewtools_SELALL; i <= viewtools_FRAME; i++)
    Icon_Unshade(pane,i);
  Icon_Shade(pane,viewtools_COORDS);
}

void viewtools_shadenosel(window_handle pane)
{
  Icon_Shade(pane,viewtools_DELETE);
  Icon_Shade(pane,viewtools_RENUM);
  Icon_Shade(pane,viewtools_ALIGN);
  Icon_Shade(pane,viewtools_SPACE);
  Icon_Shade(pane,viewtools_RESIZE);
  Icon_Shade(pane,viewtools_FRAME);
  Icon_Shade(pane,viewtools_COORDS);
}

void viewtools_shadeonesel(window_handle pane)
{
  Icon_Unshade(pane,viewtools_DELETE);
  Icon_Unshade(pane,viewtools_RENUM);
  Icon_Unshade(pane,viewtools_ALIGN);
  Icon_Unshade(pane,viewtools_SPACE);
  Icon_Unshade(pane,viewtools_RESIZE);
  Icon_Unshade(pane,viewtools_FRAME);
  Icon_Unshade(pane,viewtools_COORDS);
}

void viewtools_shadeapp(browser_winentry *winentry)
{
  switch (count_selections(winentry->handle))
  {
    case 0:
      viewtools_shadenosel(winentry->pane);
      break;
    case 1:
      viewtools_shadeonesel(winentry->pane);
      break;
    default:
      viewtools_unshade(winentry->pane);
      break;
  }
}

BOOL viewtools_selall(event_pollblock *event,void *reference)
{
  switch (event->data.mouse.button.value)
  {
    case button_SELECT:
      viewcom_selectall(reference);
      return TRUE;
    case button_ADJUST:
      viewcom_clearselection();
      return TRUE;
  }
  return FALSE;
}

BOOL viewtools_workarea(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_editworkarea(reference);
  return TRUE;
}

BOOL viewtools_visarea(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_editvisarea(reference);
  return TRUE;
}

BOOL viewtools_window(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_editwindow(reference);
  return TRUE;
}

BOOL viewtools_title(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_edittitle(reference);
  return TRUE;
}

BOOL viewtools_delete(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_delete(reference);
  return TRUE;
}

BOOL viewtools_renum(event_pollblock *event,void *reference)
{
  switch (event->data.mouse.button.value)
  {
    case button_SELECT:
      viewcom_renum(FALSE, 0, 0, reference);
      return TRUE;
    case button_ADJUST:
      viewcom_quickrenum(reference);
      return TRUE;
    default:
      return FALSE;
  }
  return FALSE;
}

BOOL viewtools_align(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_align(reference);
  return TRUE;
}

BOOL viewtools_space(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_spaceout(reference);
  return TRUE;
}

BOOL viewtools_resize(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_resize(reference);
  return TRUE;
}

BOOL viewtools_frame(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_border(reference);
  return TRUE;
}

BOOL viewtools_coords(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  viewcom_coords(reference);
  return TRUE;
}
