#include "qmp.h"



/*
          -> { "execute": "set_password", "arguments": { "protocol": "vnc",
                                                         "password": "secret" } }
          <- { "return": {} }


          -> { "execute": "query-kvm" }
          <- { "return": { "enabled": true, "present": true } }


          -> { "execute": "query-cpus" }
          <- { "return": [
                   {
                      "CPU":0,
                      "current":true,
                      "halted":false,
                      "qom_path":"/machine/unattached/device[0]",
                      "arch":"x86",
                      "pc":3227107138,
                      "thread_id":3134
                   },
                   {
                      "CPU":1,
                      "current":false,
                      "halted":true,
                      "qom_path":"/machine/unattached/device[2]",
                      "arch":"x86",
                      "pc":7108165,
                      "thread_id":3135
                   }
                ]
             }


          -> { "execute": "system_powerdown" }
          <- { "return": {} }
*/



STREAM *QMPOpen(const char *ImageName)
{
    char *Tempstr=NULL;
    STREAM *S;

    Tempstr=MCopyStr(Tempstr, "unix:", GetCurrUserHomeDir(), "/.qemu_mgr/", ImageName, ".qmp", NULL);
    S=STREAMOpen(Tempstr, "");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        //fprintf(stderr,"QMP: %s\n", Tempstr);
        STREAMWriteLine("{\"execute\": \"qmp_capabilities\"}\n", S);
        Tempstr=STREAMReadLine(Tempstr, S);
        //fprintf(stderr,"QMP: %s\n", Tempstr);
    }

    Destroy(Tempstr);
    return(S);
}


ListNode *QMPCommand(STREAM *S, const char *Cmd)
{
    char *Tempstr=NULL;
    ListNode *Msg=NULL;

    STREAMWriteLine(Cmd, S);
    //fprintf(stderr,"QMP: %s\n", Cmd);
    Tempstr=STREAMReadLine(Tempstr, S);
    //fprintf(stderr,"QMP: %s\n", Tempstr);
    Msg=ParserParseDocument("json", Tempstr);

    Destroy(Tempstr);

    return(Msg);
}



ListNode *QMPTransact(const char *ImageName, const char *Query)
{
    ListNode *Msg=NULL;
    STREAM *S;

    S=QMPOpen(ImageName);
    if (S)
    {
        Msg=QMPCommand(S, Query);
        STREAMClose(S);
    }

    return(Msg);
}


int QMPIsError(ListNode *Qmp)
{
    if (ListFindNamedItem(Qmp, "error") !=NULL) return(TRUE);
    return(FALSE);
}


static void QMPTranslateKey(int inchar, char **CharStr, int *mods)
{
    (*mods)=0;
    switch (inchar)
    {
    case ' ':
        *CharStr=CopyStr(*CharStr, "spc");
        break;
    case '	':
        *CharStr=CopyStr(*CharStr, "tab");
        break;
    case '\n':
        *CharStr=CopyStr(*CharStr, "ret");
        break;
    case '.':
        *CharStr=CopyStr(*CharStr, "dot");
        break;
    case ',':
        *CharStr=CopyStr(*CharStr, "comma");
        break;
    case '"':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "comma");
        break;
    case ';':
        *CharStr=CopyStr(*CharStr, "semicolon");
        break;
    case ':':
        *CharStr=CopyStr(*CharStr, "colon");
        break;
    case '-':
        *CharStr=CopyStr(*CharStr, "minus");
        break;
    case '=':
        *CharStr=CopyStr(*CharStr, "equal");
        break;
    case '/':
        *CharStr=CopyStr(*CharStr, "slash");
        break;
    case '\\':
        *CharStr=CopyStr(*CharStr, "backslash");
        break;
    case '|':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "backslash");
        break;
    case '*':
        *CharStr=CopyStr(*CharStr, "asterisk");
        break;
    case '&':
        *CharStr=CopyStr(*CharStr, "ampersand");
        break;
    case '+':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "equal");
        break;
    case '_':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "minus");
        break;
    case '[':
        *CharStr=CopyStr(*CharStr, "bracket_left");
        break;
    case ']':
        *CharStr=CopyStr(*CharStr, "bracket_right");
        break;
    case '{':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "bracket_left");
        break;
    case '}':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "bracket_right");
        break;
    case '<':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "comma");
        break;
    case '>':
        (*mods) |= KEYMOD_SHIFT;
        *CharStr=CopyStr(*CharStr, "dot");
        break;
    default:
        if ((inchar >='A') && (inchar <='Z'))
        {
            (*mods) |= KEYMOD_SHIFT ;
            inchar = tolower(inchar);
        }
        *CharStr=FormatStr(*CharStr, "%c", inchar);
        break;
    }
}



static void QMPWriteKey(STREAM *Dest, const char *KeyName, int mods)
{
    ListNode *Msg=NULL;
    char *Tempstr=NULL;

    Tempstr=CopyStr(Tempstr, "{ \"execute\": \"send-key\", \"arguments\": {\"keys\": [");
    if (mods & KEYMOD_SHIFT) Tempstr=MCatStr(Tempstr, "{\"type\": \"qcode\", \"data\": \"shift\"},", NULL);
    if (mods & KEYMOD_ALT) Tempstr=MCatStr(Tempstr, "{\"type\": \"qcode\", \"data\": \"alt\"},", NULL);
    Tempstr=MCatStr(Tempstr, "{\"type\": \"qcode\", \"data\": \"", KeyName, "\"}]} }", NULL);
    Msg=QMPCommand(Dest, Tempstr);

    Destroy(Tempstr);
    ParserItemsDestroy(Msg);
}


void QMPSendKey(const char *ImageName, const char *Options)
{
    char *Name=NULL, *Value=NULL, *KeyStr=NULL;
    const char *ptr;
    int mods=0;
    STREAM *In, *Out;

    ptr=GetNameValuePair(Options, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcmp(Name, "key")==0) KeyStr=CopyStr(KeyStr, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    In=STREAMFromDualFD(0,1);
    STREAMSetTimeout(In, 0);
    Out=QMPOpen(ImageName);
    if (Out)
    {
        ptr=KeyStr;
        if (ptr)
        {
            if (strncmp(ptr, "shift-", 6)==0)
            {
                mods |= KEYMOD_SHIFT;
                ptr+=6;
            }

            if (strncmp(ptr, "ctrl-", 5)==0)
            {
                mods |= KEYMOD_CTRL;
                ptr+=5;
            }

            if (strncmp(ptr, "alt-", 4)==0)
            {
                mods |= KEYMOD_ALT;
                ptr+=4;
            }

            if (ptr) QMPWriteKey(Out, ptr, mods);
        }
        STREAMClose(Out);
    }
    STREAMDestroy(In);

    Destroy(Name);
    Destroy(Value);
    Destroy(KeyStr);
}



void QMPSendText(const char *ImageName)
{
    ListNode *Msg=NULL;
    char *Tempstr=NULL, *CharStr=NULL;
    int inchar, mods;
    STREAM *In, *Out;


    In=STREAMFromDualFD(0,1);
    STREAMSetTimeout(In, 0);
    Out=QMPOpen(ImageName);
    if (Out)
    {
        inchar=STREAMReadChar(In);
        while (inchar != EOF)
        {
            QMPTranslateKey(inchar, &CharStr, &mods);
            QMPWriteKey(Out, CharStr, mods);
            inchar=STREAMReadChar(In);
        }
        STREAMClose(Out);
    }
    STREAMDestroy(In);

    ParserItemsDestroy(Msg);
    Destroy(Tempstr);
    Destroy(CharStr);
}


