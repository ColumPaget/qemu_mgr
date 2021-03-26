#include "kvm.h"

int KVMAvailable()
{
int Avail=FALSE;

    if (access("/dev/kvm", F_OK) != 0) printf("No /dev/kvm. Cannot activate KVM\n");
    else if (access("/dev/kvm", W_OK) != 0) printf("Permission denied to /dev/kvm. Cannot activate KVM\n");
    else Avail=TRUE;

    return(Avail);
}
