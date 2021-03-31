#include "stdio.h"
#include "grid_tool.h"
#include "ieee754_float.h"

u8 test1[4]={0x3f,0x99,0x99,0x9a} ;
u8 test2[4]={0x3f,0x99,0x99,0x9a} ;
u8 f0d38[4]={0XBE,0XC2,0X8F,0X5C};
//u8 Buffer[4]={0x0};
union packet
{
  float a;
  u8 b[4];
};

void get_float(void)
{
   IEEE754_to_Float(f0d38);
}

void get_ieee754(void)
{
   u8 ieee754_buffer[4]={0};
   u8 *p=ieee754_buffer;
   float a=-0.36;
   Float_to_IEEE754(a);
   float fl=IEEE754_to_Float(f0d38);
   APP_DEBUG("fl=%f",fl);
   p=Float_to_IEEE754(fl);
   APP_DEBUG("P=\r\n");
   print_buf(p,4);
}



/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
float IEEE754_to_Float(u8 *p)
{
   u8 i;
   u8 tmp;
   union packet q;

   //tmp=*p;
   //*p=*(p+3);
   //*(p+3)=tmp;
   //tmp=*(p+1);
   //*(p+1)=*(p+2);
   //*(p+2) =tmp;

   q.a=0;
   for (i=0;i<4;i++)
      q.b[3-i]=p[i];

   APP_DEBUG("get float:%f\r\n",q.a);
   return q.a;

}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
u8 *Float_to_IEEE754(float x)
{
   u8 i;
   static u8 Buffer[4]={0x0};
   static u8 *point=Buffer;

   union packet p;

   for(i=0;i<4;i++){
    p.b[i]=0;
   }
   p.a=x;
   for(i=0;i<4;i++){
    Buffer[3-i]=p.b[i];
   }
    APP_DEBUG("get ieee754:");
    print_buf(Buffer,4);
    return (u8 *)point;
}


/*
main()
{

    Float_to_IEEE754(IEEE754_to_Float(test1));
    printf("Hello, world\n");
    printf("%x\n",Buffer[0]);
    printf("%x\n",Buffer[1]);
    printf("%x\n",Buffer[2]);
    printf("%x\n",Buffer[3]);

       printf("%x\n",test1[0]);
       printf("%x\n",test1[1]);
       printf("%x\n",test1[2]);
       printf("%x\n",test1[3]);


    printf("%f",IEEE754_to_Float(Buffer));
    getch();
}
*/