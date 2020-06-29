#!/bin/sh

#
# Fix up ownership and permissions on /Library/Application Support/Wireshark;
# for some reason, it's not being owned by root:wheel, and it's not
# publicly readable and, for directories and executables, not publicly
# searchable/executable.
#
# Also take away group write permission.
#
# XXX - that may be a problem with the process of building the installer
# package; if so, that's where it *should* be fixed.
#
chown -R root:wheel "/Library/Application Support/Wireshark"
chmod -R a+rX,go-w "/Library/Application Support/Wireshark"

CHMOD_BPF_PLIST="/Library/LaunchDaemons/org.wireshark.ChmodBPF.plist"
BPF_GROUP="access_bpf"
BPF_GROUP_NAME="BPF device access ACL"

dscl . -read /Groups/"$BPF_GROUP" > /dev/null 2>&1 || \
    dseditgroup -q -o create "$BPF_GROUP"
dseditgroup -q -o edit -a "$USER" -t user "$BPF_GROUP"

cp "/Library/Application Support/Wireshark/ChmodBPF/org.wireshark.ChmodBPF.plist" \
    "$CHMOD_BPF_PLIST"
chmod u=rw,g=r,o=r "$CHMOD_BPF_PLIST"
chown root:wheel "$CHMOD_BPF_PLIST"

rm -rf /Library/StartupItems/ChmodBPF

launchctl load "$CHMOD_BPF_PLIST"
