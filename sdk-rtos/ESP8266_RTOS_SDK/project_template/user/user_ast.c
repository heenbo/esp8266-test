/*************************************************************************
 *   > File Name: user_ast.c
 *   > Author: heenbo
 *   > Mail: 379667345@qq.com 
 *   > Created Time:  2016年11月10日 星期四 17时36分35秒
 *   > Modified Time: 2016年11月10日 星期四 18时00分52秒
 ************************************************************************/

#include "esp_common.h"

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
