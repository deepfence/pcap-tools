# This tests Zeek's mechanism to prevent duplicate script loading.
#
# @TEST-EXEC: mkdir -p foo/bar
# @TEST-EXEC: echo "@load bar/test" >loader.zeek
# @TEST-EXEC: cp %INPUT foo/bar/test.zeek
# @TEST-EXEC: cp %INPUT foo/bar/test2.zeek
#
# @TEST-EXEC: ZEEKPATH=$ZEEKPATH:.:./foo zeek -b misc/loaded-scripts loader bar/test
# @TEST-EXEC: ZEEKPATH=$ZEEKPATH:.:./foo zeek -b misc/loaded-scripts loader bar/test.zeek
# @TEST-EXEC: ZEEKPATH=$ZEEKPATH:.:./foo zeek -b misc/loaded-scripts loader foo/bar/test
# @TEST-EXEC: ZEEKPATH=$ZEEKPATH:.:./foo zeek -b misc/loaded-scripts loader foo/bar/test.zeek
# @TEST-EXEC: ZEEKPATH=$ZEEKPATH:.:./foo zeek -b misc/loaded-scripts loader `pwd`/foo/bar/test.zeek
# @TEST-EXEC-FAIL: ZEEKPATH=$ZEEKPATH:.:./foo zeek -b misc/loaded-scripts loader bar/test2

global pi = 3.14;
