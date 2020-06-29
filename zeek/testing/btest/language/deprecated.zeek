# @TEST-EXEC: zeek -b no-warnings.zeek >no-warnings.out 2>&1
# @TEST-EXEC: TEST_DIFF_CANONIFIER=$SCRIPTS/diff-remove-abspath btest-diff no-warnings.out

# @TEST-EXEC: zeek -b warnings.zeek >warnings.out 2>&1
# @TEST-EXEC: TEST_DIFF_CANONIFIER=$SCRIPTS/diff-remove-abspath btest-diff warnings.out

@TEST-START-FILE no-warnings.zeek
type blah: string &deprecated;

global my_event: event(arg: string) &deprecated;

global my_hook: hook(arg: string) &deprecated;

type my_record: record {
	a: count &default = 1;
	b: string &optional &deprecated;
};

type my_enum: enum {
	RED,
	GREEN &deprecated,
	BLUE &deprecated
};

type my_other_enum: enum {
	ZERO = 0,
	ONE = 1 &deprecated,
	TWO = 2 &deprecated,
};

event zeek_init()
	{
	print ZERO;
	print ONE;
	print TWO;
	print RED;
	print GREEN;
	print BLUE;

	local l: blah = "testing";

	local ls: string = " test";

	event my_event("generate my_event please");
	schedule 1sec { my_event("schedule my_event please") };
	hook my_hook("generate my_hook please");

	local mr = my_record($a = 3, $b = "yeah");
	mr = [$a = 4, $b = "ye"];
	mr = record($a = 5, $b = "y");

	if ( ! mr?$b )
		mr$b = "nooooooo";

	mr$a = 2;
	mr$b = "noooo";
	}

event my_event(arg: string)
	{
	print arg;
	}

hook my_hook(arg: string)
	{
	print arg;
	}

function hmm(b: blah)
	{
	print b;
	}

global dont_use_me: function() &deprecated;

function dont_use_me()
	{
	dont_use_me();
	}

function dont_use_me_either() &deprecated
	{
	dont_use_me_either();
	}
@TEST-END-FILE

@TEST-START-FILE warnings.zeek
type blah: string &deprecated="type warning";

global my_event: event(arg: string) &deprecated="event warning";

global my_hook: hook(arg: string) &deprecated="hook warning";

type my_record: record {
	a: count &default = 1;
	b: string &optional &deprecated="record warning";
};

type my_enum: enum {
	RED,
	GREEN &deprecated="green warning",
	BLUE &deprecated="red warning"
};

type my_other_enum: enum {
	ZERO = 0,
	ONE = 1 &deprecated="one warning",
	TWO = 2 &deprecated="two warning",
};

event zeek_init()
	{
	print ZERO;
	print ONE;
	print TWO;
	print RED;
	print GREEN;
	print BLUE;

	local l: blah = "testing";

	local ls: string = " test";

	event my_event("generate my_event please");
	schedule 1sec { my_event("schedule my_event please") };
	hook my_hook("generate my_hook please");

	local mr = my_record($a = 3, $b = "yeah");
	mr = [$a = 4, $b = "ye"];
	mr = record($a = 5, $b = "y");

	if ( ! mr?$b )
		mr$b = "nooooooo";

	mr$a = 2;
	mr$b = "noooo";
	}

event my_event(arg: string)
	{
	print arg;
	}

hook my_hook(arg: string)
	{
	print arg;
	}

function hmm(b: blah)
	{
	print b;
	}

global dont_use_me: function() &deprecated="global function warning";

function dont_use_me()
	{
	dont_use_me();
	}

function dont_use_me_either() &deprecated="function warning"
	{
	dont_use_me_either();
	}
@TEST-END-FILE
