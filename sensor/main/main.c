#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "bme280.h"
#include "pm2008m.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

static const char *TAG = "sensor_test";

void app_main(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    i2c_master_bus_handle_t master_bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &master_bus));

    pm2008m_handle_t pm2008m;
    const pm2008m_config_t pm_config = {.timeout_ms = 1000};
    ESP_ERROR_CHECK(pm2008m_new(master_bus, &pm_config, &pm2008m));

    const i2c_config_t bme_bus_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .master.clk_speed = 100000,
    };
    i2c_bus_handle_t bme_bus = i2c_bus_create(I2C_PORT, &bme_bus_config);
    if (bme_bus == NULL) {
        ESP_LOGE(TAG, "failed to wrap I2C bus for BME280");
        return;
    }

    bme280_handle_t bme280 = bme280_create(bme_bus, BME280_I2C_ADDRESS_DEFAULT);
    if (bme280 == NULL) {
        ESP_LOGE(TAG, "BME280 not found at 0x%02x", BME280_I2C_ADDRESS_DEFAULT);
        return;
    }
    ESP_ERROR_CHECK(bme280_default_init(bme280));

    vTaskDelay(pdMS_TO_TICKS(1000));
    while (true) {
        pm2008m_data_t pm_data;
        float temperature;
        float humidity;
        float pressure;

        esp_err_t pm_ret = pm2008m_read(pm2008m, &pm_data);
        esp_err_t temp_ret = bme280_read_temperature(bme280, &temperature);
        esp_err_t hum_ret = bme280_read_humidity(bme280, &humidity);
        esp_err_t pressure_ret = bme280_read_pressure(bme280, &pressure);

        if (pm_ret == ESP_OK) {
            ESP_LOGI(TAG, "PM1.0=%u PM2.5=%u PM10=%u ug/m3",
                     pm_data.grimm.pm1_0, pm_data.grimm.pm2_5, pm_data.grimm.pm10);
        } else {
            ESP_LOGE(TAG, "PM2008M read failed: %s", esp_err_to_name(pm_ret));
        }

        if (temp_ret == ESP_OK && hum_ret == ESP_OK && pressure_ret == ESP_OK) {
            ESP_LOGI(TAG, "BME280: %.2f C, %.2f %%RH, %.2f hPa",
                     temperature, humidity, pressure);
        } else {
            ESP_LOGE(TAG, "BME280 read failed: T=%s H=%s P=%s",
                     esp_err_to_name(temp_ret), esp_err_to_name(hum_ret),
                     esp_err_to_name(pressure_ret));
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
