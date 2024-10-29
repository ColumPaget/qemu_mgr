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


static ListNode *CommandLineParseImageArgs(char *Input, CMDLINE *Cmd)
{
    const char *arg, *ptr;
    char *Tempstr=NULL;
    ListNode *Conf;

    Conf=ListCreate();
    ImageConfigUpdate(Conf, Input);
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
        else if (strcmp(arg, "-su")==0) Config->Flags |= CONF_USE_SU;
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


    Destroy(Tempstr);

    return(Conf);
}

static int CommandLineValidateCommand(int CmdType, ListNode *Config)
{
switch (CmdType)
{
case ACT_CREATE:
if (! StrValid(GetVar(Config, "iso"))) printf("ERROR: No path to iso file given. Use '-iso' option to supply one\n");
return(FALSE);
break;

case ACT_ADD:
if (! StrValid(GetVar(Config, "image"))) printf("ERROR: No image path given. Use '-img' option to supply one\n");
return(FALSE);
break;
}

return(TRUE);
}


static void CommandLineParseCreateCommand(ListNode *Actions, CMDLINE *Cmd)
{
    char *Name=NULL, *Config=NULL;
    ListNode *Vars;
    const char *arg;

    Name=CopyStr(Name, CommandLineNext(Cmd));

    Config=ConfigTemplateLoad(Config, "Generic");
    Vars=CommandLineParseImageArgs(Config, Cmd);

    Config=CopyStr(Config, "");
    Config=ImageConfigExpand(Config, Vars);

		if (CommandLineValidateCommand(ACT_CREATE, Vars)) ListAddTypedItem(Actions, ACT_CREATE, Name, CopyStr(NULL, Config));
    else ListAddTypedItem(Actions, ACT_FAIL, Name, NULL);

    ListDestroy(Vars, Destroy);
    Destroy(Name);
    Destroy(Config);
}


static void CommandLineParseCommand(int CmdType, ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;
    ListNode *Vars;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    Vars=CommandLineParseImageArgs(Config, Cmd);

    Config=CopyStr(Config, "");
    Config=ImageConfigExpand(Config, Vars);

		if (CommandLineValidateCommand(CmdType, Vars)) ListAddTypedItem(Actions, CmdType, Name, CopyStr(NULL, Config));
    else ListAddTypedItem(Actions, ACT_FAIL, Name, NULL);

    ListDestroy(Vars, Destroy);
    Destroy(Name);
    Destroy(Config);
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
    ) ListAddTypedItem(Actions, ACT_MOUNT, Name, CopyStr(NULL, Config));
    else ListAddTypedItem(Actions, ACT_MEDIA_ADD, Name, CopyStr(NULL, Config));

    Destroy(Name);
    Destroy(Config);
}

static void CommandLineParseMediaRemoveCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "dev=", arg, " ", NULL);

    ListAddTypedItem(Actions, ACT_MEDIA_REMOVE, Name, CopyStr(NULL, Config));

    Destroy(Name);
		Destroy(Config);
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

    ListAddTypedItem(Actions, ACT_SNAPSHOT, Name, CopyStr(NULL, Config));

    Destroy(Name);
		Destroy(Config);
}



static void CommandLineParseSendCommand(ListNode *Actions, int SendType, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "value=", arg, " ", NULL);

    ListAddTypedItem(Actions, SendType, Name, CopyStr(NULL, Config));

    Destroy(Name);
		Destroy(Config);
}


static void CommandLineParseScreenshotCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "file=", arg, " ", NULL);

    ListAddTypedItem(Actions, ACT_SCREENSHOT, Name, CopyStr(NULL, Config));

    Destroy(Name);
		Destroy(Config);
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

    ListAddTypedItem(Actions, ACT_DRIVE_BACKUP, Name, CopyStr(NULL, Config));

    Destroy(Name);
		Destroy(Config);
}


static void CommandLineParseInfoCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    ListAddTypedItem(Actions, ACT_INFO, Name, CopyStr(NULL, Config));

    Destroy(Name);
		Destroy(Config);
}




ListNode *CommandLineParse(int argc, char *argv[])
{
    const char *arg;
    CMDLINE *Cmd;
    ListNode *Actions;

    Actions=ListCreate();
    Cmd=CommandLineParserCreate(argc, argv);
    arg=CommandLineNext(Cmd);
    if (StrValid(arg))
    {
        if (strcasecmp(arg, "list")==0) CommandLineListCommand(Actions, Cmd);
        else if (strcasecmp(arg, "create")==0) CommandLineParseCreateCommand(Actions, Cmd);
        else if (strcasecmp(arg, "change")==0) CommandLineParseCommand(ACT_CHANGE, Actions, Cmd);
        else if (strcasecmp(arg, "add")==0) CommandLineParseCommand(ACT_ADD, Actions, Cmd);
        else if (strcasecmp(arg, "del")==0) CommandLineParseCommand(ACT_DELETE, Actions, Cmd);
        else if (strcasecmp(arg, "delete")==0) CommandLineParseCommand(ACT_DELETE, Actions, Cmd);
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
        else if (strcasecmp(arg, "-su")==0) Config->Flags |= CONF_USE_SU;
        else if (strcasecmp(arg, "-?")==0) CommandLineParseCommand(ACT_HELP, Actions, Cmd);
        else if (strcasecmp(arg, "-h")==0) CommandLineParseCommand(ACT_HELP, Actions, Cmd);
        else if (strcasecmp(arg, "-help")==0) CommandLineParseCommand(ACT_HELP, Actions, Cmd);
        else if (strcasecmp(arg, "--help")==0) CommandLineParseCommand(ACT_HELP, Actions, Cmd);
        else if (strcasecmp(arg, "--help-vnc")==0) CommandLineParseCommand(ACT_HELP_VNC, Actions, Cmd);
        else if (strcasecmp(arg, "--help-media")==0) CommandLineParseCommand(ACT_HELP_MEDIA, Actions, Cmd);
        else if (strcasecmp(arg, "--version")==0) CommandLineParseCommand(ACT_VERSION, Actions, Cmd);
        else if (strcasecmp(arg, "--v")==0) CommandLineParseCommand(ACT_VERSION, Actions, Cmd);
        else 
				{
					printf("ERROR: unknown command '%s'\n", arg);
    			ListAddTypedItem(Actions, ACT_FAIL, "", NULL);
				}
    }

    if (ListSize(Actions)==0) ListAddTypedItem(Actions, ACT_INTERACTIVE, "", NULL);

    return(Actions);
}


