/* icnedit.c */

/* #define MEM */

#include "common.h"

#include "DeskLib:Font.h"
#include "DeskLib:KernelSWIs.h"
#include "DeskLib:Validation.h"

#include "deskfont.h"
#include "icnedit.h"
#include "picker.h"
#include "round.h"
#include "tempfont.h"
#include "usersprt.h"

/* Update one icon's pointer offsets (see icnedit_updatepointers()) */
void icnedit_updatepointer(int after,int by,icon_flags *flags,
     			   icon_data *data);

/* Deletes indirected data given flags and data */
void icnedit_rawdeleteind(browser_winentry *winentry,
     			  icon_data *data,icon_flags *flags);

/* Create a physical icon */
void icnedit_createphysical(browser_winentry *winentry,int icon,
     			    icon_createblock *block);

/* Delete a physical icon, shifting subsequent ones down */
void icnedit_fulldeletephysical(window_handle window,icon_handle icon);

/* Delete icon only, leave others put */
#define icnedit_deletephysical(window, icon) Wimp_DeleteIcon((window),(icon))




font_handle icnedit_findfont(template_fontinfo *fontinfo,
	    		    font_array **fontarray)
{
  font_handle result;
  wimp_point dpi;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_findfont");
#endif

  Screen_CacheModeInfo();
  dpi.x = 180 >> screen_eig.x;
  dpi.y = 180 >> screen_eig.y;
  if (Font_FindFont(&result,fontinfo->name,
      		  		fontinfo->size.x,fontinfo->size.y,
      		  		dpi.x,dpi.y))
    return 0;

  if (!*fontarray)
  {
    int index;

    if (!flex_alloc((flex_ptr) fontarray,sizeof(font_array)))
    {
      MsgTrans_Report(messages,"FontMem",FALSE);
      Font_LoseFont(result);
      return 0;
    }

    for (index = 0;index < 256;index ++)
      (*fontarray)->fonts[index] = 0;
  }

  if ((*fontarray)->fonts[result] == 255)
  {
    MsgTrans_Report(messages,"TMFonts",FALSE);
    Font_LoseFont(result);
    (*fontarray)->fonts[result] = 255;
    return 0;
  }
  else
    (*fontarray)->fonts[result]++;

  return result;
}

void        icnedit_losefont(font_handle font,font_array **fontarray)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_losefont");
#endif
  if (!(*fontarray)->fonts[font])
    MsgTrans_Report(messages,"BadFont",FALSE);
  else
  {
    Font_LoseFont(font);
    (*fontarray)->fonts[font]--;
  }
}

void        icnedit_freeindirected(icon_flags *flags,icon_data *data)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_freeindirected");
#endif
  if (!flags->data.indirected)
  {
    return;
  }

  if (data->indirecttext.buffer && (flags->data.text ||
      (flags->data.sprite && data->indirectsprite.nameisname)))
  {
    free(data->indirecttext.buffer);
  }

  if (flags->data.text)
  {
    free(data->indirecttext.validstring);
  }
}

void        icnedit_makeeditableflags(browser_winentry *winentry, icon_flags *flags)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_makeeditableflags");
#endif
  flags->data.buttontype = iconbtype_DOUBLECLICKDRAG;
  flags->data.esg = 1;
  flags->data.selected =
  flags->data.shaded =
  flags->data.deleted = 0;
  flags->data.allowadjust =
  flags->data.needshelp = 1;
  if (choices->borders && winentry != &picker_winentry)
    flags->data.border = 1;
}

