/*************************************************************************
 *   > File Name: user_ast.c
 *   > Author: heenbo
 *   > Mail: 379667345@qq.com 
 *   > Created Time:  2016年11月10日 星期四 17时36分35秒
 *   > Modified Time: 2016年11月10日 星期四 18时48分02秒
 ************************************************************************/

#include "esp_common.h"
#include "gpio.h"

#define GPIO_SCK_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define GPIO_SCK_IO_NUM     4
#define GPIO_SCK_IO_FUNC    FUNC_GPIO4

void gpio_sck_task(void * arg)
{
	int i = 0;
	int j = 0;
	while(1)
	{
		printf(">>>>>SW version:%s %s, i = %d\n", __DATE__, __TIME__, i);
		j = 100;
		while(j--)
		{
			os_delay_us(10*1000);
		}
		i++;
	}
	
}

LOCAL uint8 sck2_level = 0;
LOCAL os_timer_t sck2_timer;

LOCAL void ICACHE_FLASH_ATTR
user_sck2_init(void)
{
    PIN_FUNC_SELECT(GPIO_SCK_IO_MUX, GPIO_SCK_IO_FUNC);
}

void ICACHE_FLASH_ATTR
user_sck2_output(uint8 level)
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_SCK_IO_NUM), level);
}

LOCAL void ICACHE_FLASH_ATTR
user_sck2_timer_cb(void)
{
    sck2_level = (~sck2_level) & 0x01;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_SCK_IO_NUM), sck2_level);
}

void ICACHE_FLASH_ATTR
user_sck2_timer_init(void)
{
#if 0 //os_timer
    os_timer_disarm(&sck2_timer);
    os_timer_setfn(&sck2_timer, (os_timer_func_t *)user_sck2_timer_cb, NULL);
    os_timer_arm(&sck2_timer, 50, 1);
    //os_timer_arm(&sck2_timer, 2000, 1);
#else
    //hw_timer_init(FRC1_SOURCE, 1);
    hw_timer_init(0, 1);
    hw_timer_set_func(user_sck2_timer_cb);
    hw_timer_arm(5*1000);
#endif
    sck2_level = 0;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_SCK_IO_NUM), sck2_level);
}

void ICACHE_FLASH_ATTR
user_sck2_timer_done(void)
{
    os_timer_disarm(&sck2_timer);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_SCK_IO_NUM), 0);
}


void gpio_sck2_task(void * arg)
{
	printf("task start fun:%s, line:%d\n", __FUNCTION__, __LINE__);
	user_sck2_init();
//	user_sck2_timer_init();
	printf("task end fun:%s, line:%d\n", __FUNCTION__, __LINE__);

	int i = 0;
	int j = 0;
#if 0
	while(1)
	{
		printf(">>>>>SW version:%s %s, i = %d, sck2_level = %d \n", __DATE__, __TIME__, i, sck2_level);
		j = 200;
		do
		{
			os_delay_us(10*1000);
			//os_delay_us(1*1000);
		}while(j--);
		i++;

	}
#endif
	while(1)
	{
			
    		sck2_level = (~sck2_level) & 0x01;
    		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_SCK_IO_NUM), sck2_level);
		os_delay_us(20);
	}
}
