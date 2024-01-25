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
    return(GetVar(CommandPaths, Command));
}




char *RunCommand(char *RetStr, const char *Command, int Flags)
{
    char *Exec=NULL, *Tempstr=NULL, *Token=NULL;
    const char *p_args, *p_Path, *p_ExecPath, *p_SudoPath;
    STREAM *S;
    time_t StartTime, Now;
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
    p_SudoPath=OSCommandFindPath("sudo");

//if we can't find an executable for the command, indicate this by returning null
    if (! StrValid(p_ExecPath))
    {
        Destroy(Exec);
        Destroy(RetStr);
        return(NULL);
    }


    if (Flags & RUNCMD_ROOT)
    {
    	if (StrValid(p_SudoPath)) Exec=MCopyStr(Exec, "cmd:sudo -S ", p_ExecPath, " ", p_args, NULL);
        else if (p_Path) Exec=MCopyStr(Exec, "cmd:su -c ' ", p_ExecPath, " ", p_args, "'", NULL);
	//Flags |= RUNCMD_NOSHELL;
    }
    else Exec=MCopyStr(Exec, "cmd:", p_ExecPath, " ", p_args, NULL);

    Tempstr=CopyStr(Tempstr, "rw pty setsid");
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

            if (! StrValid(Config->RootPassword)) 
	    {
		if (StrValid(p_SudoPath)) InteractiveQueryRootPassword("TAP networking requires sudo password");
		else InteractiveQueryRootPassword("TAP networking requires su (root) password");
	    }
            Tempstr=MCopyStr(Tempstr, Config->RootPassword, "\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }

	
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
