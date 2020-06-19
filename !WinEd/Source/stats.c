/* stats.c */

#include "common.h"

#include "DeskLib:Coord.h"

#include "icnedit.h"
#include "stats.h"

/* Template definition held in RAM */
static window_block *stats_template;

/* Dimensions of virtual icons */
static int stats_height;
static int stats_id_left,stats_id_right;
static int stats_ic_left,stats_ic_right;
static int stats_win_left,stats_win_right;
static int stats_ind_left,stats_ind_right;
/*static int stats_tot_left,stats_tot_right;*/

/* Icon numbers (for finding above dimensions) */
typedef enum {
  stats_BGRND,
  stats_ID,
  stats_IC,
  stats_WIN,
  stats_IND/*,
  stats_TOT  */
} stats_icons;

/* Event handlers */
BOOL stats_redraw(event_pollblock *event,void *reference);
BOOL stats_closeevent(event_pollblock *event,void *reference);
BOOL stats_scrollevent(event_pollblock *event,void *reference);

/* Redraw necessary elements of a row */
void stats_drawline(int line,wimp_rect *rect,char *id,int numicons,
     		    int winsize,int indsize,BOOL largest);

/* Display font and file size data */
void stats_other_info(int line,wimp_rect *rect,browser_fileinfo *browser);

/* Scan for total size of indirected data in a template (winentry)
 */
void stats_count_indirected(browser_winentry *winentry);

/* Virtual icon */
static icon_block stats_vicon;
static char stats_buffer[80];

void stats_init()
{
  icon_block *icons;

  stats_template = templates_load("Stats",0,0,0,0);
  icons = (icon_block *) ((int) stats_template + sizeof(window_block));
  stats_height = icons[stats_BGRND].workarearect.max.y -
  	       	 icons[stats_BGRND].workarearect.min.y;
  stats_id_left = icons[stats_ID].workarearect.min.x;
  stats_id_right = icons[stats_ID].workarearect.max.x;
  stats_ic_left = icons[stats_IC].workarearect.min.x;
  stats_ic_right = icons[stats_IC].workarearect.max.x;
  stats_win_left = icons[stats_WIN].workarearect.min.x;
  stats_win_right = icons[stats_WIN].workarearect.max.x;
  stats_ind_left = icons[stats_IND].workarearect.min.x;
  stats_ind_right = icons[stats_IND].workarearect.max.x;
/*
  stats_tot_left = icons[stats_TOT].workarearect.min.x;
  stats_tot_right = icons[stats_TOT].workarearect.max.x;
*/

  stats_vicon.data.indirecttext.buffer = stats_buffer;
  stats_vicon.data.indirecttext.bufflen = 80;
  stats_vicon.data.indirecttext.validstring = "";
  stats_vicon.flags.value = 0;
  stats_vicon.flags.data.text = 1;
  stats_vicon.flags.data.vcentre = 1;
  stats_vicon.flags.data.indirected = 1;
  stats_vicon.flags.data.rightjustify = 1;
  stats_vicon.flags.data.buttontype = iconbtype_NEVER;
  stats_vicon.flags.data.esg = 0;
  stats_vicon.flags.data.foreground = 7;
  stats_vicon.flags.data.background = 2;
}

void stats_open(browser_fileinfo *browser)
{
  window_state wstate;
  int x,y;
  mouse_block ptrinfo;

  if (!browser->stats)
  {
    Error_Check(Wimp_CreateWindow(stats_template,&browser->stats));
    Event_Claim(event_REDRAW,browser->stats,event_ANY,stats_redraw,browser);
    Event_Claim(event_OPEN,browser->stats,event_ANY,Handler_OpenWindow,0);
    Event_Claim(event_CLOSE,browser->stats,event_ANY,stats_closeevent,browser);
    Event_Claim(event_SCROLL,browser->stats,event_ANY,stats_scrollevent,browser);
    help_claim_window(browser->stats,"STAT");
  }

  stats_update(browser);
  Error_Check(Wimp_GetWindowState(browser->stats,&wstate));
  Wimp_GetPointerInfo(&ptrinfo);
  x = wstate.openblock.screenrect.max.x - wstate.openblock.screenrect.min.x;
  y = wstate.openblock.screenrect.max.y - wstate.openblock.screenrect.min.y;
  wstate.openblock.screenrect.min.x = ptrinfo.pos.x - 64;
  wstate.openblock.screenrect.max.y = ptrinfo.pos.y + 64;
  wstate.openblock.screenrect.max.x = ptrinfo.pos.x - 64 + x;
  wstate.openblock.screenrect.min.y = ptrinfo.pos.y + 64 - y;
  wstate.openblock.behind = -1;
  Error_Check(Wimp_OpenWindow(&wstate.openblock));
}

