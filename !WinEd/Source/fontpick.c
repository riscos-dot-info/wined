/* fontpick.c */

#include <stdlib.h>
#include <string.h>

#include "DeskLib:Error.h"
#include "DeskLib:Menu.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:SWI.h"

#include "globals.h"

#ifndef SWI_Font_ListFonts
#define SWI_Font_ListFonts 0x40091
#define SWI_Font_DecodeMenu 0x400a0
#define SWI_Font_FindField 0x400a6
#endif

menu_ptr fontpick_menudata = 0;
int fontpick_menusize = 0; /* Size of last menu tree */
int fontpick_indsize = 0; /* Size of last menu's indirected data */
char *fontpick_inddata = 0; /* Actual storage of indirected data */

menu_ptr fontpick_makemenu(char *fontname)
{
  int menusize,indsize; /* ...of menu about to be created */

  /* Find size of new font menu */
  if (Error_Check(SWI(7,6,SWI_Font_ListFonts,
  	      	      0, /* not needed */
  	      	      0, /* find size of menu tree */
  	      	      (1 << 19) | (1 << 21), /* make menu with ticks */
  	      	      0, /* size of menu buffer (not needed here) */
  	      	      0, /* find size of indirected data */
  	      	      0, /* size of indirected data (not needed here) */
  	      	      (int) fontname, /* identifier to tick */
  	      	      0,0,0, /* not needed/preserved */
  	      	      &menusize,
  	      	      0, /* preserved */
  	      	      &indsize)))
    return FALSE;

  /* If menu already exists but is too small, free it */
  if (fontpick_menudata)
  {
    if (fontpick_menusize < menusize)
    {
      free(fontpick_menudata);
      fontpick_menudata = 0;
    }
    if (fontpick_indsize < indsize)
    {
      free(fontpick_inddata);
      fontpick_inddata = 0;
    }
  }

  /* If menu buffers don't exist, malloc them */
  if (!fontpick_menudata)
  {
    fontpick_menudata = malloc(menusize);
    if (!fontpick_menudata)
    {
      MsgTrans_Report(messages,"NoMenu",FALSE);
      fontpick_menusize = 0;
      if (fontpick_inddata)
      {
        free(fontpick_inddata);
        fontpick_inddata = 0;
        fontpick_indsize = 0;
      }
      return 0;
    }
    fontpick_menusize = menusize;
  }
  if (!fontpick_inddata)
  {
    fontpick_inddata = malloc(indsize);
    if (!fontpick_inddata)
    {
      MsgTrans_Report(messages,"NoMenu",FALSE);
      free(fontpick_menudata);
      fontpick_menudata = 0;
      fontpick_menusize = 0;
      fontpick_indsize = 0;
      return 0;
    }
    fontpick_indsize = indsize;
  }

  /* Make menu */
  if (Error_Check(SWI(7,0,SWI_Font_ListFonts,
  	      	      0, /* not needed */
  	      	      (int) fontpick_menudata, /* pointer to menu tree */
  	      	      (1 << 19) | (1 << 21), /* make menu with ticks */
  	      	      fontpick_menusize, /* size of menu buffer */
  	      	      (int) fontpick_inddata, /* indirected data */
  	      	      fontpick_indsize, /* size of indirected data */
  	      	      (int) fontname))) /* identifier to tick */
  {
    MsgTrans_Report(messages,"NoMenu",FALSE);
    free(fontpick_menudata);
    fontpick_menudata = 0;
    fontpick_menusize = 0;
    if (fontpick_inddata)
    {
      free(fontpick_inddata);
      fontpick_inddata = 0;
      fontpick_indsize = 0;
    }
    return 0;
  }

  /* Won't bother checking FontList token, it doesn't affect English
     machines, and later FontManager modules are available for all */

  return fontpick_menudata;
}

void fontpick_freemenu()
{
  if (fontpick_menudata)
  {
    free(fontpick_menudata);
    fontpick_menudata = 0;
    fontpick_menusize = 0;
  }
  if (fontpick_inddata)
  {
    free(fontpick_inddata);
    fontpick_inddata = 0;
    fontpick_indsize = 0;
  }
}

void fontpick_findselection(int *selection,char *buffer)
{
  Error_Check(SWI(5,0,SWI_Font_DecodeMenu,0,(int) fontpick_menudata,
  	      	  (int) selection,(int) buffer,256));
}

void fontpick_findname(char *fullstring,char *buffer)
{
  char *field;
  int result;

  SWI(3,3,SWI_Font_FindField,0,(int) fullstring,'F',
      			     0,(int *) &field,&result);

  /* Copy up to next '\' */
  result = (int) strchr(field,'\\');
  if (result)
    result -= (int) field;
  else
    result = strlen(field);
  strncpy(buffer,field,result);
  buffer[result] = 0;
}
