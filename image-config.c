#include "image-config.h"

char *ImageConfigExpand(char *RetStr, ListNode *ConfTree)
{
    ListNode *Values, *Curr;

    Values=ParserOpenItem(ConfTree, "/");
    Curr=ListGetNext(Values);
    while (Curr)
    {
        RetStr=MCatStr(RetStr, Curr->Tag, "='", Curr->Item, "' ", NULL);
        Curr=ListGetNext(Curr);
    }

    return(RetStr);
}



void ImageConfigUpdate(ListNode *Config, const char *NewSettings)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    ListNode *Values, *Node;

    Values=ParserOpenItem(Config, "/");

    ptr=GetNameValuePair(NewSettings, " ", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "mem")==0) Name=CopyStr(Name, "memory");
        if (strcasecmp(Name, "img")==0) Name=CopyStr(Name, "image");

        if (StrValid(Name))
        {
            Node=ListFindNamedItem(Values, Name);
            //SetVar seems to have some issue with always adding values
            //to a parser list
            if (Node) Node->Item=CopyStr(Node->Item, Value);
            else SetVar(Values, Name, Value);
        }
        ptr=GetNameValuePair(ptr, " ", "=", &Name, &Value);
    }


    Destroy(Name);
    Destroy(Value);
}



STREAM *ImageConfigOpen(const char *ImageName, const char *Mode)
{
    char *Path=NULL;
    STREAM *S;

    Path=MCopyStr(Path, GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".qemu_mgr", NULL);
    MakeDirPath(Path, 0700);
    S=STREAMOpen(Path, Mode);

    Destroy(Path);
    return(S);
}


ListNode *ImageConfigLoad(const char *ImageName)
{
    char *Tempstr=NULL;
    ListNode *Config=NULL;
    STREAM *S;

    S=ImageConfigOpen(ImageName, "r");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        Config=ParserParseDocument("json", Tempstr);
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(Config);
}


void ImageConfigSave(STREAM *S, const char *ImageName, const char *Config)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL, *Token=NULL;
    const char *ptr, *tptr;

    Tempstr=MCopyStr(Tempstr, "{\n'name': '", ImageName, "',\n", NULL);
    STREAMWriteLine(Tempstr, S);

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "name")==0)
        {
            //do nothing, we have already added this
        }
        else if (strcasecmp(Name, "updated")==0)
        {
            //do nothing, we have already added this
        }
        else if ( (strcasecmp(Name, "img")==0) || (strcasecmp(Name, "image")==0) )
        {
            Tempstr=MCopyStr(Tempstr, "'image': '", Value, "',\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }
        else if ( (strcasecmp(Name, "mem")==0) || (strcasecmp(Name, "memory")==0) )
        {
            Tempstr=MCopyStr(Tempstr, "'memory': '", Value, "',\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }
        else if (strcasecmp(Name, "display")==0)
        {
            Tempstr=MCopyStr(Tempstr, "'display': '", Value, "',\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }
        else if (strcasecmp(Name, "password")==0)
        {
            Tempstr=MCopyStr(Tempstr, "'password': '", Value, "',\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }
        else if (strcasecmp(Name, "jail")==0)
        {
            tptr=GetToken(Value, "@", &Token, 0);
            Tempstr=MCopyStr(Tempstr, "'user': '", Token, "',\n", NULL);
            Tempstr=MCopyStr(Tempstr, "'chroot': '", tptr, "',\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }
        else if (StrValid(Value))
        {
            Tempstr=MCopyStr(Tempstr, "'", Name, "': '", Value, "',\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Tempstr=MCopyStr(Tempstr, "'updated': '", GetDateStr("%Y/%m/%dT%H:%M:%S", NULL), "'\n", NULL);
    STREAMWriteLine(Tempstr, S);

    STREAMWriteLine("}\n", S);

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
}

