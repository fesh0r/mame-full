/*
	mm58274c emulation

	Reference:
	* National Semiconductor MM58274C Microprocessor Compatible Real Time Clock
		<http://www.national.com/ds/MM/MM58274C.pdf>

	Todo:
	* Clock initialization will only work with the BwG: we need to provide
	  a way to customize it.
	* Save the config to NVRAM?
	* Support interrupt pin output

	Raphael Nabet, 2002
*/

#include <time.h>
#include "mm58274c.h"

static void rtc_interrupt_callback(int dummy);
static void increment_rtc(int dummy);

static struct
{
	int status;		/* status register (*read* from address 0 = control register) */
	int control;	/* control register (*write* to address 0) */

	int clk_set;	/* clock setting register */
	int int_ctl;	/* interrupt control register */


	int wday;		/* day of the week (1-7 (1=monday, 7=sunday)) */
	int years1;		/* years (BCD: 0-99) */
	int years2;
	int months1;	/* months (BCD: 1-12) */
	int months2;
	int days1;		/* days (BCD: 1-31) */
	int days2;
	int hours1;		/* hours (BCD : 0-23) */
	int hours2;
	int minutes1;	/* minutes (BCD : 0-59) */
	int minutes2;
	int seconds1;	/* seconds (BCD : 0-59) */
	int seconds2;
	int tenths;		/* tenths of second (BCD : 0-9) */
} rtc_regs;

enum
{
	st_dcf = 0x8,		/* data-changed flag */
	st_if = 0x1,		/* interrupt flag */

	ctl_test = 0x8,		/* test mode (0=normal, 1=test) (not emulated) */
	ctl_clkstop = 0x4,	/* clock start/stop (0=run, 1=stop) */
	ctl_intsel = 0x2,	/* interrupt select (0=clock setting register, 1=interrupt register) */
	ctl_intstop = 0x1,	/* interrupt start stop (0=interrupt run, 1=interrupt stop) */

	clk_set_leap = 0xc,		/* leap year counter (0 indicates a leap year) */
	clk_set_leap_inc = 0x4,	/* leap year increment */
	clk_set_pm = 0x2,		/* am/pm indicator (0 = am, 1 = pm, 0 in 24-hour mode) */
	clk_set_24 = 0x1,		/* 12/24-hour select bit (1= 24-hour mode) */

	int_ctl_rpt = 0x8,		/* 1 for repeated interrupt */
	int_ctl_dly = 0x7		/* 0 no interrupt, 1 = .1 second, 2=.5, 3=1, 4=5, 5=10, 6=30, 7=60 */
};

static void *interrupt_timer;
static const double interrupt_period_table[8] =
{
	0.,
	TIME_IN_SEC(.1),
	TIME_IN_SEC(.5),
	TIME_IN_SEC(1),
	TIME_IN_SEC(5),
	TIME_IN_SEC(10),
	TIME_IN_SEC(30),
	TIME_IN_SEC(60)
};


void mm58274c_init(void)
{
	memset(&rtc_regs, 0, sizeof(rtc_regs));

	timer_pulse(TIME_IN_SEC(.1), 0, increment_rtc);
	interrupt_timer = timer_alloc(rtc_interrupt_callback);

	{
		/* Now we copy the host clock into the rtc */
		/* All these functions should be ANSI */
		time_t cur_time = time(NULL);
		struct tm expanded_time = *localtime(& cur_time);

		rtc_regs.clk_set = expanded_time.tm_year & 3 << 2;
		rtc_regs.clk_set |= clk_set_24;

		/* The clock count starts on 1st January 1900 */
		rtc_regs.wday = expanded_time.tm_wday ? expanded_time.tm_wday : 7;
		rtc_regs.years1 = (expanded_time.tm_year / 10) % 10;
		rtc_regs.years2 = expanded_time.tm_year % 10;
		rtc_regs.months1 = (expanded_time.tm_mon + 1) / 10;
		rtc_regs.months2 = (expanded_time.tm_mon + 1) % 10;
		rtc_regs.days1 = expanded_time.tm_mday / 10;
		rtc_regs.days2 = expanded_time.tm_mday % 10;
		rtc_regs.hours1 = expanded_time.tm_hour / 10;
		rtc_regs.hours2 = expanded_time.tm_hour % 10;
		rtc_regs.minutes1 = expanded_time.tm_min / 10;
		rtc_regs.minutes2 = expanded_time.tm_min % 10;
		rtc_regs.seconds1 = expanded_time.tm_sec / 10;
		rtc_regs.seconds2 = expanded_time.tm_sec % 10;
		rtc_regs.tenths = 0;
	}
}


