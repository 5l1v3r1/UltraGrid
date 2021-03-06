# see https://en.opensuse.org/openSUSE:Build_Service_Debian_builds#packageName.dsc
DEBTRANSFORM-TAR:	ultragrid-1.5.tar.bz2
DEBTRANSFORM-FILES-TAR:	debian.tar.gz
DEBTRANSFORM-SERIES:	debian-patches-series
Format: 1.0
Source: ultragrid
Binary: ultragrid
Architecture: any
Version: 1.5-2018100400
Maintainer: 	Lukas Rucka <xrucka@fi.muni.cz>
Build-Depends: 	debhelper (>= 8.0), build-essential, make, autoconf, automake, autotools-dev, libmagickwand-dev, libjpeg-dev, freeglut3-dev, libglew1.6-dev, libsdl2-mixer-dev, libsdl2-ttf-dev, libsdl2-dev, qt5-qmake, qtbase5-dev-tools, libxxf86vm1, libx11-6, libxdamage1, portaudio19-dev, libjack-dev, libasound2-dev, libv4l-dev, zip, libavcodec-dev, liblivemedia-dev, libopencv-dev, libssl-dev, libvdpau-dev, libva-dev, libgpujpeg-dev, libcairo2-dev, ultragrid-proprietary-drivers-release-1.5, libglib2.0-dev, libcurl4-openssl-dev, git, nvidia-cuda-toolkit (>= 5.0), clang, qtbase5-dev
