#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "paradox.h"
#include "px_crypt.h"

static void px_encrypt_chunk(unsigned char src[256], unsigned char dst[256],
                             unsigned char a, unsigned char b,
                             unsigned char c, unsigned char d);

static void px_decrypt_chunk(unsigned char src[256], unsigned char dst[256],
                             unsigned char a, unsigned char b,
                             unsigned char c, unsigned char d);

static void px_encrypt_chunk2(unsigned char src[256], unsigned char dst[256],
                       int lenpassw);

static long makeLongFromBuff(long partial, unsigned char buff[256]);

/* static unsigned char encryption_table_a[256] {{{
 */
static unsigned char encryption_table_a[256] = {
	0xB2, 0xA5, 0x0C, 0xDD, 0x38, 0xFE, 0xCB, 0x5B,
	0x0C, 0x23, 0xEC, 0x6A, 0x95, 0x3E, 0xD5, 0x2D,
	0x2C, 0xF7, 0x2D, 0x30, 0xEA, 0x15, 0x98, 0xB4,
	0x5F, 0x82, 0xD3, 0xAF, 0xF4, 0x4C, 0x84, 0x16,
	0x74, 0x17, 0x05, 0x11, 0xAC, 0xDB, 0x43, 0x91,
	0x9C, 0x77, 0xA0, 0x38, 0xBE, 0xD6, 0x8F, 0x25,
	0xB1, 0xEE, 0x6D, 0x80, 0x37, 0x14, 0xA9, 0x7A,
	0x57, 0x38, 0x6C, 0x2F, 0xA6, 0x4F, 0x7C, 0x38,
	0x71, 0xD4, 0x0B, 0x51, 0xF6, 0xB9, 0x45, 0x21,
	0x1D, 0x6C, 0x4D, 0x87, 0x6E, 0xA7, 0xE7, 0x21,
	0x0D, 0x85, 0xF4, 0xCE, 0x3A, 0x81, 0x6A, 0x3E,
	0xD7, 0x32, 0xA4, 0x23, 0xAC, 0xE9, 0x01, 0x91,
	0xB0, 0xED, 0xC7, 0x46, 0x12, 0xAC, 0x15, 0x3D,
	0xFF, 0x1B, 0x7B, 0x3B, 0xBA, 0xEE, 0x2C, 0x2C,
	0x68, 0xA6, 0x62, 0x60, 0xED, 0x5B, 0x84, 0x33,
	0x61, 0x62, 0x39, 0x79, 0xD4, 0xD2, 0xEB, 0x60,
	0xB8, 0x25, 0x1D, 0xDB, 0xFF, 0x3A, 0x21, 0x9A,
	0xB0, 0xC3, 0xF7, 0x63, 0x52, 0xF5, 0x22, 0x94,
	0xF9, 0xB4, 0xB7, 0xBE, 0x9F, 0x54, 0x3B, 0xCE,
	0xE1, 0x3C, 0x73, 0xCB, 0xEA, 0x2D, 0x45, 0x00,
	0x9D, 0xC2, 0x78, 0xE7, 0x96, 0x0F, 0x14, 0x3A,
	0xA4, 0xDC, 0x01, 0x64, 0x8D, 0xD6, 0x59, 0x4E,
	0x39, 0x0F, 0x15, 0x7D, 0xF0, 0x8C, 0x8A, 0xC5,
	0x78, 0x03, 0x4A, 0xE8, 0xFB, 0xEF, 0x18, 0xB4,
	0x52, 0x31, 0x79, 0xD0, 0x8E, 0xAA, 0xC3, 0xC6,
	0xCE, 0xED, 0xE9, 0x42, 0x5A, 0xD6, 0x2D, 0x06,
	0xC7, 0x9A, 0xB1, 0x38, 0x62, 0xD9, 0x7E, 0x61,
	0x74, 0xD2, 0x46, 0xDE, 0xCB, 0x2B, 0x0C, 0x59,
	0x0B, 0x64, 0x9D, 0x1C, 0x4F, 0xB2, 0x06, 0x91,
	0x9B, 0x63, 0xB5, 0xB2, 0xA9, 0xFD, 0xAD, 0x5A,
	0x38, 0xF9, 0x13, 0x6E, 0x9D, 0x2D, 0x4B, 0x02,
	0xF9, 0x13, 0x6F, 0x50, 0xCB, 0x21, 0x09, 0xFA
};
/* }}} */

/* static unsigned char encryption_table_b[256] {{{
 */
