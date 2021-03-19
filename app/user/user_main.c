/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

// ͷ�ļ�
//==============================
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"
//==============================
#include "dht11.h"
#include "json_tree.h"
#include "kmp.h"
#include "get_time.h"
#include "DS1302.h"

// ���Ͷ���
//=================================
typedef unsigned long 		u32_t;
uint8 jishi = 0;//mqtt�ϱ���ʱ����ʱ
//=================================

// ȫ�ֱ���
MQTT_Client mqttClient;			// MQTT�ͻ���_�ṹ�塾�˱����ǳ���Ҫ��
static ETSTimer sntp_timer;		// SNTP��ʱ��
os_timer_t OS_Timer_1;		// �����ʱ��
// SNTP��ʱ��������ȡ��ǰ����ʱ��
void sntpfn()
{
    u32_t ts = 0;
    ts = sntp_get_current_timestamp();		// ��ȡ��ǰ��ƫ��ʱ��
    os_printf("current time : %s\n", sntp_get_real_time(ts));	// ��ȡ��ʵʱ��
    if (ts == 0)		// ����ʱ���ȡʧ��
    {
        os_printf("����ʱ���ȡʧ��\n");
    }
    else //(ts != 0)	// ����ʱ���ȡ�ɹ�
    {
        os_timer_disarm(&sntp_timer);	// �ر�SNTP��ʱ��
        ds1302_timer_init();
        MQTT_Connect(&mqttClient);		// ��ʼMQTT����
    }
}

// WIFI����״̬�ı䣺���� = wifiStatus
void wifiConnectCb(uint8_t status)
{
	// �ɹ���ȡ��IP��ַ
    if(status == STATION_GOT_IP)
    {
    	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));
    	// �ڹٷ����̵Ļ����ϣ�����2�����÷�����
    	sntp_setservername(0, "us.pool.ntp.org");	// ������_0��������
    	sntp_setservername(1, "ntp.sjtu.edu.cn");	// ������_1��������
    	ipaddr_aton("210.72.145.44", addr);	// ���ʮ���� => 32λ������
    	sntp_setserver(2, addr);					// ������_2��IP��ַ��
    	os_free(addr);								// �ͷ�addr
    	sntp_init();	// SNTP��ʼ��
        // ����SNTP��ʱ��[sntp_timer]
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 1000, 1);		// 1s��ʱ
    }

    // IP��ַ��ȡʧ��
    else
    {
          MQTT_Disconnect(&mqttClient);	// WIFI���ӳ���TCP�Ͽ�����
    }
}

// MQTT�ѳɹ����ӣ�ESP8266���͡�CONNECT���������յ���CONNACK��
void mqttConnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡmqttClientָ��
//    INFO("MQTT: Connected\r\n");
    // ������2����������� / ����3������Qos��
	MQTT_Subscribe(client, "test", 0);	// ��������"SW_LED"��QoS=0
	MQTT_Subscribe(client, "send_test", 0);	// ��������"SW_LED"��QoS=0
	// ������2�������� / ����3��������Ϣ����Ч�غ� / ����4����Ч�غɳ��� / ����5������Qos / ����6��Retain��
	MQTT_Publish(client, "test", "ESP8266_Online", strlen("ESP8266_Online"), 0, 0);	// ������"SW_LED"����"ESP8266_Online"��Qos=0��retain=0
}

// MQTT�ɹ��Ͽ�����
void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Disconnected\r\n");
}

// MQTT�ɹ�������Ϣ
void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}
//============================================================================
void mqtt_ctrl_success(){
	uint32_t icid = system_get_chip_id();
	char data[100] = {0};
	os_sprintf(data,JSON_Ctrl,icid,"success");
	MQTT_Publish(&mqttClient, "response_test", data, strlen(data), 0, 0);
	 os_free(data);
}
// ������MQTT��[PUBLISH]���ݡ�����		������1������ / ����2�����ⳤ�� / ����3����Ч�غ� / ����4����Ч�غɳ��ȡ�
//===============================================================================================================
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1);		// ���롾���⡿�ռ�
    char *dataBuf  = (char*)os_zalloc(data_len+1);		// ���롾��Ч�غɡ��ռ�
    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡMQTT_Clientָ��
    os_memcpy(topicBuf, topic, topic_len);	// ��������
    topicBuf[topic_len] = 0;				// �����'\0'
    os_memcpy(dataBuf, data, data_len);		// ������Ч�غ�
    dataBuf[data_len] = 0;					// �����'\0'
    INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);	// ���ڴ�ӡ�����⡿����Ч�غɡ�
    INFO("Topic_len = %d, Data_len = %d\r\n", topic_len, data_len);	// ���ڴ�ӡ�����ⳤ�ȡ�����Ч�غɳ��ȡ�



//########################################################################################
    // ���ݽ��յ���������/��Ч�غɣ�����LED����/��
    //-----------------------------------------------------------------------------------
    if( os_strcmp(topicBuf,"send_test") == 0 )			// ���� == "SW_LED"
    {
    	os_printf("����������Ϣ����\n");
    	uint32_t icid = system_get_chip_id();
    	char data2[20];
    	os_sprintf(data2,"%d",icid);
    	if(KMP_start(dataBuf,data2) == 1){
    		os_free(data2);
    		if(KMP_start(dataBuf,"on") == 1){
    			os_printf("�豸����\n");
    			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED��
    			mqtt_ctrl_success();
    		}
    		else{
    			os_printf("�豸�ر�\n");
    			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);			// LED��
    			mqtt_ctrl_success();
    		}
    	}


    }
