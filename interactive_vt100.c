#include "interactive.h"
#include "glob.h"
#include "images.h"
#include "os-commands.h"
#include "image-config.h"
#include "screenshot.h"
#include "actions.h"
#include "devices.h"
#include "mount.h"
#include "config-templates.h"
#include "vnc.h"
#include "qmp.h"
#include <wait.h>


#define BACK -1

STREAM *Term=NULL;
int term_wide, term_high;


typedef struct
{
    int Flags;
    char *Name;
    char *Title;
    char *Value;
    char *Options;
} TMenuItem;


static void TMenuItemDestroy(void *p_Item)
{
    TMenuItem *Item;

    if (p_Item)
    {
        Item=(TMenuItem *) p_Item;
        Destroy(Item->Name);
        Destroy(Item->Title);
        Destroy(Item->Value);
        Destroy(Item);
    }
}


void QEMUMGR_TerminalBottomBar(const char *Title)
{
  TerminalCursorSave(Term);
  TerminalCursorMove(Term, 0, term_high);
  TerminalPutStr(Title, Term);
  TerminalCursorRestore(Term);
}


void QEMUMGR_TerminalSetup(const char *TopTitle, const char *BottomTitle)
{

    TerminalClear(Term);

    //do bottom line first so that we're in the right place on the screen
    //when we return

    TerminalGeometry(Term, &term_wide, &term_high);

    TerminalCursorMove(Term, 0, 0);
    TerminalPutStr(TopTitle, Term);

    QEMUMGR_TerminalBottomBar(BottomTitle);
}




static void QEMUMGR_TerminalDialogImageConfigMenuAdd(ListNode *MenuRows, ListNode *ImageConf, const char *ConfigName, const char *Title, const char *Options)
{
    char *Tempstr=NULL;
    TMenuItem *Item;

    Item=(TMenuItem *) calloc(1, sizeof(TMenuItem));
    Item->Title=CopyStr(Item->Title, Title);
    Item->Name=CopyStr(Item->Name, ConfigName);
    Item->Value=CopyStr(Item->Value, ParserGetValue(ImageConf, Item->Name));
    Item->Options=CopyStr(Item->Options, Options);
    Tempstr=FormatStr(Tempstr, "%-30s %s", Title, Item->Value, NULL);
    ListAddNamedItem(MenuRows, Tempstr, Item);

    Destroy(Tempstr);
}



void QEMUMGR_TerminalVNCSettings(ListNode *ImageConf)
{
    char *Tempstr=NULL, *Settings=NULL;

    Tempstr=MCopyStr(Tempstr, "~B~wQEMU_MGR: VNC Settings~>~0\n", NULL);
    QEMUMGR_TerminalSetup(Tempstr, "~B~>~0");

//    Command=CatStr(Command, " --text 'Enter 127.0.0.1 to only allow connections from localhost.\nEnter 0.0.0.0 to allow connections from any host.\nEnter unix to only allow unix-socket connections' ");

    Tempstr=CopyStr(Tempstr, "");
    Tempstr=TerminalReadPrompt(Tempstr, "Allow Host: ",0, Term);
    if (StrValid(Tempstr)) Settings=MCopyStr(Settings, "display=vnc:", Tempstr, NULL);
    TerminalPutStr("\n", Term);

    Tempstr=CopyStr(Tempstr, "");
    Tempstr=TerminalReadPrompt(Tempstr, "Password: ",0, Term);
    if (StrValid(Tempstr)) Settings=MCatStr(Settings, " password=", Tempstr, NULL);
    TerminalPutStr("\n", Term);

    Tempstr=CopyStr(Tempstr, "");
    Tempstr=TerminalReadPrompt(Tempstr, "Viewer Startup Delay: ",0, Term);
    if (StrValid(Tempstr)) Settings=MCatStr(Settings, " vncviewer-delay=", Tempstr, NULL);
    TerminalPutStr("\n", Term);


    /*
    Tempstr=VNCBuildViewerList(Tempstr);
    Command=XDialogFormatComboValues(Command, NULL, "", Tempstr, "Launch Viewer:", FALSE);
    RetStr=MCatStr(RetStr, " vncviewer=", Token, " ", NULL);

    //Tempstr=TerminalChoice(Tempstr, Term, "prompt='Viewer Keyboard Language' options=default,ar,da,de,de-ch,en-gb,en-us,es,et,fi,fr,fr-be,fr-ch,fr-ca,fo,hu,hr,is,it,ja,mk,lt,lv,no,nl,nl-be,pl,pt,pt-br,ru,sl,sv,th,tr");
    */

    ImageConfigUpdate(ImageConf, Settings);

    Destroy(Settings);
    Destroy(Tempstr);
}



