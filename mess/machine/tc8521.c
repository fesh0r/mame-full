/* Toshiba TC8521 Real-Time Clock */
/* used in NC100, avigo 100 */

/* TODO:

-Implement date and year,
-find out how the alarm works
-find out what time and alarm are reset to... (all zeros!?)
- find out if ints trigger when timer is disabled
- find out if alarm continues to trigger if it hits the time and timer is not enabled

*/

#include "driver.h"
#include "includes/tc8521.h"

/* Registers:

Page 0 (timer):

0x00:
   seconds 0-9 (units digit)
0x01:
   seconds 0-9 (tens digit)
0x02:
   minutes 0-9 (units digit)
0x03:
   minutes 0-9 (tens digit)
0x04:
   hours 0-9 (units digit)
0x05:
   hours 0-9 (tens digit)

Page 1 (alarm):


Page 2 (RAM):
0x00-0x0c: ram



Page 3 (RAM):
0x00-0x0c: ram


0x01d:

bit 7-4: ??
bit 3: timer enable
bit 2: alarm enable
bit 1: pa1 (pa1 and pa0 are page select 0-3)
bit 0: pa0

0x01e:

bit 7-4: ??
bit 3: test 3
bit 2: test 2
bit 1: test 1
bit 0: test 0

0x01f:

bit 7-4: ??
bit 3: 1hz enable
bit 2: 16hz enable
bit 1: timer reset
bit 0: alarm reset
*/

// uncomment for verbose debugging information
#define VERBOSE

struct tc8521
{
        void *tc8521_timer;
		/* 3 register pages */
        unsigned char registers[16*4];

        struct tc8521_interface interface;

		unsigned long current_page;

        int sixteen_hz_counter;
        int hz_counter;
};


static struct tc8521 rtc;

static void tc8521_timer_callback(int dummy)
{

        /* timer enable? */
        if ((rtc.registers[0x0d] & (1<<3))!=0)
        {
                /* 16hz enabled? */
                if ((rtc.registers[0x0f] & (1<<2))!=0)
                {
                        /* yes, so call callback */
                        if (rtc.interface.interrupt_16hz_callback!=NULL)
                        {
                                rtc.interface.interrupt_16hz_callback(1);
                        }
                }

                rtc.sixteen_hz_counter++;
        
                if (rtc.sixteen_hz_counter == 16)
                {
                   rtc.sixteen_hz_counter = 0;


                   /* 1hz enabled? */
                   if ((rtc.registers[0x0f] & (1<<3))!=0)
                   {
                        /* yes, so execute callback  */

                        if (rtc.interface.interrupt_1hz_callback!=NULL)
                        {
                                rtc.interface.interrupt_1hz_callback(1);
                        }
                   }

                   /* seconds; units */
                   rtc.registers[0]++;
        
                   if (rtc.registers[0]==10)
                   {
                       rtc.registers[0] = 0;

                       /* seconds; tens */
                       rtc.registers[1]++;

                       if (rtc.registers[1]==6)
                       {
                          rtc.registers[1] = 0;

                          /* minutes; units */
                          rtc.registers[2]++;

                          if (rtc.registers[2]==10)
                          {
                              rtc.registers[2] = 0;

                              /* minutes; tens */
                              rtc.registers[3]++;

                              if (rtc.registers[3] == 6)
                              {
                                 rtc.registers[3] = 0;

                                 /* hours; units */
                                 rtc.registers[4]++;

                                 if (rtc.registers[4] == 10)
                                 {
                                    rtc.registers[4] = 0;

                                    /* hours; tens */
                                    rtc.registers[4]++;

                                    if (rtc.registers[4] == 24)
                                    {
                                      rtc.registers[4] = 0;
                                    }
                                 }
                              }
                           }
                        }
                   }
                }
        }
}



void tc8521_init(struct tc8521_interface *intf)
{
        rtc.tc8521_timer = timer_pulse(TIME_IN_HZ(16), 0, tc8521_timer_callback);
        rtc.sixteen_hz_counter = 0;
        rtc.hz_counter = 0;

        memset(&rtc, 0, sizeof(struct tc8521));

        memset(&rtc.interface, 0, sizeof(struct tc8521_interface));
        if (intf)
        {
            memcpy(&rtc.interface, intf, sizeof(struct tc8521_interface));
        }

}


void tc8521_stop(void)
{
        if (rtc.tc8521_timer!=NULL)
        {
            timer_remove(rtc.tc8521_timer);
        }

        rtc.tc8521_timer = NULL;
}

READ_HANDLER(tc8521_r)
{
        logerror("8521 RTC R: %04x %02x\r\n", offset, rtc.registers[offset]);
        
		switch (offset)
		{
			/* control registers */
			case 0x0c:
			case 0x0d:
			case 0x0e:
			case 0x0f:
				return rtc.registers[offset];
		
			default:
				break;
		}

		/* data from selected page */
		return rtc.registers[((rtc.current_page)<<4) | (offset & 0x0f)];
}


WRITE_HANDLER(tc8521_w)
{

#ifdef VERBOSE
        logerror("8521 RTC W: %04x %02x\r\n", offset, data);

		switch (offset)
        {
                case 0x0D:
                {
                        if (data & 0x08)
                        {
                            logerror("timer enable\r\n");
                        }

                        if (data & 0x04)
                        {
                            logerror("alarm enable\r\n");
                        }

						logerror("page %02x selected\r\n", data & 0x03);
	              }
                break;

                case 0x0e:
                {
                        if (data & 0x08)
                        {
                            logerror("test 3\r\n");
                        }

                        if (data & 0x04)
                        {
                            logerror("test 2\r\n");
                        }

                        if (data & 0x02)
                        {
                            logerror("test 1\r\n");
                        }

                        if (data & 0x01)
                        {
                            logerror("test 0\r\n");
                        }
                }
                break;

                case 0x0f:
                {
                        if (data & 0x08)
                        {
                           logerror("1hz enable\r\n");
                        }

                        if (data & 0x04)
                        {
                           logerror("16hz enable\r\n");
                        }

                        if (data & 0x02)
                        {
                           logerror("reset timer\r\n");
                        }

                        if (data & 0x01)
                        {
                           logerror("reset alarm\r\n");
                        }
                }
                break;

                default:
                  break;
        }
#endif
		switch (offset)
		{
			/* control registers */
			case 0x0c:
			case 0x0d:
			case 0x0e:
			case 0x0f:
				rtc.registers[offset] = data;
		
				if (offset==0x0d)
				{
					/* page to read/write */
					rtc.current_page = data & 0x03;
				}
				
				return;
			default:
				break;
		}
		/* register in selected page */
        rtc.registers[(rtc.current_page<<4) | (offset & 0x0f)] = data;
}