static unsigned char encryption_table_b[256] = {
	0x61, 0xA7, 0x79, 0x02, 0x37, 0x34, 0x6F, 0x81,
	0x01, 0xC2, 0xB2, 0xB3, 0xD6, 0x4D, 0x3E, 0x03,
	0x06, 0x60, 0x98, 0x44, 0x46, 0x68, 0x1C, 0xEB,
	0x10, 0x4A, 0x5B, 0xAE, 0x22, 0x48, 0x24, 0x42,
	0x9F, 0x55, 0x90, 0xC1, 0x7D, 0x2F, 0x6C, 0x41,
	0x4E, 0x82, 0x56, 0xD8, 0x1E, 0x32, 0x28, 0xC6,
	0xED, 0xBC, 0x3C, 0x3A, 0xE9, 0x87, 0x3B, 0x8F,
	0x86, 0x08, 0xA3, 0xFB, 0xA4, 0x62, 0x99, 0xFF,
	0x59, 0xB9, 0xDE, 0x2D, 0x58, 0x93, 0x1B, 0xB1,
	0x76, 0x2B, 0xAA, 0xD9, 0x2A, 0xAC, 0xCA, 0xF9,
	0xE0, 0xB7, 0x05, 0x11, 0x77, 0xA9, 0x8E, 0xEF,
	0xB5, 0xBB, 0x26, 0xEA, 0x8D, 0x18, 0x9B, 0xFE,
	0xC7, 0xF8, 0x5A, 0x83, 0xFD, 0x2E, 0x6B, 0x84,
	0x33, 0xFA, 0x69, 0xD4, 0x74, 0xBF, 0xCB, 0xC4,
	0x19, 0x96, 0x3F, 0xE5, 0x25, 0xF2, 0xA5, 0xD5,
	0x95, 0xF3, 0xA0, 0x39, 0xDC, 0xE4, 0xA1, 0x7F,
	0x7B, 0x49, 0x7A, 0xF1, 0xEE, 0xF7, 0x75, 0x09,
	0x91, 0x67, 0x97, 0x38, 0xD3, 0x89, 0x47, 0x31,
	0x1F, 0x2C, 0x0E, 0xCE, 0x20, 0xCC, 0x9C, 0x23,
	0x17, 0x92, 0x0F, 0xF4, 0x13, 0x64, 0x71, 0xC8,
	0xCD, 0x3D, 0xB4, 0xCF, 0xE1, 0x50, 0x52, 0xAF,
	0x6D, 0x27, 0xB6, 0x94, 0x30, 0x04, 0x8C, 0x53,
	0x4B, 0xD2, 0x4F, 0x21, 0x29, 0x6A, 0x1A, 0xEC,
	0x5C, 0x7E, 0x51, 0x9A, 0x0D, 0x85, 0xE6, 0x1D,
	0xC5, 0x88, 0xA8, 0xDD, 0x9E, 0xF6, 0x5F, 0xDA,
	0xBD, 0x6E, 0x9D, 0x54, 0xDB, 0x5E, 0xC0, 0x80,
	0x5D, 0xB8, 0x40, 0x63, 0x15, 0x12, 0xA2, 0x0C,
	0x07, 0xAD, 0x70, 0x14, 0x72, 0x16, 0xD0, 0xA6,
	0x65, 0x35, 0xBA, 0xE7, 0xAB, 0xFC, 0xC3, 0xC9,
	0xBE, 0x0B, 0xB0, 0xF5, 0x45, 0xE2, 0x4C, 0x73,
	0x0A, 0x36, 0x78, 0x43, 0x8B, 0x8A, 0xF0, 0x66,
	0xD1, 0x00, 0xDF, 0x7C, 0xE3, 0xE8, 0x57, 0xD7
};
/* }}} */

/* static unsigned char encryption_table_c[256] {{{
 */
