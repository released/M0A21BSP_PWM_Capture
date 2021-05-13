/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include	"project_config.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/
#define SYS_CLOCK       							(48000000)
#define SPI_TARGET_FREQ							(800000ul)	//(48000000ul)
#define MASTER_DATA_NUM						(2)

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

uint16_t g_au16Count[4] = {0};
volatile uint32_t g_u32IsTestOver = 0;
uint8_t duty = 30 ;
uint32_t freq = 100000 ;

uint8_t MasterToSlaveTestPattern[MASTER_DATA_NUM]={0};
uint32_t u32MasterToSlaveTestPattern[MASTER_DATA_NUM]={0};
uint8_t MasterRxBuffer[MASTER_DATA_NUM]={0};
uint16_t spiTxDataCount = 0;
uint16_t spiRxDataCount = 0;

uint32_t detect_freq = 0 ;
uint32_t detect_duty = 0 ;
/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void delay(uint16_t dly)
{
/*
	delay(100) : 14.84 us
	delay(200) : 29.37 us
	delay(300) : 43.97 us
	delay(400) : 58.5 us	
	delay(500) : 73.13 us	
	
	delay(1500) : 0.218 ms (218 us)
	delay(2000) : 0.291 ms (291 us)	
*/

	while( dly--);
}


void delay_ms(uint16_t ms)
{
	TIMER_Delay(TIMER0, 1000*ms);
}


void USCI0_IRQHandler(void)
{

    USPI_CLR_PROT_INT_FLAG(USPI0, USPI_PROTSTS_RXENDIF_Msk);

    /* Waiting for RX is not empty */
    while (USPI_GET_RX_EMPTY_FLAG(USPI0) == 1);

    /* Check RX EMPTY flag */
    while (USPI_GET_RX_EMPTY_FLAG(USPI0) == 0)
    {
        /* Read RX Buffer */
        MasterRxBuffer[spiRxDataCount++] = USPI_READ_RX(USPI0);
    }

	if (spiRxDataCount >= MASTER_DATA_NUM)
	{
    	USPI_DisableInt(USPI0, USPI_RXEND_INT_MASK);
	
		set_flag(flag_SPI_RX_Finish , ENABLE);
		spiRxDataCount = 0;
	}

}

void USPI_Master_Tx(uint8_t* u8bffer , uint16_t len)
{
	uint16_t i = 0;

    while (i < len)
    {
        USPI_WRITE_TX(USPI0, u8bffer[i++]);
		while(USPI_IS_BUSY(USPI0));
    }	
}


void USPI_Master_Loop_Process(void)
{
//    uint32_t u32DataCount = 0;
//    uint16_t i = 0;

	if (is_flag_set(flag_SPI_RX_Finish))	
	{		
		set_flag(flag_SPI_RX_Finish , DISABLE);	
		
		#if 1	//debug
		dump_buffer(MasterRxBuffer, MASTER_DATA_NUM);	
		#endif	
		reset_buffer(MasterRxBuffer , 0xFF , MASTER_DATA_NUM);	
	}	

	if (is_flag_set(flag_SPI_Transmit))
	{	
		set_flag(flag_SPI_Transmit , DISABLE);
			
		//CLEAR data
		reset_buffer(MasterToSlaveTestPattern , 0xFF , MASTER_DATA_NUM);
		reset_buffer(u32MasterToSlaveTestPattern , 0xFFFFFFFF , MASTER_DATA_NUM);		
//		reset_buffer(MasterRxBuffer , 0xFF , MASTER_DATA_NUM);

		#if 1
		MasterToSlaveTestPattern[0] = LOBYTE(detect_duty); 
		MasterToSlaveTestPattern[1] = HIBYTE(detect_duty);  
		USPI_Master_Tx(MasterToSlaveTestPattern,MASTER_DATA_NUM);
		#else	// simluate TX transfer
		if (is_flag_set(flag_SPI_TX_DataChange))
		{
			set_flag(flag_SPI_TX_DataChange , DISABLE);

			for (i = 0; i < MASTER_DATA_NUM; i++)
			{
				MasterToSlaveTestPattern[i] = 0x00 + i;
			}

			USPI_Master_Tx(MasterToSlaveTestPattern,MASTER_DATA_NUM);		

		}
		else
		{
			set_flag(flag_SPI_TX_DataChange , ENABLE);

			for (i = 0; i < MASTER_DATA_NUM; i++)
			{
				MasterToSlaveTestPattern[i] = 0x80 + i;		
			}

			USPI_Master_Tx(MasterToSlaveTestPattern,MASTER_DATA_NUM);	
		
		}
		#endif
	}	
}