READ_HANDLER(mm58274c_r)
{
	int reply;


	offset &= 0xf;

	switch (offset)
	{
	case 0x0:	/* Control Register */
		reply = rtc_regs.status;
		rtc_regs.status = 0;
		break;

	case 0x1:	/* Tenths of Seconds */
		reply = rtc_regs.tenths;
		break;

	case 0x2:	/* Units Seconds */
		reply = rtc_regs.seconds2;
		break;

	case 0x3:	/* Tens Seconds */
		reply = rtc_regs.seconds1;
		break;

	case 0x04:	/* Units Minutes */
		reply = rtc_regs.minutes2;
		break;

	case 0x5:	/* Tens Minutes */
		reply = rtc_regs.minutes1;
		break;

	case 0x6:	/* Units Hours */
		reply = rtc_regs.hours2;
		break;

	case 0x7:	/* Tens Hours */
		reply = rtc_regs.hours1;
		break;

	case 0x8:	/* Units Days */
		reply = rtc_regs.days2;
		break;

	case 0x9:	/* Tens Days */
		reply = rtc_regs.days1;
		break;

	case 0xA:	/* Units Months */
		reply = rtc_regs.months2;
		break;

	case 0xB:	/* Tens Months */
		reply = rtc_regs.months1;
		break;

	case 0xC:	/* Units Years */
		reply = rtc_regs.years2;
		break;

	case 0xD:	/* Tens Years */
		reply = rtc_regs.years1;
		break;

	case 0xE:	/* Day of Week */
		reply = rtc_regs.wday;
		break;

	case 0xF:	/* Clock Setting & Interrupt Registers */
		if (rtc_regs.control & ctl_intsel)
			/* interrupt register */
			reply = rtc_regs.int_ctl;
		else
		{	/* clock setting register */
			if (rtc_regs.clk_set & clk_set_24)
				/* 24-hour mode */
				reply = rtc_regs.clk_set & ~clk_set_pm;
			else
				/* 12-hour mode */
				reply = rtc_regs.clk_set;
		}
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}


WRITE_HANDLER(mm58274c_w)
{
	offset &= 0xf;
	data &= 0xf;

	switch (offset)
	{
	case 0x0:	/* Control Register (test mode and interrupt not emulated) */
		if ((! (rtc_regs.control & ctl_intstop)) && (data & ctl_intstop))
			/* interrupt stop */
			timer_enable(interrupt_timer, 0);
		else if ((rtc_regs.control & ctl_intstop) && (! (data & ctl_intstop)))
		{
			/* interrupt run */
			double period = interrupt_period_table[rtc_regs.int_ctl & int_ctl_dly];

			timer_adjust(interrupt_timer, period, 0, rtc_regs.int_ctl & int_ctl_rpt ? period : 0.);
		}
		if (data & ctl_clkstop)
			/* stopping the clock clears the tenth counter */
			rtc_regs.tenths = 0;
		rtc_regs.control = data;
		break;

	case 0x1:	/* Tenths of Seconds: cannot be written */
		break;

	case 0x2:	/* Units Seconds */
		rtc_regs.seconds2 = data;
		break;

	case 0x3:	/* Tens Seconds */
		rtc_regs.seconds1 = data;
		break;

	case 0x4:	/* Units Minutes */
		rtc_regs.minutes2 = data;
		break;

	case 0x5:	/* Tens Minutes */
		rtc_regs.minutes1 = data;
		break;

	case 0x6:	/* Units Hours */
		rtc_regs.hours2 = data;
		break;

	case 0x7:	/* Tens Hours */
		rtc_regs.hours1 = data;
		break;

	case 0x8:	/* Units Days */
		rtc_regs.days2 = data;
		break;

	case 0x9:	/* Tens Days */
		rtc_regs.days1 = data;
		break;

	case 0xA:	/* Units Months */
		rtc_regs.months2 = data;
		break;

	case 0xB:	/* Tens Months */
		rtc_regs.months1 = data;
		break;

	case 0xC:	/* Units Years */
		rtc_regs.years2 = data;
		break;

	case 0xD:	/* Tens Years */
		rtc_regs.years1 = data;
		break;

	case 0xE:	/* Day of Week */
		rtc_regs.wday = data;
		break;

	case 0xF:	/* Clock Setting & Interrupt Registers */
		if (rtc_regs.control & ctl_intsel)
		{
			/* interrupt register (not emulated) */
			rtc_regs.int_ctl = data;
			if (! (rtc_regs.control & ctl_intstop))
			{
				/* interrupt run */
				double period = interrupt_period_table[rtc_regs.int_ctl & int_ctl_dly];

				timer_adjust(interrupt_timer, period, 0, rtc_regs.int_ctl & int_ctl_rpt ? period : 0.);
			}
		}
		else
		{
			/* clock setting register */
			rtc_regs.clk_set = data;
			#if 0
				if (rtc_regs.clk_set & clk_set_24)
					/* 24-hour mode */
					rtc_regs.clk_set &= ~clk_set_pm;
			#endif
		}
		break;
	}
}


/*
	Set RTC interrupt flag
*/
static void rtc_interrupt_callback(int dummy)
{
	(void) dummy;

	rtc_regs.status |= st_if;
}


/*
	Increment RTC clock (timed interrupt every 1/10s)
*/
static void increment_rtc(int dummy)
{
	(void) dummy;

	if (! (rtc_regs.control & ctl_clkstop))
	{
		rtc_regs.status |= st_dcf;

		if ((++rtc_regs.tenths) == 10)
		{
			rtc_regs.tenths = 0;

			if ((++rtc_regs.seconds2) == 10)
			{
				rtc_regs.seconds2 = 0;

				if ((++rtc_regs.seconds1) == 6)
				{
					rtc_regs.seconds1 = 0;

					if ((++rtc_regs.minutes2) == 10)
					{
						rtc_regs.minutes2 = 0;

						if ((++rtc_regs.minutes1) == 6)
						{
							rtc_regs.minutes1 = 0;

							if ((++rtc_regs.hours2) == 10)
							{
								rtc_regs.hours2 = 0;

								rtc_regs.hours1++;
							}

							/* handle wrap-around */
							if ((! (rtc_regs.clk_set & clk_set_24))
									&& ((rtc_regs.hours1*10 + rtc_regs.hours2) == 12))
							{
								rtc_regs.clk_set ^= clk_set_pm;
							}
							if ((! (rtc_regs.clk_set & clk_set_24))
									&& ((rtc_regs.hours1*10 + rtc_regs.hours2) == 13))
							{
								rtc_regs.hours1 = 0;
								rtc_regs.hours2 = 1;
							}

							if ((rtc_regs.clk_set & clk_set_24)
								&& ((rtc_regs.hours1*10 + rtc_regs.hours2) == 24))
							{
								rtc_regs.hours1 = rtc_regs.hours2 = 0;
							}

							/* increment day if needed */
							if ((rtc_regs.clk_set & clk_set_24)
								? ((rtc_regs.hours1*10 + rtc_regs.hours2) == 0)
								: (((rtc_regs.hours1*10 + rtc_regs.hours2) == 12)
									&& (! (rtc_regs.clk_set & clk_set_pm))))
							{
								int days_in_month;

								if ((++rtc_regs.days2) == 10)
								{
									rtc_regs.days2 = 0;

									rtc_regs.days1++;
								}

								if ((++rtc_regs.wday) == 8)
									rtc_regs.wday = 1;

								{
									static const int days_in_month_array[] =
									{
										31,28,31, 30,31,30,
										31,31,30, 31,30,31
									};

									if (((rtc_regs.months1*10 + rtc_regs.months2) != 2) || (rtc_regs.clk_set & clk_set_leap))
										days_in_month = days_in_month_array[rtc_regs.months1*10 + rtc_regs.months2 - 1];
									else
										days_in_month = 29;
								}


								if ((rtc_regs.days1*10 + rtc_regs.days2) == days_in_month+1)
								{
									rtc_regs.days1 = 0;
									rtc_regs.days2 = 1;

									if ((++rtc_regs.months2) == 10)
									{
										rtc_regs.months2 = 0;

										rtc_regs.months1++;
									}

									if ((rtc_regs.months1*10 + rtc_regs.months2) == 13)
									{
										rtc_regs.months1 = 0;
										rtc_regs.months2 = 1;

										rtc_regs.clk_set = (rtc_regs.clk_set & ~clk_set_leap)
															| ((rtc_regs.clk_set + clk_set_leap_inc) & clk_set_leap);

										if ((++rtc_regs.years2) == 10)
										{
											rtc_regs.years2 = 0;

											if ((++rtc_regs.years1) == 10)
												rtc_regs.years1 = 0;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
