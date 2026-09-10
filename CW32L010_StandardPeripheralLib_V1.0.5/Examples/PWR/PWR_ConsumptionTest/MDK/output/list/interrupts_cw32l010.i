#line 1 "..\\USER\\src\\interrupts_cw32l010.c"
 












 
 

 
#line 1 "..\\USER\\src\\..\\inc\\main.h"
 













 
 

 







 
#line 1 "..\\..\\..\\..\\Libraries\\inc\\base_types.h"




 
 
 
#line 1 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdio.h"
 
 
 





 






 







 




  
 








#line 47 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdio.h"


  



    typedef unsigned int size_t;    









 
 

 



    typedef struct __va_list __va_list;






   




 




typedef struct __fpos_t_struct {
    unsigned __int64 __pos;
    



 
    struct {
        unsigned int __state1, __state2;
    } __mbstate;
} fpos_t;
   


 


   

 

typedef struct __FILE FILE;
   






 

#line 136 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdio.h"


extern FILE __stdin, __stdout, __stderr;
extern FILE *__aeabi_stdin, *__aeabi_stdout, *__aeabi_stderr;

#line 166 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdio.h"
    

    

    





     



   


 


   


 

   



 

   


 




   


 





    


 






extern __declspec(__nothrow) int remove(const char *  ) __attribute__((__nonnull__(1)));
   





 
extern __declspec(__nothrow) int rename(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));
   








 
extern __declspec(__nothrow) FILE *tmpfile(void);
   




 
extern __declspec(__nothrow) char *tmpnam(char *  );
   











 

extern __declspec(__nothrow) int fclose(FILE *  ) __attribute__((__nonnull__(1)));
   







 
extern __declspec(__nothrow) int fflush(FILE *  );
   







 
extern __declspec(__nothrow) FILE *fopen(const char * __restrict  ,
                           const char * __restrict  ) __attribute__((__nonnull__(1,2)));
   








































 
extern __declspec(__nothrow) FILE *freopen(const char * __restrict  ,
                    const char * __restrict  ,
                    FILE * __restrict  ) __attribute__((__nonnull__(2,3)));
   








 
extern __declspec(__nothrow) void setbuf(FILE * __restrict  ,
                    char * __restrict  ) __attribute__((__nonnull__(1)));
   




 
extern __declspec(__nothrow) int setvbuf(FILE * __restrict  ,
                   char * __restrict  ,
                   int  , size_t  ) __attribute__((__nonnull__(1)));
   















 
#pragma __printf_args
extern __declspec(__nothrow) int fprintf(FILE * __restrict  ,
                    const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   


















 
#pragma __printf_args
extern __declspec(__nothrow) int _fprintf(FILE * __restrict  ,
                     const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   



 
#pragma __printf_args
extern __declspec(__nothrow) int printf(const char * __restrict  , ...) __attribute__((__nonnull__(1)));
   




 
#pragma __printf_args
extern __declspec(__nothrow) int _printf(const char * __restrict  , ...) __attribute__((__nonnull__(1)));
   



 
#pragma __printf_args
extern __declspec(__nothrow) int sprintf(char * __restrict  , const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   






 
#pragma __printf_args
extern __declspec(__nothrow) int _sprintf(char * __restrict  , const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   



 

#pragma __printf_args
extern __declspec(__nothrow) int __ARM_snprintf(char * __restrict  , size_t  ,
                     const char * __restrict  , ...) __attribute__((__nonnull__(3)));


#pragma __printf_args
extern __declspec(__nothrow) int snprintf(char * __restrict  , size_t  ,
                     const char * __restrict  , ...) __attribute__((__nonnull__(3)));
   















 

#pragma __printf_args
extern __declspec(__nothrow) int _snprintf(char * __restrict  , size_t  ,
                      const char * __restrict  , ...) __attribute__((__nonnull__(3)));
   



 
#pragma __scanf_args
extern __declspec(__nothrow) int fscanf(FILE * __restrict  ,
                    const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   






























 
#pragma __scanf_args
extern __declspec(__nothrow) int _fscanf(FILE * __restrict  ,
                     const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   



 
#pragma __scanf_args
extern __declspec(__nothrow) int scanf(const char * __restrict  , ...) __attribute__((__nonnull__(1)));
   






 
#pragma __scanf_args
extern __declspec(__nothrow) int _scanf(const char * __restrict  , ...) __attribute__((__nonnull__(1)));
   



 
#pragma __scanf_args
extern __declspec(__nothrow) int sscanf(const char * __restrict  ,
                    const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   








 
#pragma __scanf_args
extern __declspec(__nothrow) int _sscanf(const char * __restrict  ,
                     const char * __restrict  , ...) __attribute__((__nonnull__(1,2)));
   



 

 
extern __declspec(__nothrow) int vfscanf(FILE * __restrict  , const char * __restrict  , __va_list) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) int vscanf(const char * __restrict  , __va_list) __attribute__((__nonnull__(1)));
extern __declspec(__nothrow) int vsscanf(const char * __restrict  , const char * __restrict  , __va_list) __attribute__((__nonnull__(1,2)));

extern __declspec(__nothrow) int _vfscanf(FILE * __restrict  , const char * __restrict  , __va_list) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) int _vscanf(const char * __restrict  , __va_list) __attribute__((__nonnull__(1)));
extern __declspec(__nothrow) int _vsscanf(const char * __restrict  , const char * __restrict  , __va_list) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) int __ARM_vsscanf(const char * __restrict  , const char * __restrict  , __va_list) __attribute__((__nonnull__(1,2)));

extern __declspec(__nothrow) int vprintf(const char * __restrict  , __va_list  ) __attribute__((__nonnull__(1)));
   





 
extern __declspec(__nothrow) int _vprintf(const char * __restrict  , __va_list  ) __attribute__((__nonnull__(1)));
   



 
extern __declspec(__nothrow) int vfprintf(FILE * __restrict  ,
                    const char * __restrict  , __va_list  ) __attribute__((__nonnull__(1,2)));
   






 
extern __declspec(__nothrow) int vsprintf(char * __restrict  ,
                     const char * __restrict  , __va_list  ) __attribute__((__nonnull__(1,2)));
   






 
extern __declspec(__nothrow) int __ARM_vsnprintf(char * __restrict  , size_t  ,
                     const char * __restrict  , __va_list  ) __attribute__((__nonnull__(3)));

extern __declspec(__nothrow) int vsnprintf(char * __restrict  , size_t  ,
                     const char * __restrict  , __va_list  ) __attribute__((__nonnull__(3)));
   







 

extern __declspec(__nothrow) int _vsprintf(char * __restrict  ,
                      const char * __restrict  , __va_list  ) __attribute__((__nonnull__(1,2)));
   



 
extern __declspec(__nothrow) int _vfprintf(FILE * __restrict  ,
                     const char * __restrict  , __va_list  ) __attribute__((__nonnull__(1,2)));
   



 
extern __declspec(__nothrow) int _vsnprintf(char * __restrict  , size_t  ,
                      const char * __restrict  , __va_list  ) __attribute__((__nonnull__(3)));
   



 

#pragma __printf_args
extern __declspec(__nothrow) int asprintf(char **  , const char * __restrict  , ...) __attribute__((__nonnull__(2)));
extern __declspec(__nothrow) int vasprintf(char **  , const char * __restrict  , __va_list  ) __attribute__((__nonnull__(2)));

#pragma __printf_args
extern __declspec(__nothrow) int __ARM_asprintf(char **  , const char * __restrict  , ...) __attribute__((__nonnull__(2)));
extern __declspec(__nothrow) int __ARM_vasprintf(char **  , const char * __restrict  , __va_list  ) __attribute__((__nonnull__(2)));
   








 

extern __declspec(__nothrow) int fgetc(FILE *  ) __attribute__((__nonnull__(1)));
   







 
extern __declspec(__nothrow) char *fgets(char * __restrict  , int  ,
                    FILE * __restrict  ) __attribute__((__nonnull__(1,3)));
   










 
extern __declspec(__nothrow) int fputc(int  , FILE *  ) __attribute__((__nonnull__(2)));
   







 
extern __declspec(__nothrow) int fputs(const char * __restrict  , FILE * __restrict  ) __attribute__((__nonnull__(1,2)));
   




 
extern __declspec(__nothrow) int getc(FILE *  ) __attribute__((__nonnull__(1)));
   







 




    extern __declspec(__nothrow) int (getchar)(void);

   





 
extern __declspec(__nothrow) char *gets(char *  ) __attribute__((__nonnull__(1)));
   









 
extern __declspec(__nothrow) int putc(int  , FILE *  ) __attribute__((__nonnull__(2)));
   





 




    extern __declspec(__nothrow) int (putchar)(int  );

   



 
extern __declspec(__nothrow) int puts(const char *  ) __attribute__((__nonnull__(1)));
   





 
extern __declspec(__nothrow) int ungetc(int  , FILE *  ) __attribute__((__nonnull__(2)));
   






















 

extern __declspec(__nothrow) size_t fread(void * __restrict  ,
                    size_t  , size_t  , FILE * __restrict  ) __attribute__((__nonnull__(1,4)));
   











 

extern __declspec(__nothrow) size_t __fread_bytes_avail(void * __restrict  ,
                    size_t  , FILE * __restrict  ) __attribute__((__nonnull__(1,3)));
   











 

extern __declspec(__nothrow) size_t fwrite(const void * __restrict  ,
                    size_t  , size_t  , FILE * __restrict  ) __attribute__((__nonnull__(1,4)));
   







 

extern __declspec(__nothrow) int fgetpos(FILE * __restrict  , fpos_t * __restrict  ) __attribute__((__nonnull__(1,2)));
   








 
extern __declspec(__nothrow) int fseek(FILE *  , long int  , int  ) __attribute__((__nonnull__(1)));
   














 
extern __declspec(__nothrow) int fsetpos(FILE * __restrict  , const fpos_t * __restrict  ) __attribute__((__nonnull__(1,2)));
   










 
extern __declspec(__nothrow) long int ftell(FILE *  ) __attribute__((__nonnull__(1)));
   











 
extern __declspec(__nothrow) void rewind(FILE *  ) __attribute__((__nonnull__(1)));
   





 

extern __declspec(__nothrow) void clearerr(FILE *  ) __attribute__((__nonnull__(1)));
   




 

extern __declspec(__nothrow) int feof(FILE *  ) __attribute__((__nonnull__(1)));
   


 
extern __declspec(__nothrow) int ferror(FILE *  ) __attribute__((__nonnull__(1)));
   


 
extern __declspec(__nothrow) void perror(const char *  );
   









 

extern __declspec(__nothrow) int _fisatty(FILE *   ) __attribute__((__nonnull__(1)));
    
 

extern __declspec(__nothrow) void __use_no_semihosting_swi(void);
extern __declspec(__nothrow) void __use_no_semihosting(void);
    





 











#line 1021 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdio.h"



 

#line 9 "..\\..\\..\\..\\Libraries\\inc\\base_types.h"
#line 1 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"
 
 
 
 




 








 












#line 38 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"


  



    typedef unsigned int size_t;    
#line 54 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"




extern __declspec(__nothrow) void *memcpy(void * __restrict  ,
                    const void * __restrict  , size_t  ) __attribute__((__nonnull__(1,2)));
   




 
extern __declspec(__nothrow) void *memmove(void *  ,
                    const void *  , size_t  ) __attribute__((__nonnull__(1,2)));
   







 
extern __declspec(__nothrow) char *strcpy(char * __restrict  , const char * __restrict  ) __attribute__((__nonnull__(1,2)));
   




 
extern __declspec(__nothrow) char *strncpy(char * __restrict  , const char * __restrict  , size_t  ) __attribute__((__nonnull__(1,2)));
   





 

extern __declspec(__nothrow) char *strcat(char * __restrict  , const char * __restrict  ) __attribute__((__nonnull__(1,2)));
   




 
extern __declspec(__nothrow) char *strncat(char * __restrict  , const char * __restrict  , size_t  ) __attribute__((__nonnull__(1,2)));
   






 






 

extern __declspec(__nothrow) int memcmp(const void *  , const void *  , size_t  ) __attribute__((__nonnull__(1,2)));
   





 
extern __declspec(__nothrow) int strcmp(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));
   




 
extern __declspec(__nothrow) int strncmp(const char *  , const char *  , size_t  ) __attribute__((__nonnull__(1,2)));
   






 
extern __declspec(__nothrow) int strcasecmp(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));
   





 
extern __declspec(__nothrow) int strncasecmp(const char *  , const char *  , size_t  ) __attribute__((__nonnull__(1,2)));
   






 
extern __declspec(__nothrow) int strcoll(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));
   







 

extern __declspec(__nothrow) size_t strxfrm(char * __restrict  , const char * __restrict  , size_t  ) __attribute__((__nonnull__(2)));
   













 


#line 193 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"
extern __declspec(__nothrow) void *memchr(const void *  , int  , size_t  ) __attribute__((__nonnull__(1)));

   





 

#line 209 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"
extern __declspec(__nothrow) char *strchr(const char *  , int  ) __attribute__((__nonnull__(1)));

   




 

extern __declspec(__nothrow) size_t strcspn(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));
   




 

#line 232 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"
extern __declspec(__nothrow) char *strpbrk(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));

   




 

#line 247 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"
extern __declspec(__nothrow) char *strrchr(const char *  , int  ) __attribute__((__nonnull__(1)));

   





 

extern __declspec(__nothrow) size_t strspn(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));
   



 

#line 270 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"
extern __declspec(__nothrow) char *strstr(const char *  , const char *  ) __attribute__((__nonnull__(1,2)));

   





 

extern __declspec(__nothrow) char *strtok(char * __restrict  , const char * __restrict  ) __attribute__((__nonnull__(2)));
extern __declspec(__nothrow) char *_strtok_r(char *  , const char *  , char **  ) __attribute__((__nonnull__(2,3)));

extern __declspec(__nothrow) char *strtok_r(char *  , const char *  , char **  ) __attribute__((__nonnull__(2,3)));

   

































 

extern __declspec(__nothrow) void *memset(void *  , int  , size_t  ) __attribute__((__nonnull__(1)));
   



 
extern __declspec(__nothrow) char *strerror(int  );
   





 
extern __declspec(__nothrow) size_t strlen(const char *  ) __attribute__((__nonnull__(1)));
   



 

extern __declspec(__nothrow) size_t strlcpy(char *  , const char *  , size_t  ) __attribute__((__nonnull__(1,2)));
   
















 

extern __declspec(__nothrow) size_t strlcat(char *  , const char *  , size_t  ) __attribute__((__nonnull__(1,2)));
   






















 

