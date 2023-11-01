#include "luat_debug.h"
#include "luat_adc.h"
#include "luat_gpio.h"
#include "platform_define.h"
#include "param_ctrl.h"
#include "luat_fs.h"
static int g_s_gps_status = 0;
static int g_s_relay_status = 0;

Jt808Msg jt808_message = {
    .addr = "112.125.89.8",
    .port = 46765,
    .province = 00,
    .city = 00,
    .oemid = "xxx",
    .devicetype = "xxx",
    .deviceid = "xxx",
    .vehiclecolor = 1,
    .vehiclenumber = "xxx",
    .uploadtime = 1,
    .heart_interval = 180,
    .language = 0,
    .timezone = 8,
    .apnc_mcc = 460,
    .apnc_mnc = "02",
    .protocol_num = 2,
    .fortificat_mode = 0,
    .base_upload_control = 1,
    .base_upload_time = 60,
    .base_wait_gps_time = 60,
};

void config_service_get(CmdType cmd, ParamType type, uint8_t *data, uint32_t len)
{
    switch(cmd)
    {
        case CFG_JT_PROVINCE:
            *data = jt808_message.province;
            break;
        case CFG_JT_CITY:
            *data = jt808_message.city;
            break;
        case CFG_JT_OEM_ID:
            memcpy(data, jt808_message.oemid, sizeof(jt808_message.oemid));
            break;
        case CFG_DEVICETYPE:
            memcpy(data, jt808_message.devicetype, sizeof(jt808_message.devicetype));
            break;
        case CFG_JT_DEVICE_ID:
            memcpy(data, jt808_message.deviceid, sizeof(jt808_message.devicetype));
            break;
        case CFG_JT_VEHICLE_COLOR:
            *data = jt808_message.vehiclecolor;
            break;
        case CFG_JT_VEHICLE_NUMBER:
            memcpy(data, jt808_message.vehiclenumber, sizeof(jt808_message.vehiclenumber));
            break;
        case CFG_JT_AUTH_CODE:
            memcpy(data, jt808_message.auth_code, sizeof(jt808_message.auth_code));
            break;
        case CFG_SPEED_ALARM_ENABLE:
            *data = jt808_message.speed_enable;
            break;
        case CFG_SPEED_CHECK_TIME:
            *data = jt808_message.speed_check_time;
            break;
        case CFG_SPEEDTHR:
            *data = jt808_message.speedthr;
            break;
        case CFG_TURN_ANGLE:
            *data = jt808_message.turn_angle;
            break;
        case CFG_UPLOADTIME:
            *data = jt808_message.uploadtime;
            break;
        case CFG_JT_HBINTERVAL:
            *data = jt808_message.hbinterval;
            break;
        case CFG_APN_NAME:
            memcpy(data, jt808_message.apn_name, sizeof(jt808_message.apn_name));
            break;
        case CFG_APN_USER:
            memcpy(data, jt808_message.apn_user, sizeof(jt808_message.apn_user));
            break;
        case CFG_APN_PWD:
            memcpy(data, jt808_message.apn_pwd, sizeof(jt808_message.apn_pwd));
            break;
        case CFG_HEART_INTERVAL:
            *data = jt808_message.heart_interval;
            break;
        case CFG_JT_MILIAGE:
            *data = jt808_message.miliage;
            break;
        case CFG_APNC_MCC:
            *data = jt808_message.apnc_mcc;
            break;
        case CFG_APNC_MNC:
            memcpy(data, jt808_message.apnc_mnc, sizeof(jt808_message.apnc_mnc));
            break;
        case CFG_TIME_ZONE:
            *data = jt808_message.timezone;
            break;
        case CFG_RELAY:
            *data = jt808_message.relay_control;
            break;
        case CFG_LANG:
            *data = jt808_message.language;
            break;
        case CFG_GPS:
            *data = jt808_message.gps_control;
            break;
        case CFG_SERVERADDR:
            memcpy(data, jt808_message.addr, len);
            break;
        case CFG_SERVERADDR_PORT:
            memcpy((uint32_t *)data, &(jt808_message.port), sizeof(uint32_t));
            break;
        case CFG_PROTOCOL_NUM:
            *data = jt808_message.protocol_num;
            break;
        case CFG_FORTIFICAT_MODE:
            *data = jt808_message.fortificat_mode;
            break;
        case CFG_BASE_UPLOAD_ENABLE:
            *data = jt808_message.base_upload_control;
            break;
        case CFG_BASE_UPLOAD_INTERVAL:
            *data = jt808_message.base_upload_control;
            break;
        case CFG_BASE_WAIT_GPS_TIME:
            *data = jt808_message.base_wait_gps_time;
            break;
        default:
            break;
    }
}

