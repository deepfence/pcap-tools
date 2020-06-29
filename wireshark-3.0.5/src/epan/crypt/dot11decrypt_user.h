/* dot11decrypt_user.h
 *
 * Copyright (c) 2006 CACE Technologies, Davis (California)
 * All rights reserved.
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
 */

#ifndef	_DOT11DECRYPT_USER_H
#define	_DOT11DECRYPT_USER_H

/******************************************************************************/
/*	File includes																					*/
/*																										*/
#include "dot11decrypt_interop.h"
#include "ws_symbol_export.h"

/*																										*/
/*																										*/
/******************************************************************************/

/******************************************************************************/
/*	Constant definitions																			*/
/*																										*/
/*	Decryption key types																			*/
#define	DOT11DECRYPT_KEY_TYPE_WEP		0
#define	DOT11DECRYPT_KEY_TYPE_WEP_40	1
#define	DOT11DECRYPT_KEY_TYPE_WEP_104	2
#define	DOT11DECRYPT_KEY_TYPE_WPA_PWD	3
#define	DOT11DECRYPT_KEY_TYPE_WPA_PSK	4
#define	DOT11DECRYPT_KEY_TYPE_WPA_PMK	5
#define	DOT11DECRYPT_KEY_TYPE_TKIP		6
#define	DOT11DECRYPT_KEY_TYPE_CCMP		7

/*	Decryption algorithms fields size definition (bytes)								*/
#define	DOT11DECRYPT_WEP_KEY_MINLEN		1
#define	DOT11DECRYPT_WEP_KEY_MAXLEN		32
#define	DOT11DECRYPT_WEP_40_KEY_LEN		5
#define	DOT11DECRYPT_WEP_104_KEY_LEN	13

#define	DOT11DECRYPT_WPA_PASSPHRASE_MIN_LEN	8
#define	DOT11DECRYPT_WPA_PASSPHRASE_MAX_LEN	63	/* null-terminated string, the actual length of the storage is 64	*/
#define	DOT11DECRYPT_WPA_SSID_MIN_LEN			0
#define	DOT11DECRYPT_WPA_SSID_MAX_LEN			32
#define	DOT11DECRYPT_WPA_PSK_LEN				32
/*																										*/
/*																										*/
/******************************************************************************/

/******************************************************************************/
/*	Macro definitions																				*/
/*																										*/
/*																										*/
/******************************************************************************/

/******************************************************************************/
/*	Type definitions																				*/
/*																										*/
/**
 * Struct to store info about a specific decryption key.
 */
typedef struct {
    GString    *key;
    GByteArray *ssid;
    guint       bits;
    guint       type;
} decryption_key_t;

/**
 * Key item used during the decryption process.
 */
