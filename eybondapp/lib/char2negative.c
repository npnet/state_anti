/******************************************************************************           
* name:                    
* introduce:        char goto negative
* author:           Luee                                     
******************************************************************************/ 
#include "stdio.h"
#include "grid_tool.h"


double my_atof(char *str)
{
	double m=0,n=0,x=1;
	int flag=1;
	int flag2=0;
	if(*str=='-')  //判断正负
	{
		flag2=1;
		str++;
	}
	while(*str!='\0')
	{
		if(*str<'0'|| *str >'9' )
		{	
			if(*str =='.')   //判断小数点前后
			{
				flag=0;
				str++;
				continue;
			}
			return 0;
		}
		if(flag==1)  //小数点前整数部分
		{	m*=10;
			m+=*str-'0';
		}
		else  //小数部分
		{
			x*=0.1;
			n+=x*(*str-'0');
		}
		str++;
	}
	if(flag2==0)
		return m+n;
	else 
		return (m+n)*-1;
}

/*

double my_atof(char *str)
{
	double m=0,n=0,x=1;
	int flag=1;
	int flag2=0;
	if(*str=='-')  //判断正负
	{
		flag2=1;
		str++;
	}
	while(*str!='\0')
	{
		if(*str<'0'|| *str >'9' )
		{	
			if(*str =='.')   //判断小数点前后
			{
				flag=0;
				str++;
				continue;
			}
			return 0;
		}
		if(flag==1)  //小数点前整数部分
		{	m*=10;
			m+=*str-'0';
		}
		else  //小数部分
		{
			x*=0.1;
			n+=x*(*str-'0');
		}
		str++;
	}
	if(flag2==0)
		return m+n;
	else 
		return (m+n)*-1;
}

*/
