/* WinEd IDE module */

#define message_WinEd_IDE_BROADCAST   0x601
#define message_WinEd_IDE_ACKNOWLEDGE 0x602
#define message_WinEd_IDE_EVENT       0x603

void ide_broadcast(browser_fileinfo *browser);

BOOL ide_receive(event_pollblock *event, void *reference);

void ide_sendinfo(browser_winentry *winentry, event_pollblock *event);
