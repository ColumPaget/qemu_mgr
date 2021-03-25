#include "config-templates.h"
#include "image-config.h"



static void ConfigTemplatesInit(const char *Path)
{
    STREAM *S;

    S=STREAMOpen(Path, "w");
    if (S)
    {
        STREAMWriteLine("'Generic' size=40G memory=2047 machine=pc display=std\n", S);
        STREAMWriteLine("'Win2000' size=40G memory=2047 machine=pc display=std flags='-win2k-hack'\n", S);
        STREAMWriteLine("'Modern' size=40G memory=2047 machine=q35 display=virtio disk-controller=virtio guest-audio=ac97\n", S);
        STREAMWriteLine("'microvm' size=40G memory=1024 machine=microvm display=virtio disk-controller=virtio guest-audio=ac97\n", S);
        STREAMClose(S);
    }

}


char *ConfigTemplatesGetList(char *RetStr)
{
    char *Tempstr=NULL, *Token=NULL;
    STREAM *S;

    RetStr=CopyStr(RetStr, "");
    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/vmtemplates.conf", NULL);
    if (access(Tempstr, F_OK) != 0) ConfigTemplatesInit(Tempstr);

    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            GetToken(Tempstr, "\\S", &Token, GETTOKEN_QUOTES);
            RetStr=MCatStr(RetStr, Token, ",", NULL);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}



char *ConfigTemplateLoad(char *RetStr, const char *Name)
{
    STREAM *S;
    ListNode *Config, *Curr;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    RetStr=CopyStr(RetStr, "");
    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/vmtemplates.conf", NULL);
    S=STREAMOpen(Tempstr, "r");
    if (! S) ConfigTemplatesInit(Tempstr);

    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripLeadingWhitespace(Tempstr);
            StripTrailingWhitespace(Tempstr);
            ptr=GetToken(Tempstr, "\\S", &Token, GETTOKEN_QUOTES);
            if (strcasecmp(Token, Name)==0)
            {
                RetStr=CopyStr(RetStr, ptr);
                break;
            }
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}



