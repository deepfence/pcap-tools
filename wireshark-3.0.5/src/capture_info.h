/* capture_info.h
 * capture info functions
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


/** @file
 *
 * Capture info functions.
 *
 */

#ifndef __CAPTURE_INFO_H__
#define __CAPTURE_INFO_H__

#include "capture_opts.h"
#include <capchild/capture_session.h>
#include <epan/capture_dissectors.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Current Capture info. */
typedef struct _capture_info {
    /* handle */
    gpointer        ui;             /**< user interface handle */

    /* capture info */
    packet_counts   *counts;        /**< protocol specific counters */
    gint            new_packets;    /**< packets since last update */
} capture_info;

typedef struct _info_data {
    packet_counts     counts;     /* Packet counting */
    struct wtap*      wtap;       /* current wtap file */
    capture_info      ui;         /* user interface data */
} info_data_t;

/* new packets arrived - read from wtap, count */
extern void capture_info_new_packets(int to_read, info_data_t* cap_info);

/** Create the capture info dialog */
extern void
capture_info_ui_create(capture_info *cinfo, capture_session *cap_session);

/** Update the capture info counters in the dialog */
extern void capture_info_ui_update(
capture_info    *cinfo);

/** Destroy the capture info dialog again */
extern void capture_info_ui_destroy(
capture_info    *cinfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* capture_info.h */

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
