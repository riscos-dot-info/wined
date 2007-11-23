/* picker.c */

#include "common.h"

#include "icnedit.h"
#include "picker.h"
#include "viewex.h"

browser_winentry picker_winentry;

/* Number of viewers open */
static int picker_knownactive;

void picker_init()
{
  char *indirected;
  browser_winblock *templat;
  int i; /* Sorry, but I'm using this for lots of different things */
  Debug_Printf("picker_init");

  picker_knownactive = 0;

  /* Initialise winentry */
  LinkList_Init(&picker_winentry.header);
  picker_winentry.icon = 0;
  strcpy(picker_winentry.identifier,"Picker");
  picker_winentry.fontarray = 0;
  picker_winentry.status = status_CLOSED;
  picker_winentry.browser = 0;

  /* Load templat definition */
  templat = (void *) templates_load("IcnPicker",0,0,&indirected,0);

  /* Copy to a winentry block */
  i = sizeof(window_block) + templat->window.numicons * sizeof(icon_block);
  if (!flex_alloc((flex_ptr) &picker_winentry.window,i))
  {
    MsgTrans_Report(messages,"PickMem",FALSE);
    free(templat);
    free(indirected);
    return;
  }
  memcpy(picker_winentry.window,templat,i);

  /* Process indirected data; note we keep original templat for reference,
     new copy's indirected pointers would get messed up by
     icnedit_addindirected() */
  for (i = 0;i < templat->window.numicons;i++)
  {
    if (templat->icon[i].flags.data.indirected)
    {
      if (templat->icon[i].flags.data.text ||
    	  (templat->icon[i].flags.data.sprite &&
    	   templat->icon[i].data.indirectsprite.nameisname))
      {
        picker_winentry.window->icon[i].data.indirecttext.buffer = (char *)
          icnedit_addindirectstring(&picker_winentry,
            templat->icon[i].data.indirecttext.buffer);
      }
      if (templat->icon[i].flags.data.text)
      {
        picker_winentry.window->icon[i].data.indirecttext.validstring =
          (char *) icnedit_addindirectstring(&picker_winentry,
                   templat->icon[i].data.indirecttext.validstring);
      }
    }
  }

  /* Free originals */
  free(templat);
  free(indirected);
}

void picker_open()
{
  browser_winblock *winblock;
  window_state wstate;
/*
  wimp_point corner;
  wimp_point size;
*/

  if (!picker_winentry.status)
  {
    /* Make winblock */
    if (!viewer_makewinblock(&picker_winentry,FALSE,TRUE,&winblock))
      return;

    /* Create window */
    if (!viewer_createwindow(&picker_winentry,winblock))
    {
      free(winblock);
      return;
    }
    free(winblock);
  }

  /* Set window's openblock to just above right of icon bar */
/*
  Wimp_GetWindowState(window_ICONBAR,&wstate);
  corner = wstate.openblock.screenrect.max;
*/
  Wimp_GetWindowState(picker_winentry.handle,&wstate);
/*
  size.x = wstate.openblock.screenrect.max.x -
  	   wstate.openblock.screenrect.min.x;
  size.y = wstate.openblock.screenrect.max.y -
  	   wstate.openblock.screenrect.min.y;
  wstate.openblock.screenrect.min.x = corner.x - size.x;
  wstate.openblock.screenrect.max.x = corner.x;
  wstate.openblock.screenrect.min.y = corner.y;
  wstate.openblock.screenrect.max.y = corner.y + size.y;
*/
  wstate.openblock.behind = -1;

  Wimp_OpenWindow(&wstate.openblock);
  picker_winentry.status = status_EDITING;
  viewer_claimeditevents(&picker_winentry);
}

void picker_close()
{
  if (picker_winentry.status)
    viewer_close(&picker_winentry);
}

void picker_activeinc()
{
  picker_knownactive++;
  picker_open();
}

void picker_activedec()
{
  if (--picker_knownactive < 0)
    picker_knownactive = 0;
  if (picker_knownactive == 0)
    picker_close();
}

