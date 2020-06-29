#!/bin/bash
#
# Compare ABIs of two Wireshark working copies
#
# Copyright 2013 Balint Reczey <balint at balintreczey.hu>
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Tested with abi-compliance-checker 1.96.1

function acc () {
	LIBNAME=$1
	DIR=$2
	# compare only dumped ABI descriptions first, then fall back to full comparison
	# if no difference is found
	if abi-compliance-checker -l $LIBNAME \
		-d1 $V1_PATH/$DIR/$REL_DUMP_PATH/$LIBNAME.abi.tar.gz \
		-d2 $V2_PATH/$DIR/$REL_DUMP_PATH/$LIBNAME.abi.tar.gz ; then
		abi-compliance-checker -l $LIBNAME \
			-d1 $V1_PATH/$DIR/abi-descriptor.xml -relpath1 $V1_PATH/$DIR \
			-v1 `ls  $V1_PATH/$DIR/$REL_LIB_PATH/$LIBNAME.so.?.*.*|sed 's/.*\.so\.//'` \
			-d2 $V2_PATH/$DIR/abi-descriptor.xml -relpath2 $V2_PATH/$DIR \
			-v2 `ls  $V2_PATH/$DIR/$REL_LIB_PATH/$LIBNAME.so.?.*.*|sed 's/.*\.so\.//'` \
			-check-implementation
	fi
}

V1_PATH=$1
V2_PATH=$2

# both working copies have to be built first with cmake
# make -C $V1_PATH all dumpabi
# make -C $V2_PATH all dumpabi

REL_LIB_PATH=../lib
REL_DUMP_PATH=.

acc libwiretap wiretap $V1_PATH $V2_PATH
RET=$?
acc libwsutil wsutil $V1_PATH $V2_PATH
RET=$(($RET + $?))
acc libwireshark epan $V1_PATH $V2_PATH
exit $(($RET + $?))

