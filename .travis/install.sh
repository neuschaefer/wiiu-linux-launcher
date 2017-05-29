#!/bin/sh
# Install a toolchain

if [ "$TOOLCHAIN" = debian ]; then
	sudo apt-get install gcc-powerpc-linux-gnu gcc-arm-none-eabi
elif [ "$TOOLCHAIN" = devkitppc ]; then
	wget https://raw.githubusercontent.com/devkitPro/installer/master/perl/devkitPPCupdate.pl
	perl ./devkitPPCupdate.pl
	wget https://raw.githubusercontent.com/devkitPro/installer/master/perl/devkitARMupdate.pl
	perl ./devkitARMupdate.pl
else
	echo "Unknown toolchain <$TOOLCHAIN>"
	exit 1
fi
