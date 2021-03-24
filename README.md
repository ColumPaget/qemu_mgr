SYNOPSIS
========

qemu_mgr is a very simplistic QEMU frontend/launcher. It manages VMs in a .qemu_mgr subdirectory of the users home directory. It supports creating VMs from .iso installers, importing from various types of image file, adding an image file to the managed VMs without moving or importing it, launching VMs from a menu, and various operations (including launching a VNC viewer) upon a running VM. A number of the most useful configuration options for QEMU are provided, including setting up VNC services, TUN/TAP networking, and USB device passthrough (not yet PCI passthrough though).

qemu_mgr is intended to be usable on headless systems over ssh, or on systems without a graphical environment, thus it implements three types of frontend:

* 'xdialog' forms frontend that uses zenity or qarma (not sadly yad or xdialog).
* 'terminal' frontend that uses libUseful terminal functions to create a menu-driven character-based frontend.
* 'cli' command-line interface.


