#!/usr/bin/env perl
#
# Copyright 2011 Dirk Jagdmann <doj@cubic.org>
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# SPDX-License-Identifier: GPL-2.0-or-later


# perl program to convert a Wireshark color scheme to javascript
# code. The javascript function should then be inserted into the
# pdml2html.xsl file.
#
# run this as: perl tools/colorfilters2js.pl colorfilters

print<<'EOF';
function set_node_color(node,colorname)
{
  if(dojo.isString(node))
    node = dojo.byId(node);
  if(!node) return;
  var fg;
  var bg;
EOF

my $elseflow = "";

while(<>)
{
    if(/\@(.+?)\@.+\[(\d+),(\d+),(\d+)\]\[(\d+),(\d+),(\d+)\]/)
    {
	print "  " . $elseflow . "if (colorname == '$1') {\n";
	printf("    bg='#%02x%02x%02x';\n", $2/256, $3/256, $4/256);
	printf("    fg='#%02x%02x%02x';\n", $5/256, $6/256, $7/256);
	print "  }\n";
    }
    $elseflow = "else ";
}

print<<'EOF';
  if(fg.length > 0)
    node.style.color = fg;
  if(bg.length > 0)
    node.style.background = bg;
}
EOF

exit 0;
