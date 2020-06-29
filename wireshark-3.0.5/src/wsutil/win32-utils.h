/* win32-utils.h
 * Windows utility definitions
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2006 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __WIN32UTIL_H__
#define __WIN32UTIL_H__

/*
 * This is included in ABI checking, so protect it with #ifdef _WIN32,
 * so it doesn't break ABI checking on UN*X.
 */
#ifdef _WIN32

#include "ws_symbol_export.h"

#include <glib.h>
#include <windows.h>

/**
 * @file
 * Unicode convenience routines.
 */

#ifdef	__cplusplus
extern "C" {
#endif

/** Quote the argument element if necessary, so that it will get
 * reconstructed correctly in the C runtime startup code.  Note that
 * the unquoting algorithm in the C runtime is really weird, and
 * rather different than what Unix shells do. See stdargv.c in the C
 * runtime sources (in the Platform SDK, in src/crt).
 *
 * Stolen from GLib's protect_argv(), an internal routine that quotes
 * string in an argument list so that they arguments will be handled
 * correctly in the command-line string passed to CreateProcess()
 * if that string is constructed by gluing those strings together.
 *
 * @param argv The string to be quoted.  May be NULL.
 * @return The string quoted to be used by CreateProcess
 */
WS_DLL_PUBLIC
gchar * protect_arg (const gchar *argv);

/** Generate a string for a Windows error.
 *
 * @param error The Windows error code
 * @return a localized UTF-8 string containing the corresponding error message
 */
WS_DLL_PUBLIC
const char * win32strerror(DWORD error);

/** Generate a string for a Win32 exception code.
 *
 * @param exception The exception code
 * @return a non-localized string containing the error message
 */
WS_DLL_PUBLIC
const char * win32strexception(DWORD exception);

/**
 * @brief ws_pipe_create_process Create a process and assign it to the main application
 *        job object so that it will be killed the the main application exits.
 * @param application_name Application name. Will be converted to its UTF-16 equivalent or NULL.
 * @param command_line Command line. Will be converted to its UTF-16 equivalent.
 * @param process_attributes Same as CreateProcess.
 * @param thread_attributes Same as CreateProcess.
 * @param inherit_handles Same as CreateProcess.
 * @param creation_flags Will be ORed with CREATE_SUSPENDED|CREATE_BREAKAWAY_FROM_JOB.
 * @param environment Same as CreateProcess.
 * @param current_directory Current directory. Will be converted to its UTF-16 equivalent or NULL.
 * @param startup_info Same as CreateProcess.
 * @param process_information Same as CreateProcess.
 * @return
 */
WS_DLL_PUBLIC
BOOL win32_create_process(const char *application_name, const char *command_line,
    LPSECURITY_ATTRIBUTES process_attributes, LPSECURITY_ATTRIBUTES thread_attributes,
    BOOL inherit_handles, DWORD creation_flags, LPVOID environment,
    const char *current_directory, LPSTARTUPINFO startup_info, LPPROCESS_INFORMATION process_information
);

#ifdef	__cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* __WIN32UTIL_H__ */
