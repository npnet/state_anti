/**********************    COPYRIGHT 2014-2100, eybond ************************
 * @File    : eybpub_UnixTime.c
 * @Author  : CGQ
 * @Date    : 2018-11-01
 * @Brief   :
 ******************************************************************************/
#include "eyblib_r_stdlib.h"
#include "eybpub_UnixTime.h"
#include "eybpub_Clock.h"
#include "eybpub_Debug.h"

enum TIME_DEF {
  SEC = 1,
  MIN = SEC * 60,
  HOUR = MIN * 60,
  DAY = HOUR * 24,
  YEAR = DAY * 365,
};

static u16_t mon_yday[2][12] = {
  {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
  {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};

static int isleap(int year) {
  return (year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0);
}

/*******************************************************************************
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void UnixTime_get(u8_t *time) {
  int i;
  unsigned long secs;
  u32_t msecH;
  u32_t msecL;

  Clock_t *clock = Clock_get();

  // 以平年时间计算的秒数
  secs = (clock->year - 1970) * YEAR
         + (mon_yday[isleap(clock->year)][clock->month - 1] + clock->day - 1) * DAY
         + clock->hour * HOUR + clock->min * MIN +  clock->secs;
  // 加上闰年的秒数
  for (int i = 1970; i < clock->year; i++) {
    if (isleap(i)) {
      secs += DAY;
    }
  }

  /*x * 1000 = x * 1024(x<<10) - x* 24*/

  msecH = secs >> 22;
  msecL = secs << 10;

  for (i = 0; i < 24; i++) {
    if (msecL < secs) {
      msecH--;
    }
    msecL -= secs;
  }
  r_memcpy(time, &msecL, sizeof(msecL));
  r_memcpy(&time[4], &msecH, 4);
}
/******************************************************************************/