static int QEMUMGR_TerminalDialogImageSetupMenu(ListNode *ImageConf, int x, int y)
{
    ListNode *MenuRows, *Choice;
    int RetVal=FALSE;
    TERMMENU *Menu;
    TMenuItem *Item;
    int wide, high;
    char *Tempstr=NULL, *Value=NULL;
    const char *ptr;

    MenuRows=ListCreate();
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "smp", "CPUs/SMP Threads","");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "memory", "Memory (Megabytes)","");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "machine", "Machine Type/Motherboard","q35|pc|isapc|microvm");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "display", "Display", "std vga|virtio|qxl|rage128p - ATI Rage128Pro|RV100 - ATI Radeon|cirrus|vnc|none");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "guest-audio", "Audio in Guest VM", "es1370|ac97|hda|cs4231a|gus|sb16|none");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "host-audio", "Audio Backend on Host","");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "audio-config", "Audio Config","");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "network", "Networking System","");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "disk-controller", "Disk Controller","");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "prealloc", "Prealloc Memory", "yes|no");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "fullscreen", "Fullscreen", "yes|no");
    QEMUMGR_TerminalDialogImageConfigMenuAdd(MenuRows, ImageConf, "DONE", "~e ******** DONE use these values *******~0","");

    TerminalGeometry(Term, &wide, &high);
    Choice=TerminalMenu(Term, MenuRows, x, y, wide-2, 12);

    if (Choice)
    {
        Item=(TMenuItem *) Choice->Item;
        if (strcmp(Item->Name, "DONE") !=0)
        {
            TerminalCursorMove(Term, 1, y+13);
            if (StrValid(Item->Options))
            {
                Tempstr=MCopyStr(Tempstr, "Select new value for '", Item->Title, "' :  ", NULL);
                Value=MCopyStr(Value, "select-left=~e< select-right=>~0 options='", Item->Options, "' ", NULL);
                strrep(Value, '|', ',');
                Tempstr=TerminalChoice(Tempstr, Term, Value);
                GetToken(Tempstr, "\\S", &Value, 0);
            }
            else
            {
                Tempstr=MCopyStr(Tempstr, "Enter new value for '", Item->Title, "' :  ", NULL);
                Value=CopyStr(Value, "");
                Value=TerminalReadPrompt(Value, Tempstr, 0, Term);
            }

            if (StrLen(Value)) SetVar(ImageConf, Item->Name, Value);
            TerminalPutStr("\r~>", Term);
        }
        else RetVal=TRUE;
    }
    else RetVal=BACK;


    if (RetVal==TRUE)
    {
        ptr=ParserGetValue(ImageConf, "display");
        if (strcmp(ptr, "vnc")==0) QEMUMGR_TerminalVNCSettings(ImageConf);
    }

    ListDestroy(MenuRows, TMenuItemDestroy);

    Destroy(Tempstr);
    Destroy(Value);

    return(RetVal);
}




