# Oyranos README
[![Build Status](https://travis-ci.org/oyranos-cms/oyranos.svg?branch=master)](https://travis-ci.org/oyranos-cms/oyranos)
[![Documentation](https://codedocs.xyz/oyranos-cms/oyranos.svg)](https://codedocs.xyz/oyranos-cms/oyranos/)
[![Codecov](https://codecov.io/gh/oyranos-cms/oyranos/branch/master/graph/badge.svg)](https://codecov.io/gh/oyranos-cms/oyranos)
[![Issues](https://img.shields.io/github/issues-raw/oyranos-cms/oyranos.svg?style=flat-square)](https://github.com/oyranos-cms/oyranos/issues)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

![](http://www.oyranos.org/images/oyranos_logo.svg)

The Color Management System (CMS) Oyranos allows the coordination of
device specific Informations (ICC profiles) und system wide settings.


###Links
* [About](http://www.oyranos.org/about)
* [Features](http://www.oyranos.org/features)
* [Sources & Download](http://www.oyranos.org/downloads/)
* [Documentation & Support](http://www.oyranos.org/support)
* [ChangeLog](ChangeLog.md)
* [Copyright](COPYING.md) - BSD
* [Authors](AUTHORS.md)
* [Development](http://www.oyranos.org/development)
* [Future](http://www.oyranos.org/future)
* [Packaging](PACKAGING.md)
* [Wiki](http://www.oyranos.org/wiki/index.php?title=Oyranos)


###Dependencies
####From Sources
#####Mandatory
* [icc-profiles-openicc and icc-profiles-basiccolor](http://sourceforge.net/projects/openicc/files/) - default ICC profiles
* [libXinerama, libXrandr, libXfixes and libXxf86vm](http:/www.x.org) - for X11 support
* [Xcalib](http://www.etg.e-technik.uni-erlangen.de/web/doe/xcalib/) - a vcgt/calibration loader, required on X11/Windows 
* [lcms and/or lcms2](http://www.littlecms.com) - CMM for color conversion
* [libxml2](http://www.xmlsoft.org/) - for XML parsing

#####Optional
* [libXcm >= 0.5.4] (http://www.oyranos.org/libxcm) - for monitor support (or internal fallback)
* [Yajl](http://lloyd.github.com/yajl) - JSON API (or internall fallback)
* [libOpenICC](https://github.com/OpenICC/config) - JSON/Yajl based DB API (or internall fallback)
* [Elektra](http://www.libelektra.org) - DB API and D-Bus messaging
* [FLTK](http://www.fltk.org) - (--enable-threads) is needed for the example Gui
* For the documentation use doxygen, graphviz and graphviz-gd packages.
  * [Doxygen v1.5.8 or higher is recommended](http://www.doxygen.org)
* [LCOV](http://ltp.sourceforge.net/coverage/lcov.php) - coverage docu
* [Qt](http://www.qt.io) - for a nice observer utility
* [CUPS](http://www.cups.org) - for CUPS ICC configuration support
* [libraw](http://www.libraw.org) - for cameraRAW decoding
* [exiv2](http://www.exiv2.org/) - for cameraRAW decoding
* [SANE](http://www.sane-project.org/) - only with Oyranos SANE\_CAP\_COLOUR patch
* [Cairo](http://www.cairographics.org/) - for a tool and a example
* [gettext](https://www.gnu.org/software/gettext/) - i18n

#### Debian/Ubuntu:
    sudo apt-get install locales build-essential git autoconf cmake ruby libxml2-dev libyajl-dev gettext libfltk1.3-dev libjpeg-dev libpng-dev libcairo2-dev x11proto-xf86vidmode-dev x11proto-xext-dev libxxf86vm-dev libxrandr-dev libxinerama-dev libxmu-dev libxpm-dev libxfixes-dev libxft-dev fluid ftgl-dev liblcms-dev libexiv2-dev libcups2-dev cmake
    # oyranos-0.3.3 can now be installed successfully.
    sudo apt-get install libqt4-dev                        # Synnefo
    sudo apt-get install kdelibs5-dev                      # KolorManager
    sudo apt-get install libgtk2.0-dev libglib2.0-dev libgutenprintui2-dev libopenexr-dev # CinePaint
    sudo apt-get install doxygen graphviz                  # for documentation

##### Optional
    ICC Examin to build afterwards. oyranos-config-fltk uses the profile viewer.

#### Fedora 17
    git
    gcc-c++
    elektra-devel
    exiv2-devel
    cmake
    cups-devel
    sane-backends-devel
    liblcms-devel
    liblcms2-devel
    icc-profiles-basiccolor-lstarrgb
    icc-profiles-lcms-lab
    icc-profiles-openicc-printing
    icc-profiles-openicc-rgb
    libtool
    libtool-ltdl-devel
    libxml2-devel
    libpng-devel
    libtiff-devel
    LibRaw-devel
    libXinerama-devel
    libXrandr-devel
    libXcm-devel
    fltk-devel
    fltk-fluid
    gettext
    yajl-devel
    doxygen
    graphviz
##### Optional
    cairo-devel
    libXmu-devel
    qt-devel
    netpbm-progs
    exiv2-devel

#### Arch Linux
    extra/libxml2
    extra/libxrandr
    extra/libxinerama
    extra/libxfixes
    extra/libxxf86vm
    extra/lcms
    extra/lcms2
    extra/graphviz
    extra/doxygen
    community/yajl
    AUR/libxcm
    AUR/openicc (TODO verify if correct package)
##### Optional
    extra/qt
    extra/fltk
    community/libraw
    AUR/xcalib
    AUR/elektra

  NOTE1: If you have problems installing elektra from AUR,
  use this PKGBUILD: ./extras/arch.abs/elektra/PKGBUILD

  NOTE2: To build an Arch package of Oyranos from yor local git branch,
  use './extras/arch.abs/oyranos-git/PKGBUILD' and
  (a) replace '_gitroot' variable,
  (b) replace the branch name in line 44.

#### openSUSE Leap 42.1
    zypper install cmake gcc-c++ git autoconf libtool make patch pkgconfig desktop-file-utils xdg-utils update-desktop-files Mesa-devel cups-devel libxml2-devel libyajl-devel liblcms-devel liblcms2-devel libjpeg-devel libpng-devel libtiff-devel libraw-devel libexiv2-devel xorg-x11-libXfixes-devel xorg-x11-libXmu-devel xorg-x11-libXpm-devel libXinerama-devel libXrandr-devel fltk-devel libXft-devel ftgl-devel gettext gettext-tools cairo-devel netpbm
    # oyranos-0.9.0 can now be installed successfully.
    zypper install libqt4-devel libqt5-qtbase-common-devel          # Synnefo
    zypper install patterns-openSUSE-devel_kde_frameworks           # KolorManager
    zypper install gtk2-devel libopenexr-devel gutenprint-devel     # CinePaint
    zypper install doxygen graphviz                                 # for documentation

###Building
    $ configure
    $ make
    $ make install

####Build Flags
... are typical cmake flags like CMAKE\_C\_FLAGS to tune compilation.

* CMAKE\_INSTALL\_PREFIX to install into paths and so on. Use on the command 
  line through -DCMAKE\_INSTALL\_PREFIX=/my/path .
* LIB\_SUFFIX - allows to append a architecture specific suffix like 
  LIB\_SUFFIX=64 for 64bit RedHat style Linux systems.
* XDG\_CONFIG\_DIR - path to install oyranos-monitor-setup.desktop; defaults to /etc/xdg/autostart/
* USE\_GCOV - enable gcov/lcov compiler flags on the Unix platform and the coverage target
* USE\_SYSTEM\_ELEKTRA - fail if not found
* USE\_SYSTEM\_LIBXCM - fail if not found; default is shipped fallback version
* USE\_SYSTEM\_OPENICC - fail if not found; default is shipped fallback version
* USE\_SYSTEM\_YAJL - fail if not found; default is shipped fallback version
