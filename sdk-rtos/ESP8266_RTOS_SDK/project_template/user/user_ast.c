/*************************************************************************
 *   > File Name: user_ast.c
 *   > Author: heenbo
 *   > Mail: 379667345@qq.com 
 *   > Created Time:  2016年11月10日 星期四 17时36分35秒
 *   > Modified Time: 2016年11月10日 星期四 18时48分02秒
 ************************************************************************/

#include "esp_common.h"
#include "gpio.h"

#define GPIO_PEN_SCK_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define GPIO_PEN_SCK_IO_NUM     4
#define GPIO_PEN_SCK_IO_FUNC    FUNC_GPIO4

LOCAL uint8 gpio_pen_sck_level = 0;

LOCAL void ICACHE_FLASH_ATTR
user_gpio_pen_sck_init(void)
{
    PIN_FUNC_SELECT(GPIO_PEN_SCK_IO_MUX, GPIO_PEN_SCK_IO_FUNC);
}

void gpio_pen_task(void * arg)
{
	printf("task start fun:%s, line:%d\n", __FUNCTION__, __LINE__);
	user_gpio_pen_sck_init();
	printf("task end fun:%s, line:%d\n", __FUNCTION__, __LINE__);

	while(1)
	{
			
    		gpio_pen_sck_level = (~gpio_pen_sck_level) & 0x01;
    		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_PEN_SCK_IO_NUM), gpio_pen_sck_level);
		os_delay_us(20);
	}
}
