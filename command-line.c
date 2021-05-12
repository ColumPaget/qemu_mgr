#include "command-line.h"
#include "image-config.h"
#include "config-templates.h"
#include "actions.h"
#include "devices.h"
#include "images.h"
#include "vnc.h"
#include <glob.h>


static void CommandLineListUSB()
{
    ListNode *Devs, *Curr;
    TDevice *Dev;

    Devs=ListCreate();
    DevicesGetUSB(Devs);

    Curr=ListGetNext(Devs);
    while (Curr)
    {
        Dev=(TDevice *) Curr->Item;
        if (Dev->BusNum > 0) printf("USB:%d:%- 4d  %s:%s\n", Dev->BusNum, Dev->DevNum, Dev->Manufacturer, Dev->Product);
        Curr=ListGetNext(Curr);
    }
}

static void CommandLineListTemplates()
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    Tempstr=ConfigTemplatesGetList(Tempstr);
    ptr=GetToken(Tempstr, ",", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        printf("%s\n", Token);
        ptr=GetToken(ptr, ",", &Token, GETTOKEN_QUOTES);
    }

    Destroy(Tempstr);
    Destroy(Token);
}


static void CommandLineListImages(const char *ListType)
{
    char *Tempstr=NULL, *State=NULL, *RunInfo=NULL, *Name=NULL;
    glob_t Glob;
    TImageInfo *ImageInfo;
    ListNode *ImageConf;
    int i;

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
            RunInfo=CopyStr(RunInfo, GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", ImageInfo->start_time, NULL));
            State=CopyStr(State, "UP");
            ImageInfoDestroy(ImageInfo);
        }
        else
        {
            RunInfo=CopyStr(RunInfo, "   ");
            State=CopyStr(State, "");
        }

        printf("%-5s %-20s size=%s mem=%s %s\n", State, ParserGetValue(ImageConf, "name"), ParserGetValue(ImageConf, "size"), ParserGetValue(ImageConf, "memory"), RunInfo);
        ParserItemsDestroy(ImageConf);
    }
    globfree(&Glob);

    Destroy(Tempstr);
    Destroy(RunInfo);
    Destroy(State);
    Destroy(Name);
}



static void CommandLineListCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;

    arg=CommandLineNext(Cmd);
    if (StrValid(arg))
    {
        if (strcasecmp(arg, "images")==0) CommandLineListImages("list-images");
        else if (strcasecmp(arg, "running")==0) CommandLineListImages("list-running");
        else CommandLineListImages("list-all");
    }
    else CommandLineListImages("list-all");
}


static char *CommandLineParseImageArgs(char *RetStr, CMDLINE *Cmd)
{
    const char *arg, *ptr;
    char *Tempstr=NULL;
    ListNode *Conf;

    Conf=ListCreate();
    ImageConfigUpdate(Conf, RetStr);
    RetStr=CopyStr(RetStr, "");
    arg=CommandLineNext(Cmd);
    while (arg)
    {

        if (strcmp(arg, "-size")==0) SetVar(Conf, "size", CommandLineNext(Cmd));
        else if (strcmp(arg, "-img")==0) SetVar(Conf, "img", CommandLineNext(Cmd));
        else if (strcmp(arg, "-iso")==0) SetVar(Conf, "iso", CommandLineNext(Cmd));
        else if (strcmp(arg, "-mem")==0) SetVar(Conf, "memory", CommandLineNext(Cmd));
        else if (strcmp(arg, "-mach")==0) SetVar(Conf, "machine", CommandLineNext(Cmd));
        else if (strcmp(arg, "-dc")==0) SetVar(Conf, "disk-controller", CommandLineNext(Cmd));
        else if (strcmp(arg, "-delay")==0) SetVar(Conf, "vncviewer-delay", CommandLineNext(Cmd));
        else if (strcmp(arg, "-pw")==0) SetVar(Conf, "password", CommandLineNext(Cmd));
        else if (strcmp(arg, "-pass")==0) SetVar(Conf, "password", CommandLineNext(Cmd));
        else if (strcmp(arg, "-password")==0) SetVar(Conf, "password", CommandLineNext(Cmd));
        else if (strcmp(arg, "-net")==0) SetVar(Conf, "network", CommandLineNext(Cmd));
        else if (strcmp(arg, "-user")==0) SetVar(Conf, "user", CommandLineNext(Cmd));
        else if (strcmp(arg, "-chroot")==0) SetVar(Conf, "chroot", CommandLineNext(Cmd));
        else if (strcmp(arg, "-jail")==0) SetVar(Conf, "jail", CommandLineNext(Cmd));
        else if (strcmp(arg, "-pf")==0) SetVar(Conf, "portfwd", CommandLineNext(Cmd));
        else if (strcmp(arg, "-template")==0)
        {
            arg=CommandLineNext(Cmd);
            Tempstr=ConfigTemplateLoad(Tempstr, arg);
            if (StrValid(Tempstr)) ImageConfigUpdate(Conf, Tempstr);
        }
        else if (*arg == '-') SetVar(Conf, arg+1, CommandLineNext(Cmd));
        else
        {
            ptr=GetToken(arg, "=", &Tempstr, 0);
            SetVar(Conf, Tempstr, ptr);
        }

        arg=CommandLineNext(Cmd);
    }

    RetStr=ImageConfigExpand(RetStr, Conf);

    Destroy(Tempstr);

    return(RetStr);
}


