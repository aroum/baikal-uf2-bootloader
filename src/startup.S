.set MTVT,    0x307
.set NMITVEC, 0x7C0

.section .startup._start
.globl _start
.type _start, %function

/* Entry point */
_start:
    call    Reset_Handler

/* Reset vector */
.globl Reset_Handler
.type Reset_Handler, %function

Reset_Handler:
    /* Set stack pointer (SP) to the top of the RAM */
    la      sp, __SP

    /* Initialize global pointer (GP) */
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop

    /* Set interrupt mode and trap handler address */
    la      t0, Exception_Handler
    ori     t0, t0, 0x3
    csrw    mtvec, t0

    /* Set interrupt vector table address */
    la      t0, mtvt_clic_vector_table
    csrw    MTVT, t0

    /* Set handler for Non-Maskable Interrupts */
    la      t0, NMI_Handler
    csrw    NMITVEC, t0

    /* Copy .text section to TCM */
copy_text_init:
    la      t0, __TEXT_START__
    la      t1, __TEXT_IMAGE_START__
    la      t2, __TEXT_END__
    beq     t0, t2, copy_text_end
copy_text_loop:
    lw      t3, (t1)
    sw      t3, (t0)
    add     t1, t1, 4
    add     t0, t0, 4
    blt     t0, t2, copy_text_loop
copy_text_end:

    /* Copy .rodata section to TCM */
copy_rodata_init:
    la      t0, __RODATA_START__
    la      t1, __RODATA_IMAGE_START__
    la      t2, __RODATA_END__
    beq     t0, t2, copy_rodata_end
copy_rodata_loop:
    lw      t3, (t1)
    sw      t3, (t0)
    add     t1, t1, 4
    add     t0, t0, 4
    blt     t0, t2, copy_rodata_loop
copy_rodata_end:

    /* Initialize data in RAM */
copy_data_init:
    la      t0, __DATA_START__
    la      t1, __DATA_IMAGE_START__
    la      t2, __DATA_END__
    beq     t0, t2, copy_data_end
copy_data_loop:
    lw      t3, (t1)
    sw      t3, (t0)
    add     t1, t1, 4
    add     t0, t0, 4
    blt     t0, t2, copy_data_loop
copy_data_end:

    /* Clear the .bss section */
clear_bss_init:
    la      t0, __BSS_START__
    la      t1, __BSS_END__
clear_bss_loop:
    sw      zero, (t0)
    add     t0, t0, 4
    blt     t0, t1, clear_bss_loop

    /* Call the main function */
call_main:
    li      a0, 0
    li      a1, 0
    call    main

    /* If main returns, loop infinitely */
    j       .

/* Default ISR handler */
.section .text
.balign 64, 0
.type Default_Handler, %function
Default_Handler:
    /* Infinite loop */
    j       .

/* ISR vector table */
.section .text.isr_vector,"a",%progbits

