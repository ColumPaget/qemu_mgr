#include "images.h"
#include "qmp.h"
#include <glob.h>
#include "tap-netcfg.h"
#include "os-commands.h"
#include "image-config.h"
#include "vnc.h"


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
        Result=ParserOpenItem(Qmp, "return");
        ptr=ParserGetValue(Result, "present");
        if (ptr && (strcmp(ptr, "true")==0)) Info->flags |= IMG_KVM_PRESENT;
        ptr=ParserGetValue(Result, "enabled");
        if (ptr && (strcmp(ptr, "true")==0)) Info->flags |= IMG_KVM_ACTIVE;
        ParserItemsDestroy(Qmp);

        Qmp=QMPCommand(S, "{\"execute\": \"query-name\"}\n");
        ParserItemsDestroy(Qmp);

        Qmp=QMPCommand(S, "{\"execute\": \"query-uuid\"}\n");
        ParserItemsDestroy(Qmp);

        Qmp=QMPCommand(S, "{\"execute\": \"query-status\"}\n");
        Result=ParserOpenItem(Qmp, "return");
        ptr=ParserGetValue(Result, "status");
        if (StrValid(ptr) && (strcasecmp(ptr, "paused")==0) ) Info->flags |= IMG_PAUSED;
        ParserItemsDestroy(Qmp);

        Qmp=QMPCommand(S, "{\"execute\": \"query-cpus\"}\n");
        ParserItemsDestroy(Qmp);

        Qmp=QMPCommand(S, "{\"execute\": \"query-block\"}\n");
        Result=ParserOpenItem(Qmp, "return");
        Curr=ListGetNext(Result);
        while (Curr)
        {
            Info->blockdevs=MCatStr(Info->blockdevs, ParserGetValue(Curr, "device"),":", ParserGetValue(Curr, "inserted/image/filename"), NULL);
            Info->blockdevs=CatStr(Info->blockdevs, " ");
            Curr=ListGetNext(Curr);
        }
        ParserItemsDestroy(Qmp);
        STREAMClose(S);
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