static void CommandLineParseCreateCommand(ListNode *Actions, CMDLINE *Cmd)
{
    char *Name=NULL, *Config=NULL;
    const char *arg;

    Name=CopyStr(Name, CommandLineNext(Cmd));

    Config=ConfigTemplateLoad(Config, "Generic");
    Config=CommandLineParseImageArgs(Config, Cmd);
    ListAddTypedItem(Actions, ACT_CREATE, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}


static void CommandLineParseCommand(int CmdType, ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    Config=CommandLineParseImageArgs(Config, Cmd);

    ListAddTypedItem(Actions, CmdType, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}



static void CommandLineParseMediaAddCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "dev='", arg, "' ", NULL);
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "media='", arg, "' ", NULL);

    if (
        (strncmp(arg, "tar:", 4)==0) ||
        (strncmp(arg, "tgz:", 4)==0) ||
        (strncmp(arg, "txz:", 4)==0) ||
        (strncmp(arg, "zip:", 4)==0) ||
        (strncmp(arg, "7za:", 4)==0) ||
        (strncmp(arg, "iso:", 4)==0)
    ) ListAddTypedItem(Actions, ACT_MOUNT, Name, Config);
    else ListAddTypedItem(Actions, ACT_MEDIA_ADD, Name, Config);


    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}

static void CommandLineParseMediaRemoveCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "dev=", arg, " ", NULL);

    ListAddTypedItem(Actions, ACT_MEDIA_REMOVE, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}

static void CommandLineParseSnapshotCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "dev=", arg, " ", NULL);
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "file=", arg, " ", NULL);

    ListAddTypedItem(Actions, ACT_SNAPSHOT, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}



static void CommandLineParseSendCommand(ListNode *Actions, int SendType, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "value=", arg, " ", NULL);

    ListAddTypedItem(Actions, SendType, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}

static void CommandLineParseScreenshotCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "file=", arg, " ", NULL);

    ListAddTypedItem(Actions, ACT_SCREENSHOT, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}





static void CommandLineParseBackupCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "dev=", arg, " ", NULL);
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "file=", arg, " ", NULL);

    ListAddTypedItem(Actions, ACT_DRIVE_BACKUP, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}


static void CommandLineParseInfoCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    ListAddTypedItem(Actions, ACT_INFO, Name, Config);

    Destroy(Name);

//dont do this! We've added it to the list
//Destroy(Config);
}


static void CommandLinePrintVersion()
{
    printf("qemu_mgr: version %s\n", VERSION);
}