void USPI_Master_Init(void)
{	
    USPI_Open(USPI0, USPI_MASTER, USPI_MODE_0, 8, SPI_TARGET_FREQ);
    /* Enable the automatic hardware slave selection function and configure USCI_SPI_SS pin as low-active. */
    USPI_EnableAutoSS(USPI0, 0, USPI_SS_ACTIVE_LOW);

    USPI_EnableInt(USPI0, USPI_RXEND_INT_MASK);

    spiTxDataCount = 0;
    spiRxDataCount = 0;

    NVIC_EnableIRQ(USCI0_IRQn);	
}

void CalPeriodTime(PWM_T *PWM, uint32_t u32Ch)
{
    uint16_t u16RisingTime, u16FallingTime, u16HighPeriod, u16LowPeriod, u16TotalPeriod;

    g_u32IsTestOver = 0;
	
    /* Wait PDMA interrupt (g_u32IsTestOver will be set at IRQ_Handler function) */
    while(g_u32IsTestOver == 0);
		 
    u16RisingTime = g_au16Count[1];

    u16FallingTime = g_au16Count[0];

    u16HighPeriod = g_au16Count[1] - g_au16Count[2] + 1;

    u16LowPeriod = 0xFFFF - g_au16Count[1];

    u16TotalPeriod = 0xFFFF - g_au16Count[2] + 1;

	detect_freq = SYS_CLOCK/u16TotalPeriod;
	detect_duty = u16HighPeriod*100/u16TotalPeriod;

    printf("Rising=%5d,Falling=%5d,High=%5d,Low=%5d,Total=%5d,Freq=%5d,Duty=%3d.\r\n",
           u16RisingTime, 
           u16FallingTime, 
           u16HighPeriod, 
           u16LowPeriod, 
           u16TotalPeriod,
           detect_freq,
           detect_duty);
}

void PDMA_IRQHandler(void)
{
    uint32_t status = PDMA_GET_INT_STATUS(PDMA);

    if(status & 0x1)    /* abort */
    {
        if(PDMA_GET_ABORT_STS(PDMA) & 0x1)
        {
			g_u32IsTestOver = 2;
        }
        PDMA_CLR_ABORT_FLAG(PDMA, PDMA_ABTSTS_ABTIF0_Msk);
    }
    else if(status & 0x2)      /* done */
    {
        if(PDMA_GET_TD_STS(PDMA) & 0x1)
        {
			g_u32IsTestOver = 1;
        }
        PDMA_CLR_TD_FLAG(PDMA, PDMA_TDSTS_TDIF0_Msk);
    }
    else
        printf("unknown interrupt !!\n");
}


