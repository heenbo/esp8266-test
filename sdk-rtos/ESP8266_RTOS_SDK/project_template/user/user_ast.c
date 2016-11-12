/*************************************************************************
 *   > File Name: user_ast.c
 *   > Author: heenbo
 *   > Mail: 379667345@qq.com 
 *   > Created Time:  2016年11月10日 星期四 17时36分35秒
 *   > Modified Time: 2016年11月12日 星期六 13时38分50秒
 ************************************************************************/

#include "esp_common.h"
#include "gpio.h"
#include "ets_sys.h"

#define GPIO_PEN_SCK_IO_MUX	PERIPHS_IO_MUX_GPIO5_U
#define GPIO_PEN_SCK_IO_NUM	5
#define GPIO_PEN_SCK_IO_FUNC	FUNC_GPIO5

#define GPIO_PEN_SDIO_IO_MUX	PERIPHS_IO_MUX_GPIO4_U
#define GPIO_PEN_SDIO_IO_NUM	4
#define GPIO_PEN_SDIO_IO_FUNC	FUNC_GPIO4

#define SCK_LEVEL_LOW (0)
#define SCK_LEVEL_HIGH (!(SCK_LEVEL_LOW))

#define SDIO_LEVEL_LOW (0)
#define SDIO_LEVEL_HIGH (!(SDIO_LEVEL_LOW))

#define DEFAULT_PEN_AP_SSID	"Dlink-fly"
#define DEFAULT_PEN_AP_PASSWD	"fuxxk1202steal"

static uint8 gpio_pen_sck_level = 0;
static GPIO_ConfigTypeDef * pGPIOConfig_gpio_pen_sdio = NULL;

static void user_gpio_pen_sck_init(void)
{
    PIN_FUNC_SELECT(GPIO_PEN_SCK_IO_MUX, GPIO_PEN_SCK_IO_FUNC);
}

static void user_gpio_pen_set_sck(uint8 sck_level_value)
{
    	GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_PEN_SCK_IO_NUM), sck_level_value);
}

static void user_gpio_pen_set_sdio(uint8 sdio_level_value)
{
    	GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_PEN_SDIO_IO_NUM), sdio_level_value);
}

static void user_gpio_pen_sdio_intr_func_read(uint32 * gpio_pen_sdio_data)
{
	int i = 0;
	uint32 sdio_data = 0;
	uint16 gpio_pin_mask = pGPIOConfig_gpio_pen_sdio->GPIO_Pin;

	user_gpio_pen_set_sck(SCK_LEVEL_HIGH);	//sck high 
	os_delay_us(10);			//delay 10 us
	GPIO_AS_OUTPUT(gpio_pin_mask);		//set gpio output
	user_gpio_pen_set_sdio(SDIO_LEVEL_LOW);	//write gpio low, read control
	os_delay_us(5);				//delay 5 us
	user_gpio_pen_set_sck(SCK_LEVEL_LOW);	//sck low
	os_delay_us(5);				//delay 5 us
	user_gpio_pen_set_sck(SCK_LEVEL_HIGH);	//sck high 
	os_delay_us(5);				//delay 5 us
	GPIO_AS_INPUT(gpio_pin_mask);		//set gpio input
	os_delay_us(5);				//delay 5 us
	while(i < 23)	//start, bit R/W, bit22, bit21, ... , bit0, stop
	{
		sdio_data <<= 1;	
		user_gpio_pen_set_sck(SCK_LEVEL_LOW);	//sck low
		os_delay_us(5);				//delay 5 us
		sdio_data |= GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_PEN_SDIO_IO_NUM));
		os_delay_us(5);				//delay 5 us
		user_gpio_pen_set_sck(SCK_LEVEL_HIGH);	//sck high 
		os_delay_us(10);				//delay 10 us

		i++;
		printf("read sdio_data fun:%s, line:%d, sdio_data:%d\n", __FUNCTION__, __LINE__, (sdio_data&0x01));
	}
	*gpio_pen_sdio_data = sdio_data;
	user_gpio_pen_set_sdio(SDIO_LEVEL_HIGH);	//write gpio high
	user_gpio_pen_set_sck(SCK_LEVEL_LOW);	//sck low
	os_delay_us(64);				//delay 64 us
	GPIO_AS_INPUT(gpio_pin_mask);		//set gpio input
}

