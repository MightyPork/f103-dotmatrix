#include "hw_init.h"

#include "com/iface_usart.h"
#include "com/com_fileio.h"
//#include "com/datalink.h"

#include "utils/debounce.h"
#include "utils/timebase.h"

#include "bus/event_queue.h"
#include "com/debug.h"

#include "dotmatrix.h"

// ---- Private prototypes --------

static void conf_gpio(void);
static void conf_usart(void);
static void conf_spi(void);
static void conf_systick(void);
static void conf_subsystems(void);
static void conf_irq_prios(void);
static void conf_adc(void);
static void init_display(void);
// ---- Public functions ----------

/**
 * @brief Initialize hardware resources
 */
void hw_init(void)
{
	conf_gpio();
	conf_usart();
	conf_systick();
	conf_spi();
	conf_adc();
	conf_irq_prios();
	conf_subsystems();
}


// ---- Private functions ---------


static void conf_irq_prios(void)
{
	NVIC_SetPriorityGrouping(0); // 0 bits for sub-priority

	// SysTick - highest prio, used for timeouts
	NVIC_SetPriority(SysTick_IRQn, 0); // SysTick - for timeouts
	NVIC_SetPriority(USART2_IRQn, 6); // USART - datalink
	NVIC_SetPriority(USART1_IRQn, 10); // USART - debug

	// FIXME check , probably bad ports
}


/**
 * @brief Configure SW subsystems
 */
static void conf_subsystems(void)
{
	// task scheduler subsystem
	timebase_init(20, 20);

	// event and task queues
	queues_init(30, 30);

	// initialize SBMP for ESP8266
//	dlnk_init();

	// dot matrix
	init_display();
}


static void init_display(void)
{
	DotMatrix_Init dmtx_cfg;
	dmtx_cfg.CS_GPIOx = GPIOA;
	dmtx_cfg.CS_PINx = GPIO_Pin_4;
	dmtx_cfg.SPIx = SPI1;
	dmtx_cfg.cols = 2;
	dmtx_cfg.rows = 2;

	dmtx = dmtx_init(&dmtx_cfg);

	dmtx_intensity(dmtx, 7);
}


/**
 * @brief Configure GPIOs
 */
static void conf_gpio(void)
{
	GPIO_InitTypeDef gpio_cnf;
	GPIO_StructInit(&gpio_cnf);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	// Red LED
	gpio_cnf.GPIO_Pin = GPIO_Pin_13;
	gpio_cnf.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_cnf.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOC, &gpio_cnf);

	// [ UARTs ]

	// Tx
	gpio_cnf.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_9;
	gpio_cnf.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_cnf.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gpio_cnf);

	// Rx
	gpio_cnf.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_3;
	gpio_cnf.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio_cnf);


	// A0 - analog input for ADC
	gpio_cnf.GPIO_Pin = GPIO_Pin_0;
	gpio_cnf.GPIO_Mode = GPIO_Mode_AIN;
	gpio_cnf.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gpio_cnf);

	// SPI
	gpio_cnf.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	gpio_cnf.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_cnf.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gpio_cnf);
	// SPI NSS out
	gpio_cnf.GPIO_Pin = GPIO_Pin_4;
	gpio_cnf.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_cnf.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gpio_cnf);
}


/**
 * @brief Configure USARTs
 */
static void conf_usart(void)
{
	// Debug interface, working as stdout/stderr.
	debug_iface = usart_iface_init(USART1, 115200, 256, 256);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	debug_iface->file = stdout;


	gamepad_iface = usart_iface_init(USART2, 115200, 16, 16);


	// Datalink iface
	//data_iface = usart_iface_init(USART1, 460800, 256, 256);
}

/**
 * @brief Configure SPI
 */
static void conf_spi(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_SPI1EN, ENABLE);

	SPI_InitTypeDef spi_cnf;
	SPI_StructInit(&spi_cnf);

	spi_cnf.SPI_Direction = SPI_Direction_1Line_Tx;
	spi_cnf.SPI_Mode = SPI_Mode_Master;
	spi_cnf.SPI_NSS = SPI_NSS_Soft;
	spi_cnf.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;

	SPI_Init(SPI1, &spi_cnf);

	SPI_Cmd(SPI1, ENABLE);
}


/**
 * @brief Configure 1 kHz SysTick w/ interrupt
 */
static void conf_systick(void)
{
	SysTick_Config(F_CPU / 1000);
}


static void conf_adc(void)
{
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM3EN, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBENR_DMA1EN, ENABLE);
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	// Configure the ADC
	ADC_DeInit(ADC1);
	ADC_InitTypeDef adc_cnf;
	adc_cnf.ADC_Mode = ADC_Mode_Independent;
	adc_cnf.ADC_ScanConvMode = DISABLE;
	adc_cnf.ADC_ContinuousConvMode = DISABLE;
	adc_cnf.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
	adc_cnf.ADC_DataAlign = ADC_DataAlign_Right;
	adc_cnf.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &adc_cnf);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ExternalTrigConvCmd(ADC1, ENABLE);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_7Cycles5);

	// calib
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));


	// Configure the DMA timer
	TIM_DeInit(TIM3);
	TIM_TimeBaseInitTypeDef tim_cnf;
	tim_cnf.TIM_Period = 1800;
	tim_cnf.TIM_Prescaler = 1;
	tim_cnf.TIM_ClockDivision = TIM_CKD_DIV1;
	tim_cnf.TIM_CounterMode = TIM_CounterMode_Up;
	tim_cnf.TIM_RepetitionCounter = 0x0000;
	TIM_TimeBaseInit(TIM3, &tim_cnf);

	TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
}