void PDMA_Init(void)
{
    /* Open Channel 0 */
    PDMA_Open(PDMA, 0x1);

    /* Transfer width is half word(16 bit) and transfer count is 4 */
    PDMA_SetTransferCnt(PDMA, 0, PDMA_WIDTH_16, 4);

    /* Set source address as PWM capture channel PDMA register(no increment) and destination address as g_au16Count array(increment) */
    PDMA_SetTransferAddr(PDMA, 0, (uint32_t)&PWM0->PDMACAP2_3, PDMA_SAR_FIX, (uint32_t)&g_au16Count[0], PDMA_DAR_INC);

    /* Select PDMA request source as PWM RX(PWM0 channel 2 should be PWM0 pair 2) */
    PDMA_SetTransferMode(PDMA, 0, PDMA_PWM0_P2_RX, FALSE, 0);

    /* Set PDMA as single request type for PWM */
    PDMA_SetBurstType(PDMA, 0, PDMA_REQ_SINGLE, PDMA_BURST_4);

    PDMA_EnableInt(PDMA, 0, PDMA_INT_TRANS_DONE);
    NVIC_EnableIRQ(PDMA_IRQn);

    /* Enable PDMA for PWM0 channel 2 capture function, and set capture order as falling first, */
    /* And select capture mode as both rising and falling to do PDMA transfer. */
    PWM_EnablePDMA(PWM0, 2, FALSE, PWM_CAPTURE_PDMA_RISING_FALLING_LATCH);
}


void PWM_Out_DeInit(void)
{
    /* Set PWM0 channel 0 loaded value as 0 */
    PWM_Stop(PWM0, PWM_CH_0_MASK);

    /* Wait until PWM0 channel 0 Timer Stop */
//    while((PWM0->CNT[0] & PWM_CNT_CNT_Msk) != 0);

    /* Disable Timer for PWM0 channel 0 */
    PWM_ForceStop(PWM0, PWM_CH_0_MASK);

    /* Disable PWM Output path for PWM0 channel 0 */
    PWM_DisableOutput(PWM0, PWM_CH_0_MASK);
}

void PWM_Out_Init(void)	// PB5 , PWM0_CH0 , PWM output for test
{
    /* Set PWM0 channel 0 output configuration */
    PWM_ConfigOutputChannel(PWM0, 0, freq, duty);

    /* Enable PWM Output path for PWM0 channel 0 */
    PWM_EnableOutput(PWM0, PWM_CH_0_MASK);

    /* Enable Timer for PWM0 channel 0 */
    PWM_Start(PWM0, PWM_CH_0_MASK);
}

void PWM_Cap_DeInit(void)
{
    /* Set loaded value as 0 for PWM0 channel 2 */
    PWM_Stop(PWM0, PWM_CH_2_MASK);

    /* Wait until PWM0 channel 2 current counter reach to 0 */
//    while((PWM0->CNT[2] & PWM_CNT_CNT_Msk) != 0);

    /* Disable Timer for PWM0 channel 2 */
    PWM_ForceStop(PWM0, PWM_CH_2_MASK);

    /* Disable Capture Function and Capture Input path for  PWM0 channel 2*/
    PWM_DisableCapture(PWM0, PWM_CH_2_MASK);

    /* Clear Capture Interrupt flag for PWM0 channel 2 */
    PWM_ClearCaptureIntFlag(PWM0, 2, PWM_CAPTURE_INT_FALLING_LATCH);

	PWM_DisablePDMA(PWM0, 2);

    /* Disable PDMA NVIC */
    NVIC_DisableIRQ(PDMA_IRQn);

    PDMA_Close(PDMA);
}

void PWM_Cap_Init(void)	// PB7 , PWM0_CH2 , capture
{
    /* (Note: CNR is 16 bits, so if calculated value is larger than 65536, user should increase prescale value.)
         CNR = 0xFFFF
         (Note: In capture mode, user should set CNR to 0xFFFF to increase capture frequency range.)

         Capture unit time = 1/Capture clock source frequency/prescaler
         20.8ns = 1/48,000,000/1
    */

	/* Set PWM0 channel 2 capture configuration */
	PWM_ConfigCaptureChannel(PWM0, 2, 20, 0);

	/* Enable Timer for PWM0 channel 2 */
	PWM_Start(PWM0, PWM_CH_2_MASK);

	/* Enable Capture Function for PWM0 channel 2 */
	PWM_EnableCapture(PWM0, PWM_CH_2_MASK);

	/* Enable falling capture reload */
	PWM0->CAPCTL |= PWM_CAPCTL_FCRLDEN2_Msk;

	/* Wait until PWM0 channel 2 Timer start to count */
//	while((PWM0->CNT[2]) == 0);

}