extern __declspec(__nothrow) void _membitcpybl(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitcpybb(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitcpyhl(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitcpyhb(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitcpywl(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitcpywb(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitmovebl(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitmovebb(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitmovehl(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitmovehb(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitmovewl(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
extern __declspec(__nothrow) void _membitmovewb(void *  , const void *  , int  , int  , size_t  ) __attribute__((__nonnull__(1,2)));
    














































 







#line 502 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\string.h"



 

#line 10 "..\\..\\..\\..\\Libraries\\inc\\base_types.h"
#line 1 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stddef.h"
 






 

 
 
 





 





#line 34 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stddef.h"




  typedef signed int ptrdiff_t;



  



    typedef unsigned int size_t;    
#line 57 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stddef.h"



   



      typedef unsigned short wchar_t;  
#line 82 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stddef.h"



    




   




  typedef long double max_align_t;









#line 114 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stddef.h"



 

#line 11 "..\\..\\..\\..\\Libraries\\inc\\base_types.h"
#line 1 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdint.h"
 
 





 









     
#line 27 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdint.h"
     











#line 46 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdint.h"





 

     

     
typedef   signed          char int8_t;
typedef   signed short     int int16_t;
typedef   signed           int int32_t;
typedef   signed       __int64 int64_t;

     
typedef unsigned          char uint8_t;
typedef unsigned short     int uint16_t;
typedef unsigned           int uint32_t;
typedef unsigned       __int64 uint64_t;

     

     
     
typedef   signed          char int_least8_t;
typedef   signed short     int int_least16_t;
typedef   signed           int int_least32_t;
typedef   signed       __int64 int_least64_t;

     
typedef unsigned          char uint_least8_t;
typedef unsigned short     int uint_least16_t;
typedef unsigned           int uint_least32_t;
typedef unsigned       __int64 uint_least64_t;

     

     
typedef   signed           int int_fast8_t;
typedef   signed           int int_fast16_t;
typedef   signed           int int_fast32_t;
typedef   signed       __int64 int_fast64_t;

     
typedef unsigned           int uint_fast8_t;
typedef unsigned           int uint_fast16_t;
typedef unsigned           int uint_fast32_t;
typedef unsigned       __int64 uint_fast64_t;

     




typedef   signed           int intptr_t;
typedef unsigned           int uintptr_t;


     
typedef   signed     long long intmax_t;
typedef unsigned     long long uintmax_t;




     

     





     





     





     

     





     





     





     

     





     





     





     

     






     






     






     

     


     


     


     

     
#line 216 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdint.h"

     



     






     
    
 



#line 241 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdint.h"

     







     










     











#line 305 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\stdint.h"






 
#line 12 "..\\..\\..\\..\\Libraries\\inc\\base_types.h"
#line 1 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\assert.h"
 
 
 
 





 









 





 

#line 43 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\assert.h"
    extern __declspec(__nothrow) __declspec(__noreturn) void abort(void);
    extern __declspec(__nothrow) __declspec(__noreturn) void __aeabi_assert(const char *, const char *, int) __attribute__((__nonnull__(1,2)));
#line 53 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\assert.h"

#line 77 "C:\\Keil_v5\\ARM\\ARM_Compiler_5.06u7\\Bin\\..\\include\\assert.h"





 

#line 13 "..\\..\\..\\..\\Libraries\\inc\\base_types.h"


 
 
 

     




     



 


 


 









 

 
typedef uint8_t      boolean_t;

 
typedef float        float32_t;

 
typedef double       float64_t;

 
typedef char         char_t;

 
typedef void (*func_ptr_t)(void);

 
typedef void (*func_ptr_arg1_t)(uint8_t u8Param);

typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;

typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;


typedef enum {ERROR = 0, SUCCESS = !ERROR} ErrorStatus;
 
 
 





#line 110 "..\\..\\..\\..\\Libraries\\inc\\base_types.h"






 


    






 

     
    void assert_failed(uint8_t* file, uint32_t line);




 
 
 



 
 
 



#line 28 "..\\USER\\src\\..\\inc\\main.h"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
























 





 




 












 



 
 
 

typedef enum {
 
  Reset_IRQn                = -15,               
  NonMaskableInt_IRQn       = -14,               
  HardFault_IRQn            = -13,               
  SVCall_IRQn               =  -5,               
  PendSV_IRQn               =  -2,               
  SysTick_IRQn              =  -1,               
 
  WDT_IRQn                  =   0,               
  LVD_IRQn                  =   1,               
  RTC_IRQn                  =   2,               
  FLASHRAM_IRQn             =   3,               
  SYSCTRL_IRQn              =   4,               
  GPIOA_IRQn                =   5,               
  GPIOB_IRQn                =   6,               
  ADC_IRQn                  =  12,               
  ATIM_IRQn                 =  13,               
  VC1_IRQn                  =  14,               
  VC2_IRQn                  =  15,               
  GTIM1_IRQn                =  16,               
  LPTIM_IRQn                =  19,               
  BTIM1_IRQn                =  20,               
  BTIM2_IRQn                =  21,               
  BTIM3_IRQn                =  22,               
  I2C1_IRQn                 =  23,               
  SPI_IRQn                  =  25,               
  UART1_IRQn                =  27,               
  UART2_IRQn                =  28,               
  CLKFAULT_IRQn             =  31                
} IRQn_Type;



 
 
 

 
#line 102 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


   

#line 1 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"
 




 
















 










#line 35 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"

















 




 



 

#line 1 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_version.h"
 




 
















 










 
#line 64 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"
 
 









 







#line 114 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"

#line 1 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_compiler.h"
 




 
















 




#line 29 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_compiler.h"



 
#line 1 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_armcc.h"
 




 
















 









 













   
   
   

 




 
#line 111 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_armcc.h"

 





















 



 




 






 







 






 








 






 






 








 








 

__attribute__((section(".rev16_text"))) static __inline __asm uint32_t __REV16(uint32_t value)
{
  rev16 r0, r0
  bx lr
}








 

__attribute__((section(".revsh_text"))) static __inline __asm int16_t __REVSH(int16_t value)
{
  revsh r0, r0
  bx lr
}









 









 








 




__attribute__((always_inline)) static __inline uint32_t __RBIT(uint32_t value)
{
  uint32_t result;
  uint32_t s = (4U   * 8U) - 1U;  

  result = value;                       
  for (value >>= 1U; value != 0U; value >>= 1U)
  {
    result <<= 1U;
    result |= value & 1U;
    s--;
  }
  result <<= s;                         
  return result;
}








 



#line 473 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_armcc.h"







 
__attribute__((always_inline)) static __inline int32_t __SSAT(int32_t val, uint32_t sat)
{
  if ((sat >= 1U) && (sat <= 32U))
  {
    const int32_t max = (int32_t)((1U << (sat - 1U)) - 1U);
    const int32_t min = -1 - max ;
    if (val > max)
    {
      return max;
    }
    else if (val < min)
    {
      return min;
    }
  }
  return val;
}







 
__attribute__((always_inline)) static __inline uint32_t __USAT(int32_t val, uint32_t sat)
{
  if (sat <= 31U)
  {
    const uint32_t max = ((1U << sat) - 1U);
    if (val > (int32_t)max)
    {
      return max;
    }
    else if (val < 0)
    {
      return 0U;
    }
  }
  return (uint32_t)val;
}




   


 



 





 
 






 
 





 
static __inline uint32_t __get_CONTROL(void)
{
  register uint32_t __regControl         __asm("control");
  return(__regControl);
}






 
static __inline void __set_CONTROL(uint32_t control)
{
  register uint32_t __regControl         __asm("control");
  __regControl = control;
  __isb(0xF);
}






 
static __inline uint32_t __get_IPSR(void)
{
  register uint32_t __regIPSR          __asm("ipsr");
  return(__regIPSR);
}






 
static __inline uint32_t __get_APSR(void)
{
  register uint32_t __regAPSR          __asm("apsr");
  return(__regAPSR);
}






 
static __inline uint32_t __get_xPSR(void)
{
  register uint32_t __regXPSR          __asm("xpsr");
  return(__regXPSR);
}






 
static __inline uint32_t __get_PSP(void)
{
  register uint32_t __regProcessStackPointer  __asm("psp");
  return(__regProcessStackPointer);
}






 
static __inline void __set_PSP(uint32_t topOfProcStack)
{
  register uint32_t __regProcessStackPointer  __asm("psp");
  __regProcessStackPointer = topOfProcStack;
}






 
static __inline uint32_t __get_MSP(void)
{
  register uint32_t __regMainStackPointer     __asm("msp");
  return(__regMainStackPointer);
}






 
static __inline void __set_MSP(uint32_t topOfMainStack)
{
  register uint32_t __regMainStackPointer     __asm("msp");
  __regMainStackPointer = topOfMainStack;
}






 
static __inline uint32_t __get_PRIMASK(void)
{
  register uint32_t __regPriMask         __asm("primask");
  return(__regPriMask);
}






 
static __inline void __set_PRIMASK(uint32_t priMask)
{
  register uint32_t __regPriMask         __asm("primask");
  __regPriMask = (priMask);
}


#line 764 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_armcc.h"






 
static __inline uint32_t __get_FPSCR(void)
{





   return(0U);

}






 
static __inline void __set_FPSCR(uint32_t fpscr)
{





  (void)fpscr;

}


 


 



 

#line 885 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_armcc.h"
 


#line 35 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_compiler.h"




 
#line 280 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\cmsis_compiler.h"




#line 116 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"

















 
#line 160 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"

 






 
#line 176 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"

 




 











 



 






 



 
typedef union
{
  struct
  {
    uint32_t _reserved0:28;               
    uint32_t V:1;                         
    uint32_t C:1;                         
    uint32_t Z:1;                         
    uint32_t N:1;                         
  } b;                                    
  uint32_t w;                             
} APSR_Type;

 















 
typedef union
{
  struct
  {
    uint32_t ISR:9;                       
    uint32_t _reserved0:23;               
  } b;                                    
  uint32_t w;                             
} IPSR_Type;

 






 
typedef union
{
  struct
  {
    uint32_t ISR:9;                       
    uint32_t _reserved0:15;               
    uint32_t T:1;                         
    uint32_t _reserved1:3;                
    uint32_t V:1;                         
    uint32_t C:1;                         
    uint32_t Z:1;                         
    uint32_t N:1;                         
  } b;                                    
  uint32_t w;                             
} xPSR_Type;

 





















 
typedef union
{
  struct
  {
    uint32_t nPRIV:1;                     
    uint32_t SPSEL:1;                     
    uint32_t _reserved1:30;               
  } b;                                    
  uint32_t w;                             
} CONTROL_Type;

 






 







 



 
typedef struct
{
  volatile uint32_t ISER[1U];                
        uint32_t RESERVED0[31U];
  volatile uint32_t ICER[1U];                
        uint32_t RESERVED1[31U];
  volatile uint32_t ISPR[1U];                
        uint32_t RESERVED2[31U];
  volatile uint32_t ICPR[1U];                
        uint32_t RESERVED3[31U];
        uint32_t RESERVED4[64U];
  volatile uint32_t IP[8U];                  
}  NVIC_Type;

 







 



 
typedef struct
{
  volatile const  uint32_t CPUID;                   
  volatile uint32_t ICSR;                    

  volatile uint32_t VTOR;                    



  volatile uint32_t AIRCR;                   
  volatile uint32_t SCR;                     
  volatile uint32_t CCR;                     
        uint32_t RESERVED1;
  volatile uint32_t SHP[2U];                 
  volatile uint32_t SHCSR;                   
} SCB_Type;

 















 




























 




 















 









 






 



 







 



 
typedef struct
{
  volatile uint32_t CTRL;                    
  volatile uint32_t LOAD;                    
  volatile uint32_t VAL;                     
  volatile const  uint32_t CALIB;                   
} SysTick_Type;

 












 



 



 









 

#line 602 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"








 
 







 






 







 


 







 

 














 









 


 



 





 

#line 701 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"
 





#line 716 "C:\\Users\\kason\\AppData\\Local\\Arm\\Packs\\ARM\\CMSIS\\5.9.0\\CMSIS\\Core\\Include\\core_cm0plus.h"




 





 
 












 
static __inline void __NVIC_EnableIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0)
  {
    __memory_changed();
    ((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->ISER[0U] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
    __memory_changed();
  }
}









 
static __inline uint32_t __NVIC_GetEnableIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0)
  {
    return((uint32_t)(((((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->ISER[0U] & (1UL << (((uint32_t)IRQn) & 0x1FUL))) != 0UL) ? 1UL : 0UL));
  }
  else
  {
    return(0U);
  }
}







 
static __inline void __NVIC_DisableIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0)
  {
    ((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->ICER[0U] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
    __dsb(0xF);
    __isb(0xF);
  }
}









 
static __inline uint32_t __NVIC_GetPendingIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0)
  {
    return((uint32_t)(((((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->ISPR[0U] & (1UL << (((uint32_t)IRQn) & 0x1FUL))) != 0UL) ? 1UL : 0UL));
  }
  else
  {
    return(0U);
  }
}







 
static __inline void __NVIC_SetPendingIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0)
  {
    ((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->ISPR[0U] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
  }
}







 
static __inline void __NVIC_ClearPendingIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0)
  {
    ((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->ICPR[0U] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
  }
}










 
static __inline void __NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority)
{
  if ((int32_t)(IRQn) >= 0)
  {
    ((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->IP[( (((uint32_t)(int32_t)(IRQn)) >> 2UL) )]  = ((uint32_t)(((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->IP[( (((uint32_t)(int32_t)(IRQn)) >> 2UL) )]  & ~(0xFFUL << ( ((((uint32_t)(int32_t)(IRQn)) ) & 0x03UL) * 8UL))) |
       (((priority << (8U - 2)) & (uint32_t)0xFFUL) << ( ((((uint32_t)(int32_t)(IRQn)) ) & 0x03UL) * 8UL)));
  }
  else
  {
    ((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->SHP[( (((((uint32_t)(int32_t)(IRQn)) & 0x0FUL)-8UL) >> 2UL) )] = ((uint32_t)(((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->SHP[( (((((uint32_t)(int32_t)(IRQn)) & 0x0FUL)-8UL) >> 2UL) )] & ~(0xFFUL << ( ((((uint32_t)(int32_t)(IRQn)) ) & 0x03UL) * 8UL))) |
       (((priority << (8U - 2)) & (uint32_t)0xFFUL) << ( ((((uint32_t)(int32_t)(IRQn)) ) & 0x03UL) * 8UL)));
  }
}










 
static __inline uint32_t __NVIC_GetPriority(IRQn_Type IRQn)
{

  if ((int32_t)(IRQn) >= 0)
  {
    return((uint32_t)(((((NVIC_Type *) ((0xE000E000UL) + 0x0100UL) )->IP[ ( (((uint32_t)(int32_t)(IRQn)) >> 2UL) )] >> ( ((((uint32_t)(int32_t)(IRQn)) ) & 0x03UL) * 8UL) ) & (uint32_t)0xFFUL) >> (8U - 2)));
  }
  else
  {
    return((uint32_t)(((((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->SHP[( (((((uint32_t)(int32_t)(IRQn)) & 0x0FUL)-8UL) >> 2UL) )] >> ( ((((uint32_t)(int32_t)(IRQn)) ) & 0x03UL) * 8UL) ) & (uint32_t)0xFFUL) >> (8U - 2)));
  }
}












 
static __inline uint32_t NVIC_EncodePriority (uint32_t PriorityGroup, uint32_t PreemptPriority, uint32_t SubPriority)
{
  uint32_t PriorityGroupTmp = (PriorityGroup & (uint32_t)0x07UL);    
  uint32_t PreemptPriorityBits;
  uint32_t SubPriorityBits;

  PreemptPriorityBits = ((7UL - PriorityGroupTmp) > (uint32_t)(2)) ? (uint32_t)(2) : (uint32_t)(7UL - PriorityGroupTmp);
  SubPriorityBits     = ((PriorityGroupTmp + (uint32_t)(2)) < (uint32_t)7UL) ? (uint32_t)0UL : (uint32_t)((PriorityGroupTmp - 7UL) + (uint32_t)(2));

  return (
           ((PreemptPriority & (uint32_t)((1UL << (PreemptPriorityBits)) - 1UL)) << SubPriorityBits) |
           ((SubPriority     & (uint32_t)((1UL << (SubPriorityBits    )) - 1UL)))
         );
}












 
static __inline void NVIC_DecodePriority (uint32_t Priority, uint32_t PriorityGroup, uint32_t* const pPreemptPriority, uint32_t* const pSubPriority)
{
  uint32_t PriorityGroupTmp = (PriorityGroup & (uint32_t)0x07UL);    
  uint32_t PreemptPriorityBits;
  uint32_t SubPriorityBits;

  PreemptPriorityBits = ((7UL - PriorityGroupTmp) > (uint32_t)(2)) ? (uint32_t)(2) : (uint32_t)(7UL - PriorityGroupTmp);
  SubPriorityBits     = ((PriorityGroupTmp + (uint32_t)(2)) < (uint32_t)7UL) ? (uint32_t)0UL : (uint32_t)((PriorityGroupTmp - 7UL) + (uint32_t)(2));

  *pPreemptPriority = (Priority >> SubPriorityBits) & (uint32_t)((1UL << (PreemptPriorityBits)) - 1UL);
  *pSubPriority     = (Priority                   ) & (uint32_t)((1UL << (SubPriorityBits    )) - 1UL);
}











 
static __inline void __NVIC_SetVector(IRQn_Type IRQn, uint32_t vector)
{

  uint32_t *vectors = (uint32_t *)((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->VTOR;
  vectors[(int32_t)IRQn + 16] = vector;




   
}









 
static __inline uint32_t __NVIC_GetVector(IRQn_Type IRQn)
{

  uint32_t *vectors = (uint32_t *)((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->VTOR;
  return vectors[(int32_t)IRQn + 16];




}





 
__declspec(noreturn) static __inline void __NVIC_SystemReset(void)
{
  __dsb(0xF);                                                          
 
  ((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->AIRCR  = ((0x5FAUL << 16U) |
                 (1UL << 2U));
  __dsb(0xF);                                                           

  for(;;)                                                            
  {
    __nop();
  }
}

 

 







 





 








 
static __inline uint32_t SCB_GetFPUType(void)
{
    return 0U;            
}


 



 





 













 
static __inline uint32_t SysTick_Config(uint32_t ticks)
{
  if ((ticks - 1UL) > (0xFFFFFFUL ))
  {
    return (1UL);                                                    
  }

  ((SysTick_Type *) ((0xE000E000UL) + 0x0010UL) )->LOAD  = (uint32_t)(ticks - 1UL);                          
  __NVIC_SetPriority (SysTick_IRQn, (1UL << 2) - 1UL);  
  ((SysTick_Type *) ((0xE000E000UL) + 0x0010UL) )->VAL   = 0UL;                                              
  ((SysTick_Type *) ((0xE000E000UL) + 0x0010UL) )->CTRL  = (1UL << 2U) |
                   (1UL << 1U)   |
                   (1UL );                          
  return (0UL);                                                      
}



 










#line 107 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"

#line 117 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


 

  #pragma push
  #pragma anon_unions
#line 142 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


 
 
 




 



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR;                           

    struct {
      volatile uint32_t EN         : 1;             
      volatile uint32_t BGREN      : 1;             
      volatile uint32_t TSEN       : 1;             
      volatile uint32_t CONT       : 1;             
      volatile uint32_t CLK        : 2;             
      volatile uint32_t ENS        : 3;             
    } CR_f;
  } ;
  volatile const  uint32_t  RESERVED;

  union {
    volatile uint32_t START;                        

    struct {
      volatile uint32_t START      : 1;             
    } START_f;
  } ;
  volatile const  uint32_t  RESERVED1;

  union {
    volatile uint32_t AWDTR;                        

    struct {
      volatile uint32_t VTL        : 12;            
      volatile const  uint32_t            : 4;
      volatile uint32_t VTH        : 12;            
    } AWDTR_f;
  } ;
  volatile const  uint32_t  RESERVED2;

  union {
    volatile uint32_t TRIGGER;                      

    struct {
      volatile uint32_t ATIMTRGO   : 1;             
      volatile uint32_t ATIMTRGO2  : 1;             
      volatile uint32_t ATIMCC1    : 1;             
      volatile uint32_t ATIMCC2    : 1;             
      volatile uint32_t ATIMCC3    : 1;             
      volatile uint32_t ATIMCC4    : 1;             
      volatile uint32_t ATIMCC5    : 1;             
      volatile uint32_t ATIMCC6    : 1;             
      volatile uint32_t GTIM1TRGO  : 1;             
      volatile uint32_t GTIM1CC1   : 1;             
      volatile uint32_t GTIM1CC2   : 1;             
      volatile uint32_t GTIM1CC3   : 1;             
      volatile uint32_t GTIM1CC4   : 1;             
      volatile uint32_t BTIM1TRGO  : 1;             
      volatile uint32_t BTIM2TRGO  : 1;             
      volatile uint32_t BTIM3TRGO  : 1;             
      volatile uint32_t SPI1       : 1;             
      volatile uint32_t UART1      : 1;             
      volatile uint32_t UART2      : 1;             
    } TRIGGER_f;
  } ;
  volatile const  uint32_t  RESERVED3;

  union {
    volatile uint32_t AWDCR;                        

    struct {
      volatile uint32_t IN0        : 1;             
      volatile uint32_t IN1        : 1;             
      volatile uint32_t IN2        : 1;             
      volatile uint32_t IN3        : 1;             
      volatile uint32_t IN4        : 1;             
      volatile uint32_t IN5        : 1;             
      volatile uint32_t IN6        : 1;             
      volatile uint32_t IN7        : 1;             
      volatile uint32_t IN8        : 1;             
      volatile uint32_t IN9        : 1;             
      volatile uint32_t IN10       : 1;             
      volatile uint32_t IN11       : 1;             
      volatile uint32_t IN12       : 1;             
      volatile uint32_t IN13       : 1;             
      volatile uint32_t IN14       : 1;             
      volatile uint32_t IN15       : 1;             
    } AWDCR_f;
  } ;
  volatile const  uint32_t  RESERVED4;

  union {
    volatile uint32_t SAMPLE;                       

    struct {
      volatile uint32_t SQRCH0     : 4;             
      volatile uint32_t SQRCH1     : 4;             
      volatile uint32_t SQRCH2     : 4;             
      volatile uint32_t SQRCH3     : 4;             
      volatile uint32_t SQRCH4     : 4;             
      volatile uint32_t SQRCH5     : 4;             
      volatile uint32_t SQRCH6     : 4;             
      volatile uint32_t SQRCH7     : 4;             
    } SAMPLE_f;
  } ;

  union {
    volatile uint32_t SQRCFR;                       

    struct {
      volatile uint32_t SQRCH0     : 4;             
      volatile uint32_t SQRCH1     : 4;             
      volatile uint32_t SQRCH2     : 4;             
      volatile uint32_t SQRCH3     : 4;             
      volatile uint32_t SQRCH4     : 4;             
      volatile uint32_t SQRCH5     : 4;             
      volatile uint32_t SQRCH6     : 4;             
      volatile uint32_t SQRCH7     : 4;             
    } SQRCFR_f;
  } ;
  volatile const  uint32_t  RESERVED5[4];

  union {
    volatile const  uint32_t RESULT0;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT0_f;
  } ;

  union {
    volatile const  uint32_t RESULT1;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT1_f;
  } ;

  union {
    volatile const  uint32_t RESULT2;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT2_f;
  } ;

  union {
    volatile const  uint32_t RESULT3;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT3_f;
  } ;

  union {
    volatile const  uint32_t RESULT4;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT4_f;
  } ;

  union {
    volatile const  uint32_t RESULT5;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT5_f;
  } ;

  union {
    volatile const  uint32_t RESULT6;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT6_f;
  } ;

  union {
    volatile const  uint32_t RESULT7;                      

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT7_f;
  } ;
  volatile const  uint32_t  RESERVED6[5];

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t EOC        : 1;             
      volatile uint32_t EOS        : 1;             
      volatile uint32_t AWDL       : 1;             
      volatile uint32_t AWDH       : 1;             
    } IER_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t EOC        : 1;             
      volatile uint32_t EOS        : 1;             
      volatile uint32_t AWDL       : 1;             
      volatile uint32_t AWDH       : 1;             
    } ICR_f;
  } ;

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t EOC        : 1;             
      volatile const  uint32_t EOS        : 1;             
      volatile const  uint32_t AWDL       : 1;             
      volatile const  uint32_t AWDH       : 1;             
    } ISR_f;
  } ;
} ADC_TypeDef;                                      



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t CEN        : 1;             
      volatile uint32_t UDIS       : 1;             
      volatile uint32_t URS        : 1;             
      volatile uint32_t OPM        : 1;             
      volatile uint32_t DIR        : 1;             
      volatile uint32_t CMS        : 2;             
      volatile uint32_t ARPE       : 1;             
      volatile uint32_t CKD        : 2;             
      volatile const  uint32_t            : 1;
      volatile uint32_t UIFREMAP   : 1;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile uint32_t CCPC       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t CCUS       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t MMS        : 3;             
      volatile uint32_t TI1S       : 1;             
      volatile uint32_t OIS1       : 1;             
      volatile uint32_t OIS1N      : 1;             
      volatile uint32_t OIS2       : 1;             
      volatile uint32_t OIS2N      : 1;             
      volatile uint32_t OIS3       : 1;             
      volatile uint32_t OIS3N      : 1;             
      volatile uint32_t OIS4       : 1;             
      volatile uint32_t OIS4N      : 1;             
      volatile uint32_t OIS5       : 1;             
      volatile uint32_t OIS5N      : 1;             
      volatile uint32_t OIS6       : 1;             
      volatile uint32_t OIS6N      : 1;             
      volatile uint32_t MMS2       : 5;             
      volatile uint32_t MMSH       : 2;             
    } CR2_f;
  } ;

  union {
    volatile uint32_t SMCR;                         

    struct {
      volatile uint32_t SMS        : 3;             
      volatile uint32_t OCCS       : 1;             
      volatile uint32_t TS         : 3;             
      volatile uint32_t MSM        : 1;             
      volatile uint32_t ETF        : 4;             
      volatile uint32_t ETPS       : 2;             
      volatile uint32_t ECE        : 1;             
      volatile uint32_t ETP        : 1;             
      volatile uint32_t SMSH       : 1;             
      volatile const  uint32_t            : 3;
      volatile uint32_t TSH        : 2;             
      volatile const  uint32_t            : 2;
      volatile uint32_t SMSPE      : 1;             
      volatile uint32_t SMSPS      : 1;             
    } SMCR_f;
  } ;

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t UIE        : 1;             
      volatile uint32_t CC1IE      : 1;             
      volatile uint32_t CC2IE      : 1;             
      volatile uint32_t CC3IE      : 1;             
      volatile uint32_t CC4IE      : 1;             
      volatile uint32_t COMIE      : 1;             
      volatile uint32_t TIE        : 1;             
      volatile uint32_t BIE        : 1;             
            uint32_t            : 8;
      volatile uint32_t CC5IE      : 1;             
      volatile uint32_t CC6IE      : 1;             
            uint32_t            : 2;
      volatile uint32_t IDXIE      : 1;             
      volatile uint32_t DIRIE      : 1;             
      volatile uint32_t IERRIE     : 1;             
      volatile uint32_t TERRIE     : 1;             
    } IER_f;
  } ;

  union {
    volatile uint32_t ISR;                          

    struct {
      volatile uint32_t UIF        : 1;             
      volatile uint32_t CC1IF      : 1;             
      volatile uint32_t CC2IF      : 1;             
      volatile uint32_t CC3IF      : 1;             
      volatile uint32_t CC4IF      : 1;             
      volatile uint32_t COMIF      : 1;             
      volatile uint32_t TIF        : 1;             
      volatile uint32_t BIF        : 1;             
      volatile uint32_t B2IF       : 1;             
      volatile uint32_t CC1OF      : 1;             
      volatile uint32_t CC2OF      : 1;             
      volatile uint32_t CC3OF      : 1;             
      volatile uint32_t CC4OF      : 1;             
      volatile uint32_t SBIF       : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t CC5IF      : 1;             
      volatile uint32_t CC6IF      : 1;             
      volatile uint32_t CC5OF      : 1;             
      volatile uint32_t CC6OF      : 1;             
      volatile uint32_t IDXF       : 1;             
      volatile uint32_t DIRF       : 1;             
      volatile uint32_t IERRF      : 1;             
      volatile uint32_t TERRF      : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t EGR;                          

    struct {
      volatile uint32_t UG         : 1;             
      volatile uint32_t CC1G       : 1;             
      volatile uint32_t CC2G       : 1;             
      volatile uint32_t CC3G       : 1;             
      volatile uint32_t CC4G       : 1;             
      volatile uint32_t COMG       : 1;             
      volatile uint32_t TG         : 1;             
      volatile uint32_t BG         : 1;             
      volatile uint32_t B2G        : 1;             
      volatile const  uint32_t            : 7;
      volatile uint32_t CC5G       : 1;             
      volatile uint32_t CC6G       : 1;             
    } EGR_f;
  } ;

  union {
    union {
      volatile uint32_t CCMR1CAP;                   

      struct {
        volatile uint32_t CC1S     : 2;             
        volatile uint32_t IC1PSC   : 2;             
        volatile uint32_t IC1F     : 4;             
        volatile uint32_t CC2S     : 2;             
        volatile uint32_t IC2PSC   : 2;             
        volatile uint32_t IC2F     : 4;             
      } CCMR1CAP_f;
    } ;

    union {
      volatile uint32_t CCMR1CMP;                   

      struct {
        volatile uint32_t CC1S     : 2;             
        volatile uint32_t OC1FE    : 1;             
        volatile uint32_t OC1PE    : 1;             
        volatile uint32_t OC1M     : 3;             
        volatile uint32_t OC1CE    : 1;             
        volatile uint32_t CC2S     : 2;             
        volatile uint32_t OC2FE    : 1;             
        volatile uint32_t OC2PE    : 1;             
        volatile uint32_t OC2M     : 3;             
        volatile uint32_t OC2CE    : 1;             
        volatile uint32_t OC1MH    : 1;             
        volatile const  uint32_t          : 7;
        volatile uint32_t OC2MH    : 1;             
      } CCMR1CMP_f;
    } ;
  };

  union {
    union {
      volatile uint32_t CCMR2CAP;                   

      struct {
        volatile uint32_t CC3S     : 2;             
        volatile uint32_t IC3PSC   : 2;             
        volatile uint32_t IC3F     : 4;             
        volatile uint32_t CC4S     : 2;             
        volatile uint32_t IC4PSC   : 2;             
        volatile uint32_t IC4F     : 4;             
      } CCMR2CAP_f;
    } ;

    union {
      volatile uint32_t CCMR2CMP;                   

      struct {
        volatile uint32_t CC3S     : 2;             
        volatile uint32_t OC3FE    : 1;             
        volatile uint32_t OC3PE    : 1;             
        volatile uint32_t OC3M     : 3;             
        volatile uint32_t OC3CE    : 1;             
        volatile uint32_t CC4S     : 2;             
        volatile uint32_t OC4FE    : 1;             
        volatile uint32_t OC4PE    : 1;             
        volatile uint32_t OC4M     : 3;             
        volatile uint32_t OC4CE    : 1;             
        volatile uint32_t OC3MH    : 1;             
        volatile const  uint32_t          : 7;
        volatile uint32_t OC4MH    : 1;             
      } CCMR2CMP_f;
    } ;
  };

  union {
    volatile uint32_t CCER;                         

    struct {
      volatile uint32_t CC1E       : 1;             
      volatile uint32_t CC1P       : 1;             
      volatile uint32_t CC1NE      : 1;             
      volatile uint32_t CC1NP      : 1;             
      volatile uint32_t CC2E       : 1;             
      volatile uint32_t CC2P       : 1;             
      volatile uint32_t CC2NE      : 1;             
      volatile uint32_t CC2NP      : 1;             
      volatile uint32_t CC3E       : 1;             
      volatile uint32_t CC3P       : 1;             
      volatile uint32_t CC3NE      : 1;             
      volatile uint32_t CC3NP      : 1;             
      volatile uint32_t CC4E       : 1;             
      volatile uint32_t CC4P       : 1;             
      volatile uint32_t CC4NE      : 1;             
      volatile uint32_t CC4NP      : 1;             
      volatile uint32_t CC5E       : 1;             
      volatile uint32_t CC5P       : 1;             
      volatile uint32_t CC5NE      : 1;             
      volatile uint32_t CC5NP      : 1;             
      volatile uint32_t CC6E       : 1;             
      volatile uint32_t CC6P       : 1;             
      volatile uint32_t CC6NE      : 1;             
      volatile uint32_t CC6NP      : 1;             
    } CCER_f;
  } ;

  union {
    volatile uint32_t CNT;                          

    struct {
      volatile uint32_t CNT        : 16;            
      volatile const  uint32_t            : 15;
      volatile const  uint32_t UIFCPY     : 1;             
    } CNT_f;
  } ;

  union {
    volatile uint32_t PSC;                          

    struct {
      volatile uint32_t PSC        : 16;            
    } PSC_f;
  } ;

  union {
    volatile uint32_t ARR;                          

    struct {
      volatile uint32_t ARR        : 16;            
    } ARR_f;
  } ;

  union {
    volatile uint32_t RCR;                          

    struct {
      volatile uint32_t REP        : 16;            
    } RCR_f;
  } ;

  union {
    volatile uint32_t CCR1;                         

    struct {
      volatile uint32_t CCR1       : 16;            
    } CCR1_f;
  } ;

  union {
    volatile uint32_t CCR2;                         

    struct {
      volatile uint32_t CCR2       : 16;            
    } CCR2_f;
  } ;

  union {
    volatile uint32_t CCR3;                         

    struct {
      volatile uint32_t CCR3       : 16;            
    } CCR3_f;
  } ;

  union {
    volatile uint32_t CCR4;                         

    struct {
      volatile uint32_t CCR4       : 16;            
    } CCR4_f;
  } ;

  union {
    volatile uint32_t BDTR;                         

    struct {
      volatile uint32_t DTG        : 8;             
      volatile uint32_t LOCK       : 2;             
      volatile uint32_t OSSI       : 1;             
      volatile uint32_t OSSR       : 1;             
      volatile uint32_t BKE        : 1;             
      volatile uint32_t BKP        : 1;             
      volatile uint32_t AOE        : 1;             
      volatile uint32_t MOE        : 1;             
      volatile uint32_t BKF        : 4;             
      volatile uint32_t BK2F       : 4;             
      volatile uint32_t BK2E       : 1;             
      volatile uint32_t BK2P       : 1;             
    } BDTR_f;
  } ;

  union {
    volatile uint32_t CCR5;                         

    struct {
      volatile uint32_t CCR5       : 16;            
      volatile const  uint32_t            : 10;
      volatile uint32_t GC5C1      : 1;             
      volatile uint32_t GC5C2      : 1;             
      volatile uint32_t GC5C3      : 1;             
      volatile uint32_t GC5C4      : 1;             
      volatile uint32_t GC5C5      : 1;             
      volatile uint32_t GC5C6      : 1;             
    } CCR5_f;
  } ;

  union {
    volatile uint32_t CCR6;                         

    struct {
      volatile uint32_t CCR6       : 16;            
      volatile const  uint32_t            : 10;
      volatile uint32_t GC6C1      : 1;             
      volatile uint32_t GC6C2      : 1;             
      volatile uint32_t GC6C3      : 1;             
      volatile uint32_t GC6C4      : 1;             
      volatile uint32_t GC6C5      : 1;             
      volatile uint32_t GC6C6      : 1;             
    } CCR6_f;
  } ;

  union {
    union {
      volatile uint32_t CCMR3CAP;                   

      struct {
        volatile uint32_t CC5S     : 2;             
        volatile uint32_t IC5PSC   : 2;             
        volatile uint32_t IC5F     : 4;             
        volatile uint32_t CC6S     : 2;             
        volatile uint32_t IC6PSC   : 2;             
        volatile uint32_t IC6F     : 4;             
      } CCMR3CAP_f;
    } ;

    union {
      volatile uint32_t CCMR3CMP;                   

      struct {
        volatile uint32_t CC5S     : 2;             
        volatile uint32_t OC5FE    : 1;             
        volatile uint32_t OC5PE    : 1;             
        volatile uint32_t OC5M     : 3;             
        volatile uint32_t OC5CE    : 1;             
        volatile uint32_t CC6S     : 2;             
        volatile uint32_t OC6FE    : 1;             
        volatile uint32_t OC6PE    : 1;             
        volatile uint32_t OC6M     : 3;             
        volatile uint32_t OC6CE    : 1;             
        volatile uint32_t OC5MH    : 1;             
        volatile const  uint32_t          : 7;
        volatile uint32_t OC6MH    : 1;             
      } CCMR3CMP_f;
    } ;
  };

  union {
    volatile uint32_t DTR2;                         

    struct {
      volatile uint32_t DTGF       : 8;             
      volatile const  uint32_t            : 8;
      volatile uint32_t DTAE       : 1;             
      volatile uint32_t DTPE       : 1;             
    } DTR2_f;
  } ;

  union {
    volatile uint32_t ECR;                          

    struct {
      volatile uint32_t IE         : 1;             
      volatile uint32_t IDIR       : 2;             
      volatile const  uint32_t            : 2;
      volatile uint32_t FIDX       : 1;             
      volatile uint32_t IPOS       : 2;             
    } ECR_f;
  } ;

  union {
    volatile uint32_t TISEL1;                       

    struct {
      volatile uint32_t TI1SEL     : 4;             
      volatile const  uint32_t            : 4;
      volatile uint32_t TI2SEL     : 4;             
      volatile const  uint32_t            : 4;
      volatile uint32_t TI3SEL     : 4;             
      volatile const  uint32_t            : 4;
      volatile uint32_t TI4SEL     : 4;             
    } TISEL1_f;
  } ;

  union {
    volatile uint32_t AF1;                          

    struct {
      volatile uint32_t BKINE      : 1;             
      volatile uint32_t BKVC1E     : 1;             
      volatile uint32_t BKVC2E     : 1;             
      volatile const  uint32_t            : 6;
      volatile uint32_t BKINP      : 1;             
      volatile uint32_t BKVC1P     : 1;             
      volatile uint32_t BKVC2P     : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t ETRSEL     : 4;             
    } AF1_f;
  } ;

  union {
    volatile uint32_t AF2;                          

    struct {
      volatile uint32_t BK2INE     : 1;             
      volatile uint32_t BK2VC1E    : 1;             
      volatile uint32_t BK2VC2E    : 1;             
      volatile const  uint32_t            : 6;
      volatile uint32_t BK2INP     : 1;             
      volatile uint32_t BK2VC1P    : 1;             
      volatile uint32_t BK2VC2P    : 1;             
      volatile const  uint32_t            : 4;
      volatile uint32_t OCRSEL     : 3;             
    } AF2_f;
  } ;
  volatile const  uint32_t  RESERVED;

  union {
    volatile uint32_t TISEL2;                       

    struct {
      volatile uint32_t TI5SEL     : 4;             
      volatile const  uint32_t            : 4;
      volatile uint32_t TI6SEL     : 4;             
    } TISEL2_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t UIF        : 1;             
      volatile uint32_t CC1IF      : 1;             
      volatile uint32_t CC2IF      : 1;             
      volatile uint32_t CC3IF      : 1;             
      volatile uint32_t CC4IF      : 1;             
      volatile uint32_t COMIF      : 1;             
      volatile uint32_t TIF        : 1;             
      volatile uint32_t BIF        : 1;             
      volatile uint32_t B2IF       : 1;             
      volatile uint32_t CC1OF      : 1;             
      volatile uint32_t CC2OF      : 1;             
      volatile uint32_t CC3OF      : 1;             
      volatile uint32_t CC4OF      : 1;             
      volatile uint32_t SBIF       : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t CC5IF      : 1;             
      volatile uint32_t CC6IF      : 1;             
      volatile uint32_t CC5OF      : 1;             
      volatile uint32_t CC6OF      : 1;             
      volatile uint32_t IDXF       : 1;             
      volatile uint32_t DIRF       : 1;             
      volatile uint32_t IERRF      : 1;             
      volatile uint32_t TERRF      : 1;             
    } ICR_f;
  } ;
} ATIM_TypeDef;                                     



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t EN         : 1;             
      volatile uint32_t UDIS       : 1;             
      volatile uint32_t URS        : 1;             
      volatile uint32_t ONESHOT    : 1;             
      volatile const  uint32_t            : 7;
      volatile uint32_t UIFREMAP   : 1;             
      volatile const  uint32_t            : 3;
      volatile uint32_t TOGEN      : 1;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile const  uint32_t            : 4;
      volatile uint32_t MMS        : 3;             
    } CR2_f;
  } ;

  union {
    volatile uint32_t SMCR;                         

    struct {
      volatile uint32_t SMS        : 3;             
      volatile uint32_t RSTISRC    : 4;             
      volatile uint32_t TRGISRC    : 4;             
      volatile uint32_t MSM        : 1;             
      volatile uint32_t TRGIFLT    : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t RSTIPOL    : 1;             
      volatile uint32_t TRGIPOL    : 1;             
    } SMCR_f;
  } ;

  union {
    volatile uint32_t IER;                         

    struct {
      volatile uint32_t UIE        : 1;             
      volatile const  uint32_t            : 5;
      volatile uint32_t TIE        : 1;             
    } IER_f;
  } ;

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t UIF        : 1;             
      volatile const  uint32_t            : 5;
      volatile const  uint32_t TIF        : 1;             
    } ISR_f;
  } ;

  union {
    volatile  uint32_t EGR;                          

    struct {
      volatile  uint32_t UG         : 1;             
      volatile const  uint32_t            : 5;
      volatile  uint32_t TG         : 1;             
    } EGR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t UIF        : 1;             
      volatile const  uint32_t            : 5;
      volatile uint32_t TIF        : 1;             
    } ICR_f;
  } ;
  volatile const  uint32_t  RESERVED[2];

  union {
    volatile uint32_t CNT;                          

    struct {
      volatile uint32_t CNT        : 16;            
      volatile const  uint32_t            : 15;
      volatile const  uint32_t UIFCPY     : 1;             
    } CNT_f;
  } ;

  union {
    volatile uint32_t PSC;                          

    struct {
      volatile uint32_t PSC        : 16;            
    } PSC_f;
  } ;

  union {
    volatile uint32_t ARR;                          

    struct {
      volatile uint32_t ARR        : 16;            
    } ARR_f;
  } ;
} BTIM_TypeDef;                                     



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR;                           

    struct {
      volatile uint32_t MODE       : 4;             
    } CR_f;
  } ;
  volatile const  uint32_t  RESERVED;

  union {
    volatile uint32_t DR;                           

    struct {
      volatile uint32_t DR         : 8;             
    } DR_f;
  } ;

  union {
    volatile const  uint32_t RESULT;                       

    struct {
      volatile const  uint32_t RESULT     : 16;            
    } RESULT_f;
  } ;
} CRC_TypeDef;                                      



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t MODE       : 2;             
      volatile const  uint32_t            : 3;
      volatile const  uint32_t SECURITY   : 2;             
      volatile const  uint32_t            : 9;
      volatile  uint32_t KEY        : 16;            
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile uint32_t WAIT       : 3;             
      volatile const  uint32_t            : 13;
      volatile  uint32_t KEY        : 16;            
    } CR2_f;
  } ;

  union {
    volatile uint32_t PAGELOCK;                     

    struct {
      volatile uint32_t LOCK0      : 1;             
      volatile uint32_t LOCK1      : 1;             
      volatile uint32_t LOCK2      : 1;             
      volatile uint32_t LOCK3      : 1;             
      volatile uint32_t LOCK4      : 1;             
      volatile uint32_t LOCK5      : 1;             
      volatile uint32_t LOCK6      : 1;             
      volatile uint32_t LOCK7      : 1;             
      volatile uint32_t LOCK8      : 1;             
      volatile uint32_t LOCK9      : 1;             
      volatile uint32_t LOCK10     : 1;             
      volatile uint32_t LOCK11     : 1;             
      volatile uint32_t LOCK12     : 1;             
      volatile uint32_t LOCK13     : 1;             
      volatile uint32_t LOCK14     : 1;             
      volatile uint32_t LOCK15     : 1;             
      volatile  uint32_t KEY        : 16;            
    } PAGELOCK_f;
  } ;
  volatile const  uint32_t  RESERVED[5];

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t PC         : 1;             
      volatile uint32_t PAGELOCK   : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t PROG       : 1;             
    } IER_f;
  } ;

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t PC         : 1;             
      volatile const  uint32_t PAGELOCK   : 1;             
      volatile const  uint32_t            : 2;
      volatile const  uint32_t PROG       : 1;             
      volatile const  uint32_t BUSY       : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t PC         : 1;             
      volatile uint32_t PAGELOCK   : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t PROG       : 1;             
    } ICR_f;
  } ;
  volatile const  uint32_t  RESERVED1[17];

  union {
    volatile const  uint32_t SDKCFR;                       

    struct {
      volatile const  uint32_t START      : 7;             
      volatile const  uint32_t            : 1;
      volatile const  uint32_t END        : 7;             
    } SDKCFR_f;
  } ;
} FLASH_TypeDef;                                    



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t DIR;                          

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } DIR_f;
  } ;

  union {
    volatile uint32_t OPENDRAIN;                    

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } OPENDRAIN_f;
  } ;
  volatile const  uint32_t  RESERVED[2];

  union {
    volatile uint32_t PUR;                          

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } PUR_f;
  } ;

  union {
    volatile uint32_t AFRH;                         

    struct {
      volatile uint32_t AFR8       : 3;             
    } AFRH_f;
  } ;

  union {
    volatile uint32_t AFRL;                         

    struct {
      volatile uint32_t AFR0       : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AFR1       : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AFR2       : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AFR3       : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AFR4       : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AFR5       : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AFR6       : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AFR7       : 3;             
    } AFRL_f;
  } ;

  union {
    volatile uint32_t ANALOG;                       

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } ANALOG_f;
  } ;
  volatile const  uint32_t  RESERVED1;

  union {
    volatile uint32_t RISEIE;                       

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } RISEIE_f;
  } ;

  union {
    volatile uint32_t FALLIE;                       

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } FALLIE_f;
  } ;
  volatile const  uint32_t  RESERVED2[2];

  union {
    volatile uint32_t ISR;                          

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } ICR_f;
  } ;
  volatile const  uint32_t  RESERVED3;

  union {
    volatile uint32_t FILTER;                       

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
      volatile const  uint32_t            : 7;
      volatile uint32_t FLTCLK     : 3;             
    } FILTER_f;
  } ;
  volatile const  uint32_t  RESERVED4[3];

  union {
    volatile uint32_t IDR;                          

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } IDR_f;
  } ;

  union {
    volatile uint32_t ODR;                          

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } ODR_f;
  } ;

  union {
    volatile uint32_t BRR;                          

    struct {
      volatile uint32_t BRR0       : 1;             
      volatile uint32_t BRR1       : 1;             
      volatile uint32_t BRR2       : 1;             
      volatile uint32_t BRR3       : 1;             
      volatile uint32_t BRR4       : 1;             
      volatile uint32_t BRR5       : 1;             
      volatile uint32_t BRR6       : 1;             
      volatile uint32_t BRR7       : 1;             
      volatile uint32_t BRR8       : 1;             
    } BRR_f;
  } ;

  union {
    volatile uint32_t BSRR;                         

    struct {
      volatile uint32_t BSS0       : 1;             
      volatile uint32_t BSS1       : 1;             
      volatile uint32_t BSS2       : 1;             
      volatile uint32_t BSS3       : 1;             
      volatile uint32_t BSS4       : 1;             
      volatile uint32_t BSS5       : 1;             
      volatile uint32_t BSS6       : 1;             
      volatile uint32_t BSS7       : 1;             
      volatile uint32_t BSS8       : 1;             
      volatile const  uint32_t            : 7;
      volatile uint32_t BRR0       : 1;             
      volatile uint32_t BRR1       : 1;             
      volatile uint32_t BRR2       : 1;             
      volatile uint32_t BRR3       : 1;             
      volatile uint32_t BRR4       : 1;             
      volatile uint32_t BRR5       : 1;             
      volatile uint32_t BRR6       : 1;             
      volatile uint32_t BRR7       : 1;             
      volatile uint32_t BRR8       : 1;             
    } BSRR_f;
  } ;

  union {
    volatile uint32_t TOG;                          

    struct {
      volatile uint32_t PIN0       : 1;             
      volatile uint32_t PIN1       : 1;             
      volatile uint32_t PIN2       : 1;             
      volatile uint32_t PIN3       : 1;             
      volatile uint32_t PIN4       : 1;             
      volatile uint32_t PIN5       : 1;             
      volatile uint32_t PIN6       : 1;             
      volatile uint32_t PIN7       : 1;             
      volatile uint32_t PIN8       : 1;             
    } TOG_f;
  } ;
} GPIO_TypeDef;                                  



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t CEN        : 1;             
      volatile uint32_t UDIS       : 1;             
      volatile uint32_t URS        : 1;             
      volatile uint32_t OPM        : 1;             
      volatile uint32_t DIR        : 1;             
      volatile uint32_t CMS        : 2;             
      volatile uint32_t ARPE       : 1;             
      volatile uint32_t CKD        : 2;             
      volatile const  uint32_t            : 1;
      volatile uint32_t UIFREMAP   : 1;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile const  uint32_t            : 4;
      volatile uint32_t MMS        : 3;             
      volatile uint32_t TI1S       : 1;             
      volatile const  uint32_t            : 17;
      volatile uint32_t MMSH       : 2;             
    } CR2_f;
  } ;

  union {
    volatile uint32_t SMCR;                         

    struct {
      volatile uint32_t SMS        : 3;             
      volatile uint32_t OCCS       : 1;             
      volatile uint32_t TS         : 3;             
      volatile uint32_t MSM        : 1;             
      volatile uint32_t ETF        : 4;             
      volatile uint32_t ETPS       : 2;             
      volatile uint32_t ECE        : 1;             
      volatile uint32_t ETP        : 1;             
      volatile uint32_t SMSH       : 1;             
      volatile const  uint32_t            : 3;
      volatile uint32_t TSH        : 2;             
      volatile const  uint32_t            : 2;
      volatile uint32_t SMSPE      : 1;             
      volatile uint32_t SMSPS      : 1;             
    } SMCR_f;
  } ;

  union {
    volatile uint32_t IER;                         

    struct {
      volatile uint32_t UIE        : 1;             
      volatile uint32_t CC1IE      : 1;             
      volatile uint32_t CC2IE      : 1;             
      volatile uint32_t CC3IE      : 1;             
      volatile uint32_t CC4IE      : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t TIE        : 1;             
      volatile const  uint32_t            : 13;
      volatile uint32_t IDXIE      : 1;             
      volatile uint32_t DIRIE      : 1;             
      volatile uint32_t IERRIE     : 1;             
      volatile uint32_t TERRIE     : 1;             
    } IER_f;
  } ;

  union {
    volatile uint32_t ISR;                          

    struct {
      volatile uint32_t UIF        : 1;             
      volatile uint32_t CC1IF      : 1;             
      volatile uint32_t CC2IF      : 1;             
      volatile uint32_t CC3IF      : 1;             
      volatile uint32_t CC4IF      : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t TIF        : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t CC1OF      : 1;             
      volatile uint32_t CC2OF      : 1;             
      volatile uint32_t CC3OF      : 1;             
      volatile uint32_t CC4OF      : 1;             
      volatile const  uint32_t            : 7;
      volatile uint32_t IDXF       : 1;             
      volatile uint32_t DIRF       : 1;             
      volatile uint32_t IERRF      : 1;             
      volatile uint32_t TERRF      : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t EGR;                          

    struct {
      volatile uint32_t UG         : 1;             
      volatile uint32_t CC1G       : 1;             
      volatile uint32_t CC2G       : 1;             
      volatile uint32_t CC3G       : 1;             
      volatile uint32_t CC4G       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t TG         : 1;             
    } EGR_f;
  } ;

  union {
    union {
      volatile uint32_t CCMR1CAP;                   

      struct {
        volatile uint32_t CC1S     : 2;             
        volatile uint32_t IC1PSC   : 2;             
        volatile uint32_t IC1F     : 4;             
        volatile uint32_t CC2S     : 2;             
        volatile uint32_t IC2PSC   : 2;             
        volatile uint32_t IC2F     : 4;             
      } CCMR1CAP_f;
    } ;

    union {
      volatile uint32_t CCMR1CMP;                   

      struct {
        volatile uint32_t CC1S     : 2;             
        volatile uint32_t OC1FE    : 1;             
        volatile uint32_t OC1PE    : 1;             
        volatile uint32_t OC1M     : 3;             
        volatile uint32_t OC1CE    : 1;             
        volatile uint32_t CC2S     : 2;             
        volatile uint32_t OC2FE    : 1;             
        volatile uint32_t OC2PE    : 1;             
        volatile uint32_t OC2M     : 3;             
        volatile uint32_t OC2CE    : 1;             
        volatile uint32_t OC1MH    : 1;             
        volatile const  uint32_t          : 7;
        volatile uint32_t OC2MH    : 1;             
      } CCMR1CMP_f;
    } ;
  };

  union {
    union {
      volatile uint32_t CCMR2CAP;                   

      struct {
        volatile uint32_t CC3S     : 2;             
        volatile uint32_t IC3PSC   : 2;             
        volatile uint32_t IC3F     : 4;             
        volatile uint32_t CC4S     : 2;             
        volatile uint32_t IC4PSC   : 2;             
        volatile uint32_t IC4F     : 4;             
      } CCMR2CAP_f;
    } ;

    union {
      volatile uint32_t CCMR2CMP;                   

      struct {
        volatile uint32_t CC3S     : 2;             
        volatile uint32_t OC3FE    : 1;             
        volatile uint32_t OC3PE    : 1;             
        volatile uint32_t OC3M     : 3;             
        volatile uint32_t OC3CE    : 1;             
        volatile uint32_t CC4S     : 2;             
        volatile uint32_t OC4FE    : 1;             
        volatile uint32_t OC4PE    : 1;             
        volatile uint32_t OC4M     : 3;             
        volatile uint32_t OC4CE    : 1;             
        volatile uint32_t OC3MH    : 1;             
        volatile const  uint32_t          : 7;
        volatile uint32_t OC4MH    : 1;             
      } CCMR2CMP_f;
    } ;
  };

  union {
    volatile uint32_t CCER;                         

    struct {
      volatile uint32_t CC1E       : 1;             
      volatile uint32_t CC1P       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t CC1NP      : 1;             
      volatile uint32_t CC2E       : 1;             
      volatile uint32_t CC2P       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t CC2NP      : 1;             
      volatile uint32_t CC3E       : 1;             
      volatile uint32_t CC3P       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t CC3NP      : 1;             
      volatile uint32_t CC4E       : 1;             
      volatile uint32_t CC4P       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t CC4NP      : 1;             
    } CCER_f;
  } ;

  union {
    volatile uint32_t CNT;                          

    struct {
      volatile uint32_t CNT        : 16;            
      volatile const  uint32_t            : 15;
      volatile const  uint32_t UIFCPY     : 1;             
    } CNT_f;
  } ;

  union {
    volatile uint32_t PSC;                          

    struct {
      volatile uint32_t PSC        : 16;            
    } PSC_f;
  } ;

  union {
    volatile uint32_t ARR;                          

    struct {
      volatile uint32_t ARR        : 16;            
    } ARR_f;
  } ;
  volatile const  uint32_t  RESERVED;

  union {
    volatile uint32_t CCR1;                         

    struct {
      volatile uint32_t CCR1       : 16;            
    } CCR1_f;
  } ;

  union {
    volatile uint32_t CCR2;                         

    struct {
      volatile uint32_t CCR2       : 16;            
    } CCR2_f;
  } ;

  union {
    volatile uint32_t CCR3;                         

    struct {
      volatile uint32_t CCR3       : 16;            
    } CCR3_f;
  } ;

  union {
    volatile uint32_t CCR4;                         

    struct {
      volatile uint32_t CCR4       : 16;            
    } CCR4_f;
  } ;
  volatile const  uint32_t  RESERVED1[5];

  union {
    volatile uint32_t ECR;                          

    struct {
      volatile uint32_t IE         : 1;             
      volatile uint32_t IDIR       : 2;             
      volatile const  uint32_t            : 2;
      volatile uint32_t FIDX       : 1;             
      volatile uint32_t IPOS       : 2;             
    } ECR_f;
  } ;

  union {
    volatile uint32_t TISEL;                        

    struct {
      volatile uint32_t TI1SEL     : 4;             
      volatile const  uint32_t            : 4;
      volatile uint32_t TI2SEL     : 4;             
      volatile const  uint32_t            : 4;
      volatile uint32_t TI3SEL     : 4;             
      volatile const  uint32_t            : 4;
      volatile uint32_t TI4SEL     : 4;             
    } TISEL_f;
  } ;

  union {
    volatile uint32_t AF1;                          

    struct {
      volatile const  uint32_t            : 14;
      volatile uint32_t ETRSEL     : 4;             
    } AF1_f;
  } ;

  union {
    volatile uint32_t AF2;                          

    struct {
      volatile const  uint32_t            : 16;
      volatile uint32_t OCRSEL     : 3;             
    } AF2_f;
  } ;
  volatile const  uint32_t  RESERVED2[2];

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t UIF        : 1;             
      volatile uint32_t CC1IF      : 1;             
      volatile uint32_t CC2IF      : 1;             
      volatile uint32_t CC3IF      : 1;             
      volatile uint32_t CC4IF      : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t TIF        : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t CC1OF      : 1;             
      volatile uint32_t CC2OF      : 1;             
      volatile uint32_t CC3OF      : 1;             
      volatile uint32_t CC4OF      : 1;             
      volatile const  uint32_t            : 7;
      volatile uint32_t IDXF       : 1;             
      volatile uint32_t DIRF       : 1;             
      volatile uint32_t IERRF      : 1;             
      volatile uint32_t TERRF      : 1;             
    } ICR_f;
  } ;
} GTIM_TypeDef;                                     



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t BRREN;                        

    struct {
      volatile uint32_t EN         : 1;             
    } BRREN_f;
  } ;

  union {
    volatile uint32_t BRR;                          

    struct {
      volatile uint32_t BRR        : 8;             
    } BRR_f;
  } ;

  union {
    volatile uint32_t CR;                           

    struct {
      volatile uint32_t FLT        : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t AA         : 1;             
      volatile uint32_t SI         : 1;             
      volatile uint32_t STO        : 1;             
      volatile uint32_t STA        : 1;             
      volatile uint32_t EN         : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t SCLINSRC   : 3;             
      volatile uint32_t SDAINSRC   : 3;             
    } CR_f;
  } ;

  union {
    volatile uint32_t DR;                           

    struct {
      volatile uint32_t DR         : 8;             
    } DR_f;
  } ;

  union {
    volatile uint32_t ADDR0;                        

    struct {
      volatile uint32_t GC         : 1;             
      volatile uint32_t ADDR0      : 7;             
    } ADDR0_f;
  } ;

  union {
    volatile const  uint32_t STAT;                         

    struct {
      volatile const  uint32_t STAT       : 8;             
    } STAT_f;
  } ;
  volatile const  uint32_t  RESERVED[2];

  union {
    volatile uint32_t ADDR1;                        

    struct {
      volatile const  uint32_t            : 1;
      volatile uint32_t ADDR1      : 7;             
    } ADDR1_f;
  } ;

  union {
    volatile uint32_t ADDR2;                        

    struct {
      volatile const  uint32_t            : 1;
      volatile uint32_t ADDR2      : 7;             
    } ADDR2_f;
  } ;

  union {
    volatile const  uint32_t MATCH;                        

    struct {
      volatile const  uint32_t ADDR0      : 1;             
      volatile const  uint32_t ADDR1      : 1;             
      volatile const  uint32_t ADDR2      : 1;             
    } MATCH_f;
  } ;
} I2C_TypeDef;                                      



 
 
 




 

