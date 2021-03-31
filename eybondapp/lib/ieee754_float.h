/******************************************************************************           
* name:                       
* introduce:        头文件
* author:           Luee                                     
******************************************************************************/ 
#ifndef _IEEE754_FLOAT_H_
#define _IEEE754_FLOAT_H_

float IEEE754_to_Float(u8_t *p);
u8 *Float_to_IEEE754(float x);

void get_float(void);
void get_ieee754(void);

#endif /* _IEEE754_FLOAT_H_ */