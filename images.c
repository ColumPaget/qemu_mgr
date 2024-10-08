#include "images.h"
#include "qmp.h"
#include <glob.h>
#include "tap-netcfg.h"
#include "os-commands.h"
#include "image-config.h"
#include "interactive.h"
#include "vnc.h"
#include "kvm.h"


void OutputImageInfo(TImageInfo *Info)
{
    printf("kvm: %d\n", Info->flags);
    printf("blockdevs: %s\n", Info->blockdevs);
}



TImageInfo *ImageGetRunningInfo(const char *ImageName)
{
    STREAM *S;
    ListNode *Qmp, *Result, *Curr;
    const char *ptr;
    char *Tempstr=NULL;
    struct stat Stat;
    TImageInfo *Info=NULL;

    S=QMPOpen(ImageName);
    if (S)
    {
        Info=(TImageInfo *) calloc(1, sizeof(TImageInfo));

        Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".pid", NULL);
        stat(Tempstr, &Stat);
        Info->start_time=Stat.st_mtime;

        Qmp=QMPCommand(S, "{\"execute\": \"query-kvm\"}\n");
        if (Qmp)
        {
            Result=ParserOpenItem(Qmp, "return");
            ptr=ParserGetValue(Result, "present");
            if (ptr && (strcmp(ptr, "true")==0)) Info->flags |= IMG_KVM_PRESENT;
            ptr=ParserGetValue(Result, "enabled");
            if (ptr && (strcmp(ptr, "true")==0)) Info->flags |= IMG_KVM_ACTIVE;
            ParserItemsDestroy(Qmp);
        }

        Qmp=QMPCommand(S, "{\"execute\": \"query-name\"}\n");
        if (Qmp) ParserItemsDestroy(Qmp);

        Qmp=QMPCommand(S, "{\"execute\": \"query-uuid\"}\n");
        if (Qmp) ParserItemsDestroy(Qmp);

        Qmp=QMPCommand(S, "{\"execute\": \"query-status\"}\n");
        if (Qmp)
        {
            Result=ParserOpenItem(Qmp, "return");
            ptr=ParserGetValue(Result, "status");
            if (StrValid(ptr) && (strcasecmp(ptr, "paused")==0) ) Info->flags |= IMG_PAUSED;
            ParserItemsDestroy(Qmp);
        }

        Qmp=QMPCommand(S, "{\"execute\": \"query-cpus\"}\n");
        if (Qmp) ParserItemsDestroy(Qmp);


        STREAMClose(S);
        //must do this after STREAMClose when QMP connection is freed up
        Info->blockdevs=QMPListBlockDevs(Info->blockdevs, ImageName, BD_INCLUDE_MEDIA);
    }

    Destroy(Tempstr);
    return(Info);
}


void ImageInfoDestroy(void *p_ImageInfo)
{
    TImageInfo *Img;

    Img=(TImageInfo *) p_ImageInfo;
    Destroy(Img->cpus);
    Destroy(Img->chardevs);
    Destroy(Img->blockdevs);
    Destroy(Img->jobs);
}





void ImageAdd(const char *ImageName, const char *VMConfig)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    STREAM *S;
    const char *ptr;

    S=ImageConfigOpen(ImageName, "w");
    if (S)
    {
        ImageConfigSave(S, ImageName, VMConfig);
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
}



void ImageDelete(const char *ImageName)
{
    char *Tempstr=NULL;
    glob_t Glob;
    int i;

    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".*", NULL);
    glob(Tempstr, 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        unlink(Glob.gl_pathv[i]);
    }
    globfree(&Glob);
    Destroy(Tempstr);
}


void ImageChange(const char *ImageName, const char *Options)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    STREAM *S;
    ListNode *ConfTree;
    const char *ptr;

    ConfTree=ImageConfigLoad(ImageName);
    S=ImageConfigOpen(ImageName, "w");
    if (S)
    {
        ImageConfigUpdate(ConfTree, Options);
        Tempstr=ImageConfigExpand(Tempstr, ConfTree);
        ImageConfigSave(S, ImageName, Tempstr);
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
}