typedef struct {                                 
  union {
    volatile uint32_t CR;                           

    struct {
      volatile uint32_t MOD        : 4;             
      volatile uint32_t IRSW       : 1;             
      volatile uint32_t INV        : 1;             
    } CR_f;
  } ;
} IRMOD_TypeDef;                                    



 
 
 




 

typedef struct {                                 

  union {
    volatile  uint32_t KR;                           

    struct {
      volatile  uint32_t KR         : 16;            
    } KR_f;
  } ;

  union {
    volatile uint32_t CR;                           

    struct {
      volatile uint32_t PRS        : 3;             
      volatile uint32_t ACTION     : 1;             
      volatile uint32_t IE         : 1;             
      volatile uint32_t PAUSE      : 1;             
    } CR_f;
  } ;

  union {
    volatile uint32_t ARR;                          

    struct {
      volatile uint32_t ARR        : 12;            
    } ARR_f;
  } ;

  union {
    volatile uint32_t SR;                           

    struct {
      volatile const  uint32_t CRF        : 1;             
      volatile const  uint32_t ARRF       : 1;             
      volatile const  uint32_t WINRF      : 1;             
      volatile uint32_t OV         : 1;             
      volatile const  uint32_t RUN        : 1;             
      volatile const  uint32_t RELOAD     : 1;             
    } SR_f;
  } ;

  union {
    volatile uint32_t WINR;                         

    struct {
      volatile uint32_t WINR       : 12;            
    } WINR_f;
  } ;
  volatile const  uint32_t  RESERVED[4];

  union {
    volatile const  uint32_t CNT;                          

    struct {
      volatile const  uint32_t CNT        : 12;            
    } CNT_f;
  } ;
} IWDT_TypeDef;                                     



 
 
 




 

