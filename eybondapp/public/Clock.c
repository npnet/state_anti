/**********************    COPYRIGHT 2014-2100, QIYI    ************************ 
 * @File    : Clock.c
 * @Author  : CGQ
 * @Date    : 2017-08-17
 * @Brief   : 
 ******************************************************************************/
#ifdef _PLATFORM_BC25_
#include "ql_time.h"
#include "ql_stdlib.h"
#include "ql_memory.h"
#endif

#ifdef _PLATFORM_L610_

#endif

#include "eyblib_swap.h"
// #include "FlashEquilibria.h"	//mike 20200805
#include "eyblib_r_stdlib.h" // mike 20200828
// #include "eyblib_memory.h"   // mike 20200828
#include "eybpub_Debug.h"
#include "eybpub_SysPara_File.h"   // mike 20200826
#include "eybpub_run_log.h"

#include "Clock.h"
//#include "FlashHard.h"		//mike 20200805

// #define CLOCK_FALSH_ADDR            (FLASH_SYSPARA_ADDR + FLASH_SYSPARA_SIZE) 
// #define CLOCK_FALSH_SIZE            (0x2000)

//static FlashEquilibria_t  clockHead;	//mike 20200805
static Clock_t            clock;

static u8_t leapYear(u16_t year);
static void week(void);

#ifdef _PLATFORM_BC25_
/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Clock_init(void)
{
/*    FlashEquilibria_init(&clockHead, CLOCK_FALSH_ADDR, CLOCK_FALSH_SIZE, sizeof(Clock_t), &clock);		//mike 20200826
  if (clock.year < CLOCK_MIN_YEAR || clock.year > CLOCK_MAX_YEAR) {
    clock.year = 2015;
    clock.month = 1;
    clock.day = 1;
    clock.hour = 0;
    clock.min = 0;
    clock.secs = 0;
    week();
    Clock_save();
    log_save("System clock init...\r\n");
  }*/

  ST_Time time;
  if ((Ql_GetRealLocalTime(&time)) != NULL) {
    clock.year = time.year;
    clock.month = time.month;
    clock.day = time.day;
    clock.hour = time.hour;
    clock.min = time.minute;
    clock.secs = time.second;
  } else {
    clock.year = 2015;
    clock.month = 1;
    clock.day = 1;
    clock.hour = 0;
    clock.min = 0;
    clock.secs = 0;    
  }

  week();
  Clock_save();
  APP_DEBUG("System clock init(%d.%02d.%02d %02d:%02d:%02d timezone=%02d)\r\n",
    clock.year, clock.month, clock.day, clock.hour, clock.min, clock.secs, time.timezone);
  log_save("System clock init...");
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
 *******************************************************************************/
void Clock_timeZone(Clock_t * time)
{
	Buffer_t buf;
    Ql_memset(&buf, 0, sizeof(Buffer_t));    // mike 20200828
    ST_Time st_time;
    if ((Ql_GetRealLocalTime(&st_time)) != NULL) {
      if (st_time.year >= 2020) {   // BC25不返回2017年的时间就判定是实际时间
        clock.year = st_time.year;
        clock.month = st_time.month;
        clock.day = st_time.day;
        clock.hour = st_time.hour;
        clock.min = st_time.minute;
        clock.secs = st_time.second;
      }
    } else {
      clock.year = 2015;
      clock.month = 1;
      clock.day = 1;
      clock.hour = 0;
      clock.min = 0;
      clock.secs = 0;
    }
    Ql_memcpy(time, &clock, sizeof(Clock_t));
//	SysPara_Get(TIME_ZONE_ADDR, &buf);
    parametr_get(TIME_ZONE_ADDR, &buf);

    if (buf.payload != NULL && buf.lenght > 0) {
      int timeZone = 0;
      timeZone = Swap_charNum((char *)buf.payload);
      if (timeZone >= 0 && timeZone <= 24) {
        timeZone -= 12;
      }
    }
/*	if (buf.payload != null && buf.lenght > 0)
	{
		int timeZone = 0;
		timeZone = Swap_charNum((char*)buf.payload);
		if (timeZone >= 0 && timeZone <= 24)
		{
			int hour = time->hour;;
			timeZone -= 12;
			hour += timeZone;

			if (hour < 0)
			{
				hour += 24;
				if (time->day == 1)
				{
					if (time->month == 1)
					{
						time->year--;
						time->month = 12;
						time->day = 31;
					}
					else 
					{
						time->month--;
						if (time->month == 2)
						{
							if (0 == leapYear(time->year))
							{
								time->day = 29;
							}
							else
							{
								time->day = 28;
							}
						}
						else if (((time->month < 8) && ((time->month&0x01) == 0x01))
									|| ((time->month > 7)&& ((time->month&0x01) == 0x00))
									)
						{
							
							time->day = 31;
						}
						else 
						{
							time->day = 30;
						}
					}
				}
			}
			else if (hour > 23)
			{
				hour -= 24;
				if ((time->day < 28) 
					|| ((time->month != 2) 
                    	&& (time->day < (30 + (time->month > 7 ? (time->month - 7)&0x01 : time->month &0x01))))
              		|| (time->day < 29 && 0 == leapYear(time->year))
	            )
	   			{
					time->day++;
				}
				else
				{
					time->day = 1;
					if (time->month < 12)
					{
						time->month++;
					}
					else
					{
						time->year++;
						time->month = 1;
					}
				}
			}
			time->hour = hour;
		}
	} */
//	memory_release(buf.payload);
    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
      buf.payload = NULL;
      buf.lenght = 0;
      buf.size = 0;
    }    
}
#endif