static char *ImageStartParseVNC(char *RetStr, const char *Config)
{
    char *Token=NULL;
    const char *ptr;

    ptr=GetToken(Config, ":", &Token, 0); //consume "vnc:"
    ptr=GetToken(ptr, ":", &Token, 0); //will either be a host or a port
    if (StrValid(Token) && StrValid(ptr)) RetStr=MCatStr(RetStr, Token, ":", ptr, NULL);
    else if (StrValid(Token)) RetStr=MCatStr(RetStr, Token, ":0", NULL);
    else if (StrValid(ptr)) RetStr=MCatStr(RetStr, "127.0.0.1", ":", ptr, NULL);
    else RetStr=CatStr(RetStr, "127.0.0.1:0");

    Destroy(Token);

    return(RetStr);
}





static char *ImageSetupNet(char *RetStr, const char *ImageName, ListNode *Config)
{
    char *Tempstr=NULL, *NetConfig=NULL, *Path=NULL;
    ListNode *Values;
    const char *ptr;

    Values=ParserOpenItem(Config, "/");
    NetConfig=CopyStr(NetConfig, ParserGetValue(Config, "network"));

    if (StrValid(NetConfig))
    {
        //if NetConfig doesn't equal 'none' then apply '-net nic' to all following network types
        if (strcmp(NetConfig, "none") != 0) RetStr=MCatStr(RetStr, " -net nic,model=e1000 ");

        if (strncasecmp(NetConfig, "usermode", 8)==0)
        {
            RetStr=MCatStr(RetStr, " -net user", NetConfig+8, NULL);
        }
        else if (strncasecmp(NetConfig, "tap", 3)==0) RetStr=TapSetupQemuCommandLine(RetStr, ImageName, Values);
        else RetStr=MCatStr(RetStr, " -net ", NetConfig, NULL);
    }

    ptr=ParserGetValue(Config, "portfwd");
    if (StrValid(ptr))
    {
        RetStr=MCatStr(RetStr, ",hostfwd=", ptr, NULL);
    }

    Destroy(NetConfig);
    Destroy(Tempstr);
    Destroy(Path);

    return(RetStr);
}


static char *ImageSetupPassthrough(char *RetStr, const char *ImageName, const char *Config)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    char *tptr;
    int BusNum, DevNum;

    ptr=GetToken(Config, ",", &Token, 0);
    while (ptr)
    {
        if (strncasecmp(Token, "USB:", 4)==0)
        {
            BusNum=strtol(Token+4, &tptr, 10);
            tptr++;
            DevNum=strtol(tptr, &tptr, 10);
            Tempstr=FormatStr(Tempstr, " -device usb-host,hostbus=%d,hostaddr=%d ", BusNum, DevNum);
            RetStr=CatStr(RetStr, Tempstr);
        }
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}


static const char *AlsaFormatDev(const char *Config, char **DevStr)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    Tempstr=CopyStr(Tempstr, Config);
    StrTruncChar(Tempstr, ' ');
    //if we get 'hw' that means we have a device name in format 'hw:<val>.<val>', which we have to
    //reformat to hw:<val>,,<val> to get it into qemu
    if (strncmp(Tempstr, "hw:",3)==0)
    {
        ptr=GetToken(Tempstr, ":", &Token, 0);
        ptr=GetToken(ptr, ".", &Token, 0);
        *DevStr=MCopyStr(*DevStr,"hw:", Token, ",,", ptr, NULL);
    }
    else *DevStr=CopyStr(*DevStr, Tempstr);

    Destroy(Tempstr);
    Destroy(Token);
    return(ptr);
}


