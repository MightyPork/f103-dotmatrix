#include "hw_init.h"

#include "com/iface_usart.h"
#include "com/com_fileio.h"
#include "com/datalink.h"

#include "utils/debounce.h"
#include "utils/timebase.h"

#include "bus/event_queue.h"
#include "com/debug.h"

// ---- Private prototypes --------

static void conf_gpio(void);
static void conf_usart(void);
static void conf_spi(void);
static void conf_systick(void);
static void conf_subsystems(void);
static void conf_irq_prios(void);

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
	timebase_init(15, 15);

	// event and task queues
	queues_init(15, 15);

	// initialize SBMP for ESP8266
	dlnk_init();
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

	// colorled | sonar trig
	gpio_cnf.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13;
	gpio_cnf.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_cnf.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &gpio_cnf);

	gpio_cnf.GPIO_Pin = GPIO_Pin_14;
	gpio_cnf.GPIO_Mode = GPIO_Mode_IPD;
	gpio_cnf.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &gpio_cnf);

	// A0-sonar trig | UART2 - debug, UART1 - esp
	gpio_cnf.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_9 | GPIO_Pin_10;
	gpio_cnf.GPIO_Mode = GPIO_Mode_AF_PP;
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
	debug_iface = usart_iface_init(USART2, 115200, 256, 256);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	debug_iface->file = stdout;

	// Datalink iface
	data_iface = usart_iface_init(USART1, 460800, 256, 256);
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