BOOL        icnedit_processicon(browser_winentry *winentry,
     			 icon_flags *flags,icon_data *data,
     			 wimp_rect *workarearect,BOOL editable)
{
  #ifdef WINED_DETAILEDDEBUG
  Debug_Printf("icnedit_processicon");
  #endif
  /* Font */
  if (flags->data.font)
    flags->font.handle =
      icnedit_findfont(&winentry->browser->fontinfo[flags->font.handle-1],
      		       &winentry->fontarray);

  if (flags->data.indirected)
  {
    int bufindex = (int) data->indirecttext.buffer;
    char *bufcont = (char *) ((int) winentry->window + bufindex);

    if (flags->data.text || (flags->data.sprite&&data->indirectsprite.nameisname))
    {
      #ifdef WINED_DETAILEDDEBUG
      Debug_Printf("  either data.text or data.sprite and indirectsprite.nameisname");
      #endif
      if (data->indirecttext.bufflen > 0)
      {
        data->indirecttext.buffer = malloc(data->indirecttext.bufflen);
        if (!data->indirecttext.buffer)
          return FALSE;
      }
      else
        data->indirecttext.buffer = 0;
      if (data->indirecttext.buffer && bufindex > 0)
      {
        strncpycr(data->indirecttext.buffer,bufcont,data->indirecttext.bufflen);
        #ifdef DEBUG_DETAILEDDEBUG
        Debug_Printf("  value: %s", data->indirecttext.buffer);
        #endif
      }
      else if (data->indirecttext.buffer)
        *data->indirecttext.buffer = 0;
    }
    if (flags->data.text)
    {
      int validindex = (int) data->indirecttext.validstring;
      char *validcont = (char *) ((int) winentry->window + validindex);

      if (validindex > 0)
        data->indirecttext.validstring = malloc(strlencr(validcont) + 1);
      else
        data->indirecttext.validstring = malloc(1);
      if (!data->indirecttext.validstring)
      {
        free(data->indirecttext.buffer);
        return FALSE;
      }
      if (validindex > 0)
        strcpycr(data->indirecttext.validstring,validcont);
      else
        *data->indirecttext.validstring = 0;
      if (editable && workarearect && validindex > 0)
      {
        int rindex;
        /* Convert R5/6 to R1; it won't do any harm to leave ",3" or whatever     */

        /* Note that this means the "rounded" icons of Select/Adjust button types */
        /* R5, R6 get lost. Still visible on preview though.                      */

        rindex = Validation_ScanString(data->indirecttext.validstring,'R');
        if (rindex)
        {
          switch (data->indirecttext.validstring[rindex])
          {
            case '6':
              /* Reduce size by border width first */
	      workarearect->min.x += 8;
	      workarearect->min.y += 8;
	      workarearect->max.x -= 8;
	      workarearect->max.y -= 8;
            case '5':
              data->indirecttext.validstring[rindex] = '1';
              break;
          }
        }
      }
    }
    else if (flags->data.sprite)
    {
      /*if ((int) data->indirectsprite.spritearea != 1)*/
      data->indirectsprite.spritearea = (unsigned int *) user_sprites;
    }
  }

  if (editable)
    icnedit_makeeditableflags(winentry, flags);

  #ifdef WINED_DETAILEDDEBUG
  Debug_Printf("End of icnedit_processicon Size: %d, %d, %d, %d", workarearect->min.x, workarearect->min.y, workarearect->max.x, workarearect->max.y);
  #endif

  return TRUE;
}

void        icnedit_unprocessicon(browser_winentry *winentry,
     			   icon_flags *flags,icon_data *data)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_unprocessicon");
#endif
  icnedit_freeindirected(flags,data);
  if (flags->data.font)
  {
    icnedit_losefont(flags->font.handle,&winentry->fontarray);
  }
}

void        icnedit_updatepointer(int after,int by,
     			   icon_flags *flags,icon_data *data)
{
/*#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_updatepointer");
#endif*/
  if (!flags->data.indirected)
    return;

  if ((int) data->indirecttext.buffer >= after)
    data->indirecttext.buffer += by;

  if (flags->data.text && (int) data->indirecttext.validstring >= after)
    data->indirecttext.validstring += by;
}