int QEMUMGR_TerminalDialogConfigureBanner(ListNode *ImageConf)
{
    const char *ConfigTitle[]= {"Image Name", "Image Size", "Install Type", "Install Path", "Config. Template", NULL};
    const char *ConfigNames[]= {"name", "size", "install-type", "install-path", "config-template", NULL};
    char *Tempstr=NULL, *Value=NULL, *ConfigTemplate=NULL;
    const char *ptr;
    int result=FALSE, i, j;

    for (i=0; ConfigNames[i] != NULL; i++)
    {
        QEMUMGR_TerminalSetup("~B~wQEMU_MGR: Setup New VM~>~0\n\n", "~B~w~>~0");
        for (j=0; j <= i; j++)
        {
            ptr=ParserGetValue(ImageConf, ConfigNames[j]);
            if (StrValid(ptr))
            {
                Tempstr=MCopyStr(Tempstr, ConfigTitle[j], ": ",  ptr, "\n", NULL);
                TerminalPutStr(Tempstr, Term);
            }
            else
            {
                Value=CopyStr(Value, "");
                Tempstr=MCopyStr(Tempstr, ConfigTitle[j], ": ", NULL);

                if (strcasecmp(ConfigNames[j], "install-type")==0) Value=TerminalMenuFromText(Value, Term, "Install from ISO file|Add existing disk image|Import existing disk image", 1, j+2, 40, 10);
                else if (strcasecmp(ConfigNames[j], "config-template")==0)
                {
                    TerminalPutStr("Select Configuration Template:", Term);
                    Tempstr=ConfigTemplatesGetList(Tempstr);
                    strrep(Tempstr, ',', '|');
                    Value=TerminalMenuFromText(Value, Term, Tempstr, 1, j+3, 40, 10);
                    if (StrValid(Value))
                    {
                        Tempstr=ConfigTemplateLoad(Tempstr, Value);
                        ImageConfigUpdate(ImageConf, Tempstr);
                    }
                }
                else Value=TerminalReadPrompt(Value, Tempstr, 0, Term);

                TerminalPutStr("\n", Term);
                SetVar(ImageConf, ConfigNames[j], Value);

                //we still have questions to ask!
                result=TRUE;
            }
        }
    }


    ptr=ParserGetValue(ImageConf, "install-type");
    if (strcasecmp(ptr, "install from iso file")==0) Tempstr=MCopyStr(Tempstr, "iso=", ParserGetValue(ImageConf, "install-path"), NULL);
    else if (strcasecmp(ptr, "add existing disk image")==0) Tempstr=MCopyStr(Tempstr, "img=", ParserGetValue(ImageConf, "install-path"), NULL);
    else if (strcasecmp(ptr, "import existing disk image")==0) Tempstr=MCopyStr(Tempstr, "src-img=", ParserGetValue(ImageConf, "install-path"), NULL);
    ImageConfigUpdate(ImageConf, Tempstr);

    Destroy(ConfigTemplate);
    Destroy(Tempstr);
    Destroy(Value);

    return(result);
}



void QEMUMGR_TerminalDialogConfigureImage()
{
    char *Name=NULL, *Tempstr=NULL, *InstallType=NULL, *Value=NULL;
    ListNode *ImageConf;
    int result;

    ImageConf=ListCreate();
    while (QEMUMGR_TerminalDialogConfigureBanner(ImageConf));

    result=QEMUMGR_TerminalDialogImageSetupMenu(ImageConf, 1, 8);
    while (result == FALSE)
    {
        result=QEMUMGR_TerminalDialogImageSetupMenu(ImageConf, 1, 8);
    }

//result can == 'BACK'
    if (result==TRUE)
    {
        Name=CopyStr(Name, ParserGetValue(ImageConf, "name"));
        InstallType=CopyStr(InstallType, ParserGetValue(ImageConf, "install-type"));
        Tempstr=ImageConfigExpand(Tempstr, ImageConf);

        if (strcasecmp(InstallType, "add existing disk image")==0)
        {
            ActionPerform(ACT_ADD, Name, Tempstr);
            ActionPerform(ACT_START, Name, Tempstr);
        }
        else ActionPerform(ACT_CREATE, Name, Tempstr);
    }

    ParserItemsDestroy(ImageConf);

    Destroy(InstallType);
    Destroy(Tempstr);
    Destroy(Value);
    Destroy(Name);
}