typedef struct {                                 

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t CMPM       : 1;             
      volatile const  uint32_t ARRM       : 1;             
      volatile const  uint32_t EXTTRIG    : 1;             
      volatile const  uint32_t CMPOK      : 1;             
      volatile const  uint32_t ARROK      : 1;             
      volatile const  uint32_t UP         : 1;             
      volatile const  uint32_t DOWN       : 1;             
      volatile const  uint32_t DIR        : 2;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t CMPM       : 1;             
      volatile uint32_t ARRM       : 1;             
      volatile uint32_t EXTTRIG    : 1;             
      volatile uint32_t CMPOK      : 1;             
      volatile uint32_t ARROK      : 1;             
      volatile uint32_t UP         : 1;             
      volatile uint32_t DOWN       : 1;             
    } ICR_f;
  } ;

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t CMPM       : 1;             
      volatile uint32_t ARRM       : 1;             
      volatile uint32_t EXTTRIG    : 1;             
      volatile uint32_t CMPOK      : 1;             
      volatile uint32_t ARROK      : 1;             
      volatile uint32_t UP         : 1;             
      volatile uint32_t DOWN       : 1;             
    } IER_f;
  } ;

  union {
    volatile uint32_t CFGR;                         

    struct {
      volatile uint32_t CKSEL      : 1;             
      volatile uint32_t ENCMD_CKPOL : 2;            
      volatile uint32_t CHFLT      : 2;             
      volatile const  uint32_t            : 1;
      volatile uint32_t TRIGFLT    : 2;             
      volatile const  uint32_t            : 1;
      volatile uint32_t PRS        : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t TRIGSEL    : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t TRIGEN     : 2;             
      volatile uint32_t TIMOUT     : 1;             
      volatile uint32_t WAVE       : 1;             
      volatile uint32_t WAVPOL     : 1;             
      volatile uint32_t PRELOAD    : 1;             
      volatile uint32_t COUNTMD    : 1;             
      volatile uint32_t ENC        : 1;             
      volatile uint32_t ICLKSRC    : 2;             
    } CFGR_f;
  } ;

  union {
    volatile uint32_t CR;                           

    struct {
      volatile uint32_t EN         : 1;             
      volatile uint32_t SNGSTART   : 1;             
      volatile uint32_t CNTSTART   : 1;             
      volatile uint32_t SRST       : 1;             
      volatile uint32_t ARST       : 1;             
    } CR_f;
  } ;

  union {
    volatile uint32_t CMP;                          

    struct {
      volatile uint32_t CMP        : 16;            
    } CMP_f;
  } ;

  union {
    volatile uint32_t ARR;                          

    struct {
      volatile uint32_t ARR        : 16;            
    } ARR_f;
  } ;

  union {
    volatile const  uint32_t CNT;                          

    struct {
      volatile const  uint32_t CNT        : 16;            
    } CNT_f;
  } ;
} LPTIM_TypeDef;                                    

 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t IER;                          

    struct {
      volatile  uint32_t PARITY :         1;        
    } IER_f;
  };
  
  volatile const  uint32_t ADDR;                             

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t PARITY     : 1;                   
    } ISR_f;
  };

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t PARITY     : 1;             
    } ICR_f;
  };
} RAM_TypeDef;                                   


 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t TXEN       : 1;             
      volatile uint32_t RXEN       : 1;             
      volatile uint32_t PARITY     : 1;             
      volatile uint32_t PARITYEN   : 1;             
      volatile uint32_t STOP       : 2;             
      volatile uint32_t CHLEN      : 1;             
      volatile uint32_t MSBF       : 1;             
      volatile uint32_t START      : 1;             
      volatile uint32_t OVER       : 2;             
      volatile uint32_t SIGNAL     : 1;             
      volatile uint32_t SOURCE     : 2;             
      volatile uint32_t SYNC       : 1;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile uint32_t ADDREN     : 1;             
      volatile uint32_t RXMATCHEN  : 1;             
      volatile uint32_t CTSEN      : 1;             
      volatile uint32_t RTSEN      : 1;             
      volatile uint32_t RXINV      : 1;             
      volatile uint32_t TXINV      : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t TIMCR      : 3;             
      volatile uint32_t SWAP       : 1;             
      volatile uint32_t ADCRX      : 1;             
      volatile uint32_t ADCTX      : 1;             
      volatile uint32_t LOOP       : 1;             
      volatile uint32_t RXSRC      : 3;             
    } CR2_f;
  } ;

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t TXE        : 1;             
      volatile uint32_t TC         : 1;             
      volatile uint32_t RC         : 1;             
      volatile uint32_t RXIDLE     : 1;             
      volatile uint32_t RXBRK      : 1;             
      volatile uint32_t BAUD       : 1;             
      volatile uint32_t TIMOV      : 1;             
      volatile uint32_t CTS        : 1;             
      volatile uint32_t FE         : 1;             
      volatile uint32_t PE         : 1;             
      volatile uint32_t NE         : 1;             
      volatile uint32_t ORE        : 1;             
      volatile uint32_t RXMATCH    : 1;             
    } IER_f;
  } ;

  union {
    volatile uint32_t BRRI;                         

    struct {
      volatile uint32_t BRRI       : 16;            
    } BRRI_f;
  } ;

  union {
    volatile uint32_t BRRF;                         

    struct {
      volatile uint32_t BRRF       : 4;             
    } BRRF_f;
  } ;

  union {
    volatile uint32_t TIMARR;                       

    struct {
      volatile uint32_t TIMARR     : 24;            
    } TIMARR_f;
  } ;

  union {
    volatile const  uint32_t TIMCNT;                       

    struct {
      volatile const  uint32_t TIMCNT     : 24;            
    } TIMCNT_f;
  } ;

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t TXE        : 1;             
      volatile const  uint32_t TC         : 1;             
      volatile const  uint32_t RC         : 1;             
      volatile const  uint32_t RXIDLE     : 1;             
      volatile const  uint32_t RXBRK      : 1;             
      volatile const  uint32_t BAUD       : 1;             
      volatile const  uint32_t TIMOV      : 1;             
      volatile const  uint32_t CTS        : 1;             
      volatile const  uint32_t FE         : 1;             
      volatile const  uint32_t PE         : 1;             
      volatile const  uint32_t NE         : 1;             
      volatile const  uint32_t ORE        : 1;             
      volatile const  uint32_t RXMATCH    : 1;             
      volatile const  uint32_t SLVMATCH   : 1;             
      volatile const  uint32_t TXBUSY     : 1;             
      volatile const  uint32_t CTSLV      : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile const  uint32_t            : 1;
      volatile uint32_t TC         : 1;             
      volatile uint32_t RC         : 1;             
      volatile uint32_t RXIDLE     : 1;             
      volatile uint32_t RXBRK      : 1;             
      volatile uint32_t BAUD       : 1;             
      volatile uint32_t TIMOV      : 1;             
      volatile uint32_t CTS        : 1;             
      volatile uint32_t FE         : 1;             
      volatile uint32_t PE         : 1;             
      volatile uint32_t NE         : 1;             
      volatile uint32_t ORE        : 1;             
      volatile uint32_t RXMATCH    : 1;             
    } ICR_f;
  } ;

  union {
    volatile const  uint32_t RDR;                          

    struct {
      volatile const  uint32_t RDR        : 9;             
    } RDR_f;
  } ;

  union {
    volatile  uint32_t TDR;                          

    struct {
      volatile  uint32_t TDR        : 9;             
      volatile  uint32_t IDLE       : 1;             
      volatile  uint32_t BREAK      : 1;             
    } TDR_f;
  } ;
  volatile const  uint32_t  RESERVED;

  union {
    volatile uint32_t ADDR;                         

    struct {
      volatile uint32_t ADDR       : 8;             
    } ADDR_f;
  } ;

  union {
    volatile uint32_t MASK;                         

    struct {
      volatile uint32_t MASK       : 8;             
    } MASK_f;
  } ;

  union {
    volatile uint32_t CR3;                          

    struct {
      volatile uint32_t DEM        : 1;             
      volatile uint32_t DEP        : 1;             
      volatile uint32_t DETIME     : 5;             
      volatile uint32_t LIN        : 1;             
      volatile uint32_t BRKL       : 1;             
    } CR3_f;
  } ;

  union {
    volatile uint32_t RXMATCH;                      

    struct {
      volatile uint32_t RXMATCH    : 9;             
    } RXMATCH_f;
  } ;
} UART_TypeDef;                                     



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR0;                          

    struct {
      volatile uint32_t EN         : 1;             
      volatile uint32_t ACTION     : 1;             
      volatile uint32_t SOURCE     : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t VTH        : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t FLTCLK     : 1;             
    } CR0_f;
  } ;

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t IE         : 1;             
      volatile uint32_t LEVEL      : 1;             
      volatile uint32_t FALL       : 1;             
      volatile uint32_t RISE       : 1;             
      volatile uint32_t FLTTIME    : 4;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t SR;                           

    struct {
      volatile uint32_t INTF       : 1;             
      volatile const  uint32_t FLTV       : 1;             
    } SR_f;
  } ;
} LVD_TypeDef;                                      



 
 
 




 

