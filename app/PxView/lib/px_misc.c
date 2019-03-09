/*
 * Copyright (c) 1991, 1992, 1993 Brad Eacker,
 *              (Music, Intuition, Software, and Computers)
 * All Rights Reserved
 *
 * This file was taken from the php source code ext/dbase/db_misc.c
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "px_misc.h"

#ifdef WIN32
#define localtime_r( _clock, _result ) \
	( *(_result) = *localtime( (_clock) ), \
	(_result) )

#endif

/*
 * routine to change little endian long to host long
 * these functions are used read header data
 */
long get_long_le(const char *cp)
{
	int ret;
	unsigned char *source = (unsigned char *)cp;

	if(NULL == cp)
		return 0;

	ret = *source++;
	ret += ((*source++)<<8);
	ret += ((*source++)<<16);
	ret += ((*source++)<<24);

	return ret;
}

void put_long_le(char *cp, long lval)
{
	*cp++ = lval & 0xff;
	*cp++ = (lval >> 8) & 0xff;
	*cp++ = (lval >> 16) & 0xff;
	*cp++ = (lval >> 24) & 0xff;
}

/*
 * routine to change little endian short to host short
 */
unsigned short int get_short_le(const char *cp)
{
	unsigned short int ret;
	unsigned char *source = (unsigned char *)cp;

	if(NULL == cp)
		return 0;

	ret = *source++;
	ret += ((*source++)<<8);

	return ret;
}

/*
 * routine to change little endian short to host short
 * This one returns a signed integer because reading
 * datablockhead.addDataSize needs to be done signed.
 * datablockhead.addDataSize is negativ if the datablock
 * is empty.
 */
short int get_short_le_s(const char *cp)
{
	short int ret;
	unsigned char *source = (unsigned char *)cp;

	if(NULL == cp)
		return 0;

	ret = *source++;
	ret += ((*source++)<<8);

	return ret;
}

void put_short_le(char *cp, short int sval)
{
	*cp++ = sval & 0xff;
	*cp++ = (sval >> 8) & 0xff;
}

double get_double_le(const char *cp)
{
	double ret;
	unsigned char *dp = (unsigned char *)&ret;

	if(NULL == cp)
		return 0.0;

#ifdef WORDS_BIGENDIAN
	dp[7] = *cp++;
	dp[6] = *cp++;
	dp[5] = *cp++;
	dp[4] = *cp++;
	dp[3] = *cp++;
	dp[2] = *cp++;
	dp[1] = *cp++;
	dp[0] = *cp++;
#else
	memcpy(dp, cp, 8);
#endif
	return ret;
}

void put_double_le(char *cp, double fval)
{
	unsigned char *dp = (unsigned char *)&fval;

#ifdef WORDS_BIGENDIAN
	cp[7] = *dp++;
	cp[6] = *dp++;
	cp[5] = *dp++;
	cp[4] = *dp++;
	cp[3] = *dp++;
	cp[2] = *dp++;
	cp[1] = *dp++;
	cp[0] = *dp++;
#else
	memcpy(cp, dp, 8);
#endif
}

/*
 * routine to change big endian long to host long
 * these functions are used read table data
 */
long get_long_be(const char *cp)
{
	int ret;
	unsigned char *source = (unsigned char *)cp;

	if(NULL == cp)
		return 0;

	ret = ((*source++)<<24);
	ret += ((*source++)<<16);
	ret += ((*source++)<<8);
	ret += *source++;

	return ret;
}

void put_long_be(char *cp, long lval)
{
	*cp++ = (lval >> 24) & 0xff;
	*cp++ = (lval >> 16) & 0xff;
	*cp++ = (lval >> 8) & 0xff;
	*cp++ = lval & 0xff;
}

/*
 * routine to change little endian short to host short
 */
short int get_short_be(const char *cp)
{
	short int ret;
	unsigned char *source = (unsigned char *)cp;

	if(NULL == cp)
		return 0;

	ret = ((*source++)<<8);
	ret += *source++;

	return ret;
}

void put_short_be(char *cp, short int sval)
{
	*cp++ = (sval >> 8) & 0xff;
	*cp++ = sval & 0xff;
}

double get_double_be(const char *cp)
{
	double ret;
	unsigned char *dp = (unsigned char *)&ret;

	if(NULL == cp)
		return 0.0;

#ifdef WORDS_BIGENDIAN
	memcpy(dp, cp, 8);
#else
	dp[7] = *cp++;
	dp[6] = *cp++;
	dp[5] = *cp++;
	dp[4] = *cp++;
	dp[3] = *cp++;
	dp[2] = *cp++;
	dp[1] = *cp++;
	dp[0] = *cp++;
#endif
	return ret;
}

void put_double_be(char *cp, double fval)
{
	unsigned char *dp = (unsigned char *)&fval;

#ifdef WORDS_BIGENDIAN
	memcpy(cp, dp, 8);
#else
	cp[7] = *dp++;
	cp[6] = *dp++;
	cp[5] = *dp++;
	cp[4] = *dp++;
	cp[3] = *dp++;
	cp[2] = *dp++;
	cp[1] = *dp++;
	cp[0] = *dp++;
#endif
}

void copy_fill(char *dp, char *sp, int len)
{
	while (*sp && len > 0) {
		*dp++ = *sp++;
		len--;
	}
	while (len-- > 0)
		*dp++ = ' ';
}

void copy_crimp(char *dp, char *sp, int len)
{
	while (len-- > 0) {
		*dp++ = *sp++;
	}
	*dp = 0;
	for (dp-- ; *dp == ' '; dp--) {
		*dp = 0;
	}

}

int px_get_date(char *cp) {

	return (*((int *) cp));
}

void px_set_date(char *cp, int year, int month, int day)
{
	if (month > 12)
		month = 0;
	if (day > 31)
		day = 0;
	sprintf(cp, "%d", year);
	cp[4] = month / 10 + '0';
	cp[5] = month % 10 + '0';
	cp[6] = day / 10 + '0';
	cp[7] = day % 10 + '0';
	cp[8] = 0;
}

int px_date_year(char *cp)
{
	int	year, i;

	for (year = 0, i = 0; i < 4; i++)
		year = year * 10 + (cp[i] - '0');
	return year;
}

int px_date_month(char *cp)
{
	int	month, i;

	for (month = 0, i = 4; i < 6; i++)
		month = month * 10 + (cp[i] - '0');
	return month;
}

int px_date_day(char *cp)
{
	int	day, i;

	for (day = 0, i = 6; i < 8; i++)
		day = day * 10 + (cp[i] - '0');
	return day;
}

#include <time.h>

char *px_cur_date(char *cp)
{
	struct tm *ctm, tmbuf;
	time_t	  c_time;

	c_time = time((time_t *)NULL);
	ctm = localtime_r(&c_time, &tmbuf);
	if (cp == NULL)
		cp = (char *)malloc(9);

	if (ctm == NULL || cp == NULL)
		return NULL;

	px_set_date(cp, tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday);

	return cp;
}

void hex_dump(FILE *outfp, char *p, int len) {
	int i;

	if(NULL == p)
		fprintf(outfp, "NULL");

	for(i=0; i<len; i++) {
		if(i%16 == 0)
			fprintf(outfp, "\n%08X: ", (int) &p[i]);
		fprintf(outfp, "%02X ", p[i]);
	}
	fprintf(outfp, "\n");
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

