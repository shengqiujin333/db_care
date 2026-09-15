@ GNU assembler startup for CW32L010 (Cortex-M0+), sensor project
@ Generated from Keil startup_cw32l010.s for host cross-build verification.
    .syntax unified
    .cpu cortex-m0plus
    .fpu softvfp
    .thumb

    .section .isr_vector,"a",%progbits
    .global __Vectors
    .global __Vectors_End
    .global __Vectors_Size
__Vectors:
    .word __StackTop
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word 0
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    .word WDT_IRQHandler
    .word LVD_IRQHandler
    .word RTC_IRQHandler
    .word FLASHRAM_IRQHandler
    .word SYSCTRL_IRQHandler
    .word GPIOA_IRQHandler
    .word GPIOB_IRQHandler
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word ADC_IRQHandler
    .word ATIM_IRQHandler
    .word VC1_IRQHandler
    .word VC2_IRQHandler
    .word GTIM1_IRQHandler
    .word 0
    .word 0
    .word LPTIM_IRQHandler
    .word BTIM1_IRQHandler
    .word BTIM2_IRQHandler
    .word BTIM3_IRQHandler
    .word I2C1_IRQHandler
    .word 0
    .word SPI1_IRQHandler
    .word 0
    .word UART1_IRQHandler
    .word UART2_IRQHandler
    .word 0
    .word 0
    .word CLKFAULT_IRQHandler
__Vectors_End:
    .equ __Vectors_Size, __Vectors_End - __Vectors

    .section .text
    .thumb_func
    .global Reset_Handler
Reset_Handler:
    ldr r0, =__StackTop
    mov sp, r0
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
copy_loop:
    cmp r0, r1
    bge copy_done
    ldr r3, [r2]
    str r3, [r0]
    adds r0, #4
    adds r2, #4
    b copy_loop
copy_done:
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
zero_loop:
    cmp r0, r1
    bge zero_done
    str r2, [r0]
    adds r0, #4
    b zero_loop
zero_done:
    bl SystemInit
    bl main
    b .
    .size Reset_Handler, . - Reset_Handler

@ Weak default handlers (overridden by strong definitions in app)
    .macro weak_handler name
    .weak \name
    .thumb_func
    \name:
    b .
    .endm

    weak_handler NMI_Handler
    weak_handler HardFault_Handler
    weak_handler SVC_Handler
    weak_handler PendSV_Handler
    weak_handler SysTick_Handler
    weak_handler WDT_IRQHandler
    weak_handler LVD_IRQHandler
    weak_handler RTC_IRQHandler
    weak_handler FLASHRAM_IRQHandler
    weak_handler SYSCTRL_IRQHandler
    weak_handler GPIOA_IRQHandler
    weak_handler GPIOB_IRQHandler
    weak_handler ADC_IRQHandler
    weak_handler ATIM_IRQHandler
    weak_handler VC1_IRQHandler
    weak_handler VC2_IRQHandler
    weak_handler GTIM1_IRQHandler
    weak_handler LPTIM_IRQHandler
    weak_handler BTIM1_IRQHandler
    weak_handler BTIM2_IRQHandler
    weak_handler BTIM3_IRQHandler
    weak_handler I2C1_IRQHandler
    weak_handler SPI1_IRQHandler
    weak_handler UART1_IRQHandler
    weak_handler UART2_IRQHandler
    weak_handler CLKFAULT_IRQHandler
    .end