typedef struct {                                 

  union {
    volatile  uint32_t KEY;                          

    struct {
      volatile  uint32_t KEY        : 8;             
    } KEY_f;
  } ;

  union {
    volatile uint32_t CR0;                          

    struct {
      volatile uint32_t INTERVAL   : 3;             
      volatile uint32_t H24        : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t RTC1HZ     : 2;             
      volatile uint32_t START      : 1;             
    } CR0_f;
  } ;

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t ACCESS     : 1;             
      volatile const  uint32_t WINDOW     : 1;             
      volatile const  uint32_t WAIT       : 1;             
      volatile const  uint32_t            : 5;
      volatile uint32_t SOURCE     : 3;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile uint32_t AWTPRS     : 2;             
      volatile uint32_t AWTSRC     : 1;             
      volatile uint32_t TAMPEDGE   : 1;             
      volatile uint32_t RTCOUT     : 2;             
      volatile uint32_t TAMPEN     : 1;             
      volatile uint32_t AWTEN      : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t ALARMAEN   : 1;             
      volatile uint32_t ALARMBEN   : 1;             
    } CR2_f;
  } ;

  union {
    volatile uint32_t COMPCFR1;                     

    struct {
      volatile uint32_t COMP       : 12;            
      volatile uint32_t PERIOD     : 2;             
      volatile uint32_t SIGN       : 1;             
      volatile uint32_t EN         : 1;             
    } COMPCFR1_f;
  } ;

  union {
    volatile uint32_t DATE;                         

    struct {
      volatile uint32_t DAY        : 8;             
      volatile uint32_t MONTH      : 8;             
      volatile uint32_t YEAR       : 8;             
      volatile uint32_t WEEK       : 3;             
    } DATE_f;
  } ;

  union {
    volatile uint32_t TIME;                         

    struct {
      volatile uint32_t SECOND     : 7;             
      volatile const  uint32_t            : 1;
      volatile uint32_t MINUTE     : 7;             
      volatile const  uint32_t            : 1;
      volatile uint32_t HOUR       : 6;             
    } TIME_f;
  } ;

  union {
    volatile uint32_t ALARMA;                       

    struct {
      volatile uint32_t SECOND     : 7;             
      volatile uint32_t SECONDEN   : 1;             
      volatile uint32_t MINUTE     : 7;             
      volatile uint32_t MINUTEEN   : 1;             
      volatile uint32_t HOUR       : 6;             
      volatile const  uint32_t            : 1;
      volatile uint32_t HOUREN     : 1;             
      volatile uint32_t WEEKMASK   : 7;             
    } ALARMA_f;
  } ;

  union {
    volatile uint32_t ALARMB;                       

    struct {
      volatile uint32_t SECOND     : 7;             
      volatile uint32_t SECONDEN   : 1;             
      volatile uint32_t MINUTE     : 7;             
      volatile uint32_t MINUTEEN   : 1;             
      volatile uint32_t HOUR       : 6;             
      volatile const  uint32_t            : 1;
      volatile uint32_t HOUREN     : 1;             
      volatile uint32_t WEEKMASK   : 7;             
    } ALARMB_f;
  } ;

  union {
    volatile const  uint32_t TAMPDATE;                     

    struct {
      volatile const  uint32_t DAY        : 6;             
      volatile const  uint32_t            : 2;
      volatile const  uint32_t MONTH      : 5;             
      volatile const  uint32_t WEEK       : 3;             
    } TAMPDATE_f;
  } ;

  union {
    volatile const  uint32_t TAMPTIME;                     

    struct {
      volatile const  uint32_t SECOND     : 7;             
      volatile const  uint32_t            : 1;
      volatile const  uint32_t MINUTE     : 7;             
      volatile const  uint32_t            : 1;
      volatile const  uint32_t HOUR       : 6;             
    } TAMPTIME_f;
  } ;

  union {
    volatile uint32_t AWTARR;                       

    struct {
      volatile uint32_t ARR        : 16;            
    } AWTARR_f;
  } ;

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t ALARMA     : 1;             
      volatile uint32_t ALARMB     : 1;             
      volatile uint32_t AWTIMER    : 1;             
      volatile uint32_t TAMP       : 1;             
      volatile uint32_t TAMPOV     : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t INTERVAL   : 1;             
    } IER_f;
  } ;

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t ALARMA     : 1;             
      volatile const  uint32_t ALARMB     : 1;             
      volatile const  uint32_t AWTIMER    : 1;             
      volatile const  uint32_t TAMP       : 1;             
      volatile const  uint32_t TAMPOV     : 1;             
      volatile const  uint32_t            : 1;
      volatile const  uint32_t INTERVAL   : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t ALARMA     : 1;             
      volatile uint32_t ALARMB     : 1;             
      volatile uint32_t AWTIMER    : 1;             
      volatile uint32_t TAMP       : 1;             
      volatile uint32_t TAMPOV     : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t INTERVAL   : 1;             
    } ICR_f;
  } ;

  union {
    volatile const  uint32_t AWTCNT;                       

    struct {
      volatile const  uint32_t CNT        : 16;            
    } AWTCNT_f;
  } ;

  union {
    volatile uint32_t PSC;                          

    struct {
      volatile uint32_t PSC2       : 20;            
      volatile uint32_t PSC1       : 8;             
    } PSC_f;
  } ;

  union {
    volatile const  uint32_t SSCNT;                        

    struct {
      volatile const  uint32_t SSCNT0     : 20;            
      volatile const  uint32_t SSCNT1     : 1;             
    } SSCNT_f;
  } ;
  volatile const  uint32_t  RESERVED[3];

  union {
    volatile uint32_t COMPCFR2;                     

    struct {
      volatile uint32_t PCLKCNT    : 11;            
    } COMPCFR2_f;
  } ;

  union {
    volatile uint32_t COMPCFR3;                     

    struct {
      volatile uint32_t PCLKCNT    : 11;            
    } COMPCFR3_f;
  } ;
} RTC_TypeDef;                                      



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t MSTR       : 1;             
      volatile uint32_t SSM        : 1;             
      volatile uint32_t CPHA       : 1;             
      volatile uint32_t CPOL       : 1;             
      volatile uint32_t BR         : 3;             
      volatile uint32_t LSBF       : 1;             
      volatile uint32_t WIDTH      : 4;             
      volatile uint32_t GAP        : 4;             
      volatile uint32_t MODE       : 2;             
      volatile uint32_t FLTEN      : 1;             
      volatile uint32_t MISOHD     : 1;             
      volatile uint32_t SMP        : 1;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile uint32_t EN         : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t ADCRX      : 1;             
      volatile uint32_t ADCTX      : 1;             
    } CR2_f;
  } ;

  union {
    volatile uint32_t CR3;                          

    struct {
      volatile uint32_t HDOE       : 1;             
    } CR3_f;
  } ;

  union {
    volatile uint32_t SSI;                          

    struct {
      volatile uint32_t SSI        : 1;             
    } SSI_f;
  } ;

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t TXE        : 1;             
      volatile uint32_t RXNE       : 1;             
      volatile uint32_t SSF        : 1;             
      volatile uint32_t SSR        : 1;             
      volatile uint32_t UD         : 1;             
      volatile uint32_t OV         : 1;             
      volatile uint32_t SSERR      : 1;             
      volatile uint32_t MODF       : 1;             
    } IER_f;
  } ;

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t TXE        : 1;             
      volatile const  uint32_t RXNE       : 1;             
      volatile const  uint32_t SSF        : 1;             
      volatile const  uint32_t SSR        : 1;             
      volatile const  uint32_t UD         : 1;             
      volatile const  uint32_t OV         : 1;             
      volatile const  uint32_t SSERR      : 1;             
      volatile const  uint32_t MODF       : 1;             
      volatile const  uint32_t BUSY       : 1;             
      volatile const  uint32_t SSLVL      : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t FLUSH      : 1;             
      volatile uint32_t RXNE       : 1;             
      volatile uint32_t SSF        : 1;             
      volatile uint32_t SSR        : 1;             
      volatile uint32_t UD         : 1;             
      volatile uint32_t OV         : 1;             
      volatile uint32_t SSERR      : 1;             
      volatile uint32_t MODF       : 1;             
    } ICR_f;
  } ;

  union {
    volatile uint32_t DR;                           

    struct {
      volatile uint32_t DR         : 16;            
    } DR_f;
  } ;
} SPI_TypeDef;                                      



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR0;                          

    struct {
      volatile uint32_t SYSCLK     : 3;             
      volatile uint32_t PCLKPRS    : 2;             
      volatile uint32_t HCLKPRS    : 3;             
      volatile const  uint32_t            : 8;
      volatile  uint32_t KEY        : 16;            
    } CR0_f;
  } ;

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t HSIEN      : 1;             
      volatile uint32_t HSEEN      : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t LSIEN      : 1;             
      volatile uint32_t LSEEN      : 1;             
      volatile uint32_t LSELOCK    : 1;             
      volatile uint32_t LSECCS     : 1;             
      volatile uint32_t HSECCS     : 1;             
      volatile uint32_t CLKCCS     : 1;             
      volatile const  uint32_t            : 7;
      volatile  uint32_t KEY        : 16;            
    } CR1_f;
  } ;

  union {
    volatile uint32_t CR2;                          

    struct {
      volatile uint32_t RSTIO      : 1;             
      volatile uint32_t SWDIO      : 1;             
      volatile uint32_t LOCKUP     : 1;             
      volatile uint32_t WAKEUPCLK  : 1;             
      volatile uint32_t FLASHWAIT  : 3;             
      volatile uint32_t RTCLPM     : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t HSEBKEN    : 1;             
      volatile uint32_t LSEBKEN    : 1;             
      volatile uint32_t CLLBKEN    : 1;             
      volatile uint32_t DSBKEN     : 1;             
      volatile uint32_t LVDBKEN    : 1;             
      volatile uint32_t RAMBKEN    : 1;             
      volatile  uint32_t KEY        : 16;            
    } CR2_f;
  } ;

  union {
    volatile uint32_t IER;                          

    struct {
      volatile uint32_t HSIRDY     : 1;             
      volatile uint32_t HSERDY     : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t LSIRDY     : 1;             
      volatile uint32_t LSERDY     : 1;             
      volatile uint32_t LSEFAIL    : 1;             
      volatile uint32_t HSEFAIL    : 1;             
      volatile uint32_t LSEFAULT   : 1;             
      volatile uint32_t HSEFAULT   : 1;             
      volatile const  uint32_t            : 7;
      volatile  uint32_t KEY        : 16;            
    } IER_f;
  } ;

  union {
    volatile const  uint32_t ISR;                          

    struct {
      volatile const  uint32_t HSIRDY     : 1;             
      volatile const  uint32_t HSERDY     : 1;             
      volatile const  uint32_t            : 1;
      volatile const  uint32_t LSIRDY     : 1;             
      volatile const  uint32_t LSERDY     : 1;             
      volatile const  uint32_t LSEFAIL    : 1;             
      volatile const  uint32_t HSEFAIL    : 1;             
      volatile const  uint32_t LSEFAULT   : 1;             
      volatile const  uint32_t HSEFAULT   : 1;             
      volatile const  uint32_t            : 2;
      volatile const  uint32_t HSISTABLE  : 1;             
      volatile const  uint32_t HSESTABLE  : 1;             
      volatile const  uint32_t            : 1;
      volatile const  uint32_t LSISTABLE  : 1;             
      volatile const  uint32_t LSESTABLE  : 1;             
    } ISR_f;
  } ;

  union {
    volatile uint32_t ICR;                          

    struct {
      volatile uint32_t HSIRDY     : 1;             
      volatile uint32_t HSERDY     : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t LSIRDY     : 1;             
      volatile uint32_t LSERDY     : 1;             
      volatile uint32_t LSEFAIL    : 1;             
      volatile uint32_t HSEFAIL    : 1;             
      volatile uint32_t LSEFAULT   : 1;             
      volatile uint32_t HSEFAULT   : 1;             
    } ICR_f;
  } ;

  union {
    volatile uint32_t HSI;                          

    struct {
      volatile uint32_t TRIM       : 11;            
      volatile uint32_t DIV        : 4;             
      volatile const  uint32_t STABLE     : 1;             
    } HSI_f;
  } ;

  union {
    volatile uint32_t HSE;                          

    struct {
      volatile uint32_t DRIVER     : 4;             
      volatile uint32_t WAITCYCLE  : 2;             
      volatile uint32_t MODE       : 1;             
      volatile uint32_t HEXENPOL   : 1;             
      volatile uint32_t DETCNT     : 11;            
      volatile const  uint32_t STABLE     : 1;             
      volatile uint32_t PDRIVER    : 4;             
      volatile uint32_t DIGFLT     : 1;             
    } HSE_f;
  } ;

  union {
    volatile uint32_t LSI;                          

    struct {
      volatile uint32_t TRIM       : 10;            
      volatile uint32_t WAITCYCLE  : 2;             
      volatile const  uint32_t            : 3;
      volatile const  uint32_t STABLE     : 1;             
    } LSI_f;
  } ;

  union {
    volatile uint32_t LSE;                          

    struct {
      volatile uint32_t DRIVER     : 4;             
      volatile uint32_t WAITCYCLE  : 2;             
      volatile uint32_t MODE       : 1;             
      volatile uint32_t ANAFLT     : 1;             
      volatile uint32_t PDRIVER    : 4;             
      volatile uint32_t COMP       : 1;             
      volatile uint32_t CPEN       : 1;             
      volatile uint32_t RESTRIM    : 2;             
      volatile uint32_t WP         : 1;             
      volatile uint32_t PINLOCK    : 1;             
      volatile const  uint32_t STABLE     : 1;             
    } LSE_f;
  } ;
  volatile const  uint32_t  RESERVED;

  union {
    volatile uint32_t DEBUG;                        

    struct {
      volatile uint32_t ATIM       : 1;             
      volatile uint32_t GTIM1      : 1;             
      volatile const  uint32_t            : 3;
      volatile uint32_t BTIM123    : 1;             
      volatile uint32_t LPTIM      : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t RTC        : 1;             
      volatile uint32_t IWDT       : 1;             
    } DEBUG_f;
  } ;

  union {
    volatile uint32_t AHBEN;                        

    struct {
      volatile const  uint32_t            : 1;
      volatile uint32_t FLASH      : 1;             
      volatile uint32_t CRC        : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t GPIOA      : 1;             
      volatile uint32_t GPIOB      : 1;             
            uint32_t            : 10;
      volatile uint32_t KEY        : 16;            
    } AHBEN_f;
  } ;

  union {
    volatile uint32_t APBEN2;                       

    struct {
      volatile const  uint32_t            : 1;
      volatile uint32_t RTC        : 1;             
      volatile uint32_t BTIM123    : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t IWDT       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t I2C1       : 1;             
      volatile uint32_t LPTIM      : 1;             
            uint32_t            : 8;
      volatile uint32_t KEY        : 16;            
    } APBEN2_f;
  } ;

  union {
    volatile uint32_t APBEN1;                       

    struct {
      volatile uint32_t ADC        : 1;             
      volatile uint32_t VC         : 1;             
      volatile uint32_t SPI1       : 1;             
      volatile uint32_t UART1      : 1;             
      volatile uint32_t UART2      : 1;             
      volatile uint32_t ATIM       : 1;             
      volatile uint32_t GTIM1      : 1;             
            uint32_t            : 9;
      volatile uint32_t KEY        : 16;            
    } APBEN1_f;
  } ;
  volatile const  uint32_t  RESERVED1;

  union {
    volatile uint32_t AHBRST;                       

    struct {
      volatile const  uint32_t            : 1;
      volatile uint32_t FLASH      : 1;             
      volatile uint32_t CRC        : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t GPIOA      : 1;             
      volatile uint32_t GPIOB      : 1;             
    } AHBRST_f;
  } ;

  union {
    volatile uint32_t APBRST2;                      

    struct {
      volatile const  uint32_t            : 1;
      volatile uint32_t RTC        : 1;             
      volatile uint32_t BTIM123    : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t IWDT       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t I2C1       : 1;             
      volatile uint32_t LPTIM      : 1;             
    } APBRST2_f;
  } ;

  union {
    volatile uint32_t APBRST1;                      

    struct {
      volatile uint32_t ADC        : 1;             
      volatile uint32_t VC         : 1;             
      volatile uint32_t SPI1       : 1;             
      volatile uint32_t UART1      : 1;             
      volatile uint32_t UART2      : 1;             
      volatile uint32_t ATIM       : 1;             
      volatile uint32_t GTIM1      : 1;             
    } APBRST1_f;
  } ;

  union {
    volatile uint32_t RESETFLAG;                    

    struct {
      volatile uint32_t POR        : 1;             
      volatile const  uint32_t            : 2;
      volatile uint32_t LVD        : 1;             
      volatile uint32_t IWDT       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t RSTB       : 1;             
      volatile const  uint32_t            : 1;
      volatile uint32_t LOCKUP     : 1;             
      volatile uint32_t SYSRESETREQ : 1;            
    } RESETFLAG_f;
  } ;
  volatile const  uint32_t  RESERVED2[8];

  union {
    volatile uint32_t MCO;                          

    struct {
      volatile uint32_t SOURCE     : 4;             
      volatile uint32_t DIV        : 3;             
    } MCO_f;
  } ;
} SYSCTRL_TypeDef;                                  



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR0;                          

    struct {
      volatile uint32_t EN         : 1;             
      volatile uint32_t RESP       : 1;             
      volatile uint32_t HYS        : 1;             
      volatile uint32_t IE         : 1;             
      volatile uint32_t POL        : 1;             
      volatile uint32_t WINDOW     : 1;             
      volatile uint32_t INP        : 2;             
      volatile uint32_t INN        : 2;             
    } CR0_f;
  } ;

  union {
    volatile uint32_t CR1;                          

    struct {
      volatile uint32_t FLTTIME    : 4;             
      volatile uint32_t FLTCLK     : 1;             
      volatile uint32_t FALLIE     : 1;             
      volatile uint32_t RISEIE     : 1;             
      volatile uint32_t HIGHIE     : 1;             
      volatile uint32_t BLANKATCH1 : 1;             
      volatile uint32_t BLANKATCH2 : 1;             
      volatile uint32_t BLANKATCH3 : 1;             
      volatile uint32_t BLANKATCH4 : 1;             
      volatile uint32_t BLANKATCH5 : 1;             
      volatile uint32_t BLANKATCH6 : 1;             
      volatile uint32_t BLANKTIME  : 3;             
      volatile uint32_t BLANKLVL   : 1;             
    } CR1_f;
  } ;

  union {
    volatile uint32_t SR;                           

    struct {
      volatile uint32_t INTF       : 1;             
      volatile const  uint32_t FLTV       : 1;             
    } SR_f;
  } ;
} VC_TypeDef;                                       



 
 
 




 

