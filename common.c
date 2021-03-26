#include "common.h"

TConfig *Config=NULL;


char *GetNamedValue(char *RetStr, const char *Config, const char *Requested)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcmp(Name, Requested)==0) RetStr=CopyStr(RetStr, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);
    return(RetStr);
}