void QEMUMGR_TerminalDialogDeleteImage(const char *Name)
{
    char *Tempstr=NULL, *Command=NULL;

    Tempstr=MCopyStr(Tempstr, "~R~wQEMU_MGR: DELETE Image: ~y", Name, "~>~0\n\n", NULL);
    QEMUMGR_TerminalSetup(Tempstr, "~B~w~>~0");

    Tempstr=MCopyStr(Tempstr, "~rDELETE Image~0 '", Name, "'. ARE YOU SURE?\n", NULL);
    TerminalPutStr(Tempstr, Term);

    Tempstr=TerminalChoice(Tempstr, Term, "options=yes,no");
    if ( StrValid(Tempstr) && (strcmp(Tempstr, "yes")==0) )
    {
        ActionPerform(ACT_DELETE, Name, "");
    }

    Destroy(Command);
    Destroy(Tempstr);
}



static void QEMUMGR_TerminalMountDriveMediaBanner(const char *ImageName)
{
    char *Tempstr=NULL;

    Tempstr=MCopyStr(Tempstr, "~B~wQEMU_MGR: Mount Drive Media: ~y", ImageName, "~>~0\n\n", NULL);
    QEMUMGR_TerminalSetup(Tempstr, "~B~w~>~0");

    Tempstr=CopyStr(Tempstr, "\nMount files or device into a drive in the guest VM.\nSelect a block device or select an archive type to create and mount.\nYou will then be asked to select files/directories to include.\n\n");
    TerminalPutStr(Tempstr, Term);

    Destroy(Tempstr);
}


static void QEMUMGR_TerminalMountDriveMedia(const char *ImageName)
{
    char *Command=NULL, *Tempstr=NULL, *Value=NULL;
    const char *ptr;


    QEMUMGR_TerminalMountDriveMediaBanner(ImageName);
    Tempstr=QMPListBlockDevs(Tempstr, ImageName, 0);
    strrep(Tempstr, ',', '|');

    TerminalCursorMove(Term, 2, 8);
    TerminalPutStr("Select target device in guest VM", Term);
    Value=TerminalMenuFromText(Value, Term, Tempstr, 1, 9, 40, 10);
    if (StrValid(Value))
    {
        Command=MCopyStr(Command, "dev=", Value, NULL);

        QEMUMGR_TerminalMountDriveMediaBanner(ImageName);
        Tempstr=MountFindSourceTypes(Tempstr);
        strrep(Tempstr, ',', '|');

        TerminalCursorMove(Term, 2, 8);
        TerminalPutStr("Select source type", Term);
        Value=TerminalMenuFromText(Value, Term, Tempstr, 1, 9, 40, 10);
        if (StrValid(Value))
        {
            StrTruncChar(Value, ' ');
            Command=MCatStr(Command, " media=", Value, NULL);
            if (MountTypeIsArchive(Value))
            {
                Tempstr=CopyStr(Tempstr, "");
                Tempstr=TerminalReadPrompt(Tempstr, "Enter path for source files/directory: ", 0, Term);
                Command=MCatStr(Command, ":", Tempstr, NULL);
            }
            MountItem(ImageName, Command);
        }
    }


    Destroy(Value);
    Destroy(Tempstr);
    Destroy(Command);
}