.balign 64, 0
.globl mtvt_clic_vector_table
.type mtvt_clic_vector_table, %object
mtvt_clic_vector_table:
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Software_IRQHandler       /* Machine software interrupt handler */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Timer_IRQHandler          /* Machine timer interrupt handler */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word CLIC_SW_IRQHandler        /* CLIC software interrupt handler */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word Default_Handler           /* Default handler for unused vectors */
    .word QSPI0_IRQHandler          /* QSPI0 interrupt handler */
    .word UART0_IRQHandler          /* UART0 interrupt handler */
    .word UART1_IRQHandler          /* UART1 interrupt handler */
    .word UART2_IRQHandler          /* UART2 interrupt handler */
    .word I2C0_IRQHandler           /* I2C0 interrupt handler */
    .word I2C1_IRQHandler           /* I2C1 interrupt handler */
    .word I2S0_IRQHandler           /* I2S0 interrupt handler */
    .word TIM0_Channel0_IRQHandler  /* TIM0 Channel 0 interrupt handler */
    .word TIM0_Channel1_IRQHandler  /* TIM0 Channel 1 interrupt handler */
    .word TIM0_Channel2_IRQHandler  /* TIM0 Channel 2 interrupt handler */
    .word TIM0_Channel3_IRQHandler  /* TIM0 Channel 3 interrupt handler */
    .word WDT0_IRQHandler           /* WDT0 interrupt handler */
    .word GPIO0_IRQHandler          /* GPIO0 interrupt handler */
    .word PWMG0_IRQHandler          /* PWMG0 interrupt handler */
    .word SPI0_IRQHandler           /* SPI0 interrupt handler */
    .word SPI1_IRQHandler           /* SPI1 interrupt handler */
    .word CANFD0_IRQHandler         /* CANFD0 interrupt handler */
    .word QSPI1_IRQHandler          /* QSPI1 interrupt handler */
    .word UART3_IRQHandler          /* UART3 interrupt handler */
    .word UART4_IRQHandler          /* UART4 interrupt handler */
    .word UART5_IRQHandler          /* UART5 interrupt handler */
    .word I2C2_IRQHandler           /* I2C2 interrupt handler */
    .word I2C3_IRQHandler           /* I2C3 interrupt handler */
    .word I2S1_IRQHandler           /* I2S1 interrupt handler */
    .word TIM1_Channel0_IRQHandler  /* TIM1 Channel 0 interrupt handler */
    .word TIM1_Channel1_IRQHandler  /* TIM1 Channel 1 interrupt handler */
    .word TIM1_Channel2_IRQHandler  /* TIM1 Channel 2 interrupt handler */
    .word TIM1_Channel3_IRQHandler  /* TIM1 Channel 3 interrupt handler */
    .word Default_Handler           /* Default handler for unused vectors */
    .word GPIO1_IRQHandler          /* GPIO1 interrupt handler */
    .word PWMG1_IRQHandler          /* PWMG1 interrupt handler */
    .word SPI2_IRQHandler           /* SPI2 interrupt handler */
    .word SPI3_IRQHandler           /* SPI3 interrupt handler */
    .word CANFD1_IRQHandler         /* CANFD1 interrupt handler */
    .word ADC0_IRQHandler           /* ADC0 interrupt handler */
    .word ADC1_IRQHandler           /* ADC1 interrupt handler */
    .word ADC2_IRQHandler           /* ADC2 interrupt handler */
    .word PWMA0_IRQHandler          /* PWMA0 interrupt handler */
    .word QE0_IRQHandler            /* QE0 interrupt handler */
    .word PWMA1_IRQHandler          /* PWMA1 interrupt handler */
    .word QE1_IRQHandler            /* QE1 interrupt handler */
    .word PWMA2_IRQHandler          /* PWMA2 interrupt handler */
    .word QE2_IRQHandler            /* QE2 interrupt handler */
    .word PWMA3_IRQHandler          /* PWMA3 interrupt handler */
    .word QE3_IRQHandler            /* QE3 interrupt handler */
    .word GPIO2_IRQHandler          /* GPIO2 interrupt handler */
    .word UART6_IRQHandler          /* UART6 interrupt handler */
    .word UART7_IRQHandler          /* UART7 interrupt handler */
    .word USB_IRQHandler            /* USB interrupt handler */
    .word DMA0_IRQHandler           /* DMA0 interrupt handler */
    .word DMA1_IRQHandler           /* DMA1 interrupt handler */
    .word USB_DMA_IRQHandler        /* USB DMA interrupt handler */
    .word CORE2_IRQHandler          /* CORE2 interrupt handler */
    .word CORE2_MB0_IRQHandler      /* CORE2 Mailbox 0 interrupt handler */
    .word CORE2_MB1_IRQHandler      /* CORE2 Mailbox 1 interrupt handler */
    .word EXTI_PA_IRQHandler        /* EXTI PA interrupt handler */
    .word EXTI_PB_IRQHandler        /* EXTI PB interrupt handler */
    .word EXTI_PC_IRQHandler        /* EXTI PC interrupt handler */
    .word CLKI_Mon_IRQHandler       /* CLKI Monitor interrupt handler */
    .word CORE0_BusError_IRQHandler /* CORE0 Bus Access Error interrupt handler */
    .word CORE1_BusError_IRQHandler /* CORE1 Bus Access Error interrupt handler */
    .word PLL_Unlock_IRQHandler     /* PLL Unlock interrupt handler */

.size mtvt_clic_vector_table, . - mtvt_clic_vector_table

/* Provide weak alias for each handler to the Default_Handler */

/* NMI_Handler must be 4 bytes aligned when overriden */
.weak NMI_Handler
.set NMI_Handler,Default_Handler

/* Exception_Handler must be 64 bytes aligned when overriden */
.weak Exception_Handler
.set Exception_Handler,Default_Handler

.weak Software_IRQHandler
.set Software_IRQHandler,Default_Handler

.weak Timer_IRQHandler
.set Timer_IRQHandler,Default_Handler

.weak CLIC_SW_IRQHandler
.set CLIC_SW_IRQHandler,Default_Handler

.weak QSPI0_IRQHandler
.set QSPI0_IRQHandler,Default_Handler

.weak UART0_IRQHandler
.set UART0_IRQHandler,Default_Handler

.weak UART1_IRQHandler
.set UART1_IRQHandler,Default_Handler

.weak UART2_IRQHandler
.set UART2_IRQHandler,Default_Handler

.weak I2C0_IRQHandler
.set I2C0_IRQHandler,Default_Handler

.weak I2C1_IRQHandler
.set I2C1_IRQHandler,Default_Handler

.weak I2S0_IRQHandler
.set I2S0_IRQHandler,Default_Handler

.weak TIM0_Channel0_IRQHandler
.set TIM0_Channel0_IRQHandler,Default_Handler

.weak TIM0_Channel1_IRQHandler
.set TIM0_Channel1_IRQHandler,Default_Handler

.weak TIM0_Channel2_IRQHandler
.set TIM0_Channel2_IRQHandler,Default_Handler