void        icnedit_updatepointers(browser_winentry *winentry,int after,int by)
{
  int icon;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_updatepointers");
#endif
  /* Title */
  icnedit_updatepointer(after,by,&winentry->window->window.titleflags,
  			         &winentry->window->window.title);

  for (icon = 0;icon < winentry->window->window.numicons;icon++)
    icnedit_updatepointer(after,by,&winentry->window->icon[icon].flags,
    			           &winentry->window->icon[icon].data);
}

void        icnedit_fulldeletephysical(window_handle window,icon_handle icon)
{
  window_info winfo;
  icon_createblock block;
  BOOL shift = FALSE;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_fulldeletephysical");
#endif
  icnedit_deletephysical(window,icon);
  Window_GetInfo3(window,&winfo);
  block.window = window;
  for (icon = 0;icon < winfo.block.numicons;icon++)
  {
    Wimp_GetIconState(window,icon,&block.icondata);
    if (shift)
    {
      icon_handle newicon;

      Wimp_DeleteIcon(window,icon);
      Wimp_CreateIcon(&block,&newicon);
    }
    else if (block.icondata.flags.data.deleted)
      shift = TRUE;
  }
}

void        icnedit_moveicon(browser_winentry *winentry,int icon,wimp_rect *box)
{
  icon_createblock icblock;
  char *rindex;

#ifdef WINED_DETAILEDDEBUG
  Debug_Printf("icnedit_moveicon %d", icon);
  Debug_Printf(" box: %d, %d, %d, %d", box->min.x, box->min.y, box->max.x, box->max.y);

#endif

  icblock.window = winentry->handle;
  Wimp_GetIconState(winentry->handle,icon,&icblock.icondata);

  /* Delete and deselect physical icon */
  icnedit_deletephysical(winentry->handle,icon);
  icnedit_forceredraw(winentry,icon);

  /* Make sure new box has +ve size before using it */
  if (box->max.x - box->min.x < 4)
  {
    box->max.x = box->min.x + winentry->window->icon[icon].workarearect.max.x -
    	       	 	      winentry->window->icon[icon].workarearect.min.x;
  }
  if (box->max.y - box->min.y < 4)
  {
    box->max.y = box->min.y + winentry->window->icon[icon].workarearect.max.y -
    	       	 	      winentry->window->icon[icon].workarearect.min.y;
  }
  winentry->window->icon[icon].workarearect = *box;
  icblock.icondata.workarearect = *box;
  /* If R6 validation, remove size of border */
  rindex = icnedit_valid6(winentry,icon);
  if (rindex)
  {
    icblock.icondata.workarearect.min.x += 8;
    icblock.icondata.workarearect.min.y += 8;
    icblock.icondata.workarearect.max.x -= 8;
    icblock.icondata.workarearect.max.y -= 8;
  }
  /* Create and display new physical icon */
  Wimp_CreateIcon(&icblock,&icon);
  icnedit_forceredraw(winentry,icon);
#ifdef WINED_DETAILEDDEBUG
  Debug_Printf("End of icnedit_moveicon %d", icon);
#endif
}

void        icnedit_removeindirectstring(browser_winentry *winentry,int index)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_removeindirectstring");
#endif
  int by = strlencr((char *) ((int) winentry->window + index)) + 1;

  if (flex_midextend((flex_ptr) &winentry->window,index + by,-by))
    icnedit_updatepointers(winentry,index,-by);
}

int         icnedit_addindirectstring(browser_winentry *winentry,char *string)
{
  int index;
  int len;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_addindirectstring");
#endif
  index = flex_size((flex_ptr) &winentry->window);
  if ((int) string <= 0)
    len = 0;
  else
    len = strlencr(string);
  if (!flex_midextend((flex_ptr) &winentry->window,index,len + 1))
  {
    MsgTrans_Report(messages,"IndMem",FALSE);
    return 0;
  }

  if (len)
    strcpycr((char *) ((int) winentry->window + index),string);
  else
    *((char *) ((int) winentry->window + index)) = 0;

  return index;
}

