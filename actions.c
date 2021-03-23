#include "actions.h"
#include "images.h"
#include "glob.h"
#include "mount.h"
#include "screenshot.h"
#include "qmp.h"

void ActionPerform(int Action, const char *ImageName, const char *Config)
{
    TImageInfo *Info;

    printf("%d [%s] [%s]\n", Action, ImageName, Config);

    switch (Action)
    {
    case ACT_CREATE:
        ImageCreate(ImageName, Config);
        break;

    case ACT_ADD:
        ImageAdd(ImageName, Config);
        break;

    case ACT_DELETE:
        ImageDelete(ImageName);
        break;

    case ACT_CHANGE:
        ImageChange(ImageName, Config);
        break;

    case ACT_START:
        ImageStart(ImageName, Config);
        break;

    case ACT_STOP:
        ImageStop(ImageName, Config);
        break;

    case ACT_PAUSE:
        ImagePause(ImageName, Config);
        break;

    case ACT_RESUME:
        ImageResume(ImageName, Config);
        break;

    case ACT_REBOOT:
        ImageReboot(ImageName, Config);
        break;

    case ACT_WAKEUP:
        ImageWakeup(ImageName, Config);
        break;

    case ACT_LIST_IMAGES:
        ImagesList(ImageName, Config);
        break;

    case ACT_MOUNT:
        MountItem(ImageName, Config);
        break;

    case ACT_MEDIA_ADD:
        ImageMediaAdd(ImageName, Config);
        break;

    case ACT_MEDIA_REMOVE:
        ImageMediaRemove(ImageName, Config);
        break;

    case ACT_SNAPSHOT:
        ImageTakeSnapshot(ImageName, Config);
        break;

    case ACT_DRIVE_BACKUP:
        ImageDriveBackup(ImageName, Config);
        break;

    case ACT_SEND_TEXT:
        QMPSendText(ImageName);
        break;

    case ACT_SEND_KEY:
        QMPSendKey(ImageName, Config);
        break;

    case ACT_INFO:
        Info=ImageGetRunningInfo(ImageName);
        OutputImageInfo(Info);
        break;

    case ACT_SCREENSHOT:
        ImageScreenshot(ImageName, Config);
        break;

    }

}

