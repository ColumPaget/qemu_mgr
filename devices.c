#include "devices.h"
#include <glob.h>



char *ReadSysFSFile(char *RetStr, const char *Dir, const char *File)
{
    char *Path=NULL;
    STREAM *S;

    RetStr=CopyStr(RetStr, "");
    Path=MCopyStr(Path, Dir, "/", File, NULL);
    S=STREAMOpen(Path, "r");
    if (S)
    {
        RetStr=STREAMReadLine(RetStr, S);
        StripTrailingWhitespace(RetStr);
        STREAMClose(S);
    }

    Destroy(Path);

    return(RetStr);
}


void DeviceInfoDestroy(void *p_Info)
{
    TDevice *Dev;

    Dev=(TDevice *) p_Info;
    Destroy(Dev->Manufacturer);
    Destroy(Dev->Product);
    Destroy(Dev->Serial);
    free(Dev);
}


int DevicesGetUSB(ListNode *List)
{
    char *Tempstr=NULL;
    const char *p_Dir;
    TDevice *Dev;
    glob_t Glob;
    int i;

    glob("/sys/bus/usb/devices/[0-9]*", 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        p_Dir=Glob.gl_pathv[i];
        Dev=(TDevice *) calloc(1, sizeof(TDevice));
        Tempstr=ReadSysFSFile(Tempstr, p_Dir, "busnum");
        Dev->BusNum=atoi(Tempstr);
        Tempstr=ReadSysFSFile(Tempstr, p_Dir, "devnum");
        Dev->DevNum=atoi(Tempstr);
        Tempstr=ReadSysFSFile(Tempstr, p_Dir, "speed");
        Dev->Speed=atoi(Tempstr);
        Tempstr=ReadSysFSFile(Tempstr, p_Dir, "bMaxPower");
        Dev->MaxPower=atoi(Tempstr);
        Dev->Manufacturer=ReadSysFSFile(Dev->Manufacturer, p_Dir, "manufacturer");
        Dev->Product=ReadSysFSFile(Dev->Product, p_Dir, "product");
        Dev->Serial=ReadSysFSFile(Dev->Serial, p_Dir, "serial");

        ListAddItem(List, Dev);
    }

    Destroy(Tempstr);

    return(i);
}