static unsigned char encryption_table_c[256] = {
	0xF9, 0x08, 0x03, 0x0F, 0xAD, 0x52, 0x10, 0xD8,
	0x39, 0x87, 0xF0, 0xE9, 0xD7, 0xBC, 0x92, 0x9A,
	0x18, 0x53, 0xD5, 0x9C, 0xDB, 0xD4, 0xDD, 0x98,
	0x5D, 0x70, 0xB6, 0x46, 0x16, 0xBF, 0x2C, 0x90,
	0x94, 0xB3, 0x1C, 0x97, 0x1E, 0x74, 0x5A, 0xA9,
	0x2E, 0xB4, 0x4C, 0x49, 0x91, 0x43, 0x65, 0x25,
	0xAC, 0x8F, 0x2D, 0x68, 0x05, 0xE1, 0xF1, 0x04,
	0x8B, 0x7B, 0x33, 0x36, 0x32, 0xA1, 0x0E, 0x72,
	0xD2, 0x27, 0x1F, 0xF3, 0x13, 0xEC, 0x14, 0x8E,
	0x1D, 0x81, 0x19, 0xB0, 0xEE, 0x0D, 0x28, 0xB2,
	0xA5, 0xBA, 0xA6, 0xAF, 0xCB, 0x21, 0x2A, 0xFE,
	0x44, 0x40, 0x62, 0x1A, 0xB8, 0xD0, 0xCD, 0xC6,
	0x11, 0x00, 0x3D, 0xD3, 0x9D, 0xE0, 0xF7, 0x89,
	0x15, 0x6A, 0xB5, 0x66, 0x26, 0xA8, 0xC9, 0x06,
	0xDA, 0x9E, 0xDC, 0xEF, 0x6C, 0x86, 0x48, 0x54,
	0xF2, 0x02, 0x82, 0x80, 0xFB, 0x24, 0xB9, 0x7F,
	0xCF, 0x07, 0x29, 0x63, 0x67, 0xBD, 0x38, 0x35,
	0xC1, 0x8D, 0xF5, 0xF4, 0xAE, 0x5C, 0x56, 0x37,
	0x22, 0x88, 0x99, 0x45, 0xAB, 0x78, 0x71, 0x8A,
	0x12, 0x3E, 0xBB, 0x5E, 0x96, 0xCA, 0xC4, 0x20,
	0x7A, 0x7E, 0xD6, 0x3A, 0x3C, 0x76, 0xDF, 0x01,
	0xC2, 0x55, 0x4A, 0xE4, 0x4D, 0xD9, 0x1B, 0xA7,
	0xEA, 0x47, 0x0A, 0x0B, 0xA2, 0x58, 0xAA, 0x51,
	0xD1, 0x41, 0xE2, 0x59, 0x31, 0xC8, 0xE8, 0x6D,
	0xCE, 0x23, 0x09, 0xE6, 0x6F, 0xC0, 0x2F, 0x60,
	0x9F, 0xE7, 0x4E, 0x6E, 0x95, 0xA0, 0x93, 0xA3,
	0xDE, 0xF8, 0xB1, 0x8C, 0x6B, 0x77, 0x0C, 0xFF,
	0x2B, 0x4B, 0xC7, 0xCC, 0x7C, 0xC3, 0x42, 0xFA,
	0x50, 0xA4, 0xED, 0xFC, 0x7D, 0x73, 0xBE, 0xE3,
	0xFD, 0x34, 0x5B, 0x17, 0xB7, 0x30, 0x84, 0x57,
	0xF6, 0x83, 0x75, 0x79, 0x9B, 0xEB, 0xC5, 0x85,
	0x61, 0x4F, 0x69, 0x3B, 0xE5, 0x64, 0x5F, 0x3F
};
/* }}} */


/* px_encrypt_chunk(src, dst, a, b, c, d) {{{
 */
static void px_encrypt_chunk(unsigned char src[256], unsigned char dst[256],
                             unsigned char a, unsigned char b,
                             unsigned char c, unsigned char d)
{
	unsigned char tmp[256];
	register int x = 0;
	register int y = 0;

	for (x = 0; x < 256; ++x) {
		y = (encryption_table_c[x] - d) & 0xff;
		tmp[y] = src[x] ^
			encryption_table_a[(x + a) & 0xff] ^
			encryption_table_b[(y + b) & 0xff] ^
			encryption_table_c[(y + c) & 0xff];
	}
	memcpy(dst, tmp, 256);
}
/* }}} */

/* px_encrypt_db_block(src, dest, encryption, blocksize, blockno) {{{
 */
void px_encrypt_db_block(unsigned char *src, unsigned char *dest,
                         unsigned long encryption, unsigned long blocksize,
                         unsigned long blockno)
{
	unsigned int chunk = 0;
	unsigned char a = 0;
	unsigned char b = 0;

	a = encryption & 0xff;
	b = (encryption >> 8) & 0xff;
	blocksize >>= 8;

	for (chunk = 0; chunk < blocksize; ++chunk) {
		px_encrypt_chunk(src + (chunk << 8), dest + (chunk << 8), a, b, chunk, (unsigned char)blockno);
	}
}
/* }}} */

/* px_encrypt_mb_block(src, dest, encryption, blocksize) {{{
 */
