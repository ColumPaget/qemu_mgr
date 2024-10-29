#include "help.h"


void CommandLinePrintVersion()
{
    printf("qemu_mgr: version %s\n", VERSION);
}

void CommandLinePrintHelp()
{
    printf("qemu_mgr can be run in interactive mode (follow-your-nose menus) or at the command line. The interactive menus either require zenity or qarma to be installed, or else can be run in 'terminal' mode ('qemu_mgr -i term') to get generic text-driven menus.\n\n");
    printf("Command line options are:\n\n");
    printf("qemu_mgr\n");
    printf("	run interactive mode with auto-detected dialog system\n");
    printf("qemu_mgr -su\n");
    printf("	run interactive mode with auto-detected dialog system, use 'su' rather than 'sudo' for commands requriing superuser permissions (mostly just setting up tap interfaces)\n");
    printf("qemu_mgr -i <type>\n");
    printf("	run interactive mode with specified dialog system. 'type' can be 'term', 'qarma' or 'zenity'\n");
    printf("qemu_mgr create <vm name> <options>\n");
    printf("	create a vm from a .iso file. 'options' must include a '-iso' option specifying the path to the .iso file\n");
    printf("qemu_mgr import <vm name> <options>\n");
    printf("	import a vm from an image file. 'options' must include a '-img' option specifying the path to the image file\n");
    printf("qemu_mgr add		<vm name> <options>\n");
    printf("	add an existing image file. This option doesn't create an image file in the '.qemu_mgr' directory, but uses the existing image file whereever it is. 'options' must include a '-img' option specifying the path to the image file\n");
    printf("qemu_mgr del    <vm name>\n");
    printf("qemu_mgr delete <vm name>\n");
    printf("	delete a configured vm. Only files within the '.qemu_mgr' directory are deleted. External image files will be left alone\n");
    printf("qemu_mgr change <vm name> <options>\n");
    printf("	change settings for an existing vm.\n");
    printf("qemu_mgr start	<vm name> <options>\n");
    printf("	start a configured vm, possibly changing some settings via 'options'.\n");
    printf("qemu_mgr stop	 <vm name> <options>\n");
    printf("	stop a running vm.\n");
    printf("qemu_mgr list	 <vm name> <options>\n");
    printf("	list configured VMs.\n");
    printf("qemu_mgr vnc		<vm name> <options>\n");
    printf("	connect to a vm with VNC. (VM must be configured with -display vnc)\n");
    printf("qemu_mgr send-text <vm name>\n");
    printf("	read text line-by-line from stdin and send it to a running vm.\n");
    printf("qemu_mgr send-string <vm name> <string>\n");
    printf("	send string argument to a running vm.\n");
    printf("qemu_mgr send-key	<vm name> <key name>\n");
    printf("	send keystroke 'key name' to a running vm.\n");
    printf("qemu_mgr media-add <vm name> <vm device> <path>\n");
    printf("	copy items in 'path' to a file and load/mount it into 'device' in the vm. See 'qemu_mgr --help media' for more info.\n");
    printf("qemu_mgr media-del <vm name> <vm device>\n");
    printf("	remove/unmount file in 'device' in the vm. See 'qemu_mgr --help-media' for more info.\n");
    printf("qemu_mgr screenshot <vm name> <options>\n");
    printf("	screenshot a running vm.\n");
    printf("qemu_mgr -?\n");
    printf("	print this help\n");
    printf("qemu_mgr -help\n");
    printf("	print this help\n");
    printf("qemu_mgr --help\n");
    printf("	print this help\n");
    printf("qemu_mgr --help-vnc\n");
    printf("	print help for VNC display\n");
    printf("qemu_mgr --help-media\n");
    printf("	print help for mounting files/directories into a running vm\n");
    printf("\n");
    printf("Options\nMost options can be expressed either as '-iso installer.iso' or 'iso=installer.iso'. Some options, like -iso, -img or -size only relate to the VM creation and installation step\n\n");
    printf(" -iso <path>        specify path to a .iso O.S. installer file for use in setting up a vm\n");
    printf(" -img <path>        specify path to an image file for use in setting up a vm\n");
    printf(" -template <name>   config template to use for VM. Default is the template 'Generic' which creates a disk image 40G in size, and a memorty allocation of 2047 megabytes, and a machine type of 'pc' (i440fx)\n");
    printf(" -size <size>       size of newly created image file, overriding what was specified in the selected template. The 'size' argument takes a postfix of 'M' or 'G' to specify megabytes or gigabytes.\n");
    printf(" -mem <size>        memory allocated to the VM in megabytes, overriding the selected template.\n");
    printf(" -disk-controller  <type>        disk-controller. One of 'virtio', 'scsi' or 'ide'.\n");
    printf(" -dc  <type>        disk-controller. One of 'virtio', 'scsi' or 'ide'.\n");
    printf(" -machine <type>    machine type: one of 'pc', 'q35', 'isapc' or 'microvm'.\n");
    printf(" -mach    <type>    machine type: one of 'pc', 'q35', 'isapc' or 'microvm'.\n");
    printf(" -display <type>    display type: one of 'std', 'virtio', 'qxl', 'rage128p', 'rv100', 'vnc' or 'none'. See 'qemu_mgr --help-vnc' for more info on VNC.\n");
    printf(" -prealloc <yes|no>    Preallocate memory to the vm (rather than have the vm grab memory as it needs it)\n");
    printf(" -fullscreen <yes|no>  Fullscreen graphics\n");
    printf(" -su                   Use 'su' rather than 'sudo' for privesc\n");
    printf(" -password <secret>    Password for use with VNC\n");
    printf(" -pass     <secret>    Password for use with VNC\n");
    printf(" -pw       <secret>    Password for use with VNC\n");
    printf(" -delay   <seconds>    Connect delay for VNC viewers. This can be used to prevent connecting to early and getting disconnected while the VM starts up.\n");
}


