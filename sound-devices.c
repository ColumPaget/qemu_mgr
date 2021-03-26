#include "sound-devices.h"
#include <glob.h>

char *FindSoundDevices(char *RetStr)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    glob_t Glob;
    int i;
    STREAM *S;

    RetStr=CopyStr(RetStr, "default");

    glob("/dev/dsp*", 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        RetStr=MCatStr(RetStr, ",oss:", Glob.gl_pathv[i], NULL);
    }

    S=STREAMOpen("/proc/asound/cards", "r");
    Tempstr=STREAMReadLine(Tempstr, S);
    while (Tempstr)
    {
        StripLeadingWhitespace(Tempstr);
        StripTrailingWhitespace(Tempstr);
        ptr=GetToken(Tempstr, " ", &Token, 0);
        RetStr=MCatStr(RetStr, ",alsa:", Token, ":", NULL);
        ptr=GetToken(ptr, ":", &Token, 0);
        while (isspace(*ptr)) ptr++;
        RetStr=CatStr(RetStr, ptr);

//throw away next line
        Tempstr=STREAMReadLine(Tempstr, S);

        Tempstr=STREAMReadLine(Tempstr, S);
    }
    STREAMClose(S);

    globfree(&Glob);
    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}