void stats_close(browser_fileinfo *browser)
{
  Wimp_DeleteWindow(browser->stats);
  EventMsg_ReleaseWindow(browser->stats);
  Event_ReleaseWindow(browser->stats);
  browser->stats = 0;
}

void stats_update(browser_fileinfo *browser)
{
  window_state wstate;
  wimp_rect extent;
  browser_winentry *winentry;
  int ltot = 0;
  int i;

  if (!browser->stats)
    return;

  /* Recalculate sizes */
  browser->estsize = sizeof(template_header) + 4; /* 4 for terminating zero */
  if (browser->fontinfo)
    browser->estsize += flex_size((flex_ptr) & browser->fontinfo);
  for (i = 0, winentry = LinkList_NextItem(&browser->winlist);
       winentry;
       winentry = LinkList_NextItem(winentry), ++i)
  {
    int wtot = flex_size((flex_ptr) &winentry->window);

    if (wtot > ltot)
    {
      ltot = wtot;
      browser->largest = i;
    }
    stats_count_indirected(winentry);
    browser->estsize += sizeof(template_index) + wtot;
  }

  extent.min.x = 0;
  extent.max.x = stats_ind_right + 8;
  extent.min.y = - stats_height * (browser->numwindows + 3);
  extent.max.y = 0;
  Wimp_SetExtent(browser->stats,&extent);

  Wimp_GetWindowState(browser->stats,&wstate);
  if (!wstate.flags.data.open)
    return;
  Wimp_OpenWindow(&wstate.openblock);
  extent = wstate.openblock.screenrect;
  Coord_RectToWorkArea(&extent,(convert_block *) &wstate.openblock.screenrect);
  wstate.openblock.screenrect = extent;
  Wimp_ForceRedraw((window_redrawblock *) &wstate);
}

BOOL stats_redraw(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;
  window_redrawblock redraw;
  BOOL more;
  wimp_rect wkarea;
  int line;
  int totalwin;
  int totalind;
  int totalicn;
  char total[13];
  browser_winentry *winentry;

  MsgTrans_Lookup(messages,"Total",total,sizeof(total));
  redraw.window = browser->stats;
  Wimp_RedrawWindow(&redraw,&more);
  while (more)
  {
    totalwin = totalind = totalicn = 0;
    winentry = LinkList_NextItem(&browser->winlist);
    wkarea = redraw.cliprect;
    Coord_RectToWorkArea(&wkarea,(convert_block *) &redraw.rect);
    for (line = 0; line <= browser->numwindows + 1; line++)
    {
      if (line == browser->numwindows)
      {
        stats_vicon.flags.data.hcentre = 0;
        stats_vicon.flags.data.rightjustify = 1;
        stats_vicon.flags.data.filled = 1;
        stats_drawline(line,&wkarea,total,totalicn,totalwin,totalind,FALSE);
      }
      else if (line == browser->numwindows + 1)
      {
        stats_vicon.flags.data.hcentre = 1;
        stats_vicon.flags.data.rightjustify = 0;
        stats_vicon.flags.data.filled = 0;
        stats_other_info(line,&wkarea,browser);
      }
      else
      {
        stats_vicon.flags.data.hcentre = 0;
        stats_vicon.flags.data.rightjustify = 1;
        stats_vicon.flags.data.filled = 0;
        stats_drawline(line,&wkarea,winentry->identifier,
        	       winentry->window->window.numicons,
        	       flex_size((flex_ptr) &winentry->window),
        	       winentry->indsize,
        	       line == browser->largest);
/*
        totalwin += sizeof(window_block) +
        	    winentry->window->window.numicons * sizeof(icon_block);
*/
        totalwin += flex_size((flex_ptr) &winentry->window);
        totalind += winentry->indsize;
        totalicn += winentry->window->window.numicons;
        winentry = LinkList_NextItem(winentry);
      }
    }
    Wimp_GetRectangle(&redraw,&more);
  }
  return TRUE;
}