void CommandLinePrintMediaHelp()
{
    printf("Files can be packed into and iso filesystem and mounted as a cdrom, or as a tar or zip file which can then be 'mounted' into any block device in unix\n");
    printf("ISO filesystem support requires the mkisofs utility to be available. This method has been seen to work with windows guests. You can either pick the appropriate option from the interactive menus ones a VM is running, or else use the media-add command:\n\n");
    printf("  qemu_mgr media-add MyWindowsVM ide-cd0 iso:/home/music\n\n");
    printf("This will build a .iso file with the contents of /home/music and then 'insert' it into the qemu cd-rom block device 'ide-cd0' in the vm 'MyWindowsVM'\n\n");
    printf("For unix hosts a trick can be used where any kind of file can be mounted in any qemu block device, and extracted in the guest VM. For example\n\n");
    printf("  qemu_mgr media-add MyLinuxVM fd0 tgz:/home/music\n\n");
    printf("Will use the tar utility to create a gzipped tar file and 'insert' it into floppy drive fd0. This can then be extracted in the guest with\n\n");
    printf("  tar -zcf /dev/fd0\n\n");
    printf("qemu_mgr recognizes the following prefixes to identify the kind of file to be made and mounted\n\n");
    printf("   iso:  iso9660 filesystem (requires mkisofs utility)\n");
    printf("   zip:  pkzip archive file (requires zip utility)\n");
    printf("   tar:  tar archive file (requires tar  utility)\n");
    printf("   tgz:  gzipped tar archive file (requires tar and gzip utilities)\n");
    printf("   txz:  xzipped tar archive file (requires tar and xz utilities)\n");
    printf("   7za:  7zip archive file (requires 7za utility)\n\n");
    printf("With no prefix the path is treated as a file to be mounted (which will fail if it's a directory). This allows mounting any type of file into a block device in the VM.\n");
}

void CommandLinePrintVNCHelp()
{
    printf("The display type must be specified as '-display vnc:<host>:<display>' where 'host' is the host connections are allowed to come *from* (often '127.0.0.1' or '0.0.0.0' for 'all hosts'). The 'display' argument is the display number (equates to a port number of 5900 + display number). e.g. -display vnc:127.0.0.1:4 to run a VNC service on port 5904\n");
}