void QEMUMGR_TerminalDialogManageImage(const char *Name, TImageInfo *ImageInfo)
{
    char *Command=NULL, *Tempstr=NULL, *Token=NULL;
    ListNode *ImageConf, *MenuItems, *Choice;
    const char *ptr;

    ImageConf=ImageConfigLoad(Name);

    Tempstr=MCopyStr(Tempstr, "~B~wQEMU_MGR: Manage Image: ~y", Name, "~>~0", NULL);
    QEMUMGR_TerminalSetup(Tempstr, "~B~w~>~0");

    MenuItems=ListCreate();

    ListAddNamedItem(MenuItems, "Connect with VNC", NULL);
    if (ImageInfo->flags & IMG_PAUSED) ListAddNamedItem(MenuItems, "Resume", NULL);
    else ListAddNamedItem(MenuItems, "Pause", NULL);

    ListAddNamedItem(MenuItems, "Reboot", NULL);
    ListAddNamedItem(MenuItems, "Shutdown", NULL);
    ListAddNamedItem(MenuItems, "Wakeup", NULL);
    ListAddNamedItem(MenuItems, "Kill", NULL);
    ListAddNamedItem(MenuItems, "Screenshot", NULL);
    ListAddNamedItem(MenuItems, "Mount Drive Media", NULL);


    Tempstr=QMPListBlockDevs(Tempstr, Name, BD_MOUNTED | BD_INCLUDE_MEDIA | BD_REMOVABLE);
    ptr=GetToken(Tempstr, ",", &Token, 0);
    while (ptr)
    {
        Command=MCatStr(Command, "Eject ", Token, " ", NULL);
        ListAddNamedItem(MenuItems, Command, NULL);
        ptr=GetToken(ptr, ",", &Token, 0);
    }


    Choice=TerminalMenu(Term, MenuItems, 1, 2, 40, 10);

    if (Choice)
    {
        if (strcmp(Choice->Tag, "Connect with VNC")==0) VNCLaunchViewer("tightvnc", ImageConf);
        else if (strcmp(Choice->Tag, "Pause")==0) ImagePause(Name, "");
        else if (strcmp(Choice->Tag, "Resume")==0) ImageResume(Name, "");
        else if (strcmp(Choice->Tag, "Reboot")==0) ImageReboot(Name, "");
        else if (strcmp(Choice->Tag, "Shutdown")==0) ImageStop(Name, "");
        else if (strcmp(Choice->Tag, "Kill")==0) ImageKill(Name, "");
        else if (strcmp(Choice->Tag, "Wakeup")==0) ImageWakeup(Name, "");
        else if (strcmp(Choice->Tag, "Screenshot")==0) ImageScreenshot(Name, "");
        else if (strcmp(Choice->Tag, "Mount Drive Media")==0) QEMUMGR_TerminalMountDriveMedia(Name);
        else if (strncmp(Choice->Tag, "Eject ", 6)==0)
        {
            GetToken(Choice->Tag+6, ":", &Token, 0);
            Tempstr=MCopyStr(Tempstr, "dev=", Token, NULL);
            ImageMediaRemove(Name, Tempstr);
        }
    }

    ListDestroy(MenuItems, Destroy);
    ParserItemsDestroy(ImageConf);

    Destroy(Command);
    Destroy(Tempstr);
    Destroy(Token);
}




void QEMUMGR_TerminalDialogImageConfigMenu(const char *ImageName, ListNode *ImageConf)
{
    char *Tempstr=NULL;
    int result;

    result=QEMUMGR_TerminalDialogImageSetupMenu(ImageConf, 1, 4);
    while (result == FALSE)
    {
        result=QEMUMGR_TerminalDialogImageSetupMenu(ImageConf, 1, 4);
    }

    if (result==TRUE)
    {
        Tempstr=ImageConfigExpand(Tempstr, ImageConf);
        ImageChange(ImageName, Tempstr);
    }

    Destroy(Tempstr);
}


void StartVMWithSettings(const char *ImageName)
{
    ListNode *ImageConf;
    char *Tempstr=NULL;
    int result;

    Tempstr=MCopyStr(Tempstr, "~B~wQEMU_MGR: Start VM '", ImageName, "'~>~0\n", NULL);
    QEMUMGR_TerminalSetup(Tempstr, "~B~wAwaiting Selection~>~0");

    ImageConf=ImageConfigLoad(ImageName);

    result=QEMUMGR_TerminalDialogImageSetupMenu(ImageConf, 1, 4);
    while (result == FALSE)
    {
        result=QEMUMGR_TerminalDialogImageSetupMenu(ImageConf, 1, 4);
    }

    if (result==TRUE)
    {
    	QEMUMGR_TerminalBottomBar("~B~wStarting VM~>~0");
        Tempstr=ImageConfigExpand(Tempstr, ImageConf);
        ActionPerform(ACT_START, ImageName, Tempstr);
    }

    ParserItemsDestroy(ImageConf);
    Destroy(Tempstr);
}


