/* file-ruby_marshal.h
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#ifndef __FILE_RBM_H__
#define __FILE_RBM_H__

// Dissect one ruby marshal object
void dissect_rbm_inline(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree, gint* offset, gchar** type, gchar** value);

// Extract a ruby marshal integer
void get_rbm_integer(tvbuff_t* tvb, guint offset, gint32* value, guint* len);

#endif
