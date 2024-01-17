#include "sound-devices.h"
#include <glob.h>



char *SoundDevicesLoadALSAHardware(char *RetStr)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    STREAM *S;

    RetStr=MCatStr(RetStr, ",alsa:default", NULL);
    S=STREAMOpen("/proc/asound/cards", "r");
    Tempstr=STREAMReadLine(Tempstr, S);
    while (Tempstr)
    {
        StripLeadingWhitespace(Tempstr);
        StripTrailingWhitespace(Tempstr);
        ptr=GetToken(Tempstr, " ", &Token, 0);
        RetStr=MCatStr(RetStr, ",alsa:hw:", Token, ".0 ", NULL);
        ptr=GetToken(ptr, ":", &Token, 0);
        while (isspace(*ptr)) ptr++;
        RetStr=CatStr(RetStr, ptr);

//throw away next line
        Tempstr=STREAMReadLine(Tempstr, S);

        Tempstr=STREAMReadLine(Tempstr, S);
    }
    STREAMClose(S);

    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}


char *SoundDevicesLoadALSAConfigFile(char *RetStr, const char *ConfigPath)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    STREAM *S;

    S=STREAMOpen(ConfigPath, "r");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);

        ptr=GetToken(Tempstr, "\\S|{|}", &Token, GETTOKEN_QUOTES | GETTOKEN_MULTI_SEP);
        while (ptr)
        {
            if (
                (strncmp(Token, "pcm.", 4)==0) &&
                (strcmp(Token, "pcm.!default") !=0)
            )
            {
                RetStr=MCatStr(RetStr, ",alsa:", Token+4, NULL);
            }
            ptr=GetToken(ptr, "\\S|{|}", &Token, GETTOKEN_QUOTES | GETTOKEN_MULTI_SEP);
        }

        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}


char *SoundDevicesLoadOSS(char *RetStr)
{
    glob_t Glob;
    int i;


    glob("/dev/dsp*", 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        RetStr=MCatStr(RetStr, ",oss:", Glob.gl_pathv[i], NULL);
    }


    globfree(&Glob);
    return(RetStr);
}


char *SoundDevicesLoadPULSE(char *RetStr)
{
    char *Dir=NULL, *Tempstr=NULL;
    const char *SearchDirs="/var/run:/run";
    const char *ptr;

    if (StrValid(getenv("PULSE_SERVER"))) return(CatStr(RetStr, ",pulseaudio"));

    ptr=GetToken(SearchDirs, ":", &Dir, 0);
    while (ptr)
    {
        Tempstr=FormatStr(Tempstr, "%s/%d/pulse/native", Dir, getuid());
        if (access(Tempstr, F_OK)==0)
        {
            RetStr=CatStr(RetStr, ",pulseaudio");
            break;
        }
        ptr=GetToken(ptr, ":", &Dir, 0);
    }

            RetStr=CatStr(RetStr, ",pulseaudio");
    Destroy(Tempstr);
    Destroy(Dir);
    return(RetStr);
}


char *SoundDevicesLoad(char *RetStr)
{
    RetStr=CopyStr(RetStr, "default");
    RetStr=SoundDevicesLoadOSS(RetStr);
    RetStr=SoundDevicesLoadALSAHardware(RetStr);
    RetStr=SoundDevicesLoadALSAConfigFile(RetStr, "/etc/asound.conf");
    RetStr=SoundDevicesLoadPULSE(RetStr);

    return(RetStr);
}