void QEMUMGR_TerminalDialogImageManageMenu(const char *ImageName, ListNode *ImageConf)
{
    TERMMENU *Menu;
    ListNode *MenuRows, *Choice;
    char *Tempstr=NULL;
    const char *ptr;
    int wide, high;

    MenuRows=ListCreate();
    ListAddNamedItem(MenuRows, "Start VM with these settings", NULL);
    ListAddNamedItem(MenuRows, "Start VM with other settings", NULL);
    ListAddNamedItem(MenuRows, "Configure VM", NULL);
    ListAddNamedItem(MenuRows, "~rDELETE VM~0", NULL);

    while (1)
    {
        Tempstr=MCopyStr(Tempstr, "~B~wQEMU_MGR: Manage VM '", ImageName, "'~>~0\n\n", NULL);
        QEMUMGR_TerminalSetup(Tempstr, "~B~w~>~0");

        Tempstr=FormatStr(Tempstr, " %-20s ~e%s~0\n", "CPUs/SMP Threads:", ParserGetValue(ImageConf, "smp"));
        TerminalPutStr(Tempstr, Term);
        Tempstr=FormatStr(Tempstr, " %-20s ~e%s~0\n", "Machine Type:", ParserGetValue(ImageConf, "machine"));
        TerminalPutStr(Tempstr, Term);
        Tempstr=FormatStr(Tempstr, " %-20s ~e%s~0\n", "Memory:", ParserGetValue(ImageConf, "memory"));
        TerminalPutStr(Tempstr, Term);
        Tempstr=FormatStr(Tempstr, " %-20s ~e%s~0\n", "Display Type:", ParserGetValue(ImageConf, "display"));
        TerminalPutStr(Tempstr, Term);
        Tempstr=FormatStr(Tempstr, " %-20s ~e%s~0\n", "Disk Controller:", ParserGetValue(ImageConf, "disk-controller"));
        TerminalPutStr(Tempstr, Term);
        ptr=ParserGetValue(ImageConf, "network");
        if (! StrValid(ptr)) ptr="usermode";
        if (strcasecmp(ptr, "tap")==0)
            Tempstr=FormatStr(Tempstr, " %-20s ~e%s~0  external-IP: ~e%s~0\n", "Networking:", ptr, ParserGetValue(ImageConf, "tap_extip"));
        else Tempstr=FormatStr(Tempstr, " %-20s ~e%s~0\n", "Networking:", ParserGetValue(ImageConf, "network"));
        TerminalPutStr(Tempstr, Term);
        Tempstr=MCopyStr(Tempstr, " Audio:               Guest-Side: ~e", ParserGetValue(ImageConf, "guest-audio"), "~0     Host-Side: ~e", ParserGetValue(ImageConf, "host-audio"), "~0\n",NULL);
        TerminalPutStr(Tempstr, Term);
        TerminalPutStr("\n", Term);


        TerminalGeometry(Term, &wide, &high);
        Choice=TerminalMenu(Term, MenuRows, 1, 10, 40, 8);

        if (Choice == NULL) break;
        else if (strcmp(Choice->Tag, "Start VM with these settings")==0) ActionPerform(ACT_START, ImageName, "");
        else if (strcmp(Choice->Tag, "Start VM with other settings")==0) StartVMWithSettings(ImageName);
        else if (strcmp(Choice->Tag, "Configure VM")==0) QEMUMGR_TerminalDialogImageConfigMenu(ImageName, ImageConf);
        else if (strcmp(Choice->Tag, "~rDELETE VM~0")==0)
        {
            QEMUMGR_TerminalDialogDeleteImage(ImageName);
            //if they delete the image, then break out
            break;
        }
    }

    ListDestroy(MenuRows, Destroy);
    Destroy(Tempstr);
}