BOOL stats_closeevent(event_pollblock *event,void *reference)
{
  stats_close(reference);
  return TRUE;
}

BOOL stats_scrollevent(event_pollblock *event,void *reference)
{
  scroll_window(event, stats_height, stats_height, 0);
  return TRUE;
}

void stats_drawline(int line,wimp_rect *rect,char *id,int numicons,
     		    int winsize,int indsize,BOOL largest)
{
  stats_vicon.workarearect.max.y = -stats_height * (line + 1);
  stats_vicon.workarearect.min.y = stats_vicon.workarearect.max.y -
  				   stats_height;


  if (rect->min.y > stats_vicon.workarearect.max.y ||
      rect->max.y < stats_vicon.workarearect.min.y)
    return;

  /* Identifier */
  if (rect->min.x < stats_id_right && rect->max.x > stats_id_left)
  {
    strcpy(stats_buffer,id);
    stats_vicon.workarearect.min.x = stats_id_left;
    stats_vicon.workarearect.max.x = stats_id_right;
    Wimp_PlotIcon(&stats_vicon);
  }

  /* Number of icons */
  if (rect->min.x < stats_ic_right && rect->max.x > stats_ic_left)
  {
    sprintf(stats_buffer,"%d",numicons);
    stats_vicon.workarearect.min.x = stats_ic_left;
    stats_vicon.workarearect.max.x = stats_ic_right;
    Wimp_PlotIcon(&stats_vicon);
  }

  /* Window size */
  if (rect->min.x < stats_win_right && rect->max.x > stats_win_left)
  {
    sprintf(stats_buffer,"%d",winsize);
    stats_vicon.workarearect.min.x = stats_win_left;
    stats_vicon.workarearect.max.x = stats_win_right;
    if (largest)
    {
      icon_block vicon = stats_vicon;
      vicon.flags.data.foreground = 0xb;
      Wimp_PlotIcon(&vicon);
    }
    else
      Wimp_PlotIcon(&stats_vicon);
  }

  /* Indirected size */
  if (rect->min.x < stats_ind_right && rect->max.x > stats_ind_left)
  {
    sprintf(stats_buffer,"%d",indsize);
    stats_vicon.workarearect.min.x = stats_ind_left;
    stats_vicon.workarearect.max.x = stats_ind_right;
    Wimp_PlotIcon(&stats_vicon);
  }

  /* Total size */
/*
  if (rect->min.x < stats_tot_right && rect->max.x > stats_tot_left)
  {
    sprintf(stats_buffer,"%d",winsize + indsize);
    stats_vicon.workarearect.min.x = stats_tot_left;
    stats_vicon.workarearect.max.x = stats_tot_right;
    Wimp_PlotIcon(&stats_vicon);
  }
*/
}

void stats_other_info(int line,wimp_rect *rect,browser_fileinfo *browser)
{
  stats_vicon.workarearect.max.y = -stats_height * (line + 1);
  stats_vicon.workarearect.min.y = stats_vicon.workarearect.max.y -
  				   stats_height;

  if (rect->min.y > stats_vicon.workarearect.max.y ||
      rect->max.y < stats_vicon.workarearect.min.y)
    return;

  stats_vicon.workarearect.min.x = stats_id_left;
  stats_vicon.workarearect.max.x = stats_ind_right;
  if (rect->min.x < stats_vicon.workarearect.max.x &&
      rect->max.x > stats_vicon.workarearect.min.x)
  {
    char numwins[8];
    char numfonts[8];
    char fsize[8];

    sprintf(numwins,"%d",browser->numwindows);
    sprintf(numfonts,"%d",browser->numfonts);
    sprintf(fsize,"%d",browser->estsize);
    MsgTrans_LookupPS(messages,"FInfo",stats_buffer,80,
    		      numwins,numfonts,fsize,0);
    Wimp_PlotIcon(&stats_vicon);
  }
}

void stats_count_indirected(browser_winentry *winentry)
{
  int i;

  winentry->indsize =
    icnedit_count_indirected(winentry,&winentry->window->window.title,
    			     &winentry->window->window.titleflags);

  for (i = 0; i < winentry->window->window.numicons; i++)
    winentry->indsize +=
      icnedit_count_indirected(winentry,&winentry->window->icon[i].data,
      			       &winentry->window->icon[i].flags);
}
