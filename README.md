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
