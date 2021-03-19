#include "sntp.h"
#include "get_time.h"
#include "os_type.h"			// os_XXX
#include "osapi.h"	           // os_XXX�������ʱ��
#include "ets_sys.h"
#include "ip_addr.h"			// ��"espconn.h"ʹ�á�
#include "c_types.h"			// ��������
//īˮ��
//#include "spi.h"
//#include "epd2in9.h"
//#include "epdif.h"
//#include "epdpaint.h"
//#include "imagedata.h"
//#include "mem.h"

//ds1302
#include "DS1302.h"
#include "xSntp.h"


#define	STATION_IF 0x00
#define	SOFTAP_IF 0x01
typedef unsigned long 		u32_t;
#define COLORED      0
#define UNCOLORED    1
unsigned char *frame_buffer;


u32_t ts;
uint16 i=0;

sntp_data data;




LOCAL os_timer_t second_timer, getNetSyncTimer;
LOCAL unsigned char time_buf1[8] = { 20, 19, 3, 29, 16, 20, 00, 6 }; //20����3��13��18ʱ50��00��6��
LOCAL unsigned char time_buf[8]; //��������ʱ������
LOCAL void ICACHE_FLASH_ATTR second_timer_Callback(void) {

    unsigned char i, tmp;
	time_buf[1] = DS1302_master_readByte(ds1302_year_add);  //��
	time_buf[2] = DS1302_master_readByte(ds1302_month_add);  //��
	time_buf[3] = DS1302_master_readByte(ds1302_date_add);  //��
	time_buf[4] = DS1302_master_readByte(ds1302_hr_add);  //ʱ
	time_buf[5] = DS1302_master_readByte(ds1302_min_add);  //��
	time_buf[6] = (DS1302_master_readByte(ds1302_sec_add)) & 0x7F;  //��
	time_buf[7] = DS1302_master_readByte(ds1302_day_add);  //��
	for (i = 0; i < 8; i++) {           //BCD����
		tmp = time_buf[i] / 16;
		time_buf1[i] = time_buf[i] % 16;
		time_buf1[i] = time_buf1[i] + tmp * 10;
	}
	os_printf("20%x-%d-%d  %d:%d:%d \n", time_buf[1], time_buf[2], time_buf[3],
			time_buf1[4], time_buf1[5], time_buf1[6]);
}

void second_timer_init(void) {
	os_timer_disarm(&second_timer); //�ر�second_timer
	os_timer_setfn(&second_timer, (os_timer_func_t *) second_timer_Callback,
	NULL); //���ö�ʱ���ص�����
	os_delay_us(60000); //��ʱ�ȴ��ȶ�
	os_timer_arm(&second_timer, 1000, 1); //ʹ�ܺ��붨ʱ��
}

void SyncNetTimeCallBack(void) {
    //��ȡʱ���
	uint32 ts = sntp_get_current_timestamp();
	if (ts != 0) {
		os_timer_disarm(&getNetSyncTimer);
//		os_delay_us(60000); //��ʱ�ȴ��ȶ�
//		os_delay_us(60000); //��ʱ�ȴ��ȶ�
        //����ʱ��������ؾ����ʱ��
		char *pDate = (void *) sntp_get_real_time(ts);
		data = sntp_get_time_change(pDate);
		os_printf("20%x_%x_%x_%x:%x:%x_%x\n",data.year,data.month,data.day,data.hour,data.minute,data.second,data.week);
		time_buf1[1] = data.year;
		time_buf1[2] = data.month;
		time_buf1[3] = data.day;
		time_buf1[4] = data.hour;
		time_buf1[5] = data.minute;
		time_buf1[6] = data.second;
        //д��ds1302
		DS1302_Clock_init(time_buf1);
		os_timer_arm(&second_timer, 1000, 1); //ʹ�ܺ��붨ʱ��
	}
}
//��ȡ����ʱ�䶨ʱ��
static os_timer_t os_timer2;
void timer_sntp(void)
{

	uint32 flashid = spi_flash_get_id();
	os_printf("flashid:%d\n",flashid);

	ts = sntp_get_current_timestamp();		// ��ȡ��ǰ��ƫ��ʱ��
//	    os_printf("��ȡ����ʱ�� : %s\n", sntp_get_real_time(ts));	// ��ȡ��ʵʱ��

		if(ts == 0)		// ����ʱ���ȡʧ��
	    {
	        os_printf("��ȡ����ʱ��ʧ��\n");
	    }
	    else // ����ʱ���ȡ�ɹ�
	    {
	            os_printf("����ʱ�� : %s\n", sntp_get_real_time(ts));
	    }
}
//���û�ȡʱ�䶨ʱ��
void timer_init()
{

	os_timer_disarm(&os_timer2);
	os_timer_setfn( &os_timer2, (ETSTimerFunc *) ( timer_sntp ), NULL );
	os_timer_arm( &os_timer2, 3000, true );
}

void ds1302_timer_init()
{
	os_timer_disarm(&getNetSyncTimer);
	os_timer_setfn(&getNetSyncTimer, (os_timer_func_t *) SyncNetTimeCallBack,NULL);
	os_timer_arm(&getNetSyncTimer, 1000, 1);
}




