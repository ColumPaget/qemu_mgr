#include "os-commands.h"
#include "interactive.h"
#include <wait.h>
#include <limits.h>
#include <stdlib.h>

ListNode *CommandPaths=NULL;


//don't try to use programs that 'pretend' to be the command
//for now this only checks for crayonizer, and should still
//work with, say, busybox where ifconfig might be a link
//pointing to the busybox exe
static int IsValidCommand(const char *Path)
{
    char *Tempstr=NULL;
    int result=TRUE;

    Tempstr=SetStrLen(Tempstr, PATH_MAX);
    if (realpath(Path, Tempstr))
    {
        if (strcmp(GetBasename(Tempstr), "crayonizer")==0) result=FALSE;
    }
    Destroy(Tempstr);

    return(result);
}

void FindOSCommands()
{
    const char *Commands[]= {"ifconfig", "iptables", "route", "ip", "qemu-system-x86_64", "qarma", "zenity", "yad", "xdialog", "vncviewer", "tightvnc", "tigervnc", "mkisofs", "tar", "zip", "7za", "gzip", "xz", "su", "sudo", NULL};
    char *Tempstr=NULL;
    ListNode *Files=NULL, *Curr=NULL;
    int i;

    if (! CommandPaths) CommandPaths=ListCreate();
    Tempstr=MCopyStr(Tempstr, getenv("PATH"), ":/usr/sbin:/usr/local/sbin:/sbin", NULL);

    for (i=0; Commands[i] != NULL; i++)
    {
        Files=ListCreate();
        FindFilesInPath(Commands[i], Tempstr, Files);

        Curr=ListGetNext(Files);
        while (Curr)
        {
            if (IsValidCommand(Curr->Item))
            {
                SetVar(CommandPaths, Commands[i], Curr->Item);
                break;
            }
            Curr=ListGetNext(Curr);
        }
        ListDestroy(Files, Destroy);
    }

    Destroy(Tempstr);
}

void OSCommandQemuGetVersion()
{
    const char *p_ExecPath;
    char *Tempstr=NULL;
    STREAM *S;

    p_ExecPath=OSCommandFindPath("qemu-system-x86_64");
    Tempstr=MCopyStr(Tempstr, "cmd:", p_ExecPath, " --version", NULL);
    S=STREAMOpen(Tempstr, "");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        if (strncmp(Tempstr, "QEMU emulator version ",22)==0) GetToken(Tempstr+22, "\\S", & (Config->QemuVersion), 0);
        STREAMClose(S);
    }

    Destroy(Tempstr);
}


const char *OSCommandFindPath(const char *Command)
{
		if (*Command=='/') return(Command);
    return(GetVar(CommandPaths, Command));
}



static void RunCommandPasswordTransact(STREAM *S, const char *SudoPath)
{
    int i;
    char *Tempstr=NULL;

    //wait for root password query. Will not end in '\n', so we can't just do STREAMReadLine
    for (i=0; i < 10; i++)
    {
        if (STREAMCountWaitingBytes(S) > 6) break;
        usleep(10000);
    }

    if (! StrValid(Config->RootPassword))
    {
        if (StrValid(SudoPath)) InteractiveQueryRootPassword("TAP networking requires sudo password");
        else InteractiveQueryRootPassword("TAP networking requires su (root) password");
    }
    Tempstr=MCopyStr(Tempstr, Config->RootPassword, "\n", NULL);
    STREAMWriteLine(Tempstr, S);
    STREAMFlush(S);

    Destroy(Tempstr);
}


char *RunCommand(char *RetStr, const char *Command, int Flags)
{
    char *Exec=NULL, *Cmd=NULL, *Args=NULL, *Tempstr=NULL;
    const char *p_args, *p_Path, *p_ExecPath, *p_SudoPath;
    time_t StartTime, Now;
    STREAM *S;
    int i, fd;

    for (i=0; i < 100; i++)
    {
        if (waitpid(-1, NULL, WNOHANG) < 1) break;
    }


//if we're already root (effective uid 0) then remove the RUNCMD_ROOT flag as we
//don't want to switch to root (this will only be removed in the current function because
//the Flags variable is pass-by-value
    if (geteuid()==0) Flags &= ~RUNCMD_ROOT;

    p_args=GetToken(Command, "\\S", &Tempstr, 0);
    Exec=CopyStr(Exec, OSCommandFindPath(Tempstr));
    p_SudoPath=OSCommandFindPath("sudo");

//if we can't find an executable for the command, indicate this by returning null
    if (! StrValid(Exec))
    {
        Destroy(Exec);
        Destroy(Tempstr);
        Destroy(RetStr);
        return(NULL);
    }


    if (Flags & RUNCMD_ROOT)
    {
        if (StrValid(p_SudoPath) && (! (Config->Flags & CONF_USE_SU)) ) Cmd=MCopyStr(Cmd, "cmd:sudo -b -S ", Exec, " ", p_args, NULL);
        else if (p_Path) Cmd=MCopyStr(Cmd, "cmd:su -c '", Exec, " ", p_args, "'", NULL);
    }
    else Cmd=MCopyStr(Cmd, "cmd:", Cmd, " ", p_args, NULL);

    Args=CopyStr(Args, "rw pty setsid");

    fd=open("/dev/tty", O_RDWR);
    if (fd > -1)
    {
        Tempstr=FormatStr(Tempstr, " ctty=%d", fd);
        Args=CatStr(Args, Tempstr);
    }


    if (Flags & RUNCMD_NOSHELL) Args=CatStr(Args, " noshell");

    printf("run: [%s] %s\n", Cmd, Args);

    S=STREAMOpen(Cmd, Args);
    if (S)
    {
        if (Flags & RUNCMD_ROOT) RunCommandPasswordTransact(S, p_SudoPath);

        STREAMSetTimeout(S, 50);
        StartTime=time(NULL);
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            printf("%s\n", Tempstr);
            Now=time(NULL);
            if ((Now - StartTime) > 3) break;
            Tempstr=STREAMReadLine(Tempstr, S);
        }

        sleep(1);
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
    Destroy(Cmd);
    Destroy(Args);

    return(RetStr);
}
