/* tempdefs.h */
/* Structures used for storing templates */

#ifndef __wined_tempdefs_h
#define __wined_tempdefs_h

#include "DeskLib:LinkList.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:Wimp.h"

/* Information about fonts */
typedef struct {
  wimp_point size; /* Points * 16 */
  char name[40];
} template_fontinfo;

/* Internal template file info block */
typedef struct {
  linklist_header header;
  window_handle window;
  window_handle pane;    /* Tool pane */
  int numwindows;
  int numcolumns;
  int estsize;		/* This is only used temporarily, don't use it */
  linklist_header winlist;
  BOOL altered;
  char title[256];
  int numfonts;
  template_fontinfo *fontinfo; /* Starting from 0 */
  char fontcount[255]; /* array (starting from 0) of usage of each font */
  window_handle stats; /* Statistics window */
  int largest;		/* Largest buffer total for stats */
  char namesfile[256]; /* Icon names file name */
  BOOL iconised;
} browser_fileinfo;

/* Template file header */
typedef struct {
  int fontoffset;
  int dummy[3];
} template_header;

/* Template index entry */
typedef struct {
  int offset;
  int size;
  int type;
  char identifier[wimp_MAXNAME];
} template_index;

/* To give easier access to icon defs following window def */
typedef struct {
  window_block window;
  icon_block icon[1];   /* Number irrelevant */
} browser_winblock;

typedef enum {
  status_CLOSED,
  status_EDITING,
  status_PREVIEW
} open_status;

/* Linked list entry of a window */
typedef struct {
  linklist_header header;
  browser_winblock *window;
  icon_handle icon;  /* Handle in browser */
  char identifier[wimp_MAXNAME];
  font_array *fontarray; /* For when window is open, indexed from 1 */
  open_status status;
  window_handle handle;  /* Of preview/editing window */
  window_handle pane;    /* Tool pane */
  browser_fileinfo *browser; /* Browser this window belongs to */
  int indsize;
} browser_winentry;

#endif