void        icnedit_rawdeleteind(browser_winentry *winentry,
     			  icon_data *data,icon_flags *flags)
{
  int at;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_rawdeleteind");
#endif
  if (!flags->data.indirected)
    return;
  if (flags->data.text || /* Second expression only evaluated if not text */
      (flags->data.sprite && data->indirectsprite.nameisname))
  {
    at = (int) data->indirecttext.buffer;
    if (at > 0)
      icnedit_removeindirectstring(winentry,at);
  }
  if (flags->data.text)
  {
    at = (int) data->indirecttext.validstring;
    if (at > 0)
    {
      icnedit_removeindirectstring(winentry,at);
    }
  }
}

void        icnedit_deleteindirected(browser_winentry *winentry,int icon)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_deleteindirected");
#endif
  if (icon == -1)
    icnedit_rawdeleteind(winentry,&winentry->window->window.title,
    			          &winentry->window->window.titleflags);
  else
    icnedit_rawdeleteind(winentry,&winentry->window->icon[icon].data,
    			          &winentry->window->icon[icon].flags);
}

void        icnedit_processnewicondata(browser_winentry *winentry,
     				icon_flags *flags,icon_data *data)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_processnewicondata");
#endif
  /* Add new data to winentry */
  if (!flags->data.indirected)
    return;

  if (flags->data.text ||
  	(flags->data.sprite && data->indirectsprite.nameisname))
  {
    data->indirecttext.buffer = (char *)
      icnedit_addindirectstring(winentry,data->indirecttext.buffer);
  }
  if (flags->data.text)
  {
    data->indirecttext.validstring = (char *)
      icnedit_addindirectstring(winentry,data->indirecttext.validstring);
  }
}

void        icnedit_createphysical(browser_winentry *winentry, int icon, icon_createblock *block)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_createphysical %d", icon);
#endif
  if (icnedit_processicon(winentry,
      			  &block->icondata.flags,&block->icondata.data,
      			  &block->icondata.workarearect,
      			  TRUE))
  {
    Wimp_CreateIcon(block,&icon);
    icnedit_forceredraw(winentry,icon);
  }
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("End of icnedit_createphysical %d", icon);
#endif
}

void        icnedit_changedata(browser_winentry *winentry, int icon, icon_block *data)
{
  icon_createblock iblock;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_changedata %d", icon);
#endif
  iblock.window = winentry->handle;

  /* Get rid of all stuff associated with old icon... */
  /* ...physical... */
  Wimp_GetIconState(winentry->handle,icon,
  		    &iblock.icondata); /* Info needed to unprocess icon */
  icnedit_deletephysical(winentry->handle,icon);
  icnedit_forceredraw(winentry,icon);
  icnedit_unprocessicon(winentry, &iblock.icondata.flags, &iblock.icondata.data);
  /* ...logical */
  if (winentry->window->icon[icon].flags.data.font)
  {
    font_handle ofont = winentry->window->icon[icon].flags.font.handle;
    if (tempfont_losefont(winentry->browser,ofont) &&
    	data->flags.data.font && data->flags.font.handle > ofont)
      --data->flags.font.handle;
  }
  icnedit_deleteindirected(winentry,icon);

  /* Copy data to create block so we can safely alter it etc */
  iblock.icondata = *data;

  icnedit_processnewicondata(winentry,
  			     &iblock.icondata.flags,&iblock.icondata.data);

  winentry->window->icon[icon] = iblock.icondata;

  Debug_Printf("End of icnedit_changedata, creating, size: %d %d %d %d", iblock.icondata.workarearect.min.x, iblock.icondata.workarearect.min.y, iblock.icondata.workarearect.max.x, iblock.icondata.workarearect.max.y);

  icnedit_createphysical(winentry,icon,&iblock);
}

