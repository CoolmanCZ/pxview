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

## Windows
Simply download and unzip appropriate package for your windows version from the link below and then execute PxView binary.

## Linux
The linux binary is not available. You have to build the binary manually and this requires some development tools installed in your linux distribution.

### Linux Build
Clone the Gihub repository and execute following commands in the directory where the source code is downloaded
```
git submodule init
git submodule update --recursive --remote --depth 1
./GenerateCMakeFiles.sh
mkdir -p build
cd build
cmake .. && make
```

After successfull build the binary is stored in the ```build/bin``` directory

*Note: For clang build replace ```cmake .. && make``` with ```cmake -DCMAKE_TOOLCHAIN_FILE=../upp_cmake/utils/toolchain-clang.cmake .. && make```*

# Download
Download latest windows binaries and source code from [GitHub releases](https://github.com/CoolmanCZ/pxview/releases/).

# Changelog
* 1.1.5 - submodules update, paradox library readability improvement
* 1.1.4 - submodules update, readability improvement
* 1.1.3 - submodules update, small code corrections
* 1.1.2 - core dump fix
* 1.1.1 - submodule path changed
* 1.1.0 - U++ framework code update, C++ performance and bug correction
* 1.0.0 - initial version

# Screenshots
![img1](https://github.com/CoolmanCZ/pxview/raw/master/images/img1.png "screenshot 1")

