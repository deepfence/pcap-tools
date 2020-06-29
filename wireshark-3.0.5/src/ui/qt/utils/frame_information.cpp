/* frame_information.cpp
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <epan/epan_dissect.h>
#include "epan/epan.h"
#include "epan/column.h"
#include "epan/ftypes/ftypes.h"

#include "wiretap/wtap.h"

#include "cfile.h"
#include "file.h"
#include <ui/qt/capture_file.h>

#include "frame_tvbuff.h"

#include <stdint.h>

#include <ui/qt/utils/frame_information.h>

FrameInformation::FrameInformation(CaptureFile * capfile, frame_data * fi, QObject * parent)
:QObject(parent),
 fi_(fi),
 cap_file_(capfile)
{
    wtap_rec_init(&rec_);
    ws_buffer_init(&buf_, 1500);
    loadFrameTree();
}

void FrameInformation::loadFrameTree()
{
    if ( ! fi_ || ! cap_file_ || !cap_file_->capFile())
        return;

    if (!cf_read_record_r(cap_file_->capFile(), fi_, &rec_, &buf_))
        return;

    /* proto tree, visible. We need a proto tree if there's custom columns */
    epan_dissect_init(&edt_, cap_file_->capFile()->epan, TRUE, TRUE);
    col_custom_prime_edt(&edt_, &(cap_file_->capFile()->cinfo));

    epan_dissect_run(&edt_, cap_file_->capFile()->cd_t, &rec_,
                     frame_tvbuff_new_buffer(&cap_file_->capFile()->provider, fi_, &buf_),
                     fi_, &(cap_file_->capFile()->cinfo));
    epan_dissect_fill_in_columns(&edt_, TRUE, TRUE);
}

FrameInformation::~FrameInformation()
{
    epan_dissect_cleanup(&edt_);
    wtap_rec_cleanup(&rec_);
    ws_buffer_free(&buf_);
}

bool FrameInformation::isValid()
{
    bool ret = false;

    if ( fi_ && cap_file_ && edt_.tvb )
    {
        ret = true;
    }

    return ret;
}

frame_data * FrameInformation::frameData() const
{
    return fi_;
}

int FrameInformation::frameNum() const
{
    if ( ! fi_ )
        return -1;
    return fi_->num;
}

const QByteArray FrameInformation::printableData()
{
    if ( ! fi_ )
        return QByteArray();


    int length = tvb_captured_length(edt_.tvb);
    const char *data = (const char *)tvb_get_ptr(edt_.tvb, 0, length);
    return QByteArray(data, length);
}
/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