//########################################################################################
    os_free(topicBuf);	// �ͷš����⡿�ռ�
    os_free(dataBuf);	// �ͷš���Ч�غɡ��ռ�
}
//===============================================================================================================

//��ʱһ�����ϱ�һ������
void ICACHE_FLASH_ATTR OS_Timer_1_mqtt()
{
	jishi++;
	if(jishi == 12){
		jishi = 0;
		char temp[10];
			char humi[10];
			char data[100];
			if(DHT11_Read_Data_Complete() == 0)		// ��ȡDHT11��ʪ��ֵ
			{
				//-------------------------------------------------
				// DHT11_Data_Array[0] == ʪ��_����_����
				// DHT11_Data_Array[1] == ʪ��_С��_����
				// DHT11_Data_Array[2] == �¶�_����_����
				// DHT11_Data_Array[3] == �¶�_С��_����
				// DHT11_Data_Array[4] == У���ֽ�
				// DHT11_Data_Array[5] == ��1:�¶�>=0����0:�¶�<0��
				if(DHT11_Data_Array[5] == 1)			// �¶� >= 0��
				{
		//			os_printf("\r\nʪ�� == %d.%d %RH\r\n",DHT11_Data_Array[0],DHT11_Data_Array[1]);
		//			os_printf("\r\n�¶� == %d.%d ��\r\n", DHT11_Data_Array[2],DHT11_Data_Array[3]);
					os_sprintf(temp,"%d.%d",DHT11_Data_Array[2],DHT11_Data_Array[3]);
					os_sprintf(humi,"%d.%d",DHT11_Data_Array[0],DHT11_Data_Array[1]);
				}
				else // if(DHT11_Data_Array[5] == 0)	// �¶� < 0��
				{
		//			os_printf("\r\nʪ�� == %d.%d %RH\r\n",DHT11_Data_Array[0],DHT11_Data_Array[1]);
		//			os_printf("\r\n�¶� == -%d.%d ��\r\n",DHT11_Data_Array[2],DHT11_Data_Array[3]);
					os_sprintf(temp,"%d.%d",DHT11_Data_Array[2],DHT11_Data_Array[3]);
					os_sprintf(humi,"-%d.%d",DHT11_Data_Array[0],DHT11_Data_Array[1]);
				}
		//		// OLED��ʾ��ʪ��
		//		//---------------------------------------------------------------------------------
				DHT11_NUM_Char();	// DHT11����ֵת���ַ���
			}
			uint32_t icid = system_get_chip_id();
			os_sprintf(data,JSON_Tree,icid,temp,humi);
			MQTT_Publish(&mqttClient, "test", data, strlen(data), 0, 0);
	}
}


// �����ʱ����ʼ��(ms����)
//==========================================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_Init_mqtt(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_1);	// �رն�ʱ��
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_mqtt, NULL);	// ���ö�ʱ��
	os_timer_arm(&OS_Timer_1, time_ms, time_repetitive);  // ʹ�ܶ�ʱ��
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

// user_init��entry of user application, init user function here
//===================================================================================================================
void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);	// ���ڲ�������Ϊ115200
    os_delay_us(60000);
    DS1302_master_gpio_init();

//����С�¡����
//###########################################################################
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4�����	#
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);						// LED��ʼ��	#
//###########################################################################


    CFG_Load();	// ����/����ϵͳ������WIFI������MQTT������
    // �������Ӳ�����ֵ�������������mqtt_test_jx.mqtt.iot.gz.baidubce.com�����������Ӷ˿ڡ�1883������ȫ���͡�0��NO_TLS��
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	// MQTT���Ӳ�����ֵ���ͻ��˱�ʶ����..����MQTT�û�����..����MQTT��Կ��..������������ʱ����120s��������Ự��1��clean_session��
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	// ������������(����ƶ�û�ж�Ӧ���������⣬��MQTT���ӻᱻ�ܾ�)
//	MQTT_InitLWT(&mqttClient, "Will", "ESP8266_offline", 0, 0);
	// ����MQTT��غ���
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);			// ���á�MQTT�ɹ����ӡ���������һ�ֵ��÷�ʽ
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);	// ���á�MQTT�ɹ��Ͽ�����������һ�ֵ��÷�ʽ
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);			// ���á�MQTT�ɹ���������������һ�ֵ��÷�ʽ
	MQTT_OnData(&mqttClient, mqttDataCb);					// ���á�����MQTT���ݡ���������һ�ֵ��÷�ʽ


	// ����WIFI��SSID[..]��PASSWORD[..]��WIFI���ӳɹ�����[wifiConnectCb]
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);
//	INFO("\r\nSystem started ...\r\n");
	second_timer_init();
	OS_Timer_1_Init_mqtt(5000,1);		// 5�붨ʱ(�ظ�)
}
//===================================================================================================================