static char *ImageSetupAudio(char *RetStr, ListNode *Config)
{
    char *Token=NULL, *Tempstr=NULL;
    const char *ptr;

    ptr=ParserGetValue(Config, "guest-audio");
    if ( StrValid(ptr) && (strcmp(ptr, "none") !=0) )
    {
        if (GetQemuMajorVersion() > 5)
        {
            if (strcmp(ptr, "ac97")==0) RetStr=CatStr(RetStr, " -device AC97");
            else if (strcmp(ptr, "hda")==0) RetStr=CatStr(RetStr, " -device intel-hda");
            else if (strcmp(ptr, "es1370")==0) RetStr=CatStr(RetStr, " -device ES1370");
            else RetStr=MCatStr(RetStr, " -device ", ptr, NULL);
        }
        else RetStr=MCatStr(RetStr, " -soundhw ", ptr, NULL);

        ptr=ParserGetValue(Config, "host-audio");
        ptr=GetToken(ptr, ":", &Token, 0);

        if ( (! StrValid(Token) || (strcasecmp(Token, "none")==0) || (strcasecmp(Token, "default")==0)) )
        {
            //do nothing
        }
        else
        {
            if (strcasecmp(Token, "pulseaudio")==0) RetStr=MCatStr(RetStr, " -audiodev driver=pa,id=snd0", NULL);
            if (strcasecmp(Token, "alsa")==0)
            {
                ptr=AlsaFormatDev(ptr, &Tempstr);
                RetStr=MCatStr(RetStr, " -audiodev driver=alsa,id=alsa,out.dev=", Tempstr, ",in.dev=", Tempstr, NULL);
            }
            else if (strcasecmp(Token, "oss")==0)
            {
                ptr=GetToken(ptr, ":", &Token, 0);
                RetStr=MCatStr(RetStr, " -audiodev driver=oss,id=oss,out.dev=", Token, ",in.dev=", Token, NULL);
            }


            ptr=ParserGetValue(Config, "audio-config");
            if (StrValid(ptr))
            {
                if (strcasecmp(ptr, "set by guest")==0) RetStr=MCatStr(RetStr, ",out.fixed-settings=off", NULL);
                else if (strcasecmp(ptr, "buffered 48000Hz")==0) RetStr=MCatStr(RetStr, ",out.fixed-settings=on,out.frequency=48000,out.buffer-length=80000,timer-period=5000", NULL);
                else if (strcasecmp(ptr, "buffered 44100Hz")==0) RetStr=MCatStr(RetStr, ",out.fixed-settings=on,out.frequency=44100,out.buffer-length=80000,timer-period=5000", NULL);
            }
        }


        //make sure we have a space to seperate all this from other arguments
        RetStr=CatStr(RetStr, " ");
    }

    Destroy(Token);
    Destroy(Tempstr);

    return(RetStr);
}


static char *ImageSetupDisplay(char *RetStr, ListNode *Config)
{
    char *VNCSetup=NULL;
    const char *ptr;

    ptr=ParserGetValue(Config, "display");
    if (! StrValid(ptr)) ptr="std";
    if (strcasecmp(ptr, "none")==0)
    {
        RetStr=MCatStr(RetStr, " -nographic ");
    }
    else if (strncasecmp(ptr,"vnc:", 4)==0)
    {
        VNCSetup=ImageStartParseVNC(VNCSetup, ptr);
        RetStr=MCatStr(RetStr, " -display vnc=", VNCSetup, NULL);
        ptr=ParserGetValue(Config, "password");
        if (StrValid(ptr)) RetStr=MCatStr(RetStr, ",password", NULL);


        SetVar(Config, "vnc-endpoint", VNCSetup);
        ptr=ParserGetValue(Config, "vncviewer-language");
        if (StrValid(ptr)) RetStr=MCatStr(RetStr, " -k ", ptr, " ", NULL);
    }
    else
    {
        if (strcasecmp(ptr, "ati rage128")==0) RetStr=MCatStr(RetStr, " -device ati-vga,model=rage128p ", NULL);
        else if (strcasecmp(ptr, "ati radeon RV100")==0) RetStr=MCatStr(RetStr, " -device ati-vga,model=rv100 ", NULL);
        else RetStr=MCatStr(RetStr, " -vga ", ptr, " ", NULL);

        ptr=ParserGetValue(Config, "fullscreen");
        if (strtobool(ptr)) RetStr=MCatStr(RetStr, " -full-screen ");
    }

    Destroy(VNCSetup);

    return(RetStr);
}



static char *ImageSetupUser(char *RetStr, ListNode *Config)
{
    const char *ptr;

    ptr=ParserGetValue(Config, "user");
    if (StrValid(ptr))
    {
        RetStr=MCatStr(RetStr, " -runas ", ptr, NULL);
    }

    return(RetStr);
}