typedef struct {                                 

  union {
    volatile uint32_t CR;                           

    struct {
      volatile uint32_t DIV        : 3;             
      volatile const  uint32_t            : 1;
      volatile uint32_t EN         : 1;             
      volatile uint32_t VIN        : 1;             
    } CR_f;
  } ;
} VCREF_TypeDef;                                    


   


 
 
 




 

#line 3173 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"

   


 
 
 




 

#line 3210 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"

   


 

  #pragma pop
#line 3230 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


 
 
 




 



 
 
 

 
#line 3260 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 
#line 3280 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 
#line 3318 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3335 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3374 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3383 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3392 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3401 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 


 


 


 


 


 


 




 
 
 

 
#line 3450 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3487 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3512 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3541 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3586 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3609 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3622 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3647 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3660 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3685 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3698 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3723 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3772 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 


 


 


 


 


 


 


 
#line 3813 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3828 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3853 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3860 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3869 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3878 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 
#line 3898 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3913 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 3958 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


 
 
 

 
#line 3977 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 
#line 3995 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 




 




 




 




 


 




 
 
 

 


 


 




 
 
 

 
#line 4054 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 
#line 4094 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4101 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4110 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4117 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 






 
 
 

 
#line 4147 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4166 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4185 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 
#line 4205 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4224 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4243 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4262 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4281 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4300 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4321 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4340 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4359 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4378 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4415 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4434 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


 
 
 

 
#line 4459 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4466 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4491 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4512 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4541 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4570 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4583 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4596 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4621 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4634 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4659 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4684 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 


 


 


 


 


 


 
#line 4716 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4725 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 




 
 
 

 


 


 
#line 4760 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 




 


 


 


 
#line 4784 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


 
 
 

 
#line 4797 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"


 
 
 

 


 
#line 4815 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 
#line 4831 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 




 
 
 

 
#line 4860 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4875 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4890 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4919 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 4930 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 


 



 
 
 

 


 


 


 




 
 
 

 
#line 4987 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5012 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5023 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 


 


 


 


 
#line 5045 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 


 
#line 5078 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5111 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5136 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 
 
 

 
#line 5156 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5167 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 






 
 
 

 


 
#line 5190 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5199 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5216 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 
#line 5230 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5237 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 
#line 5257 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5272 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5279 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5286 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5299 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5312 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5325 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 


 
#line 5340 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 




 
 
 

 
#line 5377 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5384 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 


 


 
#line 5407 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5428 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5445 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 




 
 
 

 
#line 5463 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5482 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5509 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5528 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5553 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5570 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5577 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5594 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5601 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5624 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5637 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5648 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5661 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5678 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5687 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5698 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5713 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5726 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 






 
 
 

 
#line 5754 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 
#line 5781 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"
 






 
 
 

 
#line 5799 "..\\..\\..\\..\\Libraries\\inc\\cw32l010.h"

   









   

   
#line 29 "..\\USER\\src\\..\\inc\\main.h"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\system_cw32l010.h"






 












 