typedef struct _DOT11DECRYPT_KEY_ITEM {
	/**
	 * Type of key. The type will remain unchanged during the
	 * processing, even if some fields could be changed (e.g., WPA
	 * fields).
	 * @note
	 * You can use constants DOT11DECRYPT_KEY_TYPE_xxx to indicate the
	 * key type.
	 */
	UINT8 KeyType;

	/**
	 * Key data.
	 * This field can be used for the following decryptographic
	 * algorithms: WEP-40, with a key of 40 bits (10 hex-digits);
	 * WEP-104, with a key of 104 bits (or 26 hex-digits); WPA or
	 * WPA2.
	 * @note
	 * For WPA/WPA2, the PMK is calculated from the PSK, and the PSK
	 * is calculated from the passphrase-SSID pair. You can enter one
	 * of these 3 values and subsequent fields will be automatically
	 * calculated.
	 * @note
	 * For WPA and WPA2 this implementation will use standards as
	 * defined in 802.11i (2004) and 802.1X (2004).
	 */
	union DOT11DECRYPT_KEY_ITEMDATA {
		struct DOT11DECRYPT_KEY_ITEMDATA_WEP {
			/**
			 * The binary value of the WEP key.
			 * @note
			 * It is accepted a key of length between
			 * DOT11DECRYPT_WEP_KEY_MINLEN and
			 * DOT11DECRYPT_WEP_KEY_MAXLEN. A WEP key
			 * standard-compliante should be either 40 bits
			 * (10 hex-digits, 5 bytes) for WEP-40 or 104 bits
			 * (26 hex-digits, 13 bytes) for WEP-104.
			 */
			UCHAR WepKey[DOT11DECRYPT_WEP_KEY_MAXLEN];
			/**
			 * The length of the WEP key. Acceptable range
			 * is [DOT11DECRYPT_WEP_KEY_MINLEN;DOT11DECRYPT_WEP_KEY_MAXLEN].
			 */
			size_t WepKeyLen;
		} Wep;

		/**
		 * WPA/WPA2 key data. Note that the decryption process
		 * will use the PMK (equal to PSK), that is calculated
		 * from passphrase-SSID pair. You can define one of these
		 * three fields and necessary fields will be automatically
		 * calculated.
		 */
		struct DOT11DECRYPT_KEY_ITEMDATA_WPA {
			UCHAR Psk[DOT11DECRYPT_WPA_PSK_LEN];
			UCHAR Ptk[DOT11DECRYPT_WPA_PTK_LEN];
		} Wpa;
	} KeyData;

        struct DOT11DECRYPT_KEY_ITEMDATA_PWD {
                /**
                 * The string (null-terminated) value of
                 * the passphrase.
                 */
                CHAR Passphrase[DOT11DECRYPT_WPA_PASSPHRASE_MAX_LEN+1];
                /**
                 * The value of the SSID (up to
                 * DOT11DECRYPT_WPA_SSID_MAX_LEN octets).
                 * @note
                 * A zero-length SSID indicates broadcast.
                 */
                CHAR Ssid[DOT11DECRYPT_WPA_SSID_MAX_LEN];
                /**
                 *The length of the SSID
                 */
                size_t SsidLen;
        } UserPwd;
} DOT11DECRYPT_KEY_ITEM, *PDOT11DECRYPT_KEY_ITEM;

/**
 * Collection of keys to use to decrypt packets
 */
typedef struct _DOT11DECRYPT_KEYS_COLLECTION {
	/**
	 * Number of stored keys
	 */
	size_t nKeys;

	/**
	 * Array of nKeys keys
	 */
	DOT11DECRYPT_KEY_ITEM Keys[256];
} DOT11DECRYPT_KEYS_COLLECTION, *PDOT11DECRYPT_KEYS_COLLECTION;
/*																										*/
/******************************************************************************/

/******************************************************************************/
/*	Function prototype declarations															*/

/**
 * Returns the decryption_key_t struct given a string describing the key.
 * @param key_string [IN] Key string in one of the following formats:
 * - 0102030405 (40/64-bit WEP)
 * - 01:02:03:04:05 (40/64-bit WEP)
 * - 0102030405060708090a0b0c0d (104/128-bit WEP)
 * - 01:02:03:04:05:06:07:08:09:0a:0b:0c:0d (104/128-bit WEP)
 * - MyPassword (WPA + plaintext password + "wildcard" SSID)
 * - MyPassword:MySSID (WPA + plaintext password + specific SSID)
 * - 01020304... (WPA + 256-bit raw key)
 * @param key_type [IN] Type of key used for string. Possibilities include:
 * - DOT11DECRYPT_KEY_TYPE_WEP (40/64-bit and 104/128-bit WEP)
 * - DOT11DECRYPT_KEY_TYPE_WPA_PWD (WPA + plaintext password + "wildcard" SSID or
 * WPA + plaintext password + specific SSID)
 * - DOT11DECRYPT_KEY_TYPE_WPA_PSK (WPA + 256-bit raw key)
 * @return A pointer to a freshly-g_malloc()ed decryption_key_t struct on
 *   success, or NULL on failure.
 * @see get_key_string(), free_key_string()
 */
WS_DLL_PUBLIC
decryption_key_t*
parse_key_string(gchar* key_string, guint8 key_type);

/**
 * Returns a newly allocated string representing the given decryption_key_t
 * struct.
 * @param dk [IN] Pointer to the key to be converted
 * @return A g_malloc()ed string representation of the key
 * @see parse_key_string()
 */
WS_DLL_PUBLIC
gchar*
get_key_string(decryption_key_t* dk);

/**
 * Releases memory associated with a given decryption_key_t struct.
 * @param dk [IN] Pointer to the key to be freed
 * @see parse_key_string()
 */
WS_DLL_PUBLIC
void
free_key_string(decryption_key_t *dk);

/******************************************************************************/

#endif /* _DOT11DECRYPT_USER_H */