void config_service_set(CmdType cmd, ParamType type, uint8_t *data, uint32_t len)
{
    FILE* fd = luat_fs_fopen(JT808_PROTOCOL_PARAM,"r");
	if (fd == NULL)
	{
		LUAT_DEBUG_PRINT("jt808_param.txt open failed");
        return ;
	}
    memset(&jt808_message,0,sizeof(jt808_message));
	luat_fs_fread(&jt808_message, sizeof(jt808_message), 1, fd);
    luat_fs_fclose(fd);

    switch(cmd)
    {
        case CFG_JT_AUTH_CODE:
            memset(jt808_message.auth_code, 0, sizeof(jt808_message.auth_code));
            memcpy(jt808_message.auth_code, data, len);
            break;
        case CFG_JT_ISREGISTERED:
            jt808_message.isregistered = *data;
            break;
        case CFG_APNC_MCC:
            jt808_message.apnc_mcc = *data;
            break;
        case CFG_APNC_MNC:
            memset(jt808_message.apnc_mnc, 0, sizeof(jt808_message.apnc_mnc));
            memcpy(jt808_message.apnc_mnc, data,len);
            break;
        case CFG_TIME_ZONE:
            jt808_message.timezone = *data;
            break;
        case CFG_RELAY:
            jt808_message.relay_control = *data;
            break;
        case CFG_LANG:
            jt808_message.language = *data;
            break;
        case CFG_GPS:
            jt808_message.gps_control = *data;
            break;
        case CFG_SPEED_ALARM_ENABLE:
            jt808_message.speed_enable = *data;
            break;
        case CFG_SPEED_CHECK_TIME:
            jt808_message.speed_check_time = *data;
            break;
        case CFG_SPEEDTHR:
            jt808_message.speedthr = *data;
            break;
        case CFG_APN_NAME:
            memset(jt808_message.apn_name, 0, sizeof(jt808_message.apn_name));
            memcpy(jt808_message.apn_name, data, len);
            break;
        case CFG_APN_USER:
            memset(jt808_message.apn_user, 0, sizeof(jt808_message.apn_user));
            memcpy(jt808_message.apn_user, data, len);
            break;
        case CFG_APN_PWD:
            memset(jt808_message.apn_pwd, 0, sizeof(jt808_message.apn_pwd));
            memcpy(jt808_message.apn_pwd, data, len);
            break;
        case CFG_SERVERADDR:
            memset(jt808_message.addr, 0, sizeof(jt808_message.addr));
            memcpy(jt808_message.addr, data, len);
            break;
        case CFG_SERVERADDR_PORT:
            jt808_message.port = *data;
            break;
        case CFG_PROTOCOL_NUM:
            jt808_message.protocol_num = *data;
            break;
        case CFG_FORTIFICAT_MODE:
            jt808_message.fortificat_mode = *data;
            break;
        case CFG_BASE_UPLOAD_ENABLE:
            jt808_message.base_upload_control = *data;
            break;
        case CFG_BASE_UPLOAD_INTERVAL:
            jt808_message.base_upload_control = *data;
            break;
        case CFG_BASE_WAIT_GPS_TIME:
            jt808_message.base_wait_gps_time = *data;
            break;
        case CFG_HEART_INTERVAL:
            jt808_message.heart_interval = *data;
            break;
        default:
            break;
    }

    FILE* fd1 = luat_fs_fopen(JT808_PROTOCOL_PARAM, "wb+");
    luat_fs_fwrite(&jt808_message, sizeof(jt808_message), 1, fd1);
    luat_fs_fclose(fd1);
}

