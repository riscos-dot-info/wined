/* usersprt.c */

#include <string.h>

#include "DeskLib:Coord.h"
#include "DeskLib:Error.h"
#include "DeskLib:File.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:SWI.h"
#include "DeskLib:WimpSWIs.h"
#include "Mem.h"

#include "common.h"
#include "globals.h"
#include "usersprt.h"

#ifdef HAVE_MEMCHECK
#include "MemCheck:MemCheck.h"
#endif

#undef SWI_Wimp_ReadSysInfo
#define SWI_Wimp_ReadSysInfo (SWI_XOS_Bit | 0x400f2)

sprite_area user_sprites;

void usersprt_modfilename(char *name)
{
  const char *suffix;
  SWI(1, 1, SWI_Wimp_ReadSysInfo, 2, &suffix);
#ifdef HAVE_MEMCHECK
  MemCheck_SetReadChecking(0);
#endif
  strcat(name, suffix);
#ifdef HAVE_MEMCHECK
  MemCheck_SetReadChecking(1);
#endif
  if (File_Size(name) <= 0)
    name[strlen(name)-2] = 0;
}

void usersprt_init()
{
  if (!flex_alloc((flex_ptr)
   &user_sprites,
  sizeof(sprite_areainfo)))
    WinEd_MsgTrans_Report(messages,"SprtArea",TRUE);

  user_sprites->areasize = sizeof(sprite_areainfo);
  /* user_sprites->numsprites = 0; */
  user_sprites->firstoffset = sizeof(sprite_areainfo);
  /* user_sprites->freeoffset = sizeof(sprite_areainfo); */
  Error_CheckFatal(Sprite_InitArea(user_sprites));
}

/* Tidy up after a change to user sprites */
void usersprt_postchange(BOOL trim)
{
  int memsize;
  browser_fileinfo *browser;
  browser_winentry *winentry;

  /* Trim off any free space from sprite area */
  if (trim)
  {
    memsize = flex_size((flex_ptr) &user_sprites);
    if (memsize > user_sprites->freeoffset)
    {
      user_sprites->areasize = user_sprites->freeoffset;
      flex_midextend((flex_ptr) &user_sprites,memsize,
      		  user_sprites->freeoffset - memsize);
    }
  }

  /* Redraw any windows which rely on user sprites */
  for (browser = LinkList_NextItem(&browser_list);
       browser;
       browser = LinkList_NextItem(browser))
  {
    for (winentry = LinkList_NextItem(&browser->winlist);
    	 winentry;
    	 winentry = LinkList_NextItem(winentry))
    {
      window_state wstate;
      window_redrawblock redraw;
      BOOL doredraw = FALSE;

      /* Is window's main sprite area user? */
      if ((int) winentry->window->window.spritearea != 1)
        doredraw = TRUE;
      else /* Do any indirect sprite icons use user sprites ? */
      {
        int icon;

        for (icon = 0;icon < winentry->window->window.numicons;icon++)
          if (winentry->window->icon[icon].flags.data.indirected &&
              winentry->window->icon[icon].flags.data.sprite &&
              (int) winentry->window->
                    icon[icon].data.indirectsprite.spritearea != 1)
          {
            doredraw = TRUE;
            break;
          }
      }

      /* Find area to redraw */
      Wimp_GetWindowState(winentry->handle,&wstate);
      redraw.window = wstate.openblock.window;
      redraw.rect = wstate.openblock.screenrect;
      redraw.scroll = wstate.openblock.scroll;
      Coord_RectToWorkArea(&redraw.rect,
      			   (convert_block *) &wstate.openblock.screenrect);

      /* Redraw window if necessary */
      if (doredraw)
        Wimp_ForceRedraw(&redraw);

      /* Title may need redrawing */
      if (winentry->window->window.titleflags.data.sprite &&
      	  (  ((int) winentry->window->window.spritearea != 1) ||
      	     (winentry->window->window.titleflags.data.indirected &&
      	       (int) winentry->window->
      	       	     window.title.indirectsprite.spritearea != 1)))
      {
        window_outline outline;

        outline.window = winentry->handle;
        Wimp_GetWindowOutline(&outline);
        outline.screenrect.min.y = wstate.openblock.screenrect.max.y;
        redraw.window = -1;
        redraw.rect = outline.screenrect;
        Wimp_ForceRedraw(&redraw);
      }
    }
  }
}

BOOL usersprt_merge(char *filename,int size,void *reference)
{
  BOOL result;
  int memsize;

  memsize = flex_size((flex_ptr) &user_sprites);
  if (!flex_midextend((flex_ptr) &user_sprites,memsize,size))
  {
    WinEd_MsgTrans_Report(messages,"SprtArea",FALSE);
    return FALSE;
  }

  user_sprites->areasize = memsize + size;
  result = !Error_Check(Sprite_Merge(user_sprites,filename));

  usersprt_postchange(TRUE);

  return result;
}

void usersprt_clear()
{
  user_sprites->areasize = user_sprites->freeoffset = 16;
  user_sprites->numsprites = 0;
  usersprt_postchange(TRUE);
}
