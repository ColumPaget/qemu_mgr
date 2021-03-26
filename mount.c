#include "mount.h"
#include "os-commands.h"
#include "images.h"

void MountDirectory(const char *ImageName, const char *MountDevice, const char *Dir, const char *Format)
{
    char *Tempstr=NULL, *QuotedDir=NULL, *FSPath=NULL;
    const char *ptr;

    QuotedDir=QuoteCharsInStr(QuotedDir, Dir, "\"' ;&");

    if (strcmp(Format, "iso")==0)
    {
        ptr=OSCommandFindPath("mkisofs");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Dir), ".iso", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -r -J -o '", FSPath, "' ", QuotedDir, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "tgz")==0)
    {
        ptr=OSCommandFindPath("tar");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Dir), ".tgz", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -zcf '", FSPath, "' ", QuotedDir, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "zip")==0)
    {
        ptr=OSCommandFindPath("zip");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Dir), ".tgz", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -r '", FSPath, "' ", QuotedDir, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "7za")==0)
    {
        ptr=OSCommandFindPath("7za");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Dir), ".7z", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " a -r '", FSPath, "' ", QuotedDir, NULL);
            system(Tempstr);
        }
    }



    if (StrValid(FSPath))
    {
        Tempstr=MCopyStr(Tempstr, "dev=", MountDevice, " media='", FSPath, "'", NULL);
        ImageMediaAdd(ImageName, Tempstr);
        printf("MOUNT: %s\n", Tempstr);
    }

    Destroy(FSPath);
    Destroy(Tempstr);
    Destroy(QuotedDir);
}


void MountItem(const char *ImageName, const char *Config)
{
    char *Name=NULL, *Value=NULL;
    char *Dir=NULL, *Dev=NULL, *Format=NULL;
    const char *ptr, *tptr;

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcmp(Name, "dev")==0) Dev=CopyStr(Dev, Value);
        if (strcmp(Name, "media")==0)
        {
            tptr=GetToken(Value, ":", &Format, 0);
            Dir=CopyStr(Dir, tptr);
        }
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    MountDirectory(ImageName, Dev, Dir, Format);

    Destroy(Name);
    Destroy(Value);
    Destroy(Format);
    Destroy(Dir);
    Destroy(Dev);
}
