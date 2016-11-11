/*************************************************************************
 *   > File Name: user_ast.c
 *   > Author: heenbo
 *   > Mail: 379667345@qq.com 
 *   > Created Time:  2016年11月10日 星期四 17时36分35秒
 *   > Modified Time: 2016年11月11日 星期五 15时34分48秒
 ************************************************************************/

#include "esp_common.h"
#include "gpio.h"
#include "ets_sys.h"

#define GPIO_PEN_SCK_IO_MUX	PERIPHS_IO_MUX_GPIO4_U
#define GPIO_PEN_SCK_IO_NUM	4
#define GPIO_PEN_SCK_IO_FUNC	FUNC_GPIO4

#if 1
#define GPIO_PEN_DATA_IO_MUX	PERIPHS_IO_MUX_GPIO5_U
#define GPIO_PEN_DATA_IO_NUM	5
#define GPIO_PEN_DATA_IO_FUNC	FUNC_GPIO5
#else
#define GPIO_PEN_DATA_IO_MUX	PERIPHS_IO_MUX_GPIO4_U
#define GPIO_PEN_DATA_IO_NUM	4
#define GPIO_PEN_DATA_IO_FUNC	FUNC_GPIO4
#endif

LOCAL uint8 gpio_pen_sck_level = 0;

LOCAL void ICACHE_FLASH_ATTR
user_gpio_pen_sck_init(void)
{
    PIN_FUNC_SELECT(GPIO_PEN_SCK_IO_MUX, GPIO_PEN_SCK_IO_FUNC);
}

int tmp_i = 0;
LOCAL void ICACHE_FLASH_ATTR
user_gpio_pen_data_intr_func(void)
{
	printf("a>>>>>>>user_gpio_pen_data_intr_func >>>> tmp_i:%d\n", tmp_i);
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	printf("b>>>>>>>user_gpio_pen_data_intr_func >>>> tmp_i:%d\n", tmp_i);
	if(gpio_status & BIT(GPIO_PEN_DATA_IO_NUM))
	{
		//disable this gpio pin interrupt
		gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_PEN_DATA_IO_NUM), GPIO_PIN_INTR_DISABLE);
		//clear interrupt status
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(GPIO_PEN_DATA_IO_NUM));

		printf("user_gpio_pen_data_intr_func >>>> tmp_i:%d\n", tmp_i);
		tmp_i++;

		//enable this gpio pin interrupt
		gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_PEN_DATA_IO_NUM), GPIO_PIN_INTR_NEGEDGE);

	}
}

LOCAL void ICACHE_FLASH_ATTR
user_gpio_pen_data_intr_init(void)
{
	printf("LINE:%d\n", __LINE__);
	GPIO_ConfigTypeDef *pGPIOConfig;
	pGPIOConfig = (GPIO_ConfigTypeDef*)zalloc(sizeof(GPIO_ConfigTypeDef));
	gpio_intr_handler_register(user_gpio_pen_data_intr_func, NULL);

	printf("LINE:%d\n", __LINE__);
	//pGPIOConfig->GPIO_IntrType = GPIO_PIN_INTR_HILEVEL;
	pGPIOConfig->GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
	pGPIOConfig->GPIO_Pullup = GPIO_PullUp_EN;
	pGPIOConfig->GPIO_Mode = GPIO_Mode_Input;
	pGPIOConfig->GPIO_Pin = (1 << GPIO_PEN_DATA_IO_NUM);
	gpio_config(pGPIOConfig);

	_xt_isr_unmask(1<<ETS_GPIO_INUM);
	printf("LINE:%d\n", __LINE__);
	
#if 0
	ETS_GPIO_INTR_DISABLE();

	ETS_GPIO_INTR_ATTACH(user_gpio_pen_data_intr_func, NULL);
	PIN_FUNC_SELECT(GPIO_PEN_DATA_IO_MUX, GPIO_PEN_DATA_IO_FUNC);
	gpio_output_set(0, 0, 0, GPIO_ID_PIN(GPIO_PEN_DATA_IO_NUM));
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(GPIO_PEN_DATA_IO_NUM));
	gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_PEN_DATA_IO_NUM), GPIO_PIN_INTR_HILEVEL);

	ETS_GPIO_INTR_ENABLE();
#endif
}

LOCAL void ICACHE_FLASH_ATTR
user_gpio_pen_data_init(void)
{
	PIN_FUNC_SELECT(GPIO_PEN_DATA_IO_MUX, GPIO_PEN_DATA_IO_FUNC);
	user_gpio_pen_data_intr_init();
	printf("LINE:%d\n", __LINE__);
}

void gpio_pen_task(void * arg)
{
	uint8 gpio_pen_data_bit = 0;
	printf("task start fun:%s, line:%d\n", __FUNCTION__, __LINE__);
//	user_gpio_pen_sck_init();
	user_gpio_pen_data_init();
	printf("task end fun:%s, line:%d\n", __FUNCTION__, __LINE__);

	uint32 j = 0;
	while(1)
	{
//		gpio_pen_data_bit = GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_PEN_DATA_IO_NUM));
		printf("read gpio_pen_data_bit fun:%s, line:%d, gpio_pen_data_bit:%d\n",
				__FUNCTION__, __LINE__, gpio_pen_data_bit);
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
