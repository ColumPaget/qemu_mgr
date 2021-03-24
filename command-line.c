#include "command-line.h"
#include "config-templates.h"
#include "actions.h"
#include "devices.h"
#include "vnc.h"


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


static void CommandLineParseListCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;

    arg=CommandLineNext(Cmd);
    if (StrValid(arg))
    {
        if (strcasecmp(arg, "images")==0) ListAddTypedItem(Actions, ACT_LIST_IMAGES, "list-images", NULL);
        else if (strcasecmp(arg, "running")==0) ListAddTypedItem(Actions, ACT_LIST_IMAGES, "list-running", NULL);
        else ListAddTypedItem(Actions, ACT_LIST_IMAGES, "list-all", NULL);
    }
    else ListAddTypedItem(Actions, ACT_LIST_IMAGES, "list-all", NULL);
}


static char *CommandLineParseImageArgs(char *RetStr, CMDLINE *Cmd)
{
    const char *arg;
		char *Tempstr=NULL;

    arg=CommandLineNext(Cmd);
    while (arg)
    {

        if (strcmp(arg, "-arch")==0) RetStr=MCatStr(RetStr, "arch=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-size")==0) RetStr=MCatStr(RetStr, "size=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-img")==0) RetStr=MCatStr(RetStr, "img=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-iso")==0) RetStr=MCatStr(RetStr, "iso=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-mem")==0) RetStr=MCatStr(RetStr, "mem=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-pw")==0) RetStr=MCatStr(RetStr, "password=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-pass")==0) RetStr=MCatStr(RetStr, "password=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-password")==0) RetStr=MCatStr(RetStr, "password=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-net")==0) RetStr=MCatStr(RetStr, "network=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-user")==0) RetStr=MCatStr(RetStr, "user=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-chroot")==0) RetStr=MCatStr(RetStr, "chroot=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-jail")==0) RetStr=MCatStr(RetStr, "jail=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-display")==0) RetStr=MCatStr(RetStr, "display=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-portfwd")==0) RetStr=MCatStr(RetStr, "portfwd=", CommandLineNext(Cmd), " ", NULL);
        else if (strcmp(arg, "-desktop")==0) RetStr=MCatStr(RetStr, "desktop_file=y ", NULL);
        else if (strcmp(arg, "-template")==0) 
				{
					Tempstr=ConfigTemplateLoad(Tempstr, CommandLineNext(Cmd));
					RetStr=MCatStr(RetStr, Tempstr, " ", NULL);
				}
        else RetStr=MCatStr(RetStr, arg, " ", NULL);

        arg=CommandLineNext(Cmd);
    }

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
    printf("MAC: %s %s\n", Name, arg);
    Config=MCatStr(Config, "dev='", arg, "' ", NULL);
    arg=CommandLineNext(Cmd);
    printf("DEV: %s %s\n", Name, arg);
    Config=MCatStr(Config, "media='", arg, "' ", NULL);

    if (
        (strncmp(arg, "tar:", 4)==0) ||
        (strncmp(arg, "tgz:", 4)==0) ||
        (strncmp(arg, "txz:", 4)==0) ||
        (strncmp(arg, "zip:", 4)==0) ||
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



static void CommandLineParseSendKeyCommand(ListNode *Actions, CMDLINE *Cmd)
{
    const char *arg;
    char *Name=NULL, *Config=NULL;

    Name=CopyStr(Name, CommandLineNext(Cmd));
    arg=CommandLineNext(Cmd);
    Config=MCatStr(Config, "key=", arg, " ", NULL);

    ListAddTypedItem(Actions, ACT_SEND_KEY, Name, Config);

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
    else if (strcasecmp(arg, "list")==0) CommandLineParseListCommand(Actions, Cmd);
    else if (strcasecmp(arg, "create")==0) CommandLineParseCreateCommand(Actions, Cmd);
    else if (strcasecmp(arg, "change")==0) CommandLineParseCommand(ACT_CHANGE, Actions, Cmd);
    else if (strcasecmp(arg, "add")==0) CommandLineParseCommand(ACT_ADD, Actions, Cmd);
    else if (strcasecmp(arg, "import")==0) CommandLineParseCommand(ACT_ADD, Actions, Cmd);
    else if (strcasecmp(arg, "start")==0) CommandLineParseCommand(ACT_START, Actions, Cmd);
    else if (strcasecmp(arg, "stop")==0) CommandLineParseCommand(ACT_STOP, Actions, Cmd);
    else if (strcasecmp(arg, "media-add")==0) CommandLineParseMediaAddCommand(Actions, Cmd);
    else if (strcasecmp(arg, "media-del")==0) CommandLineParseMediaRemoveCommand(Actions, Cmd);
    else if (strcasecmp(arg, "media-remove")==0) CommandLineParseMediaRemoveCommand(Actions, Cmd);
    else if (strcasecmp(arg, "snapshot")==0) CommandLineParseSnapshotCommand(Actions, Cmd);
    else if (strcasecmp(arg, "backup")==0) CommandLineParseBackupCommand(Actions, Cmd);
    else if (strcasecmp(arg, "info")==0) CommandLineParseInfoCommand(Actions, Cmd);
    else if (strcasecmp(arg, "list-usb")==0) CommandLineListUSB();
    else if (strcasecmp(arg, "send-text")==0) CommandLineParseCommand(ACT_SEND_TEXT, Actions, Cmd);
    else if (strcasecmp(arg, "send-key")==0) CommandLineParseSendKeyCommand(Actions, Cmd);
    else if (strcasecmp(arg, "screenshot")==0) CommandLineParseScreenshotCommand(Actions, Cmd);
    else if (strcasecmp(arg, "list-templates")==0) CommandLineListTemplates();
    else if (strcasecmp(arg, "vnc")==0) VNCConnect(CommandLineNext(Cmd));
    else if (strcasecmp(arg, "-i")==0)
    {
        arg=CommandLineNext(Cmd);
        printf("arg: %s\n", arg);
        ListAddTypedItem(Actions, ACT_INTERACTIVE, arg, NULL);
    }

    return(Actions);
}


