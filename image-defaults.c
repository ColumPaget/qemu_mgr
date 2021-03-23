#include "image-defaults.h"
#include "image-config.h"

static void ConfigTemplatesInit(const char *Path)
{
    STREAM *S;

    S=STREAMOpen(Path, "w");
    if (S)
    {
        ImageConfigSave(S, "VM Image Defaults", "size=40G memory=2047");
        STREAMClose(S);
    }

}

char *ConfigTemplatesLoad(char *RetStr)
{
    STREAM *S;
    ListNode *Config, *Curr;
    char *Tempstr=NULL;

    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.qemu_mgr/image.defaults", NULL);
    S=STREAMOpen(Tempstr, "r");
    printf("ID: %d %s\n", S, Tempstr);
    if (! S) ConfigTemplatesInit(Tempstr);

    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        Config=ParserParseDocument("json", Tempstr);
        Curr=ListGetNext(Config);
        while (Curr)
        {
            RetStr=MCatStr(RetStr, " ", Curr->Tag, "='", (const char *) Curr->Item, "' ", NULL);
            Curr=ListGetNext(Curr);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    return(RetStr);
}
