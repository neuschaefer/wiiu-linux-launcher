#!/bin/sh

VERSION=`git describe --always --abbrev=12`
RELEASE=`git log -n1 "--pretty=format:%ad" "--date=format:%Y%m%d%H%M%S"`

if [ -z "$VERSION" ]; then VERSION=unknown; fi
if [ -z "$RELEASE" ]; then RELEASE=unknown; fi

cat << __EOF__
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<app version="1">
	<name>Wii U Linux Launcher</name>
	<coder>jn</coder>
	<version>$VERSION</version>
	<release_date>$RELEASE</release_date>
	<short_description>Run linux on your Wii U (WIP)</short_description>
	<long_description>
		This program loads a linux kernel/initrd/devicetree from the SD
		card and (more or less) performs kexec to run it.

		Work in progress.
	</long_description>
</app>
__EOF__
