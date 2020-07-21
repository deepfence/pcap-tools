/* Copyright (C) 2019 Open Information Security Foundation
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

// Author: Pierre Chifflier <chifflier@wzdftpd.net>
use nom::error::{ErrorKind, ParseError};

#[derive(Debug)]
pub enum SmbError {
    BadEncoding,
    RecordTooSmall,
    NomError(ErrorKind),
}

impl<I> ParseError<I> for SmbError {
    fn from_error_kind(_input: I, kind: ErrorKind) -> Self {
        SmbError::NomError(kind)
    }
    fn append(_input: I, kind: ErrorKind, _other: Self) -> Self {
        SmbError::NomError(kind)
    }
}
