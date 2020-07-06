/* monitor.c */

#include "common.h"

#include "monitor.h"
#include "picker.h"

/* Icon names exported by WinEd */

/* Icons for window monitor */
typedef enum {
  monitor_WINDOW = 4,
  monitor_ICON = 5,
  monitor_XMIN = 6,
  monitor_YMIN = 7,
  monitor_XMAX = 8,
  monitor_YMAX = 9,
  monitor_SIZE = 10,
  monitor_NAME = 12
} monitor_icons;

BOOL monitor_isactive;
BOOL monitor_moved = FALSE;
BOOL monitor_dragging = FALSE;

BOOL monitor_isopen;

window_handle monitor_window;

browser_winentry *monitor_winentry;

static int monitor_numactive;

static icon_handle monitor_lasticon = event_ANY;

BOOL monitor_close(event_pollblock *event,void *reference);

void monitor_init()
{
  window_block *templat;
  Log(log_DEBUG, "monitor_init");

  templat = templates_load("monitor",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&monitor_window));
  free(templat);

  Event_Claim(event_OPEN,monitor_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,monitor_window,event_ANY,monitor_close,0);
  help_claim_window(monitor_window,"MON");

  monitor_numactive = 0;
  monitor_isactive = FALSE;
  monitor_isopen = FALSE;
}

void monitor_open()
{
  window_state wstate;
  Log(log_DEBUG, "monitor_open");

  Wimp_GetWindowState(monitor_window,&wstate);
  wstate.openblock.behind = -1;
  Wimp_OpenWindow(&wstate.openblock);
  monitor_isopen = TRUE;
}

BOOL monitor_close(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "monitor_close");
  Wimp_CloseWindow(monitor_window);
  monitor_isopen = FALSE;
  if (monitor_isactive)
    monitor_deactivate(event,monitor_winentry);
  return TRUE;
}

void monitor_activeinc()
{
  Log(log_DEBUG, "monitor_activeinc");
  monitor_numactive++;
  monitor_open();
}

void monitor_activedec()
{
  Log(log_DEBUG, "monitor_activedec");
  if (--monitor_numactive <= 0)
  {
    monitor_numactive = 0;
    monitor_close(0,0);
  }
}

BOOL monitor_activate(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "monitor_activate");
  /* Set to winentry of viewer with pointer focus */
  monitor_winentry = reference;
  if (monitor_isopen && (!monitor_isactive))
  {
    char buffer[16];

    Event_Claim(event_NULL,event_ANY,event_ANY,monitor_update,reference);
    strcpycr(buffer,monitor_winentry->identifier);
    if (monitor_winentry->status == status_EDITING)
      strcat(buffer," *");
    Icon_SetText(monitor_window,monitor_WINDOW,buffer);
    monitor_isactive = TRUE;
  }
  return TRUE;
}

BOOL monitor_deactivate(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "monitor_deactivate");
  /* Check for the monitor_dragging flag as we want the monitor to stay active    */
  /* while we're dragging. This function gets called by a pointer-leave even when */
  /* a drag starts so would otherwise silence the monitor                         */
  if (monitor_isactive && (!monitor_dragging))
  {
    Event_Release(event_NULL,event_ANY,event_ANY,monitor_update,reference);
    monitor_isactive = FALSE;
    Icon_SetText(monitor_window,monitor_WINDOW,"");
    Icon_SetText(monitor_window,monitor_ICON,"");
    Icon_SetText(monitor_window,monitor_XMIN,"");
    Icon_SetText(monitor_window,monitor_YMIN,"");
    Icon_SetText(monitor_window,monitor_XMAX,"");
    Icon_SetText(monitor_window,monitor_YMAX,"");
    Icon_SetText(monitor_window,monitor_SIZE,"");
    Icon_SetText(monitor_window,monitor_NAME,"");
  }
  monitor_lasticon = event_ANY;
  return TRUE;
}

