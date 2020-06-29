# @TEST-EXEC: zeek -b base/misc/version.zeek %INPUT
# @TEST-EXEC: btest-diff .stdout
# @TEST-EXEC: TEST_DIFF_CANONIFIER="$SCRIPTS/diff-remove-abspath" btest-diff .stderr

# good versions
print Version::parse("1.5");
print Version::parse("2.0");
print Version::parse("2.6");
print Version::parse("2.5-beta");
print Version::parse("2.5.1-debug");
print Version::parse("2.5-beta-12");
print Version::parse("2.5-12-debug");
print Version::parse("2.5.2-beta-12-debug");
print Version::parse("2.5.2-beta5-12-debug");
print Version::parse("1.12.20-beta-2562-debug");
print Version::parse("2.6-936");
print Version::parse("12.5");
print Version::parse("3.0.0");
print Version::parse("3.0.1");
print Version::parse("3.1.0");
print Version::parse("3.0.0-rc");
print Version::parse("3.0.0-rc.37");
print Version::parse("3.0.0-rc2.13");
print Version::parse("3.0.0-rc.37-debug");
print Version::parse("3.0.0-rc2.13-debug");
print Version::parse("3.1.0-dev.42");
print Version::parse("3.1.0-dev.42-debug");

# bad versions
print Version::parse("1");
print Version::parse("1.12-beta-drunk");
print Version::parse("JustARandomString");

# check that current running version of Zeek parses without error
Version::parse(zeek_version());

@TEST-START-NEXT

@if ( Version::number >= 20500 )
print "yup";
@endif

@if ( Version::parse("1.5")$version_number < 20500 )
print "yup";
@endif

@if ( Version::at_least("2.5") )
print "yup";
@endif

@if ( Version::at_least("99.9") )
print "Either something broke or the unit test didn't plan to survive this far into the future";
@endif
