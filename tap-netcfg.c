#include "tap-netcfg.h"
#include "os-commands.h"

static char *TapNetWriteConfig(char *Path, const char *VMName, const char *IP, const char *Netmask, const char *RunAs, int Flags)
{
    STREAM *S;
    char *Tempstr=NULL, *FPath=NULL;

    Path=MCopyStr(Path, GetCurrUserHomeDir(), "/.qemu_mgr/", VMName, ".ifup", NULL);
    S=STREAMOpen(Path, "w");
    if (S)
    {
        Tempstr=MCopyStr(Tempstr, "#!/bin/sh\n", OSCommandFindPath("ifconfig"), " $1 ", IP, "\n", NULL);
        Tempstr=MCatStr(Tempstr, OSCommandFindPath("ifconfig"), " $1 up \n", NULL);
        STREAMWriteLine(Tempstr, S);

        if (Flags & NET_TAP_FW_LOCAL)
        {
            Tempstr=MCopyStr(Tempstr, OSCommandFindPath("iptables"), " -I INPUT -i $1 -j ACCEPT\n", NULL);
            Tempstr=MCatStr(Tempstr,  OSCommandFindPath("iptables"), " -I OUTPUT -o $1 -j ACCEPT\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }

        if (Flags & NET_TAP_FW_REMOTE)
        {
            Tempstr=MCopyStr(Tempstr, OSCommandFindPath("iptables"), " -I FORWARD -i $1 -j ACCEPT\n", NULL);
            Tempstr=MCatStr(Tempstr,  OSCommandFindPath("iptables"), " -I FORWARD -o $1 -j ACCEPT\n", NULL);
            Tempstr=MCatStr(Tempstr,  OSCommandFindPath("iptables"), " -t nat -I POSTROUTING -s ", IP, " -j MASQUERADE\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }

        if (StrValid(RunAs))
        {
            FPath=MCopyStr(FPath, GetCurrUserHomeDir(), "/.qemu_mgr/", VMName, ".pid\n", NULL);
            Tempstr=MCopyStr(Tempstr, "chown ", RunAs, " ", FPath, NULL);
            FPath=MCopyStr(FPath, GetCurrUserHomeDir(), "/.qemu_mgr/", VMName, ".qmp\n", NULL);
            Tempstr=MCatStr(Tempstr, "chown ", RunAs, " ", FPath, NULL);
            STREAMWriteLine(Tempstr, S);
        }

        STREAMClose(S);
        chmod(Path, 0700);
    }

    Destroy(Tempstr);

    return(Path);
}



char *TapSetupQemuCommandLine(char *CmdLine, const char *ImageName, ListNode *Values)
{
    char *Path=NULL;
    const char *ptr;
    int Flags=0, uid;
    struct passwd *user_info;

    ptr=GetVar(Values, "tap_local_access");
    if (ptr && (strcasecmp(ptr, "yes")==0)) Flags |= NET_TAP_FW_LOCAL;

    ptr=GetVar(Values, "tap_remote_access");
    if (ptr && (strcasecmp(ptr, "yes")==0)) Flags |= NET_TAP_FW_REMOTE;


    SetVar(Values, "sudo", "y");


    uid=getuid();
    if (uid > 0)
    {
        user_info=getpwuid(uid);
        if (user_info) SetVar(Values, "runas", user_info->pw_name);
    }

    ptr=GetVar(Values, "tap_extip");
    Path=TapNetWriteConfig(Path, ImageName, ptr, "", ParserGetValue(Values, "runas"), Flags);
    CmdLine=MCatStr(CmdLine, " -net tap,script=", Path, " ", NULL);

    Destroy(Path);

    return(CmdLine);
}