void ImageCreate(const char *ImageName, const char *VMConfig)
{
    ListNode *ConfTree;
    char *Tempstr=NULL, *Path=NULL, *BusyMsg=NULL;
    const char *p_Src;
    pid_t pid;

    Path=MCopyStr(Path, GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".img", NULL);
    Tempstr=MCopyStr(Tempstr, VMConfig, " image=", Path, NULL);
    ImageAdd(ImageName, Tempstr);
    ConfTree=ImageConfigLoad(ImageName);

    p_Src=ParserGetValue(ConfTree, "iso");
    if (StrValid(p_Src))
    {
        Tempstr=MCopyStr(Tempstr, "qemu-img create -f qcow2 ", Path,  " ", ParserGetValue(ConfTree, "size"), NULL);
        pid=InteractiveBusyWindow("qemu_mgr: Busy", "Please wait, creating image file");
        system(Tempstr);
        if (pid > 0) kill(pid, SIGTERM);

        Tempstr=MCopyStr(Tempstr, "qemu-system-x86_64", " ", " -m 2047 ", NULL);
        if (KVMAvailable()) Tempstr=CatStr(Tempstr, " -enable-kvm -cpu host ");
        Tempstr=MCatStr(Tempstr, " ", ParserGetValue(ConfTree, "image"), " -cdrom '", p_Src, "'", NULL);
        Spawn(Tempstr, "");
    }
    else
    {
        p_Src=ParserGetValue(ConfTree, "src-img");

        Tempstr=MCopyStr(Tempstr, "qemu-img convert -O qcow2 ",  " '", p_Src, "' '", ParserGetValue(ConfTree, "image"), "'", NULL);

        pid=InteractiveBusyWindow("qemu_mgr: Busy", "Please wait, importing image");
        system(Tempstr);
        if (pid > 0) kill(pid, SIGTERM);
    }

    Destroy(BusyMsg);
    Destroy(Tempstr);
    Destroy(Path);
}



