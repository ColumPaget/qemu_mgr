#include "command-line.h"
#include "images.h"
#include "os-commands.h"
#include "interactive.h"
#include "actions.h"
#include "sound-devices.h"

int main(int argc, char *argv[])
{
    ListNode *Actions, *Curr;
    char *Tempstr=NULL;

    signal(SIGPIPE, SIG_IGN);
    Config=(TConfig *) calloc(1, sizeof(TConfig));
    FindOSCommands();

    if (! StrValid(OSCommandFindPath("qemu-system-x86_64"))) 
    {
	printf("ERROR: Can't find qemu-system-x86_64. Is it installed?\n");
	exit(1);
    }

OSCommandQemuGetVersion();

    Config->SoundDevices=SoundDevicesLoad(Config->SoundDevices);

    Actions=CommandLineParse(argc, argv);
    Curr=ListGetNext(Actions);
    while (Curr)
    {
        if (Curr->ItemType==ACT_INTERACTIVE)
        {
            InteractiveChooseMode(Curr->Tag);
            while (InteractiveSetup((const char *) Curr->Item));
        }
        else ActionPerform(Curr->ItemType,  Curr->Tag, (const char *) Curr->Item);
        Curr=ListGetNext(Curr);
    }
}
