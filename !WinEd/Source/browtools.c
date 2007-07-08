/* browtools.c */

#include "common.h"

#include "browser.h"
#include "browtools.h"
#include "title.h"

typedef enum {
  browtools_CREATE,
  browtools_SELALL,
  browtools_CLEARSEL,
  browtools_SAVE,
  browtools_COPY,
  browtools_RENAME,
  browtools_DELETE,
  browtools_STATS
} browtools_icons;

/* Leaf menu is opened from this file to save cluttering up browser.c more */
extern menu_ptr browser_leafmenu;
extern char browser_leafdata[];

/* Click event handlers */
BOOL browtools_create(event_pollblock *event,void *reference);
BOOL browtools_selall(event_pollblock *event,void *reference);
BOOL browtools_save(event_pollblock *event,void *reference);
BOOL browtools_copy(event_pollblock *event,void *reference);
BOOL browtools_rename(event_pollblock *event,void *reference);
BOOL browtools_delete(event_pollblock *event,void *reference);
BOOL browtools_stats(event_pollblock *event,void *reference);

static window_block *browtools_template;

int browtools_paneheight;

void browtools_init()
{
  browtools_template = templates_load("BrowTools",0,0,0,0);
  browtools_template->spritearea = browser_sprites;
  browtools_paneheight = browtools_template->screenrect.max.y -
                         browtools_template->screenrect.min.y;
}

void browtools_newpane(browser_fileinfo *browser)
{
  Error_Check(Wimp_CreateWindow(browtools_template,&browser->pane));
  Event_Claim(event_CLICK,browser->pane,browtools_CREATE,
              browtools_create,browser);
  Event_Claim(event_CLICK,browser->pane,browtools_SELALL,
              browtools_selall,browser);
  Event_Claim(event_CLICK,browser->pane,browtools_SAVE,
              browtools_save,browser);
  Event_Claim(event_CLICK,browser->pane,browtools_COPY,
              browtools_copy,browser);
  Event_Claim(event_CLICK,browser->pane,browtools_RENAME,
              browtools_rename,browser);
  Event_Claim(event_CLICK,browser->pane,browtools_DELETE,
              browtools_delete,browser);
  Event_Claim(event_CLICK,browser->pane,browtools_STATS,
              browtools_stats,browser);
  help_claim_window(browser->pane,"BT");
}

void browtools_open(window_openblock *openblock,browser_fileinfo *browser,
	BOOL parent)
{
  window_state panestate/*,parentstate*/;
  wimp_rect rqopen;

  /* Remember corner of where parent is meant to be opened */
  rqopen = openblock->screenrect;

  /* Open pane relative to this */
  Wimp_GetWindowState(browser->pane,&panestate);
  panestate.openblock.screenrect = openblock->screenrect;
  panestate.openblock.screenrect.min.y =
    panestate.openblock.screenrect.max.y - browtools_paneheight;
  if (openblock->behind != browser->pane)
  {
    panestate.openblock.behind = openblock->behind;
    openblock->behind = browser->pane;
  }
  Wimp_OpenWindow(&panestate.openblock);

  /* Open parent behind pane */
  if (parent) Wimp_OpenWindow(openblock);

  /* If top left corner isn't where expected reopen pane */
  if (rqopen.min.x != openblock->screenrect.min.x ||
  	rqopen.min.y != openblock->screenrect.min.y ||
  	rqopen.max.x != openblock->screenrect.max.x ||
  	rqopen.max.y != openblock->screenrect.max.y)
  {
    panestate.openblock = *openblock;
    panestate.openblock.screenrect.min.y =
      panestate.openblock.screenrect.max.y - browtools_paneheight;
    panestate.openblock.window = browser->pane;
    Wimp_OpenWindow(&panestate.openblock);
  }
}

/* Close and delete tool pane */
void browtools_deletepane(browser_fileinfo *browser)
{
  Event_ReleaseWindow(browser->pane);
  EventMsg_ReleaseWindow(browser->pane);
  Wimp_CloseWindow(browser->pane);
  Wimp_DeleteWindow(browser->pane);
  browser->pane = 0;
}

void browtools_unshade(window_handle pane)
{
  icon_handle i;

  for (i = browtools_CREATE; i <= browtools_STATS; i++)
    Icon_Unshade(pane,i);
}

void browtools_shadenosel(window_handle pane)
{
  Icon_Shade(pane,browtools_COPY);
  Icon_Shade(pane,browtools_RENAME);
  Icon_Shade(pane,browtools_DELETE);
}

void browtools_shademulsel(window_handle pane)
{
  Icon_Shade(pane,browtools_COPY);
  Icon_Shade(pane,browtools_RENAME);
  Icon_Unshade(pane,browtools_DELETE);
}

void browtools_shadeapp(browser_fileinfo *browser)
{
  switch (count_selections(browser->window))
  {
    case 0:
      browtools_shadenosel(browser->pane);
      break;
    case 1:
      browtools_unshade(browser->pane);
      break;
    default:
      browtools_shademulsel(browser->pane);
      break;
  }
}

BOOL browtools_selall(event_pollblock *event,void *reference)
{
  switch (event->data.mouse.button.value)
  {
    case button_SELECT:
      browcom_selall(reference);
      return TRUE;
    case button_ADJUST:
      browcom_clearsel();
      return TRUE;
  }
  return FALSE;
}

BOOL browtools_save(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = (browser_fileinfo *) reference;

  switch (event->data.mouse.button.value)
  {
    case button_ADJUST:
      /* We don't need to check for overwriting another file here as we're only saving to the location we loaded from */
      if (strchr(browser->title, ',') || strchr(browser->title, ':'))
        if (browser_save(browser->title,reference,FALSE))
          browser_settitle(browser,NULL,FALSE);
      return TRUE;
    case button_SELECT:
      browcom_save(reference,
      		event->data.mouse.pos.x-64,event->data.mouse.pos.y+64,
               FALSE,FALSE);
      return TRUE;
    default:
      return FALSE;
  }
  return TRUE;
}

BOOL browtools_delete(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  browcom_delete(reference);
  return TRUE;
}

BOOL browtools_stats(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  browcom_stats(reference);
  return TRUE;
}

BOOL browtools_create(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  browcom_create(FALSE, 0,0, reference);
  return TRUE;
}

BOOL browtools_copy(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  browcom_copy(FALSE, 0,0, reference);
  return TRUE;
}

BOOL browtools_rename(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;
  browcom_rename(FALSE, 0,0, reference);
  return TRUE;
}