int         icnedit_create(browser_winentry *winentry,icon_block *data)
{
  int at;
  icon_block *block;
  icon_createblock cblock;
  int icon;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_create");
#endif

  /* Find where to insert icon block */
  icon = winentry->window->window.numicons++;
  at = sizeof(window_block) + icon * sizeof(icon_block);

  /* Insert blank icon block in winentry */
  if (!flex_midextend((flex_ptr) &winentry->window,
      		     at,sizeof(icon_block)))
  {
    winentry->window->window.numicons--;
    MsgTrans_Report(messages,"IconMem",FALSE);
    return -1;
  }

  /* Correct indirected offsets, making sure it doesn't get confused by
     phantom values in created block, not that it should really matter */
  block = (icon_block *) ((int) winentry->window + at);
  block->flags.data.indirected = 0;
  block->flags.data.font = 0;
  icnedit_updatepointers(winentry,at,sizeof(icon_block));

  /* Set up temporary block */
  cblock.window = winentry->handle;
  cblock.icondata = *data;

  /* Use temporary block to create new log & phys */
  icnedit_processnewicondata(winentry,
  			     &cblock.icondata.flags,&cblock.icondata.data);

  winentry->window->icon[icon] = cblock.icondata;

  icnedit_createphysical(winentry,icon,&cblock);

  return icon;
}

void        icnedit_deleteicon(browser_winentry *winentry,int icon)
{
  icon_block istate;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_deleteicon %d", icon);
#endif
  /* Get rid of physical icon */
  Wimp_GetIconState(winentry->handle,icon,&istate);
  icnedit_fulldeletephysical(winentry->handle,icon);
  icnedit_forceredraw(winentry,icon);
  icnedit_unprocessicon(winentry,&istate.flags,&istate.data);

  /* Get rid of logical icon's stuff */
  icnedit_deleteindirected(winentry,icon);
  if (istate.flags.data.font)
    tempfont_losefont(winentry->browser,
    		      winentry->window->icon[icon].flags.font.handle);

  /* Remove icon definition */
  flex_midextend((flex_ptr) &winentry->window,
  		(int) sizeof(window_block) + (icon + 1) * sizeof(icon_block),
  		(int) -sizeof(icon_block));

  /* Correct numicons */
  winentry->window->window.numicons--;

  /* Correct indirected pointers using 1 instead of 0 for 'after' in
     case any pointers are null */
  icnedit_updatepointers(winentry,1,(int) -sizeof(icon_block));
}

icon_handle icnedit_copyicon(browser_winentry *source,int icon,
     		      browser_winentry *dest,wimp_rect *bounds)
{
  icon_block newdata;
  char *valid;
  int rindex;
  int border;
  icon_handle newicon;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_copyicon %d", icon);
#endif
  /* Set up independent icon_block for icnedit_create */
  newdata = source->window->icon[icon];
  newdata.workarearect = *bounds;
  /* Find fixed indirected data from physical icon */
  if (newdata.flags.data.indirected)
  {
    icon_block istate;

    Wimp_GetIconState(source->handle,icon,&istate);
    newdata.data = istate.data;
  }
  /* Copy logical font if necessary */
  if (newdata.flags.data.font)
    newdata.flags.font.handle = tempfont_findfont(dest->browser,
      &source->browser->fontinfo[newdata.flags.font.handle-1]);

  /* Physical data may contain R1 instead of R6, check */
  border = icnedit_bordertype(source,icon);
  if (border == 5 || border == 6)
  {
    valid = newdata.data.indirecttext.validstring;
    rindex = Validation_ScanString(valid,'R');
    valid[rindex] = border + '0';
  }
  else
  {
    valid = NULL;
    rindex = 0;
  }

  newicon = icnedit_create(dest,&newdata);

  /* if valid6, need to restore R1 to original phys data */
  if (border == 5 || border == 6)
    valid[rindex] = '1';

  return newicon;
}