#ifdef _PLATFORM_L610_
void Clock_init(void) {
  week();
  Clock_save();
  log_save("System clock init...");
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
 *******************************************************************************/
void Clock_timeZone(Clock_t * time) {
}
#endif
/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Clock_Set(Clock_t *clk)
{
    if (clk->year < CLOCK_MIN_YEAR || clk->year > CLOCK_MAX_YEAR
        || clk->month > 12 || clk->day > 31 || clk->hour > 24)
    {
        return;
    }
    //FlashEquilibria_write(&clockHead, &clock);    // mike 20200805
    r_memcpy(&clock, clk, sizeof(Clock_t));
    week();
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
Clock_t *Clock_get(void)
{
    return &clock;
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Clock_Add(void)
{
    if (clock.secs < 59)
    {
        clock.secs++;
    }
    else if (clock.min < 59)
    {
        if (clock.min%10 == 0)
        {
            Clock_save();
        }
        clock.secs = 0;
        clock.min++;
    }
    else if (clock.hour < 23)
    {
        clock.secs = 0;
        clock.min = 0;
        clock.hour++;
    }
    else if ((clock.day < 28)
              || ((clock.month != 2) 
                    && (clock.day < (30 + (clock.month > 7 ? (clock.month - 7)&0x01 : clock.month &0x01))))
              || ( clock.day < 29 && 0 == leapYear(clock.year))
            )
    {
        clock.secs = 0;
        clock.min = 0;
        clock.hour = 0;
        clock.day++;
    }
    else if (clock.month < 12)
    {
        clock.secs = 0;
        clock.min = 0;
        clock.hour = 0;
        clock.day = 1;
        clock.month++;
    }
    else 
    {
        clock.secs = 0;
        clock.min = 0;
        clock.hour = 0;
        clock.day = 1;
        clock.month = 1;
        clock.year++;
    }
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Clock_save(void)
{
//	FlashEquilibria_write(&clockHead, &clock);	// mike 20200805
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
static u8_t leapYear(u16_t year)
{
    if((year%400) == 0 || (((year&0x03) == 0) && ((year%100) != 0)))
    {
        return 0;
    }
    else
    {
        return 1;
    } 
}
/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
u8_t runTimeCheck(u8_t start, u8_t end)
{
	u8_t i = 0;

	if (start <= end)
	{
		Clock_t c;
		
		Clock_timeZone(&c);
//        APP_PRINT("System clock (%d.%02d.%02d %02d:%02d:%02d)\r\n",c.year, c.month, c.day, c.hour, c.min, c.secs);
		
		if (c.hour < start || c.hour > end)
		{
			i = 1;
		}
	}

	return i;
}


/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
static void week(void)
{
    u16_t year = clock.year;
    u16_t month = clock.month;
    
    if(clock.month == 1 || clock.month == 2) 
    {
        month += 12;
        year--;
    }
    
    clock.week = (clock.day + 2*month + 3*(month + 1)/5 + year + year/4 - year/100 + year/400)%7;
}
/******************************************************************************/
