#include "sensors/bme280_sensor.h"
#include "sensors/sensor_manager.h"
#include <driver/i2c.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "BME280";

#define I2C_MASTER_SCL_GPIO 17
#define I2C_MASTER_SDA_GPIO 16
#define BME280_ADDR 0x76
#define I2C_NUM I2C_NUM_0

/* BME280 register addresses */
#define BME280_REG_CHIP_ID      0xD0
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_CONFIG       0xF5
#define BME280_REG_DATA_START   0xF7
#define BME280_REG_CALIB_00     0x88
#define BME280_REG_CALIB_26     0xE1

/* Calibration data */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bme280_data_t bme_data = {0};
static sensor_data_t bme280_sensors[3];
static bool initialized = false;
static bme280_calib_t calib = {0};
static int32_t t_fine = 0;

static esp_err_t bme280_write_reg(uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t bme280_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &data[len - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t bme280_read_calibration(void)
{
    uint8_t buf[26];
    esp_err_t err = bme280_read_regs(BME280_REG_CALIB_00, buf, 26);
    if (err != ESP_OK) return err;
    
    calib.dig_T1 = (uint16_t)(buf[1] << 8 | buf[0]);
    calib.dig_T2 = (int16_t)(buf[3] << 8 | buf[2]);
    calib.dig_T3 = (int16_t)(buf[5] << 8 | buf[4]);
    calib.dig_P1 = (uint16_t)(buf[7] << 8 | buf[6]);
    calib.dig_P2 = (int16_t)(buf[9] << 8 | buf[8]);
    calib.dig_P3 = (int16_t)(buf[11] << 8 | buf[10]);
    calib.dig_P4 = (int16_t)(buf[13] << 8 | buf[12]);
    calib.dig_P5 = (int16_t)(buf[15] << 8 | buf[14]);
    calib.dig_P6 = (int16_t)(buf[17] << 8 | buf[16]);
    calib.dig_P7 = (int16_t)(buf[19] << 8 | buf[18]);
    calib.dig_P8 = (int16_t)(buf[21] << 8 | buf[20]);
    calib.dig_P9 = (int16_t)(buf[23] << 8 | buf[22]);
    calib.dig_H1 = buf[25];
    
    uint8_t buf2[7];
    err = bme280_read_regs(BME280_REG_CALIB_26, buf2, 7);
    if (err != ESP_OK) return err;
    
    calib.dig_H2 = (int16_t)(buf2[1] << 8 | buf2[0]);
    calib.dig_H3 = buf2[2];
    calib.dig_H4 = (int16_t)((buf2[3] << 4) | (buf2[4] & 0x0F));
    calib.dig_H5 = (int16_t)((buf2[5] << 4) | (buf2[4] >> 4));
    calib.dig_H6 = (int8_t)buf2[6];
    
    return ESP_OK;
}

static float bme280_compensate_temperature(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
}

static float bme280_compensate_pressure(int32_t adc_P)
{
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    if (var1 == 0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (float)((uint32_t)p) / 256.0f / 100.0f; /* hPa */
}

static float bme280_compensate_humidity(int32_t adc_H)
{
    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (float)(v_x1_u32r >> 12) / 1024.0f;
}

esp_err_t bme280_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing BME280 sensor");
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_GPIO,
        .scl_io_num = I2C_MASTER_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(I2C_NUM, &conf);
    i2c_driver_install(I2C_NUM, conf.mode, 0, 0, 0);
    
    /* Verify chip ID */
    uint8_t chip_id = 0;
    esp_err_t err = bme280_read_regs(BME280_REG_CHIP_ID, &chip_id, 1);
    if (err != ESP_OK || (chip_id != 0x60 && chip_id != 0x58)) {
        ESP_LOGE(TAG, "BME280 not found (chip_id=0x%02X, err=%s)", chip_id, esp_err_to_name(err));
        /* Continue anyway — sensor may be missing in development */
    } else {
        ESP_LOGI(TAG, "BME280 detected (chip_id=0x%02X)", chip_id);
    }
    
    /* Read calibration data */
    bme280_read_calibration();
    
    /* Configure: humidity oversampling x1 */
    bme280_write_reg(BME280_REG_CTRL_HUM, 0x01);
    /* Configure: temp & pressure oversampling x1, normal mode */
    bme280_write_reg(BME280_REG_CTRL_MEAS, 0x27);
    /* Configure: standby 1000ms, filter off */
    bme280_write_reg(BME280_REG_CONFIG, 0xA0);
    
    bme280_sensors[0].id = 20;
    strncpy(bme280_sensors[0].name, "Temperature_2", sizeof(bme280_sensors[0].name) - 1);
    bme280_sensors[0].type = SENSOR_TYPE_TEMPERATURE;
    bme280_sensors[0].status = SENSOR_STATUS_OK;
    bme280_sensors[0].value = 0.0f;
    bme280_sensors[0].min_value = -40.0f;
    bme280_sensors[0].max_value = 85.0f;
    bme280_sensors[0].threshold_min = 18.0f;
    bme280_sensors[0].threshold_max = 30.0f;
    bme280_sensors[0].enabled = true;
    bme280_sensors[0].alarm_enabled = true;
    
    bme280_sensors[1].id = 21;
    strncpy(bme280_sensors[1].name, "Humidity_2", sizeof(bme280_sensors[1].name) - 1);
    bme280_sensors[1].type = SENSOR_TYPE_HUMIDITY;
    bme280_sensors[1].status = SENSOR_STATUS_OK;
    bme280_sensors[1].value = 0.0f;
    bme280_sensors[1].min_value = 0.0f;
    bme280_sensors[1].max_value = 100.0f;
    bme280_sensors[1].threshold_min = 40.0f;
    bme280_sensors[1].threshold_max = 80.0f;
    bme280_sensors[1].enabled = true;
    bme280_sensors[1].alarm_enabled = true;
    
    bme280_sensors[2].id = 22;
    strncpy(bme280_sensors[2].name, "Pressure", sizeof(bme280_sensors[2].name) - 1);
    bme280_sensors[2].type = SENSOR_TYPE_PRESSURE;
    bme280_sensors[2].status = SENSOR_STATUS_OK;
    bme280_sensors[2].value = 0.0f;
    bme280_sensors[2].min_value = 870.0f;
    bme280_sensors[2].max_value = 1084.0f;
    bme280_sensors[2].threshold_min = 950.0f;
    bme280_sensors[2].threshold_max = 1050.0f;
    bme280_sensors[2].enabled = true;
    bme280_sensors[2].alarm_enabled = false;
    
    initialized = true;
    return ESP_OK;
}

esp_err_t bme280_read(float *temperature, float *humidity, float *pressure)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    uint8_t raw_data[8];
    esp_err_t err = bme280_read_regs(BME280_REG_DATA_START, raw_data, 8);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BME280 data: %s", esp_err_to_name(err));
        bme_data.valid = false;
        return err;
    }
    
    int32_t adc_P = (int32_t)((raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4));
    int32_t adc_T = (int32_t)((raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4));
    int32_t adc_H = (int32_t)((raw_data[6] << 8) | raw_data[7]);
    
    /* Temperature must be computed first (sets t_fine for pressure/humidity) */
    *temperature = bme280_compensate_temperature(adc_T);
    *pressure = bme280_compensate_pressure(adc_P);
    *humidity = bme280_compensate_humidity(adc_H);
    
    bme_data.temperature = *temperature;
    bme_data.humidity = *humidity;
    bme_data.pressure = *pressure;
    bme_data.valid = true;
    
    return ESP_OK;
}

esp_err_t bme280_read_all(void)
{
    float temp = 0, hum = 0, pres = 0;
    esp_err_t ret = bme280_read(&temp, &hum, &pres);
    
    bme280_sensors[0].value = temp;
    bme280_sensors[1].value = hum;
    bme280_sensors[2].value = pres;
    
    if (ret != ESP_OK) {
        for (int i = 0; i < 3; i++) {
            bme280_sensors[i].status = SENSOR_STATUS_ERROR;
        }
    } else {
        for (int i = 0; i < 3; i++) {
            bme280_sensors[i].status = SENSOR_STATUS_OK;
        }
    }
    
    return ret;
}

bme280_data_t bme280_get_data(void)
{
    return bme_data;
}

sensor_data_t* get_bme280_sensors(uint8_t *count)
{
    *count = 3;
    return bme280_sensors;
}