void QEMUMGR_TerminalDialogStartImage(const char *Name)
{
    char *Tempstr=NULL, *Action=NULL;
    ListNode *ImageConf;
    const char *ptr;

    Tempstr=MCopyStr(Tempstr, "~B~wQEMU_MGR: Start VM '", Name, "'~>~0\n", NULL);
    QEMUMGR_TerminalSetup(Tempstr, "~B~w~>~0");
    ImageConf=ImageConfigLoad(Name);
    if (ImageConf)
    {
        QEMUMGR_TerminalDialogImageManageMenu(Name, ParserOpenItem(ImageConf, "/"));
        ParserItemsDestroy(ImageConf);
    }

    Destroy(Tempstr);
    Destroy(Action);
}


int QEMUMGR_TerminalDialogSetup(const char *SetupInfo)
{
    char *Tempstr=NULL, *Name=NULL;
    char *Command=NULL;
    const char *ptr;
    ListNode *MenuItems, *Choice, *ImageConf;
    TImageInfo *ImageInfo;
    TERMMENU *Menu;
    int RetVal=FALSE, i;
    glob_t Glob;

    if (! Term)
    {
        Term=STREAMFromDualFD(0,1);
        TerminalInit(Term, TERM_RAWKEYS | TERM_SAVE_ATTRIBS);
    }


    while (1)
    {
        QEMUMGR_TerminalSetup("~B~wQEMU_MGR version 1.0~>~0\n","~B~w~>~0");

        MenuItems=ListCreate();
        ListAddNamedItem(MenuItems, "~m~e** Setup New Image **~0", CopyStr(NULL, "setup"));

        Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/*.qemu_mgr", NULL);
        glob(Tempstr, 0, 0, &Glob);

        for (i=0; i < Glob.gl_pathc; i++)
        {
            Name=CopyStr(Name, GetBasename(Glob.gl_pathv[i]));
            StrRTruncChar(Name, '.');
            ImageConf=ImageConfigLoad(Name);
            ImageInfo=ImageGetRunningInfo(Name);
            if (ImageInfo)
            {
                if (ImageInfo->flags & IMG_PAUSED) ptr="~rPAUSED~0";
                else ptr="~yRUNNING~0";

                Tempstr=FormatStr(Tempstr, "%-20s %-08s %s", Name, ptr, GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", ImageInfo->start_time, NULL));
                ImageInfoDestroy(ImageInfo);
            }
            else Tempstr=CopyStr(Tempstr, Name);

            ListAddNamedItem(MenuItems, Tempstr, CopyStr(NULL, Name));
            ParserItemsDestroy(ImageConf);
        }

        Choice=TerminalMenu(Term, MenuItems, 1, 2, term_wide-2, 10);

        if (Choice)
        {
            RetVal=TRUE;
            if (strcmp(Choice->Item, "setup") ==0) QEMUMGR_TerminalDialogConfigureImage();
            else
            {
                Name=CopyStr(Name, Choice->Item);
                ImageInfo=ImageGetRunningInfo(Name);
                if (ImageInfo)
                {
                    QEMUMGR_TerminalDialogManageImage(Name, ImageInfo);
                    ImageInfoDestroy(ImageInfo);
                }
                else QEMUMGR_TerminalDialogStartImage(Name);
            }
        }
        else
        {
            RetVal=FALSE;
            break;
        }

        ListDestroy(MenuItems, Destroy);
        globfree(&Glob);
    }

    TerminalClear(Term);
    TerminalCursorMove(Term, 0, 0);
    TerminalReset(Term);

    Destroy(Command);
    Destroy(Tempstr);
    Destroy(Name);

    return(RetVal);
}

