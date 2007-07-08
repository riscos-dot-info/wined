#include "DeskLib:Font.h"
#include "common.h"

#include <limits.h>

#include "DeskLib:KernelSWIs.h"
#include "DeskLib:Str.h"
#include "DeskLib:Validation.h"

#include "deskfont.h"
#include "icnedit.h"
#include "minsize.h"
#include "round.h"
#include "tempfont.h"
#include "usersprt.h"

/* Font_ScanString */
#ifndef SWI_Font_ScanString
#define SWI_Font_ScanString 0x400a1
#endif

typedef union {
  int value;
  struct {
    unsigned int dummy0_4    : 5;
    unsigned int useR5       : 1;
    unsigned int useR6       : 1;
    unsigned int useR7       : 1;
    unsigned int useR0       : 1;
    unsigned int kern        : 1;
    unsigned int reverse     : 1;
    unsigned int dummy11_16  : 6;
    unsigned int getcaretpos : 1;
    unsigned int getboundbox : 1;
    unsigned int getmatrix   : 1;
    unsigned int getsplit    : 1;
    unsigned int dummy21_31  : 11;
  } data;
} font_scanflags;

typedef struct {
  wimp_point spaceoffset;
  wimp_point letteroffset;
  int	     splitchar;
  wimp_rect  bounds;
} font_coordblock;

/*
os_error *Font_ScanString(font_handle handle,char *string,char **splitpoint,
	 		  int flags,wimp_point *offset,
	 		  font_coordblock *coordblock,int *matrix,int len,
	 		  int *splitchars);
*/

#define Font_ScanString(handle,string,splitpoint,flags,offset,coordblock, \
			matrix,len,splitchars) \
  SWI(8,8,SWI_Font_ScanString,(handle),(int) (string),(flags), \
      			      (offset)->x,(offset)->y,(int) (coordblock), \
      			      (int) (matrix),(len), \
      			      0,(int *) (splitpoint),0, \
      			      &(offset)->x,&(offset)->y,0,0,(splitchars))

static int icnedit_stringwidth(char *string, font_handle font,
	wimp_point *result)
{
  font_handle tfont;
  wimp_point splitpoint;
  font_coordblock coordblock;
  int width;
  wimp_point lres;

  Str_MakeASCIIZ(string, strlencr(string) + 1);
  if (font)
    tfont = font;
  else
    tfont = deskfont_handle;
  splitpoint.x = INT_MAX;
  splitpoint.y = INT_MAX;
  coordblock.spaceoffset.x = 0;
  coordblock.spaceoffset.y = 0;
  coordblock.letteroffset.x = 0;
  coordblock.letteroffset.y = 0;
  coordblock.splitchar = -1;
  Error_Check(Font_ScanString(tfont, string, 0, (1 << 5) | (1 << 8) | (1 << 18),
  		      &splitpoint,&coordblock,0,strlencr(string),0));
  lres.x = (coordblock.bounds.max.x - coordblock.bounds.min.x + 399) / 400;
  lres.y = (coordblock.bounds.max.y - coordblock.bounds.min.y + 399) / 400;
  round_up_point(&lres);
  if (!font)
  {
    width = 16 * strlencr(string);
    lres.y = 32;
    if (width > lres.x)
      lres.x = width;
    else
      width = lres.x;
  }
  else
    width = lres.x;
  if (result)
    *result = lres;
  return width;
}

static char twelvechars[13];

static wimp_point minsize_textsize(icon_block *logblock, icon_block *physblock,
	browser_winentry *winentry, icon_handle icon)
{
  wimp_point textsize;
  char *text;
  int Lvalid;
  font_handle fh;

  if (logblock->flags.data.indirected)
    text = physblock->data.indirecttext.buffer;
  else
  {
    strncpy(twelvechars,physblock->data.text,12);
    twelvechars[12] = 0;
    text = twelvechars;
  }
  if (text[0] < ' ')
  {
    textsize.x = 0;
    textsize.y = 0;
    return textsize;
  }
  if (logblock->flags.data.font)
    fh = physblock->flags.font.handle;
  else
    fh = 0;
  if (logblock->flags.data.indirected &&
      (Lvalid =
        Validation_ScanString(physblock->data.indirecttext.validstring,'L'),
      Lvalid))
  {
    int widest;
    int rows = 0;
    int width;
    int i = 0;

    widest = logblock->workarearect.max.x - logblock->workarearect.min.x -
             LMARGIN;
    /* Allow for border */
    if (logblock->flags.data.border)
      widest -= 2 * icnedit_borderwidth(winentry, icon);
    /* Find longest word (is it wider than current width?) */
    do
    {
      width = 0;
      while (text[i++] > ' ')
        width += 16;
      if (width > widest) widest = width;
    }
    while (text[i - 1] > 31);
    /* Scan string to find number of rows & width of widest row */
    textsize.x = 0;
    i = 0;
    do
    {
      int split = 0;
      int spliti = i;

      width = 0;
      do
      {
        while (text[i++] > ' ')
          width += 16;
        if (width < widest)
        {
          split = width;
          spliti = i;
        }
        width += 16; /* Allow for space just found */
      }
      while (width < widest && text[i - 1] > 31);
      width -= 16; /* Remove width of space just found */
      if (width > widest)
      {
        width = split;
        i = spliti;
      }
      if (width > textsize.x)
        textsize.x = width;
      rows++;
    }
    while (text[i - 1] > 31);
    /* Check for trailing space */
    if (text[i - 1] == ' ')
    {
      width += 16;
      if (width > textsize.x)
        textsize.x = width;
    }
    /* Add extra margin */
    textsize.x += LMARGIN - 12;
    sscanf(&physblock->data.indirecttext.validstring[Lvalid],"%d",&i);
    if (!i) i = 40;
    textsize.y = rows * i + (4 - 12);
  }
  else
  {
    icnedit_stringwidth(text, fh, &textsize);
  }
  textsize.x += 12;
  textsize.y += 12;

  return textsize;
}