static void CommandLinePrintHelp()
{
    printf("qemu_mgr can be run in interactive mode (follow-your-nose menus) or at the command line. The interactive menus either require zenity or qarma to be installed, or else can be run in 'terminal' mode ('qemu_mgr -i term') to get generic text-driven menus.\n\n");
    printf("Command line options are:\n\n");
    printf("qemu_mgr\n");
    printf("	run interactive mode with auto-detected dialog system\n");
    printf("qemu_mgr -i <type>\n");
    printf("	run interactive mode with specified dialog system. 'type' can be 'term', 'qarma' or 'zenity'\n");
    printf("qemu_mgr create <vm name> <options>\n");
    printf("	create a vm from a .iso file. 'options' must include a '-iso' option specifying the path to the .iso file\n");
    printf("qemu_mgr import <vm name> <options>\n");
    printf("	import a vm from an image file. 'options' must include a '-img' option specifying the path to the image file\n");
    printf("qemu_mgr add		<vm name> <options>\n");
    printf("	add an existing image file. This option doesn't create an image file in the '.qemu_mgr' directory, but uses the existing image file whereever it is. 'options' must include a '-img' option specifying the path to the image file\n");
    printf("qemu_mgr change <vm name> <options>\n");
    printf("	change settings for an existing vm.\n");
    printf("qemu_mgr start	<vm name> <options>\n");
    printf("	start a configured vm, possibly changing some settings via 'options'.\n");
    printf("qemu_mgr stop	 <vm name> <options>\n");
    printf("	stop a running vm.\n");
    printf("qemu_mgr list	 <vm name> <options>\n");
    printf("	list configured VMs.\n");
    printf("qemu_mgr vnc		<vm name> <options>\n");
    printf("	connect to a vm with VNC. (VM must be configured with -display vnc)\n");
    printf("qemu_mgr send-text <vm name> <options>\n");
    printf("	read text line-by-line from stdin and send it to a running vm.\n");
    printf("qemu_mgr send-key	<vm name> <key name>\n");
    printf("	send keystroke 'key name' to a running vm.\n");
    printf("qemu_mgr media-add <vm name> <vm device> <path>\n");
    printf("	copy items in 'path' to a file and load/mount it into 'device' in the vm. See 'qemu_mgr --help media' for more info.\n");
    printf("qemu_mgr media-del <vm name> <vm device>\n");
    printf("	remove/unmount file in 'device' in the vm. See 'qemu_mgr --help-media' for more info.\n");
    printf("qemu_mgr screenshot <vm name> <options>\n");
    printf("	screenshot a running vm.\n");
    printf("qemu_mgr -?\n");
    printf("	print this help\n");
    printf("qemu_mgr -help\n");
    printf("	print this help\n");
    printf("qemu_mgr --help\n");
    printf("	print this help\n");
    printf("qemu_mgr --help-vnc\n");
    printf("	print help for VNC display\n");
    printf("qemu_mgr --help-media\n");
    printf("	print help for mounting files/directories into a running vm\n");
    printf("\n");
    printf("Options\nMost options can be expressed either as '-iso installer.iso' or 'iso=installer.iso'. Some options, like -iso, -img or -size only relate to the VM creation and installation step\n\n");
    printf(" -iso <path>        specify path to a .iso O.S. installer file for use in setting up a vm\n");
    printf(" -img <path>        specify path to an image file for use in setting up a vm\n");
    printf(" -template <name>   config template to use for VM. Default is the template 'Generic' which creates a disk image 40G in size, and a memorty allocation of 2047 megabytes, and a machine type of 'pc' (i440fx)\n");
    printf(" -size <size>       size of newly created image file, overriding what was specified in the selected template. The 'size' argument takes a postfix of 'M' or 'G' to specify megabytes or gigabytes.\n");
    printf(" -mem <size>        memory allocated to the VM in megabytes, overriding the selected template.\n");
    printf(" -disk-controller  <type>        disk-controller. One of 'virtio', 'scsi' or 'ide'.\n");
    printf(" -dc  <type>        disk-controller. One of 'virtio', 'scsi' or 'ide'.\n");
    printf(" -machine <type>    machine type: one of 'pc', 'q35', 'isapc' or 'microvm'.\n");
    printf(" -mach    <type>    machine type: one of 'pc', 'q35', 'isapc' or 'microvm'.\n");
    printf(" -display <type>    display type: one of 'std', 'virtio', 'qxl', 'rage128p', 'rv100', 'vnc' or 'none'. See 'qemu_mgr --help-vnc' for more info on VNC.\n");
    printf(" -prealloc <yes|no>    Preallocate memory to the vm (rather than have the vm grab memory as it needs it)\n");
    printf(" -fullscreen <yes|no>  Fullscreen graphics\n");
    printf(" -password <secret>    Password for use with VNC\n");
    printf(" -pass     <secret>    Password for use with VNC\n");
    printf(" -pw       <secret>    Password for use with VNC\n");
    printf(" -delay   <seconds>    Connect delay for VNC viewers. This can be used to prevent connecting to early and getting disconnected while the VM starts up.\n");
}


static void CommandLinePrintMediaHelp()
{
    printf("Files can be packed into and iso filesystem and mounted as a cdrom, or as a tar or zip file which can then be 'mounted' into any block device in unix\n");
    printf("ISO filesystem support requires the mkisofs utility to be available. This method has been seen to work with windows guests. You can either pick the appropriate option from the interactive menus ones a VM is running, or else use the media-add command:\n\n");
    printf("  qemu_mgr media-add MyWindowsVM ide-cd0 iso:/home/music\n\n");
    printf("This will build a .iso file with the contents of /home/music and then 'insert' it into the qemu cd-rom block device 'ide-cd0' in the vm 'MyWindowsVM'\n\n");
    printf("For unix hosts a trick can be used where any kind of file can be mounted in any qemu block device, and extracted in the guest VM. For example\n\n");
    printf("  qemu_mgr media-add MyLinuxVM fd0 tgz:/home/music\n\n");
    printf("Will use the tar utility to create a gzipped tar file and 'insert' it into floppy drive fd0. This can then be extracted in the guest with\n\n");
    printf("  tar -zcf /dev/fd0\n\n");
    printf("qemu_mgr recognizes the following prefixes to identify the kind of file to be made and mounted\n\n");
    printf("   iso:  iso9660 filesystem (requires mkisofs utility)\n");
    printf("   zip:  pkzip archive file (requires zip utility)\n");
    printf("   tar:  tar archive file (requires tar  utility)\n");
    printf("   tgz:  gzipped tar archive file (requires tar and gzip utilities)\n");
    printf("   txz:  xzipped tar archive file (requires tar and xz utilities)\n");
    printf("   7za:  7zip archive file (requires 7za utility)\n\n");
    printf("With no prefix the path is treated as a file to be mounted (which will fail if it's a directory). This allows mounting any type of file into a block device in the VM.\n");

}