#line 23 "..\\..\\..\\..\\Libraries\\inc\\system_cw32l010.h"
#line 24 "..\\..\\..\\..\\Libraries\\inc\\system_cw32l010.h"


 



 
extern uint32_t SystemCoreClock;          



 



 



 



 







 




 



 

extern void SystemInit (void);            
extern void SystemCoreClockUpdate (void); 
extern void FirmwareDelay(uint32_t DlyCnt);


 













#line 30 "..\\USER\\src\\..\\inc\\main.h"
#line 1 "..\\USER\\src\\..\\inc\\interrupts_cw32l010.h"
 












 
 

 







 
 

 


 
 

 


 
 

 


 
 

 


 

extern void NMI_Handler(void);
extern void HardFault_Handler(void);
extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void WDT_IRQHandler(void);
extern void LVD_IRQHandler(void);
extern void RTC_IRQHandler(void);
extern void FLASHRAM_IRQHandler(void);
extern void SYSCTRL_IRQHandler(void);
extern void GPIOA_IRQHandler(void);
extern void GPIOB_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void ATIM_IRQHandler(void);
extern void VC1_IRQHandler(void);
extern void VC2_IRQHandler(void);
extern void GTIM1_IRQHandler(void);
extern void LPTIM_IRQHandler(void);
extern void BTIM1_IRQHandler(void);
extern void BTIM2_IRQHandler(void);
extern void BTIM3_IRQHandler(void);
extern void I2C1_IRQHandler(void);
extern void SPI1_IRQHandler(void);
extern void UART1_IRQHandler(void);
extern void UART2_IRQHandler(void);
extern void CLKFAULT_IRQHandler(void);

 

 








 
#line 31 "..\\USER\\src\\..\\inc\\main.h"


 
 
 


 
 
 


 
 
 
 
 
 


 
 
 








 
#line 19 "..\\USER\\src\\interrupts_cw32l010.c"
#line 20 "..\\USER\\src\\interrupts_cw32l010.c"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"















 
 








 
 
 
#line 30 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

 
 
 

typedef struct
{
    uint32_t Pins;
    uint32_t Mode;
    uint32_t IT;    
} GPIO_InitTypeDef;

typedef enum
{
    GPIO_Pin_RESET = 0,
    GPIO_Pin_SET
} GPIO_PinState;

















#line 81 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"



#line 90 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"









#line 117 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 132 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"



#line 153 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 168 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"




#line 190 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 205 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"




#line 227 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 242 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"



#line 254 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 262 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"





#line 285 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 300 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"





#line 314 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"


#line 323 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"



#line 334 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 343 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 351 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 360 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 369 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 378 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 387 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 396 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 405 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"


#line 415 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 424 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 433 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"


#line 443 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 452 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"

#line 461 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"


#line 471 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_gpio.h"




void GPIO_RST2GPIO(void);
void GPIO_GPIO2RST(void);
void GPIO_SWD2GPIO(void);
void GPIO_GPIO2SWD(void);
void GPIO_LockPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pins);
void GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pins);
void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void GPIO_ConfigFilter(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pins, uint32_t FltClk);
void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pins, GPIO_PinState PinState);
void GPIO_Write(GPIO_TypeDef* GPIOx, uint16_t Value);
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pins);
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);





#line 21 "..\\USER\\src\\interrupts_cw32l010.c"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"









 















 





 
 
 
#line 36 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"

 









 
 
 


 




 
typedef struct
{
    uint32_t                    ADC_SampTime;          
    uint32_t                    ADC_InputChannel;            
}ADC_ChannelTypeDef;
typedef struct
{
    uint32_t                    ADC_ClkDiv;                
    
    uint32_t                    ADC_ConvertMode;           

    uint32_t                    ADC_SQREns;     
    
    ADC_ChannelTypeDef          ADC_IN0;          
    
    ADC_ChannelTypeDef          ADC_IN1;          
    
    ADC_ChannelTypeDef          ADC_IN2;          
    
    ADC_ChannelTypeDef          ADC_IN3;          
    
    ADC_ChannelTypeDef          ADC_IN4;          
    
    ADC_ChannelTypeDef          ADC_IN5;          
    
    ADC_ChannelTypeDef          ADC_IN6;          
    
    ADC_ChannelTypeDef          ADC_IN7;          
            
} ADC_InitTypeDef;




 
typedef struct
{
    ADC_InitTypeDef   ADC_InitStruct;         
    
    uint32_t          ADC_WatchdogCHx;                

    uint32_t          ADC_WatchdogVth;             

    uint32_t          ADC_WatchdogVtl;             
    
    uint32_t          ADC_WatchdogOverHighIrq;     
    
    uint32_t          ADC_WatchdogUnderLowIrq;     
} ADC_WatchdogTypeDef;





 
typedef struct
{
    boolean_t  ADC_WdthIrq;    

    boolean_t  ADC_WdtlIrq;    

    boolean_t  ADC_EosIrq;     

    boolean_t  ADC_EocIrq;     
} ADC_IrqTypeDef;





 



 









 



 









 




























 
#line 202 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"

#line 219 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"
                                            
                                             



 
#line 241 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"






 




 






 
#line 268 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"
    
#line 277 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"

#line 286 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"







 















 

#line 330 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"





 
#line 352 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_adc.h"




 









 



 

void ADC_DeInit(void);

void ADC_Init(ADC_InitTypeDef* ADC_InitStruct);

ErrorStatus ADC_Enable(void);

void ADC_Disable(void);
  

void ADC_WatchdogInit(ADC_WatchdogTypeDef* ADC_WatchdogStruct);



void ADC_ITConfig(uint16_t ADC_IT, FunctionalState NewState);

void ADC_GetITStatusAll(volatile uint8_t* pFlagAdcIrq);

ITStatus ADC_GetITStatus(uint16_t ADC_IT);

void ADC_ClearITPendingAll(void);

void ADC_ClearITPendingBit(uint16_t ADC_IT);


void ADC_SoftwareStartConvCmd(FunctionalState NewState);


void ADC_ExtTrigCfg(uint16_t ADC_TRIG, FunctionalState NewState);



uint16_t ADC_GetConversionValue(uint32_t SqrCHx);

void ADC_GetSqr0Result(uint16_t* pAdcResult);

void ADC_GetSqr1Result(uint16_t* pAdcResult);

void ADC_GetSqr2Result(uint16_t* pAdcResult);

void ADC_GetSqr3Result(uint16_t* pAdcResult);

void ADC_GetSqr4Result(uint16_t* pAdcResult);

void ADC_GetSqr5Result(uint16_t* pAdcResult);

void ADC_GetSqr6Result(uint16_t* pAdcResult);

void ADC_GetSqr7Result(uint16_t* pAdcResult);

void ADC_GetSqr8Result(uint16_t* pAdcResult);

void ADC_GetSqr9Result(uint16_t* pAdcResult);

void ADC_GetSqr10Result(uint16_t* pAdcResult);

void ADC_GetSqr11Result(uint16_t* pAdcResult);




void ADC_SetTs(uint32_t enTs);

float ADC_GetTs(float RefVoltage, uint32_t AdcValue);








 
 
 
#line 22 "..\\USER\\src\\interrupts_cw32l010.c"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"















 
 







 
#line 27 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"
#line 28 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"


 

typedef enum {DOWN_STOP = 0,  UP_STOP, DOWN_RUN, UP_RUN} ENCStatus;





 

typedef struct
{
    uint32_t LPTIM_ClockSource;
    uint32_t LPTIM_CounterMode;
    uint32_t LPTIM_Period;
    uint32_t LPTIM_Prescaler;
} LPTIM_InitTypeDef;


 










#line 76 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"













#line 96 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"

#line 104 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"




#line 115 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"

#line 123 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"



#line 132 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_lptim.h"





 
 
void LPTIM_DeInit(void);
void LPTIM_Init(LPTIM_InitTypeDef* LPTIM_InitStruct);
void LPTIM_PrescalerConfig(uint32_t Prescaler);
void LPTIM_CounterModeConfig(uint32_t LPTIM_CounterMode);
void LPTIM_SetAutoreload(uint16_t Autoreload, uint32_t ReloadMode);
uint32_t LPTIM_GetCounter(void);
uint8_t LPTIM_GetPrescaler(void);
void LPTIM_SelectOnePulseMode(uint16_t LPTIM_OPMode);
void LPTIM_Cmd(FunctionalState NewState);

void LPTIM_InternalClockConfig(uint32_t LPTIM_ICLK_Source);
void LPTIM_PWMStart(uint32_t Period, uint32_t Pulse, uint32_t Polarity);