int ImageStart(const char *ImageName, const char *Options)
{
    ListNode *VMConfig;
    char *Command=NULL, *Tempstr=NULL, *Path=NULL;
    char *ManagementDir=NULL;
    const char *ptr;
    int RunAsRoot=FALSE;

    VMConfig=ImageConfigLoad(ImageName);
    if (! VMConfig)
    {
        Tempstr=MCopyStr(Tempstr, "No registered VM matches '", ImageName, "'", NULL);
        InteractiveErrorMessage("Fatal Error", Tempstr);
        Destroy(Tempstr);
        return(FALSE);
    }

    ManagementDir=MCopyStr(ManagementDir, GetCurrUserHomeDir(), "/.qemu_mgr/", NULL);
    ImageConfigUpdate(VMConfig, Options);
    Path=CopyStr(Path, ParserGetValue(VMConfig, "image"));

    if (access(Path, F_OK)==0)
    {
        Command=MCopyStr(Command, "qemu-system-x86_64", " -name ", ImageName,  NULL);
        if (KVMAvailable()) Command=CatStr(Command, " -enable-kvm -cpu host ");

        ptr=ParserGetValue(VMConfig, "smp");
        //if (StrValid(ptr)) Command=MCatStr(Command, " -smp sockets=1,dies=1,cores=", ptr, NULL);
        if (StrValid(ptr)) Command=MCatStr(Command, " -smp ", ptr, NULL);

        ptr=ParserGetValue(VMConfig, "machine");
        if (StrValid(ptr)) Command=MCatStr(Command, " -M ", ptr, NULL);

        Command=MCatStr(Command, " -pidfile ", ManagementDir, ImageName, ".pid", NULL);
        Command=CatStr(Command, " -usb -device usb-tablet ");
        ptr=ParserGetValue(VMConfig, "disk-controller");
        if (StrValid(ptr))
        {
            if (strcasecmp(ptr, "scsi")==0) Command=MCatStr(Command, " -device megasas,id=scsi0 -drive if=", ptr, ",file=", Path, NULL);
            else Command=MCatStr(Command, " -drive if=", ptr, ",file=", Path, NULL);
        }
        else Command=MCatStr(Command, " '", Path, "' ", NULL);

        //Command=MCatStr(Command, " -device sdhci-pci -device sd-card,drive=mmc0 -drive id=mmc0,if=none ", NULL);


        Command=ImageSetupDisplay(Command, VMConfig);


        ptr=ParserGetValue(VMConfig, "chroot");
        if (StrValid(ptr))
        {
            Command=MCatStr(Command, " -chroot ", ptr, NULL);
        }

        ptr=ParserGetValue(VMConfig, "memory");
        if (StrValid(ptr))
        {
            Command=MCatStr(Command, " -m ", ptr, NULL);
        }

        Command=ImageSetupAudio(Command, VMConfig);

        ptr=ParserGetValue(VMConfig, "prealloc");
        if (strtobool(ptr)) Command=CatStr(Command, " -mem-prealloc ");

        ptr=ParserGetValue(VMConfig, "network");
        if (StrValid(ptr)) Command=ImageSetupNet(Command, ImageName, VMConfig);

        ptr=ParserGetValue(VMConfig, "passthrough");
        if (StrValid(ptr)) Command=ImageSetupPassthrough(Command, ImageName, ptr);

        //this must happen after SetupNetwork, as SetupNetwork can specify the need to use sudo
        //for TAP/TUN networking
        Command=ImageSetupUser(Command, VMConfig);

        Path=MCopyStr(Path, ManagementDir, ImageName, ".qmp", NULL);
        MakeDirPath(Path, 0700);
        Command=MCatStr(Command, " -qmp unix:", Path, ",server,nowait ", NULL);


        ptr=ParserGetValue(VMConfig, "sudo");

        if ( (getuid() > 0) && StrValid(ptr) )
        {
            Path=RunCommand(Path, "/sbin/modprobe tap", RUNCMD_ROOT);
            Path=RunCommand(Path, "/sbin/modprobe tun", RUNCMD_ROOT);
						printf("MP: %s\n", Path);
            Path=RunCommand(Path, Command, RUNCMD_ROOT | RUNCMD_NOSHELL);
        }
        else Spawn(Command, "setsid");

        sleep(1);

        ptr=ParserGetValue(VMConfig, "password");
        if (StrValid(ptr)) VNCSetPassword(ImageName, ptr);

        ptr=ParserGetValue(VMConfig, "vncviewer");
        if (StrValid(ptr)) VNCLaunchViewer(ptr, VMConfig);
    }
    else
    {
        Tempstr=MCopyStr(Tempstr, "No image file '", Path, "'", NULL);
        InteractiveErrorMessage("Fatal Error", Tempstr);
    }

    Destroy(ManagementDir);
    Destroy(Command);
    Destroy(Tempstr);
    Destroy(Path);

    return(TRUE);
}

int ImagePause(const char *ImageName, const char *Options)
{
    ListNode *Qmp;

    Qmp=QMPTransact(ImageName, "{\"execute\": \"stop\"}\n");
    ParserItemsDestroy(Qmp);
    return(TRUE);
}

int ImageResume(const char *ImageName, const char *Options)
{
    ListNode *Qmp;

    Qmp=QMPTransact(ImageName, "{\"execute\": \"cont\"}\n");
    ParserItemsDestroy(Qmp);
    return(TRUE);
}

int ImageReboot(const char *ImageName, const char *Options)
{
    ListNode *Qmp;

    Qmp=QMPTransact(ImageName, "{\"execute\": \"system_reset\"}\n");
    ParserItemsDestroy(Qmp);
    return(TRUE);
}


int ImageStop(const char *ImageName, const char *Options)
{
    ListNode *Qmp;

    Qmp=QMPTransact(ImageName, "{\"execute\": \"system_powerdown\"}\n");
    ParserItemsDestroy(Qmp);
    return(TRUE);
}

int ImageWakeup(const char *ImageName, const char *Options)
{
    ListNode *Qmp;

    Qmp=QMPTransact(ImageName, "{\"execute\": \"system_wakeup\"}\n");
    ParserItemsDestroy(Qmp);
    return(TRUE);
}

int ImageKill(const char *ImageName, const char *Options)
{
    ListNode *Qmp;
    char *Tempstr=NULL;
    pid_t pid;
    STREAM *S;

    Qmp=QMPTransact(ImageName, "{\"execute\": \"system_powerdown\"}\n");
    ParserItemsDestroy(Qmp);
    sleep(3);


    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".pid", NULL);
    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        pid=atoi(Tempstr);
        if (pid > 1) kill(pid, SIGKILL);
        STREAMClose(S);
        sleep(3);
    }

    return(TRUE);
}


