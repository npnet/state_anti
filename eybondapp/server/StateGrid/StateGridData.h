/**************Copyright(C)��2015-2026��Shenzhen Eybond Co., Ltd.***************
  *@brief   : StateGridData.h 
  *@notes   : 2018.10.30 CGQ establish
*******************************************************************************/
#ifndef __STATE_GRID_DATA_H
#define __STATE_GRID_DATA_H	 

#include "typedef.h"

#define POINT_TAB_FILE_ADDR       (0x003E0000 )



typedef struct
{
  u32_t id;
  u8_t type;
  u8_t lenght;
  u8_t addr;
  u16_t rsv;
  Buffer_t value;
}StateGridPoint_t;

typedef struct
{
  int count;
  StateGridPoint_t tab[0];
}StateGridPointTab_t;


typedef struct
{
    int count;
    int array[0];
}PointArray_t;

typedef StateGridPointTab_t *(*FileCheck)(void *pointFile, int fileSize);
typedef int (*Status)(void);
typedef void (*SysCommand)(void);
typedef void (*Release)(void);

typedef struct
{
  FileCheck check;
  SysCommand cmd;
  Status     status;
  Release    release;
}StateGridDevice_t;

int getGridDataHandle(StateGridDevice_t **pDevice, StateGridPointTab_t **pPointTab);
int StateGrid_pointTab(void);
int StateGrid_dataStatus(void);
int StateGrid_pointValueLenght(u8_t type, u8_t note, u16_t rev);
int StateGrid_pointLenght(u8_t type, u8_t note, u16_t rev);
int  StateGrid_dataGet(Buffer_t *buf, u8_t * time, PointArray_t *point, u32_t *numbers);
void StateGrid_dataCollect(void);
void StateGrid_destroy(void);

#endif //__STATE_GRID_DATA_H
/******************************************************************************/