int config_relay_set(int status)
{
    int ret;
    static int flag = 0;
    if(flag == 0)
    {
        luat_gpio_cfg_t gpio_cfg;
        luat_gpio_set_default_cfg(&gpio_cfg);

        gpio_cfg.pin = HAL_GPIO_24;
        luat_gpio_open(&gpio_cfg);
        ret = luat_gpio_set(HAL_GPIO_24, status);
        flag = 1;
    }
    else
        ret = luat_gpio_set(HAL_GPIO_24, status);

    g_s_relay_status = status;

    return ret;
}

int config_relay_get(void)
{
    return g_s_relay_status;
}

int config_gps_set(int status)
{
    int ret;
    static int flag = 0;
    if(flag == 0)
    {
        luat_gpio_cfg_t gpio_cfg;
        luat_gpio_set_default_cfg(&gpio_cfg);

        gpio_cfg.pin = HAL_GPIO_13;
        gpio_cfg.mode = LUAT_GPIO_OUTPUT;
        gpio_cfg.alt_fun = 4;
        luat_gpio_open(&gpio_cfg);
        ret = luat_gpio_set(HAL_GPIO_13, status);
    }
    else
        ret = luat_gpio_set(HAL_GPIO_13, status);

    g_s_gps_status = status;

    return ret;
}

int config_gps_get(void)
{
    return g_s_gps_status;
}

int config_vbat_get(void)
{
    int val, val2;
    luat_adc_open(11, NULL);
    luat_adc_read(11, &val, &val2);
    LUAT_DEBUG_PRINT("vbat: %d %d\n",val, val2);
    luat_adc_close(11);
    return val2;
}

int config_input_volt_get(void)
{
    int val, val2;
    luat_adc_ctrl_param_t ctrl_param;
	ctrl_param.range = LUAT_ADC_AIO_RANGE_1_2;
	luat_adc_ctrl(0, LUAT_ADC_SET_GLOBAL_RANGE, ctrl_param);
    luat_adc_open(0, NULL);
    luat_adc_read(0, &val, &val2);
    LUAT_DEBUG_PRINT("input_volt: %d %d\n",val, (val2*76)/1000);
    luat_adc_close(0);

    return (val2*76)/1000;
}

int config_charge_get(void)
{
    int status;
    ChargeType type;
    luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);

	gpio_cfg.pin = HAL_GPIO_22;
    gpio_cfg.mode = LUAT_GPIO_INPUT;
	luat_gpio_open(&gpio_cfg);
	status = luat_gpio_get(HAL_GPIO_22);
    luat_gpio_close(HAL_GPIO_22);

    if(status == 0)
        type = CHARGING;
    else if((status == 1)&&(config_input_volt_get() < 6000))
        type = UNCHARGED;
    else
        type = CHARGEFULL;

    return type;
}

int config_accelerated_speed_set(int status)
{
    int ret;
    static int flag = 0;
    if(flag == 0)
    {
        luat_gpio_cfg_t gpio_cfg;
        luat_gpio_set_default_cfg(&gpio_cfg);

        gpio_cfg.pin = HAL_GPIO_25;
        luat_gpio_open(&gpio_cfg);
        ret = luat_gpio_set(HAL_GPIO_25, status);
    }
    else
        ret = luat_gpio_set(HAL_GPIO_25, status);

    return ret;
}


