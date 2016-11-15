/*************************************************************************
 *   > File Name: user_ast.c
 *   > Author: heenbo
 *   > Mail: 379667345@qq.com 
 *   > Created Time:  2016年11月10日 星期四 17时36分35秒
 *   > Modified Time: 2016年11月14日 星期一 21时00分22秒
 ************************************************************************/

#include "esp_common.h"

#include "gpio.h"
#include "ets_sys.h"
#include "lwip/sockets.h"
#include "espressif/espconn.h"

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

//udp client
#define DEFAULT_SERVER_UDP_PORT 12121
#define DEFAULT_SERVER_UDP_IP	"255.255.255.255"
//#define DEFAULT_SERVER_UDP_IP	"192.168.199.235"
static uint8 user_udp_client_init_flag = 0;
static uint8 user_udp_client_code_flag = 0;
static uint8 user_udp_client_uninit_flag = 0;
static esp_udp remote_udp;
static uint32 remote_ip = 0;
static struct espconn ptrespconn;

static int32 sock_fd;
static struct sockaddr_in server_addr;

static uint8 gpio_pen_sck_level = 0;
static GPIO_ConfigTypeDef * pGPIOConfig_gpio_pen_sdio = NULL;
static uint32 gpio_pen_sdio_value = 0;

static void user_udp_client_init(void);

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
//	printf("a>>>>>>>user_gpio_pen_sdio_intr_func >>>> tmp_i:%d\n", tmp_i);
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

//	printf("b>>>>>>>user_gpio_pen_sdio_intr_func >>>> tmp_i:%d\n", tmp_i);
	if(gpio_status & BIT(GPIO_PEN_SDIO_IO_NUM))
	{
		//disable this gpio pin interrupt
		gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_PEN_SDIO_IO_NUM), GPIO_PIN_INTR_DISABLE);
		//clear interrupt status
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(GPIO_PEN_SDIO_IO_NUM));

		user_gpio_pen_sdio_intr_func_read(&gpio_pen_sdio_value);

		printf("user_gpio_pen_sdio_intr_func >>>> tmp_i:%d, gpio_pen_sdio_value:0x%x <=> %d\n",
				tmp_i, gpio_pen_sdio_value, gpio_pen_sdio_value);

		switch(gpio_pen_sdio_value)
		{
			case 0x60fff8:
				user_udp_client_init_flag = 1;
				break;
			case 0x60fff7:
				user_udp_client_uninit_flag = 1;
				break;
			default:
				user_udp_client_code_flag = 1;
				break;
		}

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

static void user_default_pen_wifi_config(void)
{
	struct station_config * wifi_config = (struct station_config *)zalloc(sizeof(struct station_config));
	sprintf(wifi_config->ssid, DEFAULT_PEN_AP_SSID);
	sprintf(wifi_config->password, DEFAULT_PEN_AP_PASSWD);
	wifi_station_set_config(wifi_config);
	free(wifi_config);
	wifi_station_connect();
}

static void user_udp_client_init(void)
{
	ptrespconn.type = ESPCONN_UDP;
	remote_udp.remote_port= DEFAULT_SERVER_UDP_PORT;
	remote_udp.remote_ip[0] = 255;
	remote_udp.remote_ip[1] = 255;
	remote_udp.remote_ip[2] = 255;
	remote_udp.remote_ip[3] = 255;
	ptrespconn.proto.udp = &(remote_udp);
	espconn_create(&ptrespconn);

	printf("ESP8266 UDP task > user_udp_client_init OK!\n");
}

static void user_udp_client_uninit(void)
{
	espconn_delete(&ptrespconn);
	printf("ESP8266 UDP task > user_udp_client_uninit OK!\n");
}

void gpio_pen_task(void * arg)
{
	struct sockaddr_in from_addr;
	memset(&from_addr, 0, sizeof(from_addr));
	int from_len = sizeof(struct sockaddr_in);
	int ret = 0;

	printf("task start fun:%s, line:%d\n", __FUNCTION__, __LINE__);
	//pen sck gpio init
	user_gpio_pen_sck_init();
	//pen sdio gpio init
	user_gpio_pen_sdio_init();

	//wifi config & connect
	user_default_pen_wifi_config();

	uint8 udp_msg[50] = "hahahahahah aaa\n\0";

	uint32 j = 0;
	while(1)
	{
		printf("read gpio_pen_sdio_bit fun:%s, line:%d >>>>>>>>>>>>>>>>\n",
				__FUNCTION__, __LINE__);
		j = 5000;
		while(j--)
		{
			
			if(1 == user_udp_client_init_flag)
			{
				user_udp_client_init_flag = 0;
				user_udp_client_init();
			}

			if(1 == user_udp_client_uninit_flag)
			{
				user_udp_client_uninit_flag = 0;
				user_udp_client_uninit();
			}

			if(1 == user_udp_client_code_flag)
			{
				user_udp_client_code_flag = 0;
				sprintf(udp_msg, "code:%d", gpio_pen_sdio_value);
				ptrespconn.type = ESPCONN_UDP;
				remote_udp.remote_port= DEFAULT_SERVER_UDP_PORT;
				remote_udp.remote_ip[0] = 255;
				remote_udp.remote_ip[1] = 255;
				remote_udp.remote_ip[2] = 255;
				remote_udp.remote_ip[3] = 255;
				//printf("send udp if i is:%d\n", wifi_get_broadcast_if());
				printf("ip:%u, port:%u, type:0x%x\n", ptrespconn.proto.udp->remote_ip, 
						ptrespconn.proto.udp->remote_port, ptrespconn.type);
				printf("ip[0]:%d", (ptrespconn.proto.udp->remote_ip[0])&0xff);
				printf("ip[1]:%d", (ptrespconn.proto.udp->remote_ip[1])&0xff);
				printf("ip[2]:%d", (ptrespconn.proto.udp->remote_ip[2])&0xff);
				printf("ip[3]:%d\n", (ptrespconn.proto.udp->remote_ip[3])&0xff);
				ret = espconn_sendto(&ptrespconn, udp_msg, strlen(udp_msg));
				printf("ip:%u, port:%u, type:0x%x\n", ptrespconn.proto.udp->remote_ip, 
						ptrespconn.proto.udp->remote_port, ptrespconn.type);
				printf("ip[0]:%d", (ptrespconn.proto.udp->remote_ip[0])&0xff);
				printf("ip[1]:%d", (ptrespconn.proto.udp->remote_ip[1])&0xff);
				printf("ip[2]:%d", (ptrespconn.proto.udp->remote_ip[2])&0xff);
				printf("ip[3]:%d\n", (ptrespconn.proto.udp->remote_ip[3])&0xff);
				if(0 != ret)
				{
					printf("LINE:%d, sendto error: ret = %d\n", __LINE__, ret);
					perror ("socket ret\n");
				}
				else
				{
					printf("LINE:%d, sendto success, udp_msg:%s, len:%d", __LINE__, udp_msg, strlen(udp_msg));
				}

			}
			os_delay_us(1000);
		}

		sprintf(udp_msg, "%s\n", "hafdsafsdafsdafsdafesp");
	}

	while(0)
	{
    		gpio_pen_sck_level = (~gpio_pen_sck_level) & 0x01;
    		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_PEN_SCK_IO_NUM), gpio_pen_sck_level);
		os_delay_us(20);
	}
	printf("task end fun:%s, line:%d\n", __FUNCTION__, __LINE__);
	//task free
	vTaskDelete(NULL);
}
