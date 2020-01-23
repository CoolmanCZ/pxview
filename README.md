# PxView - Ultimate++ application

PxView is the GUI application to open and manipulate Paradox database files.

This application is based on the [pxlib](http://pxlib.sourceforge.net/) - a simply and still small C library to read and write Paradox DB files. It supports all versions starting from 3.0. It currently has a very limited set of functions like to open a DB file, read its header and read every single record. It can read and write blob data. The write support is still a bit experimental.

# AppVeyor build status

[AppVeyor](https://www.appveyor.com) is used to build Windows binaries and deploy them to the GitHub releases directory.

|branch  | status|
|---     |---    |
|master  |[![Build status](https://ci.appveyor.com/api/projects/status/github/CoolmanCZ/pxview?svg=true)](https://ci.appveyor.com/project/CoolmanCZ/pxview)|

*The Windows executable is currently not signed, which will show a warning when you run the .exe. Press 'more info' -> 'run anyway' to skip the warning.*

# Installation

Simply download and unzip the package from the link below and then execute PxView binary.

# Download

Download latest windows binaries and source code from [GitHub releases](https://github.com/CoolmanCZ/pxview/releases/).

# Changelog

* 1.1.2 - core dump fix
* 1.1.1 - submodule path changed
* 1.1.0 - U++ framework code update, C++ performance and bug correction
* 1.0.0 - initial version

# Screenshots

![img1](https://github.com/CoolmanCZ/pxview/raw/master/images/img1.png "screenshot 1")

