/* This file was modified slightly to make it part of
   librock.
   IMPORTANT: This file should only be used (and only needed)
   when compiling the sndcal implementation files.

   If you want to call sndcal functions, you get everything
   you need from #include <librock/sndcal.h>  Do not use this
   file for that purpose.

   WARNING: The implementation uses macros which are not
   prefixed by librock_.  They  do not show in the external
   namespace of the output, but by including this file,
   they are in the #define namespace at compile time.
 */
#ifndef _INCLUDED_SDNCAL_H
#define SdnToGregorian PX_SdnToGregorian
#define GregorianToSdn PX_GregorianToSdn
#define MonthNameShort PX_MonthNameShort
#define MonthNameLong PX_MonthNameLong
#endif
#ifndef _INCLUDED_SDNCAL_H
#define _INCLUDED_SDNCAL_H

/* $selId: sdncal.h,v 2.0 1995/10/24 01:13:06 lees Exp $
 * Copyright 1993-1995, Scott E. Lee, all rights reserved.
 * Permission granted to use, copy, modify, distribute and sell so long as
 * the above copyright and this permission statement are retained in all
 * copies.  THERE IS NO WARRANTY - USE AT YOUR OWN RISK.
 */

/**************************************************************************
 *
 * This package defines a set of routines that convert calendar dates to
 * and from a serial day number (SDN).  The SDN is a serial numbering of
 * days where SDN 1 is November 25, 4714 BC in the Gregorian calendar and
 * SDN 2447893 is January 1, 1990.  This system of day numbering is
 * sometimes referred to as Julian days, but to avoid confusion with the
 * Julian calendar, it is referred to as serial day numbers here.  The term
 * Julian days is also used to mean the number of days since the beginning
 * of the current year.
 *
 * The SDN can be used as an intermediate step in converting from one
 * calendar system to another (such as Gregorian to Jewish).  It can also
 * be used for date computations such as easily comparing two dates,
 * determining the day of the week, finding the date of yesterday or
 * calculating the number of days between two dates.
 *
 * When using this software on 16 bit systems, be careful to store SDNs in
 * a long int, because it will not fit in the 16 bits that some systems
 * allocate to an int.
 *
 * For each calendar, there are two routines provided.  One converts dates
 * in that calendar to SDN and the other converts SDN to calendar dates.
 * The routines are named SdnTo<CALENDAR>() and <CALENDAR>ToSdn(), where
 * <CALENDAR> is the name of the calendar system.
 *
 * SDN values less than one are not supported.  If a conversion routine
 * returns an SDN of zero, this means that the date given is either invalid
 * or is outside the supported range for that calendar.
 *
 * At least some validity checks are performed on input dates.  For
 * example, a negative month number will result in the return of zero for
 * the SDN.  A returned SDN greater than one does not necessarily mean that
 * the input date was valid.  To determine if the date is valid, convert it
 * to SDN, and if the SDN is greater than zero, convert it back to a date
 * and compare to the original.  For example:
 *
 *    int y1, m1, d1;
 *    int y2, m2, d2;
 *    long int sdn;
 *    ...
 *    sdn = GregorianToSdn(y1, m1, d1);
 *    if (sdn > 0) {
 *        SdnToGregorian(sdn, &y2, &m2, &d2);
 *        if (y1 == y2 && m1 == m2 && d1 == d2) {
 *            ... date is valid ...
 *        }
 *    }
 *
 **************************************************************************/

/* Gregorian calendar conversions. */
void SdnToGregorian(long int sdn, int *pYear, int *pMonth, int *pDay);
long int GregorianToSdn(int year, int month, int day);
extern char *MonthNameShort[13];
extern char *MonthNameLong[13];

#endif /* _INCLUDED_SDNCAL_H */