int ImageMediaAdd(const char *ImageName, const char *Options)
{
    char *Dev=NULL, *Media=NULL, *Tempstr=NULL;
    char *Name=NULL, *Value=NULL;
    ListNode *Qmp;
    const char *ptr;

    ptr=GetNameValuePair(Options, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "media")==0) Media=CopyStr(Media, Value);
        if (strcasecmp(Name, "dev")==0) Dev=CopyStr(Dev, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"blockdev-change-medium\", \"arguments\": { \"device\": \"", Dev, "\", \"filename\": \"", Media, "\", \"format\": \"raw\" } }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    if (QMPIsError(Qmp))
    {
        ParserItemsDestroy(Qmp);

        //try older 'change' command
        Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"change\", \"arguments\": { \"device\": \"", Dev, "\", \"target\": \"", Media, "\", \"arg\": \"raw\"} }\n", NULL);
        Qmp=QMPTransact(ImageName, Tempstr);
    }

    ParserItemsDestroy(Qmp);

    Destroy(Dev);
    Destroy(Media);
    Destroy(Name);
    Destroy(Value);
    Destroy(Tempstr);
}



int ImageMediaRemove(const char *ImageName, const char *Options)
{
    char *Dev=NULL, *Tempstr=NULL;
    char *Name=NULL, *Value=NULL;
    ListNode *Qmp;
    const char *ptr;

    ptr=GetNameValuePair(Options, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "dev")==0) Dev=CopyStr(Dev, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"eject\", \"arguments\": { \"device\": \"", Dev, "\", \"force\": true} }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    ParserItemsDestroy(Qmp);

    Destroy(Dev);
    Destroy(Name);
    Destroy(Value);
    Destroy(Tempstr);
}


int ImageTakeSnapshot(const char *ImageName, const char *Options)
{
    char *Dev=NULL, *SnapFile=NULL, *Tempstr=NULL;
    char *Name=NULL, *Value=NULL;
    ListNode *Qmp;
    const char *ptr;

    ptr=GetNameValuePair(Options, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "dev")==0) Dev=CopyStr(Dev, Value);
        if (strcasecmp(Name, "file")==0) SnapFile=CopyStr(SnapFile, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"blockdev-snapshot-sync\", \"arguments\": { \"device\": \"", Dev, "\", \"snapshot-file\": \"", SnapFile, "\"} }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    ParserItemsDestroy(Qmp);

    Destroy(Dev);
    Destroy(SnapFile);
    Destroy(Name);
    Destroy(Value);
    Destroy(Tempstr);
}


int ImageDriveBackup(const char *ImageName, const char *Options)
{
    char *Dev=NULL, *BackupFile=NULL, *TempFile=NULL, *Tempstr=NULL;
    char *Name=NULL, *Value=NULL;
    ListNode *Qmp;
    const char *ptr;

    TempFile=MCopyStr(TempFile, GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".snap", NULL);
    ptr=GetNameValuePair(Options, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "dev")==0) Dev=CopyStr(Dev, Value);
        if (strcasecmp(Name, "file")==0) BackupFile=CopyStr(BackupFile, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"blockdev-snapshot-sync\", \"arguments\": { \"device\": \"", Dev, "\", \"snapshot-file\": \"", TempFile, "\"} }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    ParserItemsDestroy(Qmp);

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"drive-backup\", \"arguments\": { \"device\": \"", Dev, "\", \"target\": \"", BackupFile, "\", \"sync\": \"full\"} }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    ParserItemsDestroy(Qmp);

    while (1)
    {
        Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"query-jobs\" }\n", NULL);
        Qmp=QMPTransact(ImageName, Tempstr);
        ParserItemsDestroy(Qmp);


        sleep(10);
    }

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"block-commit\", \"arguments\": { \"device\": \"", Dev, "\", \"top\": \"", TempFile, "\"} }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    ParserItemsDestroy(Qmp);

    unlink(TempFile);

    Destroy(Dev);
    Destroy(BackupFile);
    Destroy(Name);
    Destroy(Value);
    Destroy(Tempstr);
}

