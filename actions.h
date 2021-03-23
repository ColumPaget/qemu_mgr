#ifndef QEMU_MGR_ACTIONS
#define QEMU_MGR_ACTIONS

#include "common.h"



typedef enum {ACT_LIST_IMAGES, ACT_LIST_RUNNING, ACT_CREATE, ACT_ADD, ACT_IMPORT, ACT_CHANGE, ACT_DELETE, ACT_START, ACT_STOP, ACT_PAUSE, ACT_RESUME, ACT_REBOOT, ACT_WAKEUP, ACT_MEDIA_ADD, ACT_MEDIA_REMOVE, ACT_MOUNT, ACT_SNAPSHOT, ACT_DRIVE_BACKUP, ACT_INFO, ACT_INTERACTIVE, ACT_LIST_USB, ACT_SEND_KEY, ACT_SEND_TEXT, ACT_SCREENSHOT, ACT_LIST_TEMPLATES} EActions;


void ActionPerform(int Action, const char *ImageName, const char *Config);

#endif