void LPTIM_ITConfig(uint32_t LPTIM_IT, FunctionalState NewState);
FlagStatus LPTIM_GetFlagStatus(uint32_t LPTIM_FLAG);
void LPTIM_ClearFlag(uint32_t LPTIM_FLAG);
ITStatus LPTIM_GetITStatus(uint32_t LPTIM_IT);
void LPTIM_ClearITPendingBit(uint32_t LPTIM_IT);

ENCStatus LPTIM_GetENCStatus(void);











 
#line 23 "..\\USER\\src\\interrupts_cw32l010.c"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"















 
 







 
#line 27 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 



 

typedef struct
{
    uint32_t UART_BaudRate;            




 

    uint16_t UART_Over;                
 

    uint16_t UART_Source;              
 

    uint32_t UART_UclkFreq;             

    uint16_t UART_StartBit;            
 

    uint16_t UART_StopBits;            
 

    uint16_t UART_Parity;              


 

    uint16_t UART_Mode;                
 

    uint16_t UART_HardwareFlowControl; 
 
} UART_InitTypeDef;



 

typedef struct
{
    uint16_t UART_Clock;   
 

    uint16_t UART_Source;  
 
} UART_ClockInitTypeDef;



 



 

#line 100 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 

#line 111 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 








 

#line 131 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 

#line 144 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 







 

#line 166 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 








 







 







 
  








 
  
#line 218 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 
#line 237 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"



 

#line 260 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"

#line 270 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_uart.h"
                                            



 




 

 
void UART_SendData(UART_TypeDef* UARTx, uint16_t Data);
void UART_SendData_8bit(UART_TypeDef* UARTx, uint8_t Data);
uint16_t UART_ReceiveData(UART_TypeDef* UARTx);
uint8_t UART_ReceiveData_8bit(UART_TypeDef* UARTx);
void UART_SendString(UART_TypeDef* UARTx, char* String);

 
void UART_Init(UART_TypeDef* UARTx, UART_InitTypeDef* UART_InitStruct);
void UART_StructInit(UART_InitTypeDef* UART_InitStruct);
void UART_ClockInit(UART_TypeDef* UARTx, UART_ClockInitTypeDef* UART_ClockInitStruct);
void UART_ClockStructInit(UART_ClockInitTypeDef* UART_ClockInitStruct);
void UART1_DeInit(void);
void UART2_DeInit(void);
void UART3_DeInit(void);

 
void UART_ITConfig(UART_TypeDef* UARTx, uint16_t UART_IT, FunctionalState NewState);
ITStatus UART_GetITStatus(UART_TypeDef* UARTx, uint16_t UART_IT);
void UART_ClearITPendingBit(UART_TypeDef* UARTx, uint16_t UART_IT);
FlagStatus UART_GetFlagStatus(UART_TypeDef* UARTx, uint16_t UART_FLAG);
void UART_ClearFlag(UART_TypeDef* UARTx, uint16_t UART_FLAG);

 
void UART_DirectionModeCmd(UART_TypeDef* UARTx, uint16_t UART_DirectionMode, FunctionalState NewState);
void UART_InvPinCmd(UART_TypeDef* UARTx, uint16_t UART_InvPin, FunctionalState NewState);

 
void UART_HalfDuplexCmd(UART_TypeDef* UARTx, FunctionalState NewState);

 
void UART_DMACmd(UART_TypeDef* UARTx, uint16_t UART_DMAReq, FunctionalState NewState);

 
void UART_SetMultiMode(UART_TypeDef* UARTx, uint8_t UART_Address, uint8_t UART_AddressMsK);

 
void UART_LINCmd(UART_TypeDef* UARTx, FunctionalState NewState);
void UART_SendBreak(UART_TypeDef* UARTx, uint8_t BreakLength);
void UART_LINBreakDetectLengthConfig(UART_TypeDef* UARTx, uint16_t UART_LINBreakDetectLength);

 
void UART_TimerModeConfig(UART_TypeDef* UARTx, uint16_t UART_TimerMode);
void UART_SetAutoReload(UART_TypeDef* UARTx, uint32_t AutoReload);
uint32_t UART_GetCounter(UART_TypeDef* UARTx);








 











#line 24 "..\\USER\\src\\interrupts_cw32l010.c"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"















 
 







 
#line 27 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"
#line 28 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"


 
typedef struct
{
    uint32_t IWDT_Prescaler;          

    uint32_t IWDT_OverFlowAction;     

    FunctionalState IWDT_ITState;     

    uint32_t IWDT_Pause;              

    uint32_t IWDT_ReloadValue;        

    uint32_t IWDT_WindowValue;        
} IWDT_InitTypeDef;


 


#line 67 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"

#line 76 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"

#line 83 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"

#line 92 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"

#line 101 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_iwdt.h"




 




 
void IWDT_DeInit(void);
int IWDT_Init(IWDT_InitTypeDef *IWDT_InitStruct);

void IWDT_Cmd(void);
void IWDT_Refresh(void);

void IWDT_Unlock(void);
void IWDT_Lock(void);
void IWDT_Stop(void);

int IWDT_SetPrescaler(uint32_t WWDT_Prescaler);
int IWDT_SetWindowValue(uint32_t WindowValue);
int IWDT_SetReloadValue(uint32_t ReloadValue);
int IWDT_ITConfig(FunctionalState NewState);
FlagStatus IWDT_GetFlagStatus(uint32_t StatusBit);
void IWDT_ClearOVFlag(void);
uint32_t IWDT_GetCounterValue(void);
void IWDT_SetPeriod(uint32_t period);








 
#line 25 "..\\USER\\src\\interrupts_cw32l010.c"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"















 
 








 
#line 28 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"
#line 29 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"


typedef struct
{
    uint8_t Day;
    uint8_t Month;
    uint8_t Year;
    uint8_t Week;
} RTC_DateTypeDef;

typedef struct
{
    uint8_t Second;
    uint8_t Minute;
    uint8_t Hour;
    uint8_t AMPM;               
    uint8_t H24;                
} RTC_TimeTypeDef;

typedef struct
{
    uint32_t RTC_ClockSource;
    RTC_TimeTypeDef TimeStruct;
    RTC_DateTypeDef DateStruct;

} RTC_InitTypeDef;


typedef struct
{
    RTC_TimeTypeDef RTC_AlarmTime;
    uint32_t RTC_AlarmMask;             
} RTC_AlarmTypeDef;

typedef struct
{
    uint8_t AWT_ClockSource;
    uint16_t AWT_ARRValue;
} RTC_AWTTypeDef;

typedef struct
{
    uint32_t Freq;
    uint32_t Sign;
    uint32_t Step;
    uint32_t CompensationValue;
} RTC_CalibTypeDef;









 




 
















 

#line 116 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"









 












#line 156 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"



 
#line 173 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"










 









 

#line 203 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"





 
#line 218 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"





 














 
#line 253 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"



 

#line 265 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"





 
#line 279 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"



 







 






 




 















 
#line 327 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"














#line 347 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"





#line 358 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"




























#line 394 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"






















































#line 454 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_rtc.h"




void RTC_DeInit(void);
ErrorStatus RTC_Init(RTC_InitTypeDef *RTC_InitStruct);
void RTC_Cmd(FunctionalState NewState);
ITStatus RTC_GetITState(uint32_t RTC_ITState);
void RTC_ITConfig(uint32_t RTC_IT, FunctionalState NewState);
void RTC_ClearITPendingBit(uint32_t RTC_IT);
void RTC_SetTime(RTC_TimeTypeDef* RTC_TimeStruct);
void RTC_SetDate(RTC_DateTypeDef *RTC_Date);
void RTC_GetTime(RTC_TimeTypeDef* RTC_TimeStruct);
void RTC_GetDate(RTC_DateTypeDef *RTC_Date);
void RTC_SetAlarm(uint32_t RTC_Alarm, RTC_AlarmTypeDef* RTC_AlarmStruct);
void RTC_GetAlarm(uint32_t RTC_Alarm, RTC_AlarmTypeDef* RTC_AlarmStruct);
void RTC_AlarmCmd(uint32_t RTC_Alarm, FunctionalState NewState);
void RTC_TamperTriggerConfig(uint32_t RTC_TamperTrigger);
void RTC_TamperCmd(FunctionalState NewState);
void RTC_GetTamperDate(RTC_DateTypeDef *RTC_Date);
void RTC_GetTamperTime(RTC_TimeTypeDef* RTC_TimeStruct);
void RTC_OutputConfig(uint8_t RTC_Output);
void RTC_AWTConfig(RTC_AWTTypeDef *RCT_AWTStruct);
void RTC_AWTCmd(FunctionalState NewState);
void RTC_SetInterval(uint8_t Period);
void RTC_SetClockSource(uint32_t RTC_ClockSource);
void RTC_CalibrationConfig(RTC_CalibTypeDef *RTC_CalibStruct);
void RTC_CalibrationCmd(FunctionalState NewState);
void RTC_AWT_PSC_set(uint8_t PSC1A,uint32_t PSC2A);
uint8_t RTC_BinToBCD(uint8_t Value);
uint8_t RTC_BCDToBin(uint8_t Value);





#line 26 "..\\USER\\src\\interrupts_cw32l010.c"
#line 1 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"









 















 

 







 
#line 38 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"








 




#line 67 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 76 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 85 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

 
#line 95 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 102 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

 
#line 115 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"















                                               



 



#line 154 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 171 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

 








 



#line 193 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 200 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"
















#line 232 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"





 














#line 262 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"














#line 292 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"



 
#line 302 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 311 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

 
#line 327 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"



 




#line 343 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

 
















 
#line 370 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 378 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 386 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"


 
#line 395 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"















#line 418 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 427 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 435 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"

#line 443 "..\\..\\..\\..\\Libraries\\inc\\cw32l010_sysctrl.h"




extern void SYSCTRL_HCLKPRS_Config(uint32_t HCLKPRS);
extern void SYSCTRL_PCLKPRS_Config(uint32_t PCLKPRS);
extern void SYSCTRL_SYSCLKSRC_Config(uint32_t SYSCLKSRC);
extern void SYSCTRL_CCS_Config(uint32_t CCS, FunctionalState NewState);
extern void SYSCTRL_LSELOCK_Config(uint32_t LSELOCK);
extern void SYSCTRL_WAKEUPCLK_Config(uint32_t WAKEUPCLK);
extern void SYSCTRL_LOCKUP_Config(uint32_t LOCKUP);
extern void SYSCTRL_SWDIO_Config(uint32_t SWDIO);
extern void SYSCTRL_BrakeConfig(uint32_t BrakeType, FunctionalState NewState);
extern int SYSCTRL_DeInit(void);
extern void SYSCTRL_SystemCoreClockUpdate(uint32_t NewFreq);
extern int SYSCTRL_HSI_Enable(uint32_t HSIDiv);
extern void SYSCTRL_HSI_Disable(void);
extern int SYSCTRL_LSI_Enable(void);
extern void SYSCTRL_LSI_Disable(void);
extern void SYSCTRL_HSEWaitClockSet(uint32_t WaitClock);
extern void SYSCTRL_HSEEnablePinPolSet(uint32_t Polarity);
int SYSCTRL_HSE_Enable(uint32_t Mode, uint32_t FreqIn, uint8_t Driver, uint32_t Flt);
void SYSCTRL_HSE_Disable(void);
extern int SYSCTRL_LSE_Enable(uint32_t Mode, uint8_t Driver);
extern void SYSCTRL_LSE_Disable(void);
extern int SYSCTRL_SysClk_Switch(uint32_t NewClk);
extern void SYSCTRL_ITConfig(uint32_t SYSCTRL_IT, FunctionalState NewState);
extern ITStatus SYSCTRL_GetITStatus(uint32_t SYSCTRL_IT);
extern void SYSCTRL_ClearITPendingBit(uint32_t SYSCTRL_IT);
extern FlagStatus SYSCTRL_GetStableFlag(uint32_t SYSCTRL_STABLEFLAG);
extern uint32_t SYSCTRL_GetAllStableFlag(void);
extern void SYSCTRL_AHBPeriphClk_Enable(uint32_t Periph, FunctionalState NewState);
extern void SYSCTRL_APBPeriphClk_Enable1(uint32_t Periph, FunctionalState NewState);
extern void SYSCTRL_APBPeriphClk_Enable2(uint32_t Periph, FunctionalState NewState);
extern void SYSCTRL_AHBPeriphReset(uint32_t Periph, FunctionalState NewState);
extern void SYSCTRL_APBPeriphReset1(uint32_t Periph, FunctionalState NewState);
extern void SYSCTRL_APBPeriphReset2(uint32_t Periph, FunctionalState NewState);
extern FlagStatus SYSCTRL_GetRstFlag(uint32_t SYSCTRL_RSTFLAG);
extern uint32_t SYSCTRL_GetAllRstFlag(void);
extern void SYSCTRL_ClearRstFlag(uint32_t SYSCTRL_RSTFLAG);

extern void SYSCTRL_PCLK_OUT(void);
extern void SYSCTRL_MCO_OUT(uint8_t Source, uint8_t Div);

extern uint32_t SYSCTRL_GetHClkFreq(void);
extern uint32_t SYSCTRL_GetPClkFreq(void);

void SYSCTRL_GotoSleep(void);
void SYSCTRL_GotoDeepSleep(void);
void SYSCTRL_ConfigSleepOnExit(FunctionalState state);





#line 27 "..\\USER\\src\\interrupts_cw32l010.c"
 
 
 


 
 
 


 
 
 


 
 
 


 
 
 


 
 
 


 
 
 

 
 
extern volatile uint8_t gFlagIrq;
extern uint16_t gCntEoc;
extern volatile uint8_t gKeyStatus ; 
extern volatile uint8_t gLPTIMWakeUpIrq ; 
extern volatile uint8_t gRTCWakeUpIrq;
extern volatile uint8_t gUARTWakeUpIrq;
extern volatile uint8_t gRecvChar;
 


 
 
 


 
void NMI_Handler(void)
{
     

     
}



 
void HardFault_Handler(void)
{
     

     
    while (1)
    {
         

         
    }
}




 
void SVC_Handler(void)
{
     

     
}




 
void PendSV_Handler(void)
{
     

     
}


 
 
 
 
 
 



 
void WDT_IRQHandler(void)
{
     
    
     
}



 
void LVD_IRQHandler(void)
{
     
     
}



 
void RTC_IRQHandler(void)
{
     
    if (((RTC_TypeDef*) 0x40004400UL)->ISR & (0x40UL))
    {
        ((RTC_TypeDef*) 0x40004400UL)->ICR = 0x00;
        gRTCWakeUpIrq = 1;
    }
     
}



 
void FLASHRAM_IRQHandler(void)
{
     

     
}



 
void SYSCTRL_IRQHandler(void)
{
     

     
}



 
extern volatile uint8_t gKeyStatus;
void GPIOA_IRQHandler(void)
{
     
    if (((GPIO_TypeDef*) 0x48000000UL)->ISR_f.PIN6)
    {
        ((GPIO_TypeDef*) 0x48000000UL)->ICR_f.PIN6 = 0;
        gKeyStatus = 1;
    }        
 
     
}



 
void GPIOB_IRQHandler(void)
{
     
    
 

     
}



 
void GPIOC_IRQHandler(void)
{
       
 

     
}



 
void ADC_IRQHandler(void)
{
     


     
}



 
void ATIM_IRQHandler(void)
{
     

     
}



 
void VC1_IRQHandler(void)
{
     
     
}



 
void VC2_IRQHandler(void)
{
     

     
}



 
void GTIM1_IRQHandler(void)
{
     

     
}



 
void LPTIM_IRQHandler(void)
{
     
	if (((LPTIM_TypeDef*) 0x40006000UL)->ISR & (0x2UL))
    {        
        ((LPTIM_TypeDef*) 0x40006000UL)->ICR_f.ARRM = 0;
        gLPTIMWakeUpIrq=1;
    }
     
}



 
void BTIM1_IRQHandler(void)
{
     

     
}



 
void BTIM2_IRQHandler(void)
{
     

     
}



 
void BTIM3_IRQHandler(void)
{
     

     
}



 
void I2C1_IRQHandler(void)
{
     

     
}



 
void SPI1_IRQHandler(void)
{
     

     
}



 
void UART1_IRQHandler(void)
{
     
    if (((UART_TypeDef*) 0x40000C00UL)->ISR_f.RC)
    {
        ((UART_TypeDef*) 0x40000C00UL)->ICR_f.RC = 0;
        gUARTWakeUpIrq = 1;
        gRecvChar = ((UART_TypeDef*) 0x40000C00UL)->RDR;        
    }
     
}



 
void UART2_IRQHandler(void)
{
     
    
     
}



 
void CLKFAULT_IRQHandler(void)
{
     

     
}









 

 

 