char       *icnedit_valid6(browser_winentry *winentry,int icon)
{
  char *valid;
  int rindex;
  icon_block *data;

/*#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_valid6 %d", icon);
#endif*/
  data = &winentry->window->icon[icon];
  if (data->flags.data.indirected && data->flags.data.text &&
      (int) data->data.indirecttext.validstring > 0)
  {
    valid = data->data.indirecttext.validstring +
    	    (int) winentry->window;
    rindex = Validation_ScanString(valid,'R');
    if (rindex && valid[rindex] == '6')
      return valid + rindex;
  }
  return 0;
}

int         icnedit_bordertype(browser_winentry *winentry,int icon)
{
  char *valid;
  int rindex;
  icon_block *data;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_bordertype %d", icon);
#endif
  data = &winentry->window->icon[icon];
  if (data->flags.data.indirected && data->flags.data.text &&
      (int) data->data.indirecttext.validstring > 0)
  {
    valid = data->data.indirecttext.validstring +
    	    (int) winentry->window;
    rindex = Validation_ScanString(valid,'R');
    if (rindex)
      return valid[rindex] - '0';
  }
  return 0;
}

int         icnedit_borderwidth(browser_winentry *winentry, int icon)
{
#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_borderwidth %d", icon);
#endif
  if (!winentry->window->icon[icon].flags.data.border)
    return 0;
  switch (icnedit_bordertype(winentry,icon))
  {
    case 3:
    case 4:
      return 16;
    case 6:
    case 7:
      return 24;
    default:
      return 8;
  }
  return 0;
}

void        icnedit_forceredraw(browser_winentry *winentry,int icon)
{
  window_redrawblock redraw;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_forceredraw %d, size: %d, %d, %d, %d", icon, winentry->window->icon[icon].workarearect.min.x, winentry->window->icon[icon].workarearect.min.y, winentry->window->icon[icon].workarearect.max.x, winentry->window->icon[icon].workarearect.max.y);
#endif
  redraw.window = winentry->handle;
  redraw.rect.min.x = winentry->window->icon[icon].workarearect.min.x;
  redraw.rect.min.y = winentry->window->icon[icon].workarearect.min.y;
  redraw.rect.max.x = winentry->window->icon[icon].workarearect.max.x + 1;
  redraw.rect.max.y = winentry->window->icon[icon].workarearect.max.y + 1;
  Wimp_ForceRedraw(&redraw);
}

/*
static char *icnedit_splitpoint(char *string, font_handle font, int width,
	int *split_ofst)
{
  char *spt;

  if (font)
  {
    wimp_point splitpoint;
    font_coordblock coordblock;

    splitpoint.x = width;
    splitpoint.y = INT_MAX;
    coordblock.spaceoffset.x = 0;
    coordblock.spaceoffset.y = 0;
    coordblock.letteroffset.x = 0;
    coordblock.letteroffset.y = 0;
    coordblock.splitchar = -1;
    Font_ScanString(physblock.flags.font.handle,text,&spt,(1 << 5)|(1 << 18),
    		      &splitpoint,&coordblock,0,strlencr(text),0);
    *split_ofst = splitpoint.x;
  }
  return spt;
}
*/

/* Returns width of a string in OS units
   font == 0 means return larger of system font & deskfont
   Both dimensions are also stored in result if != 0 */
int         icnedit_count_indirected(browser_winentry *winentry,
    			     icon_data *data,icon_flags *flags)
{
  int size;

#ifdef WINED_DETAILEDDEBUG
Debug_Printf("icnedit_count_indirected");
#endif
  if (!flags->data.indirected)
    return 0;

  if (flags->data.text)
  {
    size = data->indirecttext.bufflen;
    if ((int) data->indirecttext.validstring > 0)
      size += strlencr(data->indirecttext.validstring+(int)winentry->window) +
      	       1;
  }
  else if (flags->data.sprite)
    size = 12;
  else return 0;

  return size;
}
