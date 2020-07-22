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

/**
 * Internal template file info block.
 *
 * This forms the contents of the browser_list linked list.
 */
typedef struct {
  linklist_header header;
  window_handle window;          /**< The handle of the browser window.                                  */
  window_handle pane;            /**< The handle of the browser tool pane.                               */
  int numwindows;                /**< The number of window templates in the file.                        */
  int numcolumns;                /**< The number of columns in the browser window.                       */
  int estsize;                   /**< A temporary store for the stats window file size; don't use it.    */
  linklist_header winlist;       /**< A linked list of browser_winentry structures defining the windows. */
  BOOL altered;                  /**< TRUE if the file has been altered since it was last saved.         */
  char title[256];               /**< The indirected title buffer for the browser window.                */
  unsigned int numfonts;         /**< The number of fonts currently used in the file.                    */
  template_fontinfo *fontinfo;   /**< A flex block array holding font details; starting from 0.          */
  char fontcount[255];           /**< Array holding usage of each font; starting from 0.                 */
  window_handle stats;           /**< The handle of the file's statistics window, if open, or 0.         */
  int largest;                   /**< The largest window buffer total size for the statistics window.    */
  char namesfile[256];           /**< The path name of the last names export file to have been saved.    */
  BOOL iconised;                 /**< TRUE if the browser window is currently iconised.                  */
  task_handle idetask;           /**< The task handle of the registered IDE task, or 0 if none.          */
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

/**
 * The statuses applicable to a template window within a file.
 */

typedef enum {
  status_CLOSED,                 /**< The window is currently closed.                                    */
  status_EDITING,                /**< The window is currently open for editing.                          */
  status_PREVIEW                 /**< The window is currently being previewed.                           */
} open_status;

/**
 * Internal window template info block.
 * 
 * This forms the contents of the winlist linked list in the browser_fileinfo struct.
 */
typedef struct {
  linklist_header header;
  browser_winblock *window;      /**< The window and icon definitions associated with the template.      */
  icon_handle icon;              /**< The handle of the icon representing the window in the browser.     */
  char identifier[wimp_MAXNAME]; /**< The name (identifier) of the window within the template file.      */
  font_array *fontarray;         /**< The font usage array, when the window is open, indexed from 1.     */
  open_status status;            /**< The current status (closed, editing, preview) of the window.       */
  window_handle handle;          /**< The handle of the preview or editing window.                       */
  window_handle pane;            /**< The handle of the editing window's tool pane.                      */
  browser_fileinfo *browser;     /**< The Browser instance that this window belongs to.                  */
  int indsize;                   /**< The indirected data size for the window; calculated by stats.      */
  BOOL iconised;                 /**< TRUE if the edit/preview window is currently iconised.             */
} browser_winentry;

/* IDE message data structures */

/* Broadcast & acknowledgement structure */
typedef struct
{
  window_handle handle;
  char filename[232];
} ide_handshake;

/* These must add up to 228 bytes each */
typedef struct
{
  button_state button; /*   4 */
  icon_handle icon;    /*   4 */
  char identifier[16]; /*  16 */
  char iconname[204];  /* 204 */
} ide_click_block;

typedef union
{
  ide_click_block click;
  char bytes[228];
} ide_synthetic_data;

typedef struct
{
  window_handle handle;
  int event;
  ide_synthetic_data data;
} ide_event;


#endif
