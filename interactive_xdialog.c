#include "interactive.h"
#include "glob.h"
#include "images.h"
#include "os-commands.h"
#include "image-config.h"
#include "actions.h"
#include "vnc.h"
#include "devices.h"
#include <wait.h>



void XDialogFindXDialogCommand(const char *XDialogCommandList)
{
    char *Token=NULL;
    const char *ptr, *p_Cmd;

    ptr=GetToken(XDialogCommandList, ",", &Token, 0);
    while (ptr)
    {
        p_Cmd=OSCommandFindPath(Token);
        if (StrValid(p_Cmd))
        {
            Config->DialogCmd=CopyStr(Config->DialogCmd, Token);
            break;
        }
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Destroy(Token);
}


char *XDialogRun(char *RetStr, const char *Command, int ReturnExitStatus)
{
    char *Tempstr=NULL;
    STREAM *S;
    int ExitCode;
    pid_t pid;

    RetStr=CopyStr(RetStr, "");
    Tempstr=MCopyStr(Tempstr, Config->DialogCmd, " ", Command, NULL);
    S=STREAMSpawnCommand(Tempstr, "");
    if (S)
    {
        pid=atoi(STREAMGetValue(S, "PeerPID"));
        STREAMSetTimeout(S, 0);
        RetStr=STREAMReadLine(RetStr, S);
        StripTrailingWhitespace(RetStr);
        STREAMClose(S);
    }

    waitpid(pid, &ExitCode, 0);
    if (ReturnExitStatus)
    {
        if (ExitCode==0) RetStr=CopyStr(RetStr, "TRUE");
        else RetStr=CopyStr(RetStr, "FALSE");
    }

    Destroy(Tempstr);

    return(RetStr);
}


char *XDialogMenu(char *RetStr, const char *Title, const char *MenuHeader, const char *MenuItems)
{
    char *Command=NULL;

    Command=MCopyStr(Command, " --list --title '", Title, "' --column '", MenuHeader, "' ", MenuItems, NULL);
    RetStr=XDialogRun(RetStr, Command, FALSE);

    Destroy(Command);
    return(RetStr);
}

char *XDialogQuery(char *RetStr, const char *Title, const char *Text)
{
    char *Command=NULL;

    Command=MCopyStr(Command, " --entry --title '", Title, "' --text '", Text, "'", NULL);
    RetStr=XDialogRun(RetStr, Command, FALSE);

    Destroy(Command);
    return(RetStr);
}


static char *XDialogFormatComboValues(char *RetStr, ListNode *ImageConf, const char *Field, const char *Options, const char *Title, int CropCompare)
{
    char *ComboValues=NULL;
    char *Token=NULL, *Compare=NULL, *Selected=NULL, *UnSelected=NULL;
    const char *ptr, *p_Current=NULL;


    if (ImageConf) p_Current=ParserGetValue(ImageConf, Field);
    if (StrValid(p_Current))
    {
        ptr=GetToken(Options, ",", &Token, 0);

        while (ptr)
        {
            Compare=CopyStr(Compare, Token);
            if (CropCompare) StrTruncChar(Compare, ' ');

            if (strcmp(p_Current, Compare) != 0) UnSelected=MCatStr(UnSelected, "|", Token, NULL);
            else Selected=CopyStr(Selected, Token);
            ptr=GetToken(ptr, ",", &Token, 0);
        }

        if (! StrValid(Selected)) Selected=CopyStr(Selected, p_Current);

        ComboValues=MCopyStr(ComboValues, Selected, UnSelected, NULL);
    }
    else
    {
        ComboValues=CopyStr(ComboValues, Options);
        strrep(ComboValues, ',', '|');
    }


    RetStr=MCatStr(RetStr, " --add-combo '", Title, ":' --combo-values '", ComboValues, "' ",NULL);

    Destroy(ComboValues);
    Destroy(Selected);
    Destroy(UnSelected);
    Destroy(Compare);
    Destroy(Token);

    return(RetStr);
}



static char *XDialogSetupSource(char *Setup, const char *InstallType, const char *InstallSource)
{
    char *Command=NULL, *Tempstr=NULL, *Title=NULL, *ConfigField=NULL;

    if (strcasecmp(InstallType, "install from iso")==0)
    {
        Title=CopyStr(Title, "Select .iso file to install OS from.");
        ConfigField=CopyStr(ConfigField, "iso");
    }
    else if (strcasecmp(InstallType, "add existing disk image")==0)
    {
        Title=CopyStr(Title, "Select disk image.");
        ConfigField=CopyStr(ConfigField, "img");
    }
    else if (strcasecmp(InstallType, "import existing disk image")==0)
    {
        Title=CopyStr(Title, "Select disk image to copy/import.");
        ConfigField=CopyStr(ConfigField, "src-img");
    }

    Command=MCopyStr(Command, " --file-selection --title '", Title, "'", NULL);
    Tempstr=XDialogRun(Tempstr, Command, FALSE);

    Setup=MCatStr(Setup, " ", ConfigField, "='", Tempstr,"' ", NULL);

    Destroy(ConfigField);
    Destroy(Command);
    Destroy(Tempstr);
    Destroy(Title);

    return(Setup);
}





char *XDialogSetupNetwork(char *Setup, const char *NetType)
{
    char *Tempstr=NULL, *Token=NULL, *Command=NULL;
    const char *ptr;

    if (strcmp(NetType, "tap")==0)
    {
        Command=MCopyStr(Command, " --forms --title 'Setup TAP Networking' ", NULL);
        Command=MCatStr(Command, " --add-entry 'External IP:' ", NULL);
        Command=XDialogFormatComboValues(Command, NULL, "tap_local_access", "yes,no", "Mod firewall to access host", FALSE);
        Command=XDialogFormatComboValues(Command, NULL, "tap_remote_access", "yes,no", "Mod firewall to access network", FALSE);

        Tempstr=XDialogRun(Tempstr, Command, FALSE);

        Setup=CatStr(Setup, " network=tap ");
        ptr=GetToken(Tempstr, "|", &Token, 0);
        Setup=MCatStr(Setup, " tap_extip=", Token, NULL);

        ptr=GetToken(ptr, "|", &Token, 0);
        Setup=MCatStr(Setup, " tap_local_access=", Token, NULL);

        ptr=GetToken(ptr, "|", &Token, 0);
        Setup=MCatStr(Setup, " tap_remote_access=", Token, NULL);

        printf("TAP: %s\n", Setup);
    }
    else Setup=MCatStr(Setup, " network=", NetType, " ", NULL);

    Destroy(Command);
    Destroy(Tempstr);
    Destroy(Token);

    return(Setup);
}


char *QEMUFormatAudio(char *RetStr, const char *GuestDev, const char *HostDev, const char *HostConf)
{
    char *Token=NULL;
    const char *ptr;

    RetStr=MCatStr(RetStr, " guest-audio=", GuestDev, NULL);
    if (strcmp(GuestDev, "none") !=0)
    {
        ptr=GetToken(HostDev, ":", &Token, 0);
        RetStr=MCatStr(RetStr, " host-audio=", Token, ":", NULL);
        ptr=GetToken(ptr, ":", &Token, 0);
        RetStr=CatStr(RetStr, Token);

        RetStr=MCatStr(RetStr, " audio-config='", HostConf, "' ", NULL);
    }

    Destroy(Token);

    return(RetStr);
}



char *XDialogSetupVNC(char *RetStr)
{
    char *Command=NULL, *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    Command=MCopyStr(Command, " --forms --title 'qemu_mgr: Setup VNC' ", NULL);
    Command=CatStr(Command, " --text 'Enter 127.0.0.1 to only allow connections from localhost.\nEnter 0.0.0.0 to allow connections from any host.\nEnter unix to only allow unix-socket connections' ");
    Command=MCatStr(Command, " --add-entry 'Allow Host:' ", NULL);
    Command=MCatStr(Command, " --add-entry 'Password:' ", NULL);
    Tempstr=VNCBuildViewerList(Tempstr);
    Command=XDialogFormatComboValues(Command, NULL, "", Tempstr, "Launch Viewer:", FALSE);
    Tempstr=MCopyStr(Tempstr, "default,ar,da,de,de-ch,en-gb,en-us,es,et,fi,fr,fr-be,fr-ch,fr-ca,fo,hu,hr,is,it,ja,mk,lt,lv,no,nl,nl-be,pl,pt,pt-br,ru,sl,sv,th,tr");
    Command=XDialogFormatComboValues(Command, NULL, "", Tempstr, "Viewer Keyboard Language:", FALSE);
    Command=MCatStr(Command, " --add-entry 'Viewer Startup Delay:' ", NULL);

    Tempstr=XDialogRun(Tempstr, Command, FALSE);
    ptr=GetToken(Tempstr, "|", &Token, 0);

    if (! StrValid(Token)) RetStr=CatStr(RetStr, " display=vnc:127.0.0.1 ");
    else RetStr=MCatStr(RetStr, " display=vnc:", Token, " ", NULL);

    ptr=GetToken(ptr, "|", &Token, 0);
    if (StrValid(ptr)) RetStr=MCatStr(RetStr, "password=", Token, NULL);

    ptr=GetToken(ptr, "|", &Token, 0);
    if ( StrValid(Token) && (strcmp(Token, "none") != 0) )
    {
        RetStr=MCatStr(RetStr, " vncviewer=", Token, " ", NULL);
    }

    ptr=GetToken(ptr, "|", &Token, 0);
    if ( StrValid(Token) && (strcmp(Token, "default") != 0) )
    {
        RetStr=MCatStr(RetStr, " vncviewer-language=", Token, " ", NULL);
    }

    ptr=GetToken(ptr, "|", &Token, 0);
    if ( StrValid(Token))
    {
        RetStr=MCatStr(RetStr, " vncviewer-delay=", Token, " ", NULL);
    }
    Destroy(Command);
    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}



char *XDialogSetupPassthrough(char *RetStr)
{
    ListNode *Curr, *Devices;
    char *Command=NULL, *Tempstr=NULL;
    TDevice *Dev;

    Devices=ListCreate();
    DevicesGetUSB(Devices);
    Curr=ListGetNext(Devices);

    Command=MCopyStr(Command, " --list --multiple --title 'Passthrough Devices' --text 'Select Devices to pass-through to the guest VM. These devices will not be available to the host system while the VM is running.' --ok-label 'Select' --cancel-label 'Cancel' --column 'Address' --column 'Name' --column 'Manufacturer' ", NULL);
    while (Curr)
    {
        Dev=(TDevice *) Curr->Item;
        if (Dev->BusNum > 0)
        {
            Tempstr=FormatStr(Tempstr, "'%s:%04d:%04d' '%s' '%s' ", "USB", Dev->BusNum, Dev->DevNum, Dev->Product, Dev->Manufacturer);
            Command=CatStr(Command, Tempstr);
        }
        Curr=ListGetNext(Curr);
    }

    Tempstr=XDialogRun(Tempstr, Command, FALSE);
    if (StrValid(Tempstr))
    {
        strrep(Tempstr, '|', ',');
        RetStr=MCatStr(RetStr, " passthrough='", Tempstr, "' ", NULL);
    }
    printf("DEVS: %s\n", Tempstr);

    Destroy(Command);
    Destroy(Tempstr);

    return(RetStr);
}



char *XDialogQueryVMSetup(char *RetStr, int Action, ListNode *ImageConf, char **RetName, char **RetInstallType)
{
    char *Command=NULL, *Tempstr=NULL, *Token=NULL, *Name=NULL;
    char *NetType=NULL, *InstallSource=NULL, *AudioBackend=NULL, *AudioConfig=NULL;
    const char *ptr="";


    Command=MCopyStr(Command, " --forms --cancel-label 'Back' --title 'Setup New VM Image' ", NULL);
    if (Action==ACT_CREATE)
    {
        Command=MCatStr(Command, " --add-entry 'Image Name:' ", NULL);
        Command=MCatStr(Command, " --add-entry 'Image Size:' ", NULL);
        Command=XDialogFormatComboValues(Command, NULL, "", "install from iso,add existing disk image,import existing disk image", "Install Type", FALSE);
        Command=XDialogFormatComboValues(Command, NULL, "", "local disk,network path", "Install Source", FALSE);
    }
    else
    {
        Name=CopyStr(Name, ParserGetValue(ImageConf, "name"));
        Command=MCopyStr(Command, " --forms --cancel-label 'Back' --title 'VM Image: ", Name, "' ", NULL);
        Command=XDialogFormatComboValues(Command, ImageConf, "", "start vm,save config,start vm and save config,DELETE VM", "Action", FALSE);
    }


    Command=XDialogFormatComboValues(Command, ImageConf, "smp", "8,4,2,1,other", "SMP Threads", FALSE);
    Command=XDialogFormatComboValues(Command, ImageConf, "machine", "pc i440fx - suitable for old O.S.,q35 suitable for recent O.S.,isapc - ISA only PC,microvm - minimalist virtio machine", "Machine Type", TRUE);
    Command=XDialogFormatComboValues(Command, ImageConf, "memory", "128,256,512,1024,2046,other", "Memory (Meg)", FALSE);
    Command=XDialogFormatComboValues(Command, ImageConf, "display", "std vga,virtio,qxl,rage128p - ATI Rage128Pro,RV100 - ATI Radeon,cirrus,vnc,none", "Display", TRUE);
    Command=XDialogFormatComboValues(Command, ImageConf, "guest-audio", "es1370,ac97,hda,cs4231a,gus,sb16,none", "Guest Audio in VM",TRUE);
    Command=XDialogFormatComboValues(Command, ImageConf, "host-audio", Config->SoundDevices, "Host Audio Device", FALSE);
    Command=XDialogFormatComboValues(Command, ImageConf, "audio-config", "qemu default,set by guest,buffered 44100Hz,buffered 48000Hz", "Audio Settings",FALSE);
    Command=XDialogFormatComboValues(Command, ImageConf, "network", "usermode,tap,none", "Network",TRUE);
    Command=XDialogFormatComboValues(Command, ImageConf, "disk-controller", "ide,scsi,virtio", "Disk Controller",TRUE);
    Command=XDialogFormatComboValues(Command, ImageConf, "prealloc", "no,yes", "Prealloc Memory",FALSE);
    Command=XDialogFormatComboValues(Command, ImageConf, "fullscreen", "no,yes", "Fullscreen",FALSE);
    Command=XDialogFormatComboValues(Command, ImageConf, "passthrough", "no,yes", "Passthrough Devices",FALSE);


    printf("RUN: %s\n", Command);
    Tempstr=XDialogRun(Tempstr, Command, FALSE);
    ptr=Tempstr;

    if (Action==ACT_CREATE)
    {
        ptr=GetToken(ptr, "|", &Name, 0);
        RetStr=MCopyStr(RetStr, " name=", Name, " ", NULL);
        *RetName=CopyStr(*RetName, Name);
        ptr=GetToken(ptr, "|", &Token, 0);
        RetStr=MCatStr(RetStr, " size=", Token, " ", NULL);
        ptr=GetToken(ptr, "|", RetInstallType, 0);
        ptr=GetToken(ptr, "|", &InstallSource, 0);
    }
//in 'ACT_START' mode this will be the action
    else ptr=GetToken(ptr, "|", RetInstallType, 0);

    ptr=GetToken(ptr, "|", &Token, 0);
    RetStr=MCatStr(RetStr, " smp=", Token, " ", NULL);

    ptr=GetToken(ptr, "|", &Token, 0);
    StrTruncChar(Token, ' ');
    RetStr=MCatStr(RetStr, " machine=", Token, NULL);

    ptr=GetToken(ptr, "|", &Token, 0);
    RetStr=MCatStr(RetStr, " mem=", Token, " ", NULL);

    ptr=GetToken(ptr, "|", &Token, 0);
    if (strcasecmp(Token, "std vga")==0) Token=CopyStr(Token, "std");
    if (strcasecmp(Token, "vnc")==0) RetStr=XDialogSetupVNC(RetStr);
    else RetStr=MCatStr(RetStr, " display='", Token, "' ", NULL);

    ptr=GetToken(ptr, "|", &Token, 0);
    ptr=GetToken(ptr, "|", &AudioBackend, 0);
    ptr=GetToken(ptr, "|", &AudioConfig, 0);
    RetStr=QEMUFormatAudio(RetStr, Token, AudioBackend, AudioConfig);
    ptr=GetToken(ptr, "|", &NetType, 0);
    RetStr=XDialogSetupNetwork(RetStr, NetType);
    ptr=GetToken(ptr, "|", &Token, 0);
    RetStr=MCatStr(RetStr, " disk-controller=", Token, " ", NULL);
    ptr=GetToken(ptr, "|", &Token, 0);
    RetStr=MCatStr(RetStr, " prealloc=", Token, " ", NULL);
    ptr=GetToken(ptr, "|", &Token, 0);
    RetStr=MCatStr(RetStr, " fullscreen=", Token, " ", NULL);

    ptr=GetToken(ptr, "|", &Token, 0);
    printf("PASS: %s\n", Token);
    if (strtobool(Token)) RetStr=XDialogSetupPassthrough(RetStr);


    if (Action==ACT_CREATE) RetStr=XDialogSetupSource(RetStr, *RetInstallType, InstallSource);

    printf("Setup: %s\n", RetStr);

    Destroy(InstallSource);
    Destroy(AudioBackend);
    Destroy(AudioConfig);
    Destroy(Command);
    Destroy(Tempstr);
    Destroy(NetType);
    Destroy(Token);
    Destroy(Name);

    return(RetStr);
}

void XDialogConfigureImage()
{
    char *Setup=NULL, *Name=NULL, *InstallType=NULL;

    /*
    ImageConf=ImageConfigLoad(Name);
    if (ImageConf)
    {
    */
//ptr=ParserGetValue(ImageConf, "memory");
    Setup=XDialogQueryVMSetup(Setup, ACT_CREATE, NULL, &Name, &InstallType);

    printf("INSTALL TYPE: [%s] %s\n", InstallType, Setup);
    if (strcasecmp(InstallType, "add existing disk image")==0)
    {
        ActionPerform(ACT_ADD, Name, Setup);
        ActionPerform(ACT_START, Name, Setup);
    }
    else ActionPerform(ACT_CREATE, Name, Setup);

//ParserItemsDestroy(ImageConf);
//}

    Destroy(InstallType);
    Destroy(Setup);
    Destroy(Name);
}


void XDialogDeleteImage(const char *Name)
{
    char *Tempstr=NULL, *Command=NULL;

    Command=MCopyStr(Command, " --question --title 'qemu_mgr: delete VM' --text 'Really DELETE ", Name, " VM?'", NULL);
    Tempstr=XDialogRun(Tempstr, Command, TRUE);

    if ( StrValid(Tempstr) && (strcmp(Tempstr, "TRUE")==0) )
    {
        ActionPerform(ACT_DELETE, Name, "");
    }

    Destroy(Command);
    Destroy(Tempstr);
}


void XDialogManageImage(const char *Name, TImageInfo *ImageInfo)
{
    char *Command=NULL, *Tempstr=NULL;

    Command=MCopyStr(Command, " --list --title 'Running VM: ", Name, "' --ok-label 'Select' --cancel-label 'Back' ", NULL);
    Command=CatStr(Command, "'Connect with VNC' ");
    if (ImageInfo->flags & IMG_PAUSED) Command=CatStr(Command, "'Resume' ");
    else Command=CatStr(Command, "'Pause' ");
    Command=CatStr(Command, "'Reboot' ");
    Command=CatStr(Command, "'Shutdown' ");
    Command=CatStr(Command, "'Wakeup' ");
    Command=CatStr(Command, "'Screenshot' ");

    Tempstr=XDialogRun(Tempstr, Command, FALSE);

    if (StrValid(Tempstr))
    {
        if (strcmp(Tempstr, "Connect with VNC")==0) VNCConnect(Name);
        if (strcmp(Tempstr, "Pause")==0) ImagePause(Name, "");
        if (strcmp(Tempstr, "Resume")==0) ImageResume(Name, "");
        if (strcmp(Tempstr, "Reboot")==0) ImageReboot(Name, "");
        if (strcmp(Tempstr, "Shutdown")==0) ImageStop(Name, "");
        if (strcmp(Tempstr, "Wakeup")==0) ImageWakeup(Name, "");
// if (strcmp(Tempstr, "Screenshot")==0) ImageScreenshot(Name, "");
    }

    Destroy(Command);
    Destroy(Tempstr);
}

void XDialogStartImage(const char *Name)
{
    char *Tempstr=NULL, *Action=NULL;
    ListNode *ImageConf;
    const char *ptr;

    ImageConf=ImageConfigLoad(Name);
    if (ImageConf)
    {
        Tempstr=XDialogQueryVMSetup(Tempstr, ACT_START, ImageConf, NULL, &Action);

        if (StrValid(Tempstr))
        {
            if (strcmp(Action, "start vm")==0) ActionPerform(ACT_START, Name, Tempstr);
            else if (strcmp(Action, "save config")==0) ActionPerform(ACT_CHANGE, Name, Tempstr);
            else if (strcmp(Action, "start vm and save config")==0)
            {
                ActionPerform(ACT_CHANGE, Name, Tempstr);
                ActionPerform(ACT_START, Name, Tempstr);
            }
            else if (strcmp(Action, "DELETE VM")==0) XDialogDeleteImage(Name);
        }

        ParserItemsDestroy(ImageConf);
    }

    Destroy(Tempstr);
    Destroy(Action);
}


int XDialogSetup(const char *SetupInfo)
{
    char *Tempstr=NULL, *Name=NULL;
    char *Command=NULL;
    const char *ptr;
    ListNode *ImageConf;
    TImageInfo *ImageInfo;
    glob_t Glob;
    int RetVal=FALSE, i;

    XDialogFindXDialogCommand("qarma,zenity,xdialog");

    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/*.qemu_mgr", NULL);
    glob(Tempstr, 0, 0, &Glob);

    Command=MCopyStr(Command, " --list --title 'Available VM Images' --text 'Choose \"Setup New Image\" to install a new VM. Otherwise select a VM to start or manage' --column 'Name' --column 'State' --column 'Start Time' ", NULL);
    Command=CatStr(Command, "'Setup New Image' '' '' ");
    for (i=0; i < Glob.gl_pathc; i++)
    {
        Name=CopyStr(Name, GetBasename(Glob.gl_pathv[i]));
        StrRTruncChar(Name, '.');
        ImageConf=ImageConfigLoad(Name);
        ImageInfo=ImageGetRunningInfo(Name);
        if (ImageInfo)
        {
            if (ImageInfo->flags & IMG_PAUSED) ptr="PAUSED";
            else ptr="RUN";

            Tempstr=FormatStr(Tempstr, "'%s' '%s' '%s' ", Name, ptr, GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", ImageInfo->start_time, NULL));
            ImageInfoDestroy(ImageInfo);
        }
        else Tempstr=MCopyStr(Tempstr, "'", Name, "' '' '' ", NULL);

        Command=CatStr(Command, Tempstr);
        ParserItemsDestroy(ImageConf);
    }


    Tempstr=XDialogRun(Tempstr, Command, FALSE);

    if (StrValid(Tempstr))
    {
        RetVal=TRUE;
        if (strcmp(Tempstr, "Setup New Image")==0)
        {
            XDialogConfigureImage();
        }
        else
        {
            ptr=GetToken(Tempstr, " ", &Name, GETTOKEN_QUOTES);
            ImageInfo=ImageGetRunningInfo(Name);
            if (ImageInfo) XDialogManageImage(Name, ImageInfo);
            else XDialogStartImage(Name);
        }
    }
    else RetVal=FALSE;

    globfree(&Glob);

    Destroy(Command);
    Destroy(Tempstr);
    Destroy(Name);

    return(RetVal);
}



const char *XDialogQueryRootPassword(const char *Title)
{
    Config->RootPassword=XDialogQuery(Config->RootPassword, Title, "");

    return(Config->RootPassword);
}