int tmp_i = 0;
static void user_gpio_pen_sdio_intr_func(void)
{
	printf("a>>>>>>>user_gpio_pen_sdio_intr_func >>>> tmp_i:%d\n", tmp_i);
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	printf("b>>>>>>>user_gpio_pen_sdio_intr_func >>>> tmp_i:%d\n", tmp_i);
	if(gpio_status & BIT(GPIO_PEN_SDIO_IO_NUM))
	{
		//disable this gpio pin interrupt
		gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_PEN_SDIO_IO_NUM), GPIO_PIN_INTR_DISABLE);
		//clear interrupt status
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(GPIO_PEN_SDIO_IO_NUM));

		uint32 gpio_pen_sdio_value = 0;
		user_gpio_pen_sdio_intr_func_read(&gpio_pen_sdio_value);

		printf("user_gpio_pen_sdio_intr_func >>>> tmp_i:%d, gpio_pen_sdio_value:0x%x <=> %d\n",
				tmp_i, gpio_pen_sdio_value, gpio_pen_sdio_value);
		tmp_i++;
		//enable this gpio pin interrupt
		gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_PEN_SDIO_IO_NUM), GPIO_PIN_INTR_LOLEVEL);

	}
}

static void user_gpio_pen_sdio_intr_init(void)
{
	printf("LINE:%d\n", __LINE__);
	pGPIOConfig_gpio_pen_sdio = (GPIO_ConfigTypeDef*)zalloc(sizeof(GPIO_ConfigTypeDef));
	gpio_intr_handler_register(user_gpio_pen_sdio_intr_func, NULL);

	printf("LINE:%d\n", __LINE__);
	pGPIOConfig_gpio_pen_sdio->GPIO_IntrType = GPIO_PIN_INTR_LOLEVEL;
	pGPIOConfig_gpio_pen_sdio->GPIO_Pullup = GPIO_PullUp_EN;
	pGPIOConfig_gpio_pen_sdio->GPIO_Mode = GPIO_Mode_Input;
	pGPIOConfig_gpio_pen_sdio->GPIO_Pin = (1 << GPIO_PEN_SDIO_IO_NUM);
	gpio_config(pGPIOConfig_gpio_pen_sdio);

	_xt_isr_unmask(1<<ETS_GPIO_INUM);
	printf("LINE:%d\n", __LINE__);
	
}

static void user_gpio_pen_sdio_init(void)
{
	PIN_FUNC_SELECT(GPIO_PEN_SDIO_IO_MUX, GPIO_PEN_SDIO_IO_FUNC);
	user_gpio_pen_sdio_intr_init();
	printf("LINE:%d\n", __LINE__);
}

static void default_pen_wifi_config(void)
{
	struct station_config * wifi_config = (struct station_config *)zalloc(sizeof(struct station_config));
	sprintf(wifi_config->ssid, DEFAULT_PEN_AP_SSID);
	sprintf(wifi_config->password, DEFAULT_PEN_AP_PASSWD);
	wifi_station_set_config(wifi_config);
	free(wifi_config);
	wifi_station_connect();
}

void gpio_pen_task(void * arg)
{
	uint8 gpio_pen_sdio_bit = 0;
	printf("task start fun:%s, line:%d\n", __FUNCTION__, __LINE__);
	user_gpio_pen_sck_init();
	user_gpio_pen_sdio_init();
	printf("task end fun:%s, line:%d\n", __FUNCTION__, __LINE__);

	default_pen_wifi_config();

	uint32 j = 0;
	while(1)
	{
//		gpio_pen_sdio_bit = GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_PEN_SDIO_IO_NUM));
		printf("read gpio_pen_sdio_bit fun:%s, line:%d, gpio_pen_sdio_bit:%d\n",
				__FUNCTION__, __LINE__, gpio_pen_sdio_bit);
		j = 5000;
		while(j--)
		{
			os_delay_us(1000);
		}
		
	}

	while(0)
	{
    		gpio_pen_sck_level = (~gpio_pen_sck_level) & 0x01;
    		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_PEN_SCK_IO_NUM), gpio_pen_sck_level);
		os_delay_us(20);
	}
}
