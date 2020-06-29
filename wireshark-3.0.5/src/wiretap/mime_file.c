/* mime_file.c
 *
 * MIME file format decoder for the Wiretap library.
 *
 * This is for use with Wireshark dissectors that handle file
 * formats (e.g., because they handle a particular MIME media type).
 * It breaks the file into chunks of at most WTAP_MAX_PACKET_SIZE_STANDARD,
 * each of which is reported as a packet, so that files larger than
 * WTAP_MAX_PACKET_SIZE_STANDARD can be handled by reassembly.
 *
 * The "MIME file" dissector does the reassembly, and hands the result
 * off to heuristic dissectors to try to identify the file's contents.
 *
 * Wiretap Library
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wtap-int.h"
#include "file_wrappers.h"
#include <wsutil/buffer.h>
#include "mime_file.h"

typedef struct {
	const guint8 *magic;
	guint magic_len;
} mime_files_t;

/*
 * Written by Marton Nemeth <nm127@freemail.hu>
 * Copyright 2009 Marton Nemeth
 * The JPEG and JFIF specification can be found at:
 *
 * http://www.jpeg.org/public/jfif.pdf
 * http://www.w3.org/Graphics/JPEG/itu-t81.pdf
 */
static const guint8 jpeg_jfif_magic[] = { 0xFF, 0xD8, /* SOF */
					  0xFF        /* start of the next marker */
					};

/* <?xml */
static const guint8 xml_magic[]    = { '<', '?', 'x', 'm', 'l' };
static const guint8 png_magic[]    = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
static const guint8 gif87a_magic[] = { 'G', 'I', 'F', '8', '7', 'a'};
static const guint8 gif89a_magic[] = { 'G', 'I', 'F', '8', '9', 'a'};
static const guint8 elf_magic[]    = { 0x7F, 'E', 'L', 'F'};
static const guint8 btsnoop_magic[]    = { 'b', 't', 's', 'n', 'o', 'o', 'p', 0};
static const guint8 pcap_magic[]           = { 0xA1, 0xB2, 0xC3, 0xD4 };
static const guint8 pcap_swapped_magic[]   = { 0xD4, 0xC3, 0xB2, 0xA1 };
static const guint8 pcapng_premagic[]      = { 0x0A, 0x0D, 0x0D, 0x0A };

/* File does not start with it */
static const guint8 pcapng_xmagic[]         = { 0x1A, 0x2B, 0x3C, 0x4D };
static const guint8 pcapng_swapped_xmagic[] = { 0x4D, 0x3C, 0x2B, 0x1A };

static const mime_files_t magic_files[] = {
	{ jpeg_jfif_magic, sizeof(jpeg_jfif_magic) },
	{ xml_magic, sizeof(xml_magic) },
	{ png_magic, sizeof(png_magic) },
	{ gif87a_magic, sizeof(gif87a_magic) },
	{ gif89a_magic, sizeof(gif89a_magic) },
	{ elf_magic, sizeof(elf_magic) },
	{ btsnoop_magic, sizeof(btsnoop_magic) },
	{ pcap_magic, sizeof(pcap_magic) },
	{ pcap_swapped_magic, sizeof(pcap_swapped_magic) },
	{ pcapng_premagic, sizeof(pcapng_premagic) }
};

#define	N_MAGIC_TYPES	(sizeof(magic_files) / sizeof(magic_files[0]))

wtap_open_return_val
mime_file_open(wtap *wth, int *err, gchar **err_info)
{
	char magic_buf[128]; /* increase buffer size when needed */
	int bytes_read;
	gboolean found_file;
	/* guint file_ok; */
	guint i;

	guint read_bytes = 12;

	for (i = 0; i < N_MAGIC_TYPES; i++)
		read_bytes = MAX(read_bytes, magic_files[i].magic_len);

	read_bytes = (guint)MIN(read_bytes, sizeof(magic_buf));
	bytes_read = file_read(magic_buf, read_bytes, wth->fh);

	if (bytes_read < 0) {
		*err = file_error(wth->fh, err_info);
		return WTAP_OPEN_ERROR;
	}
	if (bytes_read == 0)
		return WTAP_OPEN_NOT_MINE;

	found_file = FALSE;
	for (i = 0; i < N_MAGIC_TYPES; i++) {
		if ((guint) bytes_read >= magic_files[i].magic_len && !memcmp(magic_buf, magic_files[i].magic, MIN(magic_files[i].magic_len, (guint) bytes_read))) {
			if (!found_file) {
				if (magic_files[i].magic == pcapng_premagic) {
					if (memcmp(magic_buf + 8, pcapng_xmagic, sizeof(pcapng_xmagic)) &&
							memcmp(magic_buf + 8, pcapng_swapped_xmagic, sizeof(pcapng_swapped_xmagic)))
						continue;
				}
				found_file = TRUE;
			} else
				return WTAP_OPEN_NOT_MINE;	/* many files matched, bad file */
		}
	}

	if (!found_file)
		return WTAP_OPEN_NOT_MINE;

	if (file_seek(wth->fh, 0, SEEK_SET, err) == -1)
		return WTAP_OPEN_ERROR;

	wth->file_type_subtype = WTAP_FILE_TYPE_SUBTYPE_MIME;
	wth->file_encap = WTAP_ENCAP_MIME;
	wth->file_tsprec = WTAP_TSPREC_SEC;
	wth->subtype_read = wtap_full_file_read;
	wth->subtype_seek_read = wtap_full_file_seek_read;
	wth->snapshot_length = 0;

	return WTAP_OPEN_MINE;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