.weak TIM0_Channel3_IRQHandler
.set TIM0_Channel3_IRQHandler,Default_Handler

.weak WDT0_IRQHandler
.set WDT0_IRQHandler,Default_Handler

.weak GPIO0_IRQHandler
.set GPIO0_IRQHandler,Default_Handler

.weak PWMG0_IRQHandler
.set PWMG0_IRQHandler,Default_Handler

.weak SPI0_IRQHandler
.set SPI0_IRQHandler,Default_Handler

.weak SPI1_IRQHandler
.set SPI1_IRQHandler,Default_Handler

.weak CANFD0_IRQHandler
.set CANFD0_IRQHandler,Default_Handler

.weak QSPI1_IRQHandler
.set QSPI1_IRQHandler,Default_Handler

.weak UART3_IRQHandler
.set UART3_IRQHandler,Default_Handler

.weak UART4_IRQHandler
.set UART4_IRQHandler,Default_Handler

.weak UART5_IRQHandler
.set UART5_IRQHandler,Default_Handler

.weak I2C2_IRQHandler
.set I2C2_IRQHandler,Default_Handler

.weak I2C3_IRQHandler
.set I2C3_IRQHandler,Default_Handler

.weak I2S1_IRQHandler
.set I2S1_IRQHandler,Default_Handler

.weak TIM1_Channel0_IRQHandler
.set TIM1_Channel0_IRQHandler,Default_Handler

.weak TIM1_Channel1_IRQHandler
.set TIM1_Channel1_IRQHandler,Default_Handler

.weak TIM1_Channel2_IRQHandler
.set TIM1_Channel2_IRQHandler,Default_Handler

.weak TIM1_Channel3_IRQHandler
.set TIM1_Channel3_IRQHandler,Default_Handler

.weak GPIO1_IRQHandler
.set GPIO1_IRQHandler,Default_Handler

.weak PWMG1_IRQHandler
.set PWMG1_IRQHandler,Default_Handler

.weak SPI2_IRQHandler
.set SPI2_IRQHandler,Default_Handler

.weak SPI3_IRQHandler
.set SPI3_IRQHandler,Default_Handler

.weak CANFD1_IRQHandler
.set CANFD1_IRQHandler,Default_Handler

.weak ADC0_IRQHandler
.set ADC0_IRQHandler,Default_Handler

.weak ADC1_IRQHandler
.set ADC1_IRQHandler,Default_Handler

.weak ADC2_IRQHandler
.set ADC2_IRQHandler,Default_Handler

.weak PWMA0_IRQHandler
.set PWMA0_IRQHandler,Default_Handler

.weak QE0_IRQHandler
.set QE0_IRQHandler,Default_Handler

.weak PWMA1_IRQHandler
.set PWMA1_IRQHandler,Default_Handler

.weak QE1_IRQHandler
.set QE1_IRQHandler,Default_Handler

.weak PWMA2_IRQHandler
.set PWMA2_IRQHandler,Default_Handler

.weak QE2_IRQHandler
.set QE2_IRQHandler,Default_Handler

.weak PWMA3_IRQHandler
.set PWMA3_IRQHandler,Default_Handler

.weak QE3_IRQHandler
.set QE3_IRQHandler,Default_Handler

.weak GPIO2_IRQHandler
.set GPIO2_IRQHandler,Default_Handler

.weak UART6_IRQHandler
.set UART6_IRQHandler,Default_Handler

.weak UART7_IRQHandler
.set UART7_IRQHandler,Default_Handler

.weak USB_IRQHandler
.set USB_IRQHandler,Default_Handler

.weak DMA0_IRQHandler
.set DMA0_IRQHandler,Default_Handler

.weak DMA1_IRQHandler
.set DMA1_IRQHandler,Default_Handler

.weak USB_DMA_IRQHandler
.set USB_DMA_IRQHandler,Default_Handler

.weak CORE2_IRQHandler
.set CORE2_IRQHandler,Default_Handler

.weak CORE2_MB0_IRQHandler
.set CORE2_MB0_IRQHandler,Default_Handler

.weak CORE2_MB1_IRQHandler
.set CORE2_MB1_IRQHandler,Default_Handler

.weak EXTI_PA_IRQHandler
.set EXTI_PA_IRQHandler,Default_Handler

.weak EXTI_PB_IRQHandler
.set EXTI_PB_IRQHandler,Default_Handler

.weak EXTI_PC_IRQHandler
.set EXTI_PC_IRQHandler,Default_Handler

.weak CLKI_Mon_IRQHandler
.set CLKI_Mon_IRQHandler,Default_Handler

.weak CORE0_BusError_IRQHandler
.set CORE0_BusError_IRQHandler,Default_Handler

.weak CORE1_BusError_IRQHandler
.set CORE1_BusError_IRQHandler,Default_Handler

.weak PLL_Unlock_IRQHandler
.set PLL_Unlock_IRQHandler,Default_Handler
