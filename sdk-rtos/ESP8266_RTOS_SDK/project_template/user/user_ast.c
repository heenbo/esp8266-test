/*************************************************************************
 *   > File Name: user_ast.c
 *   > Author: heenbo
 *   > Mail: 379667345@qq.com 
 *   > Created Time:  2016年11月10日 星期四 17时36分35秒
 *   > Modified Time: 2016年11月14日 星期一 10时33分09秒
 ************************************************************************/

#include "esp_common.h"
#include "gpio.h"
#include "ets_sys.h"
#include "lwip/sockets.h"
#include "espconn.h"

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
#define DEFAULT_SERVER_UDP_PORT 16161
#define DEFAULT_SERVER_UDP_IP	"192.168.199.235"
static user_udp_client_init_flag = 0;
static user_udp_client_code_flag = 0;
static struct espconn ptrespconn;

static int32 sock_fd;
static struct sockaddr_in server_addr;

static uint8 gpio_pen_sck_level = 0;
static GPIO_ConfigTypeDef * pGPIOConfig_gpio_pen_sdio = NULL;

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
		//printf("read sdio_data fun:%s, line:%d, sdio_data:%d\n", __FUNCTION__, __LINE__, (sdio_data&0x01));
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
	uint8 pen_code[20] = {0};

//	printf("b>>>>>>>user_gpio_pen_sdio_intr_func >>>> tmp_i:%d\n", tmp_i);
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

		sprintf(pen_code, "pencode: %d\n", gpio_pen_sdio_value);
		//sendto(sock_fd,(uint8*)pen_code, strlen(pen_code), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
		if(0x60fff8 == gpio_pen_sdio_value)
		{
		//	user_udp_client_init();
			user_udp_client_init_flag = 1;
		}
		else
		{
			user_udp_client_code_flag = 1;
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
	uint32 remote_ip = 0;

	remote_ip = ipaddr_addr(DEFAULT_SERVER_UDP_IP);
	ptrespconn.type = ESPCONN_UDP;
	ptrespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	ptrespconn.proto.udp->remote_port = DEFAULT_SERVER_UDP_PORT;
	memcpy(ptrespconn.proto.udp->remote_ip, &remote_ip, sizeof(remote_ip));
//	espconn_regist_recvcb(&ptrespconn, user_devicefind_recv);
//	espconn_regist_sentcb(&ptrespconn, pen_code_send_cb);
	espconn_create(&ptrespconn);

	printf("ESP8266 UDP task > user_udp_client_init OK!\n");
}

void gpio_pen_task(void * arg)
{
	struct sockaddr_in from_addr;
	memset(&from_addr, 0, sizeof(from_addr));
	int from_len = sizeof(struct sockaddr_in);
	int ret = 0;

	uint8 gpio_pen_sdio_bit = 0;
	printf("task start fun:%s, line:%d\n", __FUNCTION__, __LINE__);
	//pen sck gpio init
	user_gpio_pen_sck_init();
	//pen sdio gpio init
	user_gpio_pen_sdio_init();

	//wifi config & connect
	user_default_pen_wifi_config();

//	user_udp_client_init();

	uint8 udp_msg[50] = "hahahahahah aaa\n\0";

	uint32 j = 0;
	while(1)
	{
//		gpio_pen_sdio_bit = GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_PEN_SDIO_IO_NUM));
		printf("read gpio_pen_sdio_bit fun:%s, line:%d, gpio_pen_sdio_bit:%d\n",
				__FUNCTION__, __LINE__, gpio_pen_sdio_bit);
		j = 4000;
		while(j--)
		{
			
			if(1 == user_udp_client_init_flag)
			{
				user_udp_client_init_flag = 0;
				user_udp_client_init();
			}

			if(1 == user_udp_client_code_flag)
			{
				user_udp_client_code_flag = 0;
				sprintf(udp_msg, "%s\n", "hafdsafsdafsdafsdafesp\n\0");
				printf("line: %d, user_udp_client_code_flag: %d\n", __LINE__, user_udp_client_code_flag);
//				ret = sendto(sock_fd, (uint8 *)udp_msg, strlen(udp_msg), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
				ret = espconn_send(&ptrespconn, udp_msg, strlen(udp_msg));
				if(0 != ret)
				{
					printf("LINE:%d, sendto error: ret = %d\n", __LINE__, ret);
					perror ("socket ret\n");
				}

			}
			os_delay_us(1000);
		}

		sprintf(udp_msg, "%s\n", "hafdsafsdafsdafsdafesp");
//		recvfrom(sock_fd, (uint8 *)udp_msg, 100, 0,(struct sockaddr *)&from_addr,(socklen_t *)&fromlen);
		sendto(sock_fd, (uint8 *)udp_msg, strlen(udp_msg), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
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