BOOL monitor_update(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  browser_winentry *winentry = reference;
  char buffer[16];
  wimp_point size;
  convert_block con;
  wimp_point new_pos, offset;
  wimp_rect final;
  int sizeoficname = 0;
  icon_block istate;

  /* Find size of icon name indirected icon data in monitor window */
  Wimp_GetIconState(monitor_window, monitor_NAME, &istate);
  if ((istate.flags.data.text) && (istate.flags.data.indirected))
    sizeoficname = istate.data.indirecttext.bufflen;

  /* Get info on icon under pointer */
  Wimp_GetPointerInfo(&ptrinfo);

  if (monitor_dragging)
  {
    /*
       This bit is separate to the standard update below because while dragging, the pointer
       will probably not be over the icon we're dragging, which is what we need to know about.
       The solution is done by taking a record of the pointer position (drag_startpoint) and
       the icon's stats when the drag starts (in drag_moveicon, drag_moveselection or
       drag_resizeicon) then just working out how the change in pointer position will affect
       the eventual form of the moved icon.
    */

    Window_GetCoords(ptrinfo.window, &con);
    new_pos.x = Coord_XToWorkArea(ptrinfo.pos.x, &con);
    new_pos.y = Coord_XToWorkArea(ptrinfo.pos.y, &con);

    offset.x = new_pos.x - drag_startpoint.x;
    offset.y = new_pos.y - drag_startpoint.y;

    final.min.x = drag_startdims.min.x + offset.x;
    final.min.y = drag_startdims.min.y + offset.y;
    final.max.x = drag_startdims.max.x + offset.x;
    final.max.y = drag_startdims.max.y + offset.y;

    /* Drags are always going to be rounded down to the nearest multiple of 4, so reflect that in monitor */
    /* We can resize by 1, if Ctrl is held down. This is a bit inconsistent so isn't documented!         */
    if (!(    (!(drag_sidemove.min.x && drag_sidemove.max.y && drag_sidemove.max.x && drag_sidemove.min.y))
         && (Kbd_KeyDown(inkey_CTRL)) ))
      /* E.g. we're not (resizing with shift down) */
      round_down_box(&final);

    if (drag_sidemove.min.x) Icon_SetInteger(monitor_window,monitor_XMIN, final.min.x);
    if (drag_sidemove.max.y) Icon_SetInteger(monitor_window,monitor_YMIN, final.min.y);
    if (drag_sidemove.max.x) Icon_SetInteger(monitor_window,monitor_XMAX, final.max.x);
    if (drag_sidemove.min.y) Icon_SetInteger(monitor_window,monitor_YMAX, final.max.y);

    if (!(drag_sidemove.min.x && drag_sidemove.max.y && drag_sidemove.max.x && drag_sidemove.min.y))
    /* We're resizing, not moving */
    {
      size.x = Icon_GetInteger(monitor_window, monitor_XMAX) - Icon_GetInteger(monitor_window, monitor_XMIN);
      size.y = Icon_GetInteger(monitor_window, monitor_YMAX) - Icon_GetInteger(monitor_window, monitor_YMIN);

      snprintf(buffer, 16, "%d x %d", size.x, size.y);
      Icon_SetText(monitor_window, monitor_SIZE, buffer);
    }
  }
  else
  {
    if ( (ptrinfo.window == winentry->handle) && ((ptrinfo.icon != monitor_lasticon) || monitor_moved) )
    {
      monitor_moved = FALSE;
      if (ptrinfo.icon == -1) /* Show dimensions of visible part of WA */
      {
        size.x = winentry->window->window.screenrect.max.x -
        	       winentry->window->window.screenrect.min.x;
        size.y = winentry->window->window.screenrect.max.y -
        	       winentry->window->window.screenrect.min.y;
        MsgTrans_Lookup(messages,"WorkArea",buffer,16);
        Icon_SetText(monitor_window,monitor_ICON,buffer);
        Icon_SetText(monitor_window,monitor_NAME,""); /* Clear icon name */
        Icon_SetInteger(monitor_window,monitor_XMIN,
        		      winentry->window->window.workarearect.min.x +
        		      winentry->window->window.scroll.x);
        Icon_SetInteger(monitor_window,monitor_YMIN,
        		      winentry->window->window.workarearect.max.y +
        		      winentry->window->window.scroll.y - size.y);
        Icon_SetInteger(monitor_window,monitor_XMAX,
        		      winentry->window->window.workarearect.min.x +
        		      winentry->window->window.scroll.x + size.x);
        Icon_SetInteger(monitor_window,monitor_YMAX,
        		      winentry->window->window.workarearect.max.y +
        		      winentry->window->window.scroll.y);
        sprintf(buffer,"%d x %d",size.x,size.y);
        Icon_SetText(monitor_window,monitor_SIZE,buffer);
      }
      else
      {
        int retlen;
        char icname[sizeoficname];

        retlen = extract_iconname(winentry, ptrinfo.icon, icname, sizeoficname);

        if (retlen)
        {
          Log(log_DEBUG, "Over icon named '%s' (name length:%d)", LogPreBuffer(icname), retlen);
        }
        else
          /* Clear icon name if no name was found */
          icname[0] = 0;

        Icon_SetInteger(monitor_window, monitor_ICON, ptrinfo.icon);
        Icon_SetText(monitor_window    ,monitor_NAME, icname);
        Icon_SetInteger(monitor_window, monitor_XMIN, winentry->window->icon[ptrinfo.icon].workarearect.min.x);
        Icon_SetInteger(monitor_window, monitor_YMIN, winentry->window->icon[ptrinfo.icon].workarearect.min.y);
        Icon_SetInteger(monitor_window, monitor_XMAX, winentry->window->icon[ptrinfo.icon].workarearect.max.x);
        Icon_SetInteger(monitor_window, monitor_YMAX, winentry->window->icon[ptrinfo.icon].workarearect.max.y);
        sprintf(buffer,"%d x %d",
        	      winentry->window->icon[ptrinfo.icon].workarearect.max.x -
        	      winentry->window->icon[ptrinfo.icon].workarearect.min.x,
        	      winentry->window->icon[ptrinfo.icon].workarearect.max.y -
        	      winentry->window->icon[ptrinfo.icon].workarearect.min.y);
        Icon_SetText(monitor_window,    monitor_SIZE, buffer);
      }
      monitor_lasticon = ptrinfo.icon;
    }
  }

  return FALSE;
}