void ImageAdd(const char *ImageName, const char *Config)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    STREAM *S;
    const char *ptr;

    S=ImageConfigOpen(ImageName, "w");
    if (S)
    {
        ImageConfigSave(S, ImageName, Config);
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



void ImageCreate(const char *ImageName, const char *Config)
{
    ListNode *ConfTree;
    char *Tempstr=NULL, *Path=NULL;
    const char *p_Src;

    Path=MCopyStr(Path, GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".img", NULL);
    Tempstr=MCopyStr(Tempstr, Config, " image=", Path, NULL);
    ImageAdd(ImageName, Tempstr);
    ConfTree=ImageConfigLoad(ImageName);

    p_Src=ParserGetValue(ConfTree, "iso");
    if (StrValid(p_Src))
    {
        Tempstr=MCopyStr(Tempstr, "qemu-img create -f qcow2 ", Path,  " ", ParserGetValue(ConfTree, "size"), NULL);
        fprintf(stderr, "CREATE: %s\n", Tempstr);
        system(Tempstr);

        Tempstr=MCopyStr(Tempstr, "qemu-system-x86_64", " ", " -m 2047 -enable-kvm ", ParserGetValue(ConfTree, "image"), " -cdrom '", p_Src, "'", NULL);
        Spawn(Tempstr, "");
    }
    else
    {
        p_Src=ParserGetValue(ConfTree, "src-img");

        Tempstr=MCopyStr(Tempstr, "qemu-img convert -O qcow2 ",  " '", p_Src, "' '", ParserGetValue(ConfTree, "image"), "'", NULL);
        fprintf(stderr, "RUN: %s\n", Tempstr);
        system(Tempstr);
    }

    Destroy(Tempstr);
    Destroy(Path);
}




static char *ImageStartParseVNC(char *RetStr, const char *Config)
{
    char *Token=NULL;
    const char *ptr;

    ptr=GetToken(Config, ":", &Token, 0); //consume "vnc:"
    ptr=GetToken(ptr, ":", &Token, 0); //will either be a host or a port
    if (StrValid(ptr)) RetStr=MCatStr(RetStr, Token, ":", ptr, NULL);
    else RetStr=MCatStr(RetStr, "127.0.0.1:", Token, NULL);

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



static char *ImageSetupAudio(char *RetStr, ListNode *Config)
{
    char *Token=NULL;
    const char *ptr;

    ptr=ParserGetValue(Config, "guest-audio");
    if ( StrValid(ptr) && (strcmp(ptr, "none") !=0) )
    {
        RetStr=MCatStr(RetStr, " -soundhw ", ptr, NULL);

        ptr=ParserGetValue(Config, "host-audio");
        ptr=GetToken(ptr, ":", &Token, 0);

        if ( (! StrValid(Token) || (strcasecmp(Token, "none")==0) || (strcasecmp(Token, "default")==0)) )
        {
            //do nothing
        }
        else
        {
            if (strcasecmp(Token, "alsa")==0)
            {
                ptr=GetToken(ptr, ":", &Token, 0);
                RetStr=MCatStr(RetStr, " -audiodev driver=alsa,id=alsa,out.dev=hw:", Token, NULL);
            }
            else if (strcasecmp(Token, "oss")==0)
            {
                ptr=GetToken(ptr, ":", &Token, 0);
                RetStr=MCatStr(RetStr, " -audiodev driver=oss,id=oss,out.dev=", Token, NULL);
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

int ImageStart(const char *ImageName, const char *Options)
{
    ListNode *Config;
    char *Command=NULL, *Tempstr=NULL, *Path=NULL;
    char *ManagementDir=NULL;
    const char *ptr;
    int RunAsRoot=FALSE;

    Config=ImageConfigLoad(ImageName);
    if (! Config) return(FALSE);

    ManagementDir=MCopyStr(ManagementDir, GetCurrUserHomeDir(), "/.qemu_mgr/", NULL);
    ImageConfigUpdate(Config, Options);
    Path=CopyStr(Path, ParserGetValue(Config, "image"));

    Command=MCopyStr(Command, "qemu-system-x86_64", " -name ", ImageName,  " -enable-kvm -cpu host ", NULL);

    ptr=ParserGetValue(Config, "smp");
    //if (StrValid(ptr)) Command=MCatStr(Command, " -smp sockets=1,dies=1,cores=", ptr, NULL);
    if (StrValid(ptr)) Command=MCatStr(Command, " -smp ", ptr, NULL);

    ptr=ParserGetValue(Config, "machine");
    if (StrValid(ptr)) Command=MCatStr(Command, " -M ", ptr, NULL);

    Command=MCatStr(Command, " -pidfile ", ManagementDir, ImageName, ".pid", NULL);
    Command=CatStr(Command, " -usb -device usb-tablet ");
    ptr=ParserGetValue(Config, "disk-controller");
    if (StrValid(ptr))
    {
        if (strcasecmp(ptr, "scsi")==0) Command=MCatStr(Command, " -device megasas,id=scsi0 -drive if=", ptr, ",file='", Path, "'", NULL);
        else Command=MCatStr(Command, " -drive if=", ptr, ",file='", Path, "'", NULL);
    }
    else Command=MCatStr(Command, " '", Path, "' ", NULL);

    Command=MCatStr(Command, " -device sdhci-pci -device sd-card,drive=mmc0 -drive id=mmc0,if=none ", NULL);


    Command=ImageSetupDisplay(Command, Config);


    ptr=ParserGetValue(Config, "chroot");
    if (StrValid(ptr))
    {
        Command=MCatStr(Command, " -chroot ", ptr, NULL);
    }

    ptr=ParserGetValue(Config, "memory");
    if (StrValid(ptr))
    {
        Command=MCatStr(Command, " -m ", ptr, NULL);
    }

    Command=ImageSetupAudio(Command, Config);

    ptr=ParserGetValue(Config, "prealloc");
    if (strtobool(ptr)) Command=CatStr(Command, " -mem-prealloc ");

    ptr=ParserGetValue(Config, "network");
    if (StrValid(ptr)) Command=ImageSetupNet(Command, ImageName, Config);

    ptr=ParserGetValue(Config, "passthrough");
    if (StrValid(ptr)) Command=ImageSetupPassthrough(Command, ImageName, ptr);

    //this must happen after SetupNetwork, as SetupNetwork can specify the need to use sudo
    //for TAP/TUN networking
    Command=ImageSetupUser(Command, Config);

    Path=MCopyStr(Path, ManagementDir, ImageName, ".qmp", NULL);
    MakeDirPath(Path, 0700);
    Command=MCatStr(Command, " -qmp unix:", Path, ",server,nowait ", NULL);


    fprintf(stderr, "Run: %s\n", Command);
    ptr=ParserGetValue(Config, "sudo");

    if ( (getuid() > 0) && StrValid(ptr) )
    {
        Path=RunCommand(Path, "/sbin/modprobe tap tun", RUNCMD_ROOT | RUNCMD_DAEMON);
        Path=RunCommand(Path, Command, RUNCMD_ROOT | RUNCMD_DAEMON);
    }
    else Spawn(Command, "setsid");

    sleep(1);

    ptr=ParserGetValue(Config, "password");
    if (StrValid(ptr)) VNCSetPassword(ImageName, ptr);

    ptr=ParserGetValue(Config, "vncviewer");
    if (StrValid(ptr)) VNCLaunchViewer(ptr, Config);

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
        printf("MT: %s\n", Tempstr);
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

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"eject\", \"arguments\": { \"id\": \"", Dev, "\"} }\n", NULL);
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

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"drive-backup\", \"arguments\": { \"device\": \"", Dev, "\", \"target\": \"", SnapFile, "\", \"sync\": \"full\"} }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    ParserItemsDestroy(Qmp);

    Destroy(Dev);
    Destroy(SnapFile);
    Destroy(Name);
    Destroy(Value);
    Destroy(Tempstr);
}