void GPIO_Init (void)
{
	// EVM LED_R
    GPIO_SetMode(PA, BIT2, GPIO_MODE_OUTPUT);

	// EVM button
    GPIO_SetMode(PC, BIT2, GPIO_MODE_OUTPUT);	
}


void TMR1_IRQHandler(void)
{
//	static uint32_t LOG = 0;
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
//        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PA2 ^= 1;
		}

		if ((get_tick() % 2) == 0)
		{
			set_flag(flag_SPI_Transmit , ENABLE);
		}
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				duty += 10;
				if ( duty > 90)
				{
					duty = 90;
				}

				break;
			
			case '2':
				duty -= 10;

				if ( duty < 10)
				{
					duty = 10;
				}					
			
				break;	

			case '3':
				freq += 100;
				if ( freq >= 200000)
				{
					freq = 200000;
				}
			
				break;
			
			case '4':
				freq -= 100;

				if ( freq < 1000)
				{
					freq = 1000;
				}
	
				break;	

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}

void UART0_IRQHandler(void)
{

    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART0_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif	

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk|CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk|CLK_STATUS_HXTSTB_Msk);

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk|CLK_PWRCTL_LXTEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk|CLK_STATUS_LXTSTB_Msk);	

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

    CLK_EnableModuleClock(PWM0_MODULE);

    CLK_EnableModuleClock(PDMA_MODULE);	

    CLK_EnableModuleClock(USCI0_MODULE);	

    /* Reset PWM0 module */
    SYS_ResetModule(PWM0_RST);

    /* Reset PDMA module */
    SYS_ResetModule(PDMA_RST);

    /* Set PB multi-function pins for UART0 RXD=PB.6 and TXD=PB.4 */
    SYS->GPB_MFP1 = (SYS->GPB_MFP1 & ~(SYS_GPB_MFP1_PB4MFP_Msk | SYS_GPB_MFP1_PB6MFP_Msk)) |        \
                    (SYS_GPB_MFP1_PB4MFP_UART0_TXD | SYS_GPB_MFP1_PB6MFP_UART0_RXD);


    /* Set PB multi-function pins for PWM0 Channel 0 and 2 */
    SYS->GPB_MFP1 = (SYS->GPB_MFP1 & ~(SYS_GPB_MFP1_PB5MFP_Msk | SYS_GPB_MFP1_PB7MFP_Msk)) |
                    (SYS_GPB_MFP1_PB5MFP_PWM0_CH0 | SYS_GPB_MFP1_PB7MFP_PWM0_CH2);

    /* Set USCI_SPI0 multi-function pins */
	/*
		SPI CLK : USCIx_CLK 		: PC6
		SPI SS  : USCIx_CTL0		: PC7
		SPI MOSI : USCIx_DAT0 		: PC5
		SPI MISO : USCIx_DAT1 		: PC4
	*/
    SYS->GPC_MFP1 = (SYS->GPC_MFP1 & ~(SYS_GPC_MFP1_PC4MFP_Msk | SYS_GPC_MFP1_PC5MFP_Msk | SYS_GPC_MFP1_PC6MFP_Msk | SYS_GPC_MFP1_PC7MFP_Msk)) |
                    (SYS_GPC_MFP1_PC4MFP_USCI0_DAT1 | SYS_GPC_MFP1_PC5MFP_USCI0_DAT0 | SYS_GPC_MFP1_PC6MFP_USCI0_CLK | SYS_GPC_MFP1_PC7MFP_USCI0_CTL0);


   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	GPIO_Init();
	TIMER1_Init();

	USPI_Master_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {
		PWM_Out_Init();	

		PWM_Cap_Init();

		PDMA_Init();

		/* Capture the Input Waveform Data */
		CalPeriodTime(PWM0, 2);

		PWM_Out_DeInit();

		PWM_Cap_DeInit();

		USPI_Master_Loop_Process();

    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
