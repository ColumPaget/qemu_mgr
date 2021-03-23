#include "vnc.h"
#include "qmp.h"


char *VNCBuildViewerList(char *RetStr)
{
    const char *Viewers="vncviewer,tightvncviewer,tigervncviewer";
    char *Token=NULL, *Tempstr=NULL;
    const char *ptr;

    ptr=GetToken(Viewers, ",", &Token, 0);
    while (ptr)
    {
        Tempstr=FindFileInPath(Tempstr, Token, getenv("PATH"));
        if (StrValid(Tempstr))
        {
            if (! StrValid(RetStr)) RetStr=MCopyStr(RetStr, "none,", Tempstr, NULL);
            else RetStr=MCopyStr(RetStr, ",", Tempstr, NULL);
        }
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}


int VNCSetPassword(const char *ImageName, const char *Password)
{
    ListNode *Qmp;
    char *Tempstr=NULL;

    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"change-vnc-password\", \"arguments\": { \"password\": \"", Password, "\" } }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);
    if (QMPIsError(Qmp))
    {
        //try again with deprecated 'change' command
        Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"change\", \"arguments\": { \"device\": \"vnc\", \"target\": \"password\", \"arg\": \"", Password, "\" } }\n", NULL);
        Qmp=QMPTransact(ImageName, Tempstr);
    }

    Destroy(Tempstr);
    return(TRUE);
}



char *VNCGetInfo(char *RetStr, const char *ImageName)
{
    ListNode *Qmp;
    char *Tempstr=NULL;
    const char *ptr;
    ListNode *Response;

    RetStr=CopyStr(RetStr, "");
    Tempstr=MCopyStr(Tempstr, "{ \"execute\": \"query-vnc\" }\n", NULL);
    Qmp=QMPTransact(ImageName, Tempstr);

    if (! QMPIsError(Qmp))
    {
        Response=ParserOpenItem(Qmp, "/return");
        RetStr=MCopyStr(RetStr, ParserGetValue(Response, "family"), ":", NULL);
        RetStr=MCatStr(RetStr, ParserGetValue(Response, "host"), ":", NULL);

        ptr=ParserGetValue(Response, "service");
        Tempstr=FormatStr(Tempstr, "%d", atoi(ptr) - 5900);
        RetStr=CatStr(RetStr, Tempstr);
    }

    ParserItemsDestroy(Qmp);

    Destroy(Tempstr);
    return(RetStr);
}



void VNCLaunchViewer(const char *ViewerList, ListNode *Config)
{
    char *Tempstr=NULL, *Token=NULL, *Viewer=NULL;
    const char *ptr="";

    ptr=GetToken(ViewerList, ",", &Token, 0);
    while (ptr)
    {
        Tempstr=FindFileInPath(Tempstr, Token, getenv("PATH"));
        printf("FIP: [%s] [%s]\n", Token, Tempstr);
        if (StrValid(Tempstr)) Viewer=CopyStr(Viewer, Token);
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    if (Config) ptr=ParserGetValue(Config, "vncviewer-delay");
    if (fork() == 0)
    {
        if (StrValid(ptr)) sleep(atoi(ptr));

        if (Config) ptr=ParserGetValue(Config, "vnc-endpoint");
        if (strncmp(ptr, "ipv4:", 5)==0) ptr+=5;
        if (strncmp(ptr, "ip4:", 4)==0) ptr+=4;
        if (strncmp(ptr, "tcp:", 4)==0) ptr+=4;
        Tempstr=MCopyStr(Tempstr, Viewer, " ", ptr, NULL);
        Spawn(Tempstr, "setsid");
        _exit(0);
    }

    Destroy(Tempstr);
    Destroy(Viewer);
    Destroy(Token);
}


void VNCConnect(const char *ImageName)
{
    char *Tempstr=NULL;
    ListNode *Vars;

    Tempstr=VNCGetInfo(Tempstr, ImageName);
    Vars=ListCreate();
    SetVar(Vars, "vnc-endpoint", Tempstr);
    Tempstr=VNCBuildViewerList(Tempstr);
    VNCLaunchViewer(Tempstr, Vars);

    Destroy(Tempstr);
    ListDestroy(Vars, Destroy);
}