static void CommandLinePrintVNCHelp()
{
    printf("The display type must be specified as '-display vnc:<host>:<display>' where 'host' is the host connections are allowed to come *from* (often '127.0.0.1' or '0.0.0.0' for 'all hosts'). The 'display' argument is the display number (equates to a port number of 5900 + display number). e.g. -display vnc:127.0.0.1:4 to run a VNC service on port 5904\n");
}


ListNode *CommandLineParse(int argc, char *argv[])
{
    const char *arg;
    CMDLINE *Cmd;
    ListNode *Actions;

    Actions=ListCreate();
    Cmd=CommandLineParserCreate(argc, argv);
    arg=CommandLineNext(Cmd);
    if ( ! StrValid(arg))
    {
        ListAddTypedItem(Actions, ACT_INTERACTIVE, "", NULL);
    }
    else if (strcasecmp(arg, "list")==0) CommandLineListCommand(Actions, Cmd);
    else if (strcasecmp(arg, "create")==0) CommandLineParseCreateCommand(Actions, Cmd);
    else if (strcasecmp(arg, "change")==0) CommandLineParseCommand(ACT_CHANGE, Actions, Cmd);
    else if (strcasecmp(arg, "add")==0) CommandLineParseCommand(ACT_ADD, Actions, Cmd);
    else if (strcasecmp(arg, "import")==0) CommandLineParseCommand(ACT_ADD, Actions, Cmd);
    else if (strcasecmp(arg, "start")==0) CommandLineParseCommand(ACT_START, Actions, Cmd);
    else if (strcasecmp(arg, "stop")==0) CommandLineParseCommand(ACT_STOP, Actions, Cmd);
    else if (strcasecmp(arg, "media-add")==0) CommandLineParseMediaAddCommand(Actions, Cmd);
    else if (strcasecmp(arg, "media-del")==0) CommandLineParseMediaRemoveCommand(Actions, Cmd);
    else if (strcasecmp(arg, "snapshot")==0) CommandLineParseSnapshotCommand(Actions, Cmd);
    else if (strcasecmp(arg, "backup")==0) CommandLineParseBackupCommand(Actions, Cmd);
    else if (strcasecmp(arg, "info")==0) CommandLineParseInfoCommand(Actions, Cmd);
    else if (strcasecmp(arg, "list-usb")==0) CommandLineListUSB();
    else if (strcasecmp(arg, "send-text")==0) CommandLineParseCommand(ACT_SEND_TEXT, Actions, Cmd);
    else if (strcasecmp(arg, "send-key")==0) CommandLineParseSendCommand(Actions, ACT_SEND_KEY, Cmd);
    else if (strcasecmp(arg, "send-string")==0) CommandLineParseSendCommand(Actions, ACT_SEND_STRING, Cmd);
    else if (strcasecmp(arg, "screenshot")==0) CommandLineParseScreenshotCommand(Actions, Cmd);
    else if (strcasecmp(arg, "list-templates")==0) CommandLineListTemplates();
    else if (strcasecmp(arg, "vnc")==0) VNCConnect(CommandLineNext(Cmd));
    else if (strcasecmp(arg, "-i")==0)
    {
        arg=CommandLineNext(Cmd);
        ListAddTypedItem(Actions, ACT_INTERACTIVE, arg, NULL);
    }
    else if (strcasecmp(arg, "-?")==0) CommandLinePrintHelp();
    else if (strcasecmp(arg, "-h")==0) CommandLinePrintHelp();
    else if (strcasecmp(arg, "-help")==0) CommandLinePrintHelp();
    else if (strcasecmp(arg, "--help")==0) CommandLinePrintHelp();
    else if (strcasecmp(arg, "--help-vnc")==0) CommandLinePrintVNCHelp();
    else if (strcasecmp(arg, "--help-media")==0) CommandLinePrintMediaHelp();

    return(Actions);
}