static wimp_point minsize_spritesize(icon_block *logblock,
	icon_block *physblock)
{
  wimp_point spritesize;
  char *spritename;
  sprite_info sprtinfo;
  BOOL foundsprite;

  if (logblock->flags.data.indirected)
  {
    if (logblock->flags.data.text) /* Find from S validation */
    {
      int nameindex =
        Validation_ScanString(physblock->data.indirecttext.validstring,'S');

      if (nameindex)
      {
        char *punct;

        strncpy(twelvechars,
        	  &physblock->data.indirecttext.validstring[nameindex],12);
        twelvechars[12] = 0;
        punct = strchr(twelvechars,',');
        if (punct)
          *punct = 0;
        punct = strchr(twelvechars,';');
        if (punct)
          *punct = 0;
      }
      else
        *twelvechars = 0;

      spritename = twelvechars;
    }
    else
    {
      if (physblock->data.indirectsprite.nameisname)
        spritename = (char *) physblock->data.indirectsprite.name;
      else
      {
        *twelvechars = 0;
        spritename = twelvechars;
      }
    }
  }
  else /* Not indirected */
  {
    strncpy(twelvechars,physblock->data.text,12);
    twelvechars[12] = 0;
    spritename = twelvechars;
  }

  /* Now we've found sprite name find its dimensions */
  foundsprite = (BOOL) !Sprite_ReadInfo(user_sprites,spritename,&sprtinfo);
  if (!foundsprite)
  {
    int regs[10];

    regs[0] = 40;
    regs[2] = (int) spritename;
    foundsprite = (BOOL) !Wimp_SpriteOp(regs);
    if (foundsprite)
    {
      sprtinfo = *((sprite_info *) &regs[3]);
      /* sprtinfo.width = regs[3];
      sprtinfo.height = regs[4];
      sprtinfo.maskstatus = regs[5];
      sprtinfo.mode = regs[6]; */
    }
  }
  if (!foundsprite)
  {
    spritesize.x = 0;
    spritesize.y = 0;
  }
  else
  {
    wimp_point eig;

/* AR: Added ".screen_mode" to following two lines */
    OS_ReadModeVariable(sprtinfo.mode.screen_mode,modevar_XEIGFACTOR,&eig.x);
    OS_ReadModeVariable(sprtinfo.mode.screen_mode,modevar_YEIGFACTOR,&eig.y);
    spritesize.x = sprtinfo.width << eig.x;
    spritesize.y = sprtinfo.height << eig.y;
    if (logblock->flags.data.halfsize)
    {
      spritesize.x /= 2;
      spritesize.y /= 2;
    }
  }

  return spritesize;
}

static void minsize_border(icon_block *logblock,
	browser_winentry *winentry,icon_handle icon,
	wimp_point *size)
{
  if (logblock->flags.data.border)
  {
    int width = icnedit_borderwidth(winentry,icon);

    size->x += width;
    size->y += width;
  }
}

void icnedit_minsize(browser_winentry *winentry,icon_handle icon,
     		     wimp_point *size)
{
  wimp_point textsize,spritesize;
  icon_block physblock;
  icon_block *logblock = &winentry->window->icon[icon];

  Wimp_GetIconState(winentry->handle,icon,&physblock);

  /* Find size of text */
  if (logblock->flags.data.text)
    textsize = minsize_textsize(logblock, &physblock, winentry, icon);
  else
  {
    textsize.x = 0;
    textsize.y = 0;
  }
/*fprintf(stderr, "Text size = %d,%d\n", textsize.x, textsize.y);*/

  /* Find size of sprite */
  if (logblock->flags.data.sprite)
    spritesize = minsize_spritesize(logblock, &physblock);
  else
  {
    spritesize.x = 0;
    spritesize.y = 0;
  }
/*fprintf(stderr, "Sprite size = %d,%d\n", spritesize.x, spritesize.y);*/

  /* Decide whether size depends on text or sprite or both */
  if (logblock->flags.data.text && !logblock->flags.data.sprite)
  {
    size->x = textsize.x;
    size->y = textsize.y;
  }
  else if (logblock->flags.data.sprite && !logblock->flags.data.text)
  {
    size->x = spritesize.x;
    size->y = spritesize.y;
  }
  else if (logblock->flags.data.text && logblock->flags.data.sprite)
  {
    if (logblock->flags.data.hcentre)
      size->x = (textsize.x > spritesize.x) ? textsize.x : spritesize.x;
    else
      size->x = textsize.x + spritesize.x;
    if (logblock->flags.data.vcentre)
      size->y = (textsize.y > spritesize.y) ? textsize.y : spritesize.y;
    else
      size->y = textsize.y + spritesize.y;
  }
  else
  {
    size->x = 0;
    size->y = 0;
  }
/*fprintf(stderr, "Size without border is %d,%d\n", size->x, size->y);*/

  minsize_border(logblock, winentry, icon, size);
/*fprintf(stderr, "Size with border is %d,%d\n", size->x, size->y);*/

  if (size->x < 8) size->x = 8;
  if (size->y < 8) size->y = 8;
}
