/* templates.c */
/* templat loading and memory allocation */

#include <string.h>
#include <stdlib.h>

#include "DeskLib:Error.h"
#include "DeskLib:WimpSWIs.h"

#include "globals.h"
#include "templates.h"

window_block *templates_load(char *name,int *index,font_array *font,
                             char **indbuffer,int *indsize)
{
  template_block templat;
  char id[12];

  /* Find sizes */
  templat.buffer = 0;
  if (font)
    templat.font = font;
  else
    templat.font = (font_array *)-1;
  strcpy(id,name);
  templat.name = id;
  if (index)
    templat.index = *index;
  else
    templat.index = 0;
  Error_CheckFatal(Wimp_LoadTemplate(&templat));
  if (!templat.index)
    MsgTrans_ReportPS(messages,"NoTemp",TRUE,name,0,0,0);

  /* Allocate memory */
  templat.buffer = malloc((int) templat.buffer);
  if (!templat.buffer)
    MsgTrans_ReportPS(messages,"TempMem",TRUE,name,0,0,0);
  templat.workend = templat.workfree;
  /* Return size of ind data here, while we know it */
  if (indsize)
    *indsize = (int) templat.workend;
  if (templat.workend)
  {
    templat.workfree = malloc((int) templat.workend);
    if (!templat.workfree)
    MsgTrans_ReportPS(messages,"TempMem",TRUE,name,0,0,0);
  }
  else
    templat.workfree = 0;
  /* Return indirected buffer here, before templat load alters it */
  if (indbuffer)
    *indbuffer = templat.workfree;
  templat.workend += (int) templat.workfree;

  /* Load templat */
  if (index)
    templat.index = *index;
  else
    templat.index = 0;
  Error_CheckFatal(Wimp_LoadTemplate(&templat));

  /* Return info to caller */
  if (index)
    *index = templat.index;
  return templat.buffer;
}
