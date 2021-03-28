#include "mount.h"
#include "os-commands.h"
#include "images.h"
#include <glob.h>


int MountTypeIsArchive(const char *Mount)
{
char *Token=NULL;
int RetVal=FALSE;

GetToken(Mount, ":", &Token, 0);
if (strcasecmp(Token, "file")==0) RetVal=TRUE;
if (strcasecmp(Token, "iso")==0) RetVal=TRUE;
if (strcasecmp(Token, "zip")==0) RetVal=TRUE;
if (strcasecmp(Token, "7za")==0) RetVal=TRUE;
if (strcasecmp(Token, "tar")==0) RetVal=TRUE;
if (strcasecmp(Token, "tgz")==0) RetVal=TRUE;
if (strcasecmp(Token, "txz")==0) RetVal=TRUE;

Destroy(Token);
return(RetVal);
}



static void MountPath(const char *ImageName, const char *MountDevice, const char *Path)
{
    char *Tempstr=NULL, *Quoted=NULL, *FSPath=NULL;
		char *Format=NULL;
    const char *ptr;


		Format=CopyStr(Format, "");
    Quoted=QuoteCharsInStr(Quoted, Path, "\"' ;&");
		ptr=strchr(Path, ':');
		if (ptr) 
		{
			Format=CopyStrLen(Format, Path, ptr-Path);
			Quoted=QuoteCharsInStr(Quoted, ptr+1, "\"' ;&");
		}
    else Quoted=QuoteCharsInStr(Quoted, Path, "\"' ;&");

    if (strcmp(Format, "iso")==0)
    {
        ptr=OSCommandFindPath("mkisofs");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Quoted), ".iso", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -r -J -o '", FSPath, "' ", Quoted, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "tar")==0)
    {
        ptr=OSCommandFindPath("tar");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Quoted), ".tar", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -cf '", FSPath, "' ", Quoted, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "tgz")==0)
    {
        ptr=OSCommandFindPath("tar");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Quoted), ".tgz", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -zcf '", FSPath, "' ", Quoted, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "txz")==0)
    {
        ptr=OSCommandFindPath("tar");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Quoted), ".txz", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -Jcf '", FSPath, "' ", Quoted, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "zip")==0)
    {
        ptr=OSCommandFindPath("zip");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Quoted), ".tgz", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " -r '", FSPath, "' ", Quoted, NULL);
            system(Tempstr);
        }
    }
    else if (strcmp(Format, "7za")==0)
    {
        ptr=OSCommandFindPath("7za");
        if (StrValid(ptr))
        {
            FSPath=MCopyStr(FSPath, "/tmp/", GetBasename(Quoted), ".7z", NULL);
            Tempstr=MCopyStr(Tempstr, ptr, " a -r '", FSPath, "' ", Quoted, NULL);
            system(Tempstr);
        }
    }
		else FSPath=CopyStr(FSPath, Quoted);


    if (StrValid(FSPath))
    {
        Tempstr=MCopyStr(Tempstr, "dev=", MountDevice, " media='", FSPath, "'", NULL);
        ImageMediaRemove(ImageName, Tempstr);
        ImageMediaAdd(ImageName, Tempstr);
        printf("MOUNT: %s\n", Tempstr);
    }

    Destroy(FSPath);
    Destroy(Format);
    Destroy(Tempstr);
    Destroy(Quoted);
}


void MountItem(const char *ImageName, const char *Config)
{
    char *Name=NULL, *Value=NULL;
    char *Path=NULL, *Dev=NULL;
    const char *ptr;

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcmp(Name, "dev")==0) Dev=CopyStr(Dev, Value);
        if (strcmp(Name, "media")==0) Path=CopyStr(Path, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    MountPath(ImageName, Dev, Path);

    Destroy(Name);
    Destroy(Value);
    Destroy(Path);
    Destroy(Dev);
}



char *MountFindSourceTypes(char *RetStr)
{
glob_t Glob;
const char *ptr;
int i;

RetStr=MCopyStr(RetStr, "file - filesystem image or other file,", NULL);

glob("/dev/sr[0-9]", 0, 0, &Glob);
for (i=0; i < Glob.gl_pathc; i++)
{
	RetStr=MCatStr(RetStr, Glob.gl_pathv[i], " - optical drive,", NULL);
}
globfree(&Glob);

ptr=OSCommandFindPath("mkisofs");
if (StrValid(ptr)) RetStr=CatStr(RetStr, "mkisofs - create iso9660 filesystem image,");
ptr=OSCommandFindPath("zip");
if (StrValid(ptr)) RetStr=CatStr(RetStr, "zip - create pkzip archive,");
ptr=OSCommandFindPath("7za");
if (StrValid(ptr)) RetStr=CatStr(RetStr, "7za - create 7z archive,");
ptr=OSCommandFindPath("tar");
if (StrValid(ptr))
{
RetStr=CatStr(RetStr, "tar - create uncompressed tar archive,");
ptr=OSCommandFindPath("gzip");
if (StrValid(ptr)) RetStr=CatStr(RetStr, "tgz - create gzipped tar archive,");
ptr=OSCommandFindPath("xz");
if (StrValid(ptr)) RetStr=CatStr(RetStr, "txz - create xzipped tar archive,");
}

return (RetStr);
}
