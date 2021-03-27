#include "os-commands.h"
#include "interactive.h"
#include <wait.h>

ListNode *CommandPaths=NULL;


void FindOSCommands()
{
    char *Tempstr=NULL, *File=NULL;
    const char *Commands[]= {"ifconfig", "iptables", "route", "ip", "qemu-system-x86_64", "qarma", "zenity", "yad", "xdialog", "vncviewer", "tightvnc", "tigervnc", "mkisofs", "tar", "zip", "7za", "gzip", "xz", NULL};
    int i;

    if (! CommandPaths) CommandPaths=ListCreate();
    Tempstr=MCopyStr(Tempstr, getenv("PATH"), ":/usr/sbin:/usr/local/sbin:/sbin", NULL);
    for (i=0; Commands[i] != NULL; i++)
    {
        File=FindFileInPath(File, Commands[i], Tempstr);
        SetVar(CommandPaths, Commands[i], File);
    }

    Destroy(Tempstr);
    Destroy(File);
}


const char *OSCommandFindPath(const char *Command)
{
    return(GetVar(CommandPaths, Command));
}




char *RunCommand(char *RetStr, const char *Command, int Flags)
{
    char *Exec=NULL, *Tempstr=NULL, *Token=NULL;
    const char *p_args, *p_Path, *p_ExecPath;
    STREAM *S;
    int i;

    for (i=0; i < 100; i++)
    {
        if (waitpid(-1, NULL, WNOHANG) < 1) break;
    }


//if we're already root (effective uid 0) then remove the RUNCMD_ROOT flag as we
//don't want to switch to root (this will only be removed in the current function because
//the Flags variable is pass-by-value
    if (geteuid()==0) Flags &= ~RUNCMD_ROOT;

    p_args=GetToken(Command, "\\S", &Token, 0);
    p_ExecPath=OSCommandFindPath(Token);

//if we can't find an executable for the command, indicate this by returning null
    if (! StrValid(p_ExecPath))
    {
        Destroy(Exec);
        Destroy(RetStr);
        return(NULL);
    }


    if (Flags & RUNCMD_ROOT)
    {
        if (p_Path) Exec=MCopyStr(Exec, "cmd:su -c ' ", p_ExecPath, " ", p_args, "'", NULL);
    }
    else Exec=MCopyStr(Exec, "cmd:", p_ExecPath, " ", p_args, NULL);

    Tempstr=CopyStr(Tempstr, "pty setsid");
    //if (Flags & RUNCMD_DAEMON) Tempstr=CatStr(Tempstr, " daemon");
    if (Flags & RUNCMD_NOSHELL) Tempstr=CatStr(Tempstr, " noshell");

    printf("run: [%s] %s\n", Exec, Tempstr);
    S=STREAMOpen(Exec, Tempstr);
    if (S)
    {
        if (Flags & RUNCMD_ROOT)
        {
            for (i=0; i < 10; i++)
            {
                if (STREAMCountWaitingBytes(S) > 6) break;
                usleep(10000);
            }
            if (! StrValid(Config->RootPassword)) InteractiveQueryRootPassword("TAP networking requires root password");
            Tempstr=MCopyStr(Tempstr, Config->RootPassword, "\n", NULL);
            printf("SEND: [%s]\n", Config->RootPassword);
            STREAMWriteLine(Tempstr, S);
        }
        sleep(1);
        //RetStr=STREAMReadDocument(RetStr, S);
        STREAMClose(S);
    }
    else
    {
        //we couldn't launch the command. Indicate this by returning null
        Destroy(RetStr);
        RetStr=NULL;
    }

    Destroy(Tempstr);
    Destroy(Exec);

    return(RetStr);
}
