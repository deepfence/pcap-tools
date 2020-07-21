/* Copyright (C) 2020 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

use super::ssh::SSHTransaction;
use crate::jsonbuilder::{JsonBuilder, JsonError};

fn log_ssh(tx: &SSHTransaction, js: &mut JsonBuilder) -> Result<bool, JsonError> {
    if tx.cli_hdr.protover.len() == 0 && tx.srv_hdr.protover.len() == 0 {
        return Ok(false);
    }
    if tx.cli_hdr.protover.len() > 0 {
        js.open_object("client")?;
        js.set_string_from_bytes("proto_version", &tx.cli_hdr.protover)?;
        if tx.cli_hdr.swver.len() > 0 {
            js.set_string_from_bytes("software_version", &tx.cli_hdr.swver)?;
        }
        if tx.cli_hdr.hassh.len() > 0 {
            js.set_string_from_bytes("hassh", &tx.cli_hdr.hassh)?;
        }
        if tx.cli_hdr.hassh_string.len() > 0 {
            js.set_string_from_bytes("hassh.string", &tx.cli_hdr.hassh_string)?;
        }
        js.close()?;
    }
    if tx.srv_hdr.protover.len() > 0 {
        js.open_object("server")?;
        js.set_string_from_bytes("proto_version", &tx.srv_hdr.protover)?;
        if tx.srv_hdr.swver.len() > 0 {
            js.set_string_from_bytes("software_version", &tx.srv_hdr.swver)?;
        }
        if tx.srv_hdr.hassh.len() > 0 {
            js.set_string_from_bytes("hassh", &tx.srv_hdr.hassh)?;
        }
        if tx.srv_hdr.hassh_string.len() > 0 {
            js.set_string_from_bytes("hassh.string", &tx.srv_hdr.hassh_string)?;
        }
        js.close()?;
    }
    return Ok(true);
}

#[no_mangle]
pub extern "C" fn rs_ssh_log_json(tx: *mut std::os::raw::c_void, js: &mut JsonBuilder) -> bool {
    let tx = cast_pointer!(tx, SSHTransaction);
    if let Ok(x) = log_ssh(tx, js) {
        return x;
    }
    return false;
}
