
#include "interactive.h"
#include "interactive_xdialog.h"
#include "interactive_vt100.h"


int InteractiveSetup(const char *SetupInfo)
{
    if (Config->InteractMode == IA_XDIALOG) XDialogSetup(SetupInfo);
    else if (Config->InteractMode == IA_TERMINAL) QEMUMGR_TerminalDialogSetup(SetupInfo);
}


int InteractiveBusyWindow(const char *Title, const char *Msg)
{
    if (Config->InteractMode == IA_XDIALOG) XDialogBusyWindow(Title, Msg);
//    else if (Config->InteractMode == IA_TERMINAL) QEMUMGR_TerminalDialogSetup(SetupInfo);
		else printf("%s:  %s\n", Title, Msg);
}


int InteractiveErrorMessage(const char *Title, const char *Msg)
{
    if (Config->InteractMode == IA_XDIALOG) XDialogErrorMessage(Title, Msg);
//    else if (Config->InteractMode == IA_TERMINAL) QEMUMGR_TerminalDialogSetup(SetupInfo);
		else printf("%s:  %s\n", Title, Msg);
}



int InteractiveChooseMode(const char *Hint)
{
    const char *ptr;

    if (StrValid(Hint))
    {
        if (strcasecmp(Hint, "term")==0) Config->InteractMode=IA_TERMINAL;
        else
        {
            Config->InteractMode=IA_XDIALOG;
            XDialogFindXDialogCommand(Hint);
            if (! StrValid(Config->DialogCmd))
            {
                printf("ERROR: Unable to locate dialogs program '%s'\n", Hint);
                exit(1);
            }
        }
    }
    else if (Config->InteractMode==IA_UNKNOWN)
    {
        ptr=getenv("DISPLAY");
        if (StrValid(ptr))
        {
            Config->InteractMode=IA_XDIALOG;
        }
        else Config->InteractMode=IA_TERMINAL;
    }

    return(Config->InteractMode);
}


const char *InteractiveQueryRootPassword(const char *Title)
{
    STREAM *StdIn;

    switch (Config->InteractMode)
    {
    case IA_XDIALOG:
        Config->RootPassword=XDialogQueryPassword(Config->RootPassword, "Enter Root Password", Title);
        break;

    default:
        printf("\n\r%s: ", Title);
        fflush(NULL);
        StdIn=STREAMFromFD(0);
        STREAMSetTimeout(StdIn, 0);
        Config->RootPassword=STREAMReadLine(Config->RootPassword, StdIn);
        StripCRLF(Config->RootPassword);
        STREAMDestroy(StdIn);
        break;
    }

    return(Config->RootPassword);
}


