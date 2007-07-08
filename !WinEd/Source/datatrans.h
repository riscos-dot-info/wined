/* datatrans.h */
/* Deals with RISC OS data transfer */

#ifndef __wined_datatrans_h
#define __wined_datatrans_h

#include "DeskLib:Wimp.h"

/* Type of function to load data, should return TRUE iff successful */
typedef BOOL (*datatrans_loader)(char *filename,int filesize,void *ref);

/* Gets loader to load file, replies to the Message_DataLoad and deletes
   Scrap file if appropriate. All necessary file info is deduced from *event.
   ref passes further info to loader */
void datatrans_load(event_pollblock *event,datatrans_loader loader,
                    void *ref);

/* Replies to Message_DataSave with SaveAck with "<Wimp$Scrap>" */
void datatrans_saveack(event_pollblock *event);

/* ----------------------------------------------------------------- */

/* Load SaveAs and Export templates */
void saveas_init(void);

/* Function to perform save */
typedef BOOL (*datatrans_saver)(char *filename,void *ref /* browser */,
                                BOOL selection);

/* Function to call when save is complete */
typedef void (*datatrans_complete)(void *browser,BOOL successful,BOOL safe,
              char *filename,BOOL selection);

/* Sends save message to window under pointer and sets up handlers to
   complete transfer protocol */
void datatrans_startsave(datatrans_saver saver,void *ref /* browser */,
                         BOOL selection,datatrans_complete complete);

/* Opens the Save as dialogue box with its handlers
   If leaf is TRUE, uses Wimp_CreateSubmenu, otherwise Wimp_CreateMenu */
void datatrans_saveas(char *filename,BOOL allow_selection,int x,int y,BOOL leaf,
                      datatrans_saver saver,datatrans_complete complete,
                      void *ref,BOOL select_selection,BOOL export);

/* Perform drag & drop save of selection without opening SaveAs */
void datatrans_dragndrop(datatrans_saver,datatrans_complete,browser_fileinfo *);

/* Whether saveas is open and function to release handlers */
void saveas_release(void);

/* Browser needs to be able to select Selection button */
#define saveas_SAVE 0
#define saveas_CANCEL 1
#define saveas_FILE 2
#define saveas_ICON 3
#define saveas_SELECTION 4
extern window_handle saveas_window;
extern window_handle saveas_export;



#endif
