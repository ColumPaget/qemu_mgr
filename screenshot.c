#include "screenshot.h"
#include "qmp.h"

int ImageScreenshot(const char *ImageName, const char *Options)
{
    ListNode *Qmp;
    char *Name=NULL, *Value=NULL, *Path=NULL;
    const char *ptr;

    ptr=GetNameValuePair(Options, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "file")==0) Path=CopyStr(Path, Value);
        if (strcasecmp(Name, "filename")==0) Path=CopyStr(Path, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    if (! StrValid(Path)) Path=FormatStr(Path, "%s-%s.pmm", ImageName, GetDateStr("%Y%m%d.%H:%M:%S", NULL));
    Value=MCopyStr(Value, "{\"execute\": \"screendump\", \"arguments\": {\"filename\": \"", Path, "\"}}\n");
    Qmp=QMPTransact(ImageName, Value);


    ParserItemsDestroy(Qmp);
    Destroy(Name);
    Destroy(Value);
    Destroy(Path);
    return(TRUE);
}

