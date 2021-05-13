# M0A21BSP_PWM_Capture
 M0A21BSP_PWM_Capture


update @ 2021/05/13

1. Add PWM input capture with PDMA interrupt sample code for M0A2x EVM (need to check SW on EVM)

- PWM output for test : PB5 , PWM0_CH0		

- capture : PB7 , PWM0_CH2

2. Add USCI SPI transmit (need to check SW on EVM)

- SPI CLK : USCIx_CLK 	: PC6
		
- SPI SS  : USCIx_CTL0	: PC7
		
- SPI MOSI : USCIx_DAT0 : PC5
		
- SPI MISO : USCIx_DAT1 : PC4

3. use UART terminal to control , 

- press digit 1 , to increase duty 10% , digit 2 , to decrease duty 10%

- press digit 3 , to increase freq 10 , digit 4 , to decrease freq 10


below is terminal message , for input PWM capture freq / duuty 

![image](https://github.com/released/M0A21BSP_PWM_Capture/blob/main/pwm_capture.jpg)

below is SPI TX , if duty = 30%

![image](https://github.com/released/M0A21BSP_PWM_Capture/blob/main/SPI_TX_30.jpg)

below is SPI TX , if duty = 70%

![image](https://github.com/released/M0A21BSP_PWM_Capture/blob/main/SPI_TX_70.jpg)


