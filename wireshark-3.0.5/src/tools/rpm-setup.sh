#!/bin/bash
# Setup development environment for RPM based systems such as Red Hat, Centos, Fedora, openSUSE
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# We drag in tools that might not be needed by all users; it's easier
# that way.
#

if [ "$1" = "--help" ]
then
	echo "\nUtility to setup a rpm-based system for Wireshark Development.\n"
	echo "The basic usage installs the needed software\n\n"
	echo "Usage: $0 [--install-optional] [...other options...]\n"
	echo "\t--install-optional: install optional software as well"
	echo "\t[other]: other options are passed as-is to the packet manager\n"
	exit 1
fi

# Check if the user is root
if [ $(id -u) -ne 0 ]
then
	echo "You must be root."
	exit 1
fi

for op
do
	if [ "$op" = "--install-optional" ]
	then
		ADDITIONAL=1
	else
		OPTIONS="$OPTIONS $op"
	fi
done

BASIC_LIST="gcc \
	gcc-c++ \
	flex \
	bison \
	perl \
	desktop-file-utils \
	git \
	git-review \
	glib2-devel \
	libpcap-devel \
	zlib-devel"

ADDITIONAL_LIST="libnl3-devel \
	libnghttp2-devel \
	libcap-devel \
	libgcrypt-devel \
	libssh-devel \
	krb5-devel \
	perl-Parse-Yapp \
	sbc-devel \
	libsmi-devel \
	snappy-devel \
	lz4 \
	doxygen \
	libxml2-devel \
	spandsp-devel \
	systemd-devel \
	rpm-build"

# Guess which package manager we will use
for PM in zypper dnf yum ''; do
	if type "$PM" >/dev/null 2>&1; then
		break
	fi
done

if [ -z $PM ]
then
	echo "No package managers found, exiting"
	exit 1
fi

case $PM in
	zypper)
		PM_OPT="--non-interactive"
		PM_SEARCH="search -x --provides"
		;;
	dnf)
		PM_SEARCH="info"
		;;
	yum)
		PM_SEARCH="info"
		;;
esac

echo "Using $PM ($PM_SEARCH)"

# Adds package $2 to list variable $1 if the package is found
add_package() {
	local list="$1" pkgname="$2"

	# fail if the package is not known
	$PM $PM_SEARCH "$pkgname" &> /dev/null || return 1

	# package is found, append it to list
	eval "${list}=\"\${${list}} \${pkgname}\""
}

# python3: OpenSUSE 43.3, Fedora 26
# python34: Centos 7
add_package BASIC_LIST python3 || add_package BASIC_LIST python34 ||
echo "python3 is unavailable" >&2

add_package BASIC_LIST cmake3 || add_package BASIC_LIST cmake ||
echo "cmake is unavailable" >&2

add_package BASIC_LIST glib2 || add_package BASIC_LIST libglib-2_0-0 ||
echo "glib2 is unavailable" >&2

# lua51, lua51-devel: OpenSUSE Leap 42.3 (lua would be fine too, as it installs lua52), OpenSUSE Leap 15.0 (lua installs lua53, so it wouldn't work)
# compat-lua, compat-lua-devel: Fedora 28, Fedora 29
# lua, lua-devel: CentOS 7
add_package BASIC_LIST lua51-devel || add_package BASIC_LIST compat-lua-devel || add_package BASIC_LIST lua-devel ||
echo "lua devel is unavailable" >&2

add_package BASIC_LIST lua51 || add_package BASIC_LIST compat-lua || add_package BASIC_LIST lua ||
echo "lua is unavailable" >&2

add_package BASIC_LIST libpcap || add_package BASIC_LIST libpcap1 ||
echo "libpcap is unavailable" >&2

add_package BASIC_LIST zlib || add_package BASIC_LIST libz1 ||
echo "zlib is unavailable" >&2

add_package BASIC_LIST c-ares-devel || add_package BASIC_LIST libcares-devel ||
echo "libcares-devel is unavailable" >&2

add_package BASIC_LIST qt-devel ||
echo "Qt5 devel is unavailable" >&2

add_package BASIC_LIST qt5-qtbase-devel ||
echo "Qt5 base devel is unavailable" >&2

add_package BASIC_LIST qt5-linguist || add_package BASIC_LIST libqt5-linguist-devel ||
echo "Qt5 linguist is unavailable" >&2

add_package BASIC_LIST qt5-qtsvg-devel || add_package BASIC_LIST libqt5-qtsvg-devel ||
echo "Qt5 svg is unavailable" >&2

add_package BASIC_LIST qt5-qtmultimedia-devel || add_package BASIC_LIST libqt5-qtmultimedia-devel ||
echo "Qt5 multimedia is unavailable" >&2

add_package BASIC_LIST libQt5PrintSupport-devel ||
echo "Qt5 print support is unavailable" >&2

# This in only required (and available) on OpenSUSE
add_package BASIC_LIST update-desktop-files ||
echo "update-desktop-files is unavailable" >&2

add_package BASIC_LIST perl-podlators ||
echo "perl-podlators unavailable" >&2

# libcap: CentOS 7, Fedora 28, Fedora 29
# libcap2: OpenSUSE Leap 42.3, OpenSUSE Leap 15.0
add_package ADDITIONAL_LIST libcap || add_package ADDITIONAL_LIST libcap2 ||
echo "libcap is unavailable" >&2

add_package ADDITIONAL_LIST nghttp2 || add_package ADDITIONAL_LIST libnghttp2 ||
echo "nghttp2 is unavailable" >&2

add_package ADDITIONAL_LIST snappy || add_package ADDITIONAL_LIST libsnappy1 ||
echo "snappy is unavailable" >&2

add_package ADDITIONAL_LIST lz4-devel || add_package ADDITIONAL_LIST liblz4-devel ||
echo "lz4 devel is unavailable" >&2

add_package ADDITIONAL_LIST libcap-progs || echo "cap progs are unavailable" >&2

add_package ADDITIONAL_LIST libmaxminddb-devel ||
echo "MaxMind DB devel is unavailable" >&2

add_package ADDITIONAL_LIST gnutls-devel || add_package ADDITIONAL_LIST libgnutls-devel ||
echo "gnutls devel is unavailable" >&2

add_package ADDITIONAL_LIST gettext-devel || add_package ADDITIONAL_LIST gettext-tools ||
echo "Gettext devel is unavailable" >&2

add_package ADDITIONAL_LIST perl-Pod-Html ||
echo "perl-Pod-Html is unavailable" >&2

add_package ADDITIONAL_LIST asciidoctor || add_package ADDITIONAL_LIST rubygem-asciidoctor.noarch ||
echo "asciidoctor is unavailable" >&2

add_package ADDITIONAL_LIST ninja || add_package ADDITIONAL_LIST ninja-build ||
echo "ninja is unavailable" >&2

add_package ADDITIONAL_LIST libxslt || add_package ADDITIONAL_LIST libxslt1 ||
echo "xslt is unavailable" >&2

ACTUAL_LIST=$BASIC_LIST

# Now arrange for optional support libraries
if [ $ADDITIONAL ]
then
	ACTUAL_LIST="$ACTUAL_LIST $ADDITIONAL_LIST"
fi

$PM $PM_OPT install $ACTUAL_LIST $OPTIONS

# Now arrange for optional support libraries
if [ ! $ADDITIONAL ]
then
	echo -e "\n*** Optional packages not installed. Rerun with --install-optional to have them.\n"
fi