void px_encrypt_mb_block(unsigned char *src, unsigned char *dest,
                         unsigned long encryption, unsigned long blocksize)
{
	unsigned int chunk = 0;
	unsigned char a = 0;
	unsigned char b = 0;

	a = encryption & 0xFF;
	b = (encryption >> 8) & 0xFF;
	blocksize >>= 8;

	for (chunk = 0; chunk < blocksize; ++chunk) {
		px_encrypt_chunk(src + (chunk << 8), dest + (chunk << 8), a, b, a+1, b+1);
	}
}
/* }}} */


/* px_decrypt_chunk(src, dst, a, b, c, d) {{{
 */
static void px_decrypt_chunk(unsigned char src[256], unsigned char dst[256],
                             unsigned char a, unsigned char b,
                             unsigned char c, unsigned char d)
{
	unsigned char tmp[256];
	register int x = 0;
	register int y = 0;

	for (x = 0; x < 256; ++x) {
		y = (encryption_table_c[x] - d) & 0xff;
		tmp[x] = src[y] ^
			encryption_table_a[(x + a) & 0xff] ^
			encryption_table_b[(y + b) & 0xff] ^
			encryption_table_c[(y + c) & 0xff];
	}
	memcpy(dst, tmp, 256);
}
/* }}} */

/* px_decrypt_db_block(src, dest, encryption, blocksize, blockno) {{{
 */
void px_decrypt_db_block(unsigned char *src, unsigned char *dest,
                         unsigned long encryption, unsigned long blocksize,
                         unsigned long blockno)
{
	unsigned int chunk = 0;
	unsigned char a = 0;
	unsigned char b = 0;

	a = encryption & 0xff;
	b = (encryption >> 8) & 0xff;
	blocksize >>= 8;

	for (chunk = 0; chunk < blocksize; ++chunk) {
		px_decrypt_chunk(src + (chunk << 8), dest + (chunk << 8), a, b, chunk, (unsigned char)blockno);
	}
}
/* }}} */

/* px_decrypt_mb_block(src, dest, encryption, blocksize) {{{
 */
void px_decrypt_mb_block(unsigned char *src, unsigned char *dest,
                         unsigned long encryption, unsigned long blocksize)
{
	unsigned int chunk = 0;
	unsigned char a = 0;
	unsigned char b = 0;

	a = encryption & 0xFF;
	b = (encryption >> 8) & 0xFF;
	blocksize >>= 8;

	for (chunk = 0; chunk < blocksize; ++chunk) {
		px_decrypt_chunk(src + (chunk << 8), dest + (chunk << 8), a, b, a+1, b+1);
	}
}
/* }}} */


/* px_encrypt_chunk2() {{{
 */
static void px_encrypt_chunk2(unsigned char src[256], unsigned char dst[256],
                       int lenpassw) {
	unsigned char tmp[256];
	register int i = 0;
	register int x = 0;
	register int y = 0;
	memcpy(tmp, src, 256);
	for(i=lenpassw; i<256; i++) {
		x = tmp[i-lenpassw];
		y = encryption_table_a[x] ^ i;
		tmp[i] = y;
	}
	memcpy(dst, tmp, 256);
}
/* }}} */

/* makeLongFromBuff() {{{
 */
static long makeLongFromBuff(long partial, unsigned char buff[256]) {
	long i = 0;
	long x = 0;
	long y = 0;
	long z = 0;
	long result = 0;
	while(i <= 255) {
		x = buff[i];
		y = y ^ x;
		i++;
		x = buff[i];
		z = z ^ x;
		i++;
	}
	result = (z << 8) | y;
	if(result == 0) {
		result = 1;
	}
	result = (result << 16) | partial;
	return(result);
}
/* }}} */

/* px_passwd_checksum() {{{
 *
 * Calculates the checksum as it is stored in the header field encryption
 * or encryption2 of the db file.
 */
long px_passwd_checksum(const char *aPsw) {
	unsigned char buff[256];
	long len = 0;
	long count = 0;
	long partial = 0;
	long x = 0;
	long y = 0;

	if(aPsw == NULL || aPsw[0] == '\0') {
		return 0;
	}
	len = (long)strlen(aPsw);
	count = 256;
	while(count >= len) {
		memcpy(&buff[256-count], aPsw, len);
		count -= len;
	}
	if(count > 0) {
		memcpy(&buff[256-count], aPsw, count);
	}
	px_encrypt_chunk(buff, buff, buff[0], buff[1], buff[2], buff[3]);
	x = buff[0];
	y = buff[1];
	partial = (y << 8) + x;
	memcpy(buff, aPsw, len);
	px_encrypt_chunk2(buff, buff, len);
	px_encrypt_chunk(buff, buff, buff[0], buff[20], buff[40], buff[255]);
	return(makeLongFromBuff(partial, buff));
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker noet
 * vim<600: sw=4 ts=4
 */
