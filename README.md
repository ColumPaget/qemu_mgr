[![Build Status](https://travis-ci.com/ColumPaget/qemu_mgr.svg?branch=master)](https://travis-ci.com/ColumPaget/qemu_mgr)

SYNOPSIS
========

qemu_mgr is a very simplistic QEMU frontend/launcher for x86/x86_64 virtual machines. Currently only VMs run under qemu-system-x86_64 are supported. It manages VMs in a .qemu_mgr subdirectory of the users home directory. It supports creating VMs from .iso installers, importing from various types of image file, adding an image file to the managed VMs without moving or importing it, launching VMs from a menu, and various operations (including launching a VNC viewer) upon a running VM. A number of the most useful configuration options for QEMU are provided, including setting up VNC services, TUN/TAP networking, and USB device passthrough (not yet PCI passthrough though).

qemu_mgr is intended to be usable on headless systems over ssh, or on systems without a graphical environment, thus it implements three types of frontend:

* 'xdialog' forms frontend that uses zenity or qarma (not sadly yad or xdialog).
* 'terminal' frontend that uses libUseful terminal functions to create a menu-driven character-based frontend.
* 'cli' command-line interface.

AUTHOR
======

Alaya and libUseful are (C) 2021 Colum Paget. They are released under the GPL so you may do anything with them that the GPL allows.

Email: colums.projects@gmail.com


DISCLAIMER
==========

This is free software. It comes with no guarentees and I take no responsiblity if it makes your computer explode or opens a portal to the demon dimensions, or does (or doesn't do) anything.

WHY?
====

I wanted a simple, menu-driven frontend that I could use to just create/manage/launch intel architecture operating systems in virtual machines using qemu. It's written in C because I specifically wanted to use this without having to install much else on said machines. 


USAGE
=====

qemu_mgr can be run in interactive mode (follow-your-nose menus) or at the command line. The interactive menus either require zenity or qarma to be installed, or else can be run in 'terminal' mode ('qemu_mgr -i term') to get generic text-driven menus.

Command line options are:

```
qemu_mgr
        run interactive mode with auto-detected dialog system
qemu_mgr -i <type>
        run interactive mode with specified dialog system. 'type' can be 'term', 'qarma' or 'zenity'
qemu_mgr create <vm name> <options>
        create a vm from a .iso file. 'options' must include a '-iso' option specifying the path to the .iso file
qemu_mgr import <vm name> <options>
        import a vm from an image file. 'options' must include a '-img' option specifying the path to the image file
qemu_mgr add            <vm name> <options>
        add an existing image file. This option doesn't create an image file in the '.qemu_mgr' directory, but uses the existing image file whereever it is. 'options' must include a '-img' option specifying the path to the image file
qemu_mgr change <vm name> <options>
        change settings for an existing vm.
qemu_mgr start  <vm name> <options>
        start a configured vm, possibly changing some settings via 'options'.
qemu_mgr stop    <vm name> <options>
        stop a running vm.
qemu_mgr list    <vm name> <options>
        list configured VMs.
qemu_mgr vnc            <vm name> <options>
        connect to a vm with VNC. (VM must be configured with -display vnc)
qemu_mgr send-text <vm name>
        read text line-by-line from stdin and send it to a running vm.
qemu_mgr send-string <vm name> <string>
        send '<string>' argument to a running vm.
qemu_mgr send-key       <vm name> <key name>
        send keystroke 'key name' to a running vm.
qemu_mgr media-add <vm name> <vm device> <path>
        copy items in 'path' to a file and load/mount it into 'device' in the vm. See 'qemu_mgr --help media' for more info.
qemu_mgr media-del <vm name> <vm device>
        remove/unmount file in 'device' in the vm. See 'qemu_mgr --help-media' for more info.
qemu_mgr screenshot <vm name> <options>
        screenshot a running vm.
qemu_mgr -?
        print help
qemu_mgr -help
        print help
qemu_mgr --help
        print help
qemu_mgr --help-vnc
        print help for VNC display
qemu_mgr --help-media
        print help for mounting files/directories into a running vm
```

Options
-------

Most options can be expressed either as '-iso installer.iso' or 'iso=installer.iso'. Some options, like -iso, -img or -size only relate to the VM creation and installation step

```
 -iso <path>        specify path to a .iso O.S. installer file for use in setting up a vm
 -img <path>        specify path to an image file for use in setting up a vm
 -template <name>   config template to use for VM. Default is the template 'Generic' which creates a disk image 40G in size, and a memorty allocation of 2047 megabytes, and a machine type of 'pc' (i440fx)
 -size <size>       size of newly created image file, overriding what was specified in the selected template. The 'size' argument takes a postfix of 'M' or 'G' to specify megabytes or gigabytes.
 -mem <size>        memory allocated to the VM in megabytes, overriding the selected template.
 -disk-controller  <type>        disk-controller. One of 'virtio', 'scsi' or 'ide'.
 -dc  <type>        disk-controller. One of 'virtio', 'scsi' or 'ide'.
 -machine <type>    machine type: one of 'pc', 'q35', 'isapc' or 'microvm'.
 -mach    <type>    machine type: one of 'pc', 'q35', 'isapc' or 'microvm'.
 -display <type>    display type: one of 'std', 'virtio', 'qxl', 'rage128p', 'rv100', 'vnc' or 'none'. See 'qemu_mgr --help-vnc' for more info on VNC.
 -prealloc <yes|no>    Preallocate memory to the vm (rather than have the vm grab memory as it needs it)
 -fullscreen <yes|no>  Fullscreen graphics
 -password <secret>    Password for use with VNC
 -pass     <secret>    Password for use with VNC
 -pw       <secret>    Password for use with VNC
 -delay   <seconds>    Connect delay for VNC viewers. This can be used to prevent connecting to early and getting disconnected while the VM starts up.
```
