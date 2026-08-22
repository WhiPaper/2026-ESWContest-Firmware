#include "pm2008m.h"

#include <stddef.h>
#include <stdlib.h>

#define PM2008M_I2C_ADDRESS          0x28
#define PM2008M_I2C_CLOCK_HZ         100000
#define PM2008M_FRAME_SIZE           32
#define PM2008M_FRAME_HEADER         0x16
#define PM2008M_FRAME_LENGTH         32

struct pm2008m_dev_t {
    i2c_master_dev_handle_t i2c_dev;
    uint32_t timeout_ms;
};

static uint16_t pm2008m_u16_be(const uint8_t *bytes)
{
    return ((uint16_t)bytes[0] << 8) | bytes[1];
}

static uint8_t pm2008m_checksum(const uint8_t *frame)
{
    uint8_t checksum = 0;
    for (size_t index = 0; index < PM2008M_FRAME_SIZE - 1; ++index) {
        checksum ^= frame[index];
    }
    return checksum;
}

static esp_err_t pm2008m_parse_frame(const uint8_t *frame, pm2008m_data_t *data)
{
    if (frame[0] != PM2008M_FRAME_HEADER || frame[1] != PM2008M_FRAME_LENGTH) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (frame[PM2008M_FRAME_SIZE - 1] != pm2008m_checksum(frame)) {
        return ESP_ERR_INVALID_CRC;
    }

    data->status = frame[2];
    data->measurement_mode = pm2008m_u16_be(&frame[3]);
    data->calibration = pm2008m_u16_be(&frame[5]);

    data->grimm.pm1_0 = pm2008m_u16_be(&frame[7]);
    data->grimm.pm2_5 = pm2008m_u16_be(&frame[9]);
    data->grimm.pm10 = pm2008m_u16_be(&frame[11]);

    data->tsi.pm1_0 = pm2008m_u16_be(&frame[13]);
    data->tsi.pm2_5 = pm2008m_u16_be(&frame[15]);
    data->tsi.pm10 = pm2008m_u16_be(&frame[17]);

    data->particles.particles_0_3 = pm2008m_u16_be(&frame[19]);
    data->particles.particles_0_5 = pm2008m_u16_be(&frame[21]);
    data->particles.particles_1_0 = pm2008m_u16_be(&frame[23]);
    data->particles.particles_2_5 = pm2008m_u16_be(&frame[25]);
    data->particles.particles_5_0 = pm2008m_u16_be(&frame[27]);
    data->particles.particles_10_0 = pm2008m_u16_be(&frame[29]);

    return ESP_OK;
}

esp_err_t pm2008m_new(i2c_master_bus_handle_t bus, const pm2008m_config_t *config,
                      pm2008m_handle_t *out_handle)
{
    if (bus == NULL || config == NULL || out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_handle = NULL;

    esp_err_t ret = i2c_master_probe(bus, PM2008M_I2C_ADDRESS, config->timeout_ms);
    if (ret != ESP_OK) {
        return ret;
    }

    struct pm2008m_dev_t *device = calloc(1, sizeof(*device));
    if (device == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const i2c_device_config_t i2c_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PM2008M_I2C_ADDRESS,
        .scl_speed_hz = PM2008M_I2C_CLOCK_HZ,
    };

    ret = i2c_master_bus_add_device(bus, &i2c_config, &device->i2c_dev);
    if (ret != ESP_OK) {
        free(device);
        return ret;
    }

    device->timeout_ms = config->timeout_ms;
    *out_handle = device;
    return ESP_OK;
}

esp_err_t pm2008m_read(pm2008m_handle_t handle, pm2008m_data_t *out_data)
{
    if (handle == NULL || out_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t frame[PM2008M_FRAME_SIZE];
    esp_err_t ret = i2c_master_receive(handle->i2c_dev, frame, sizeof(frame), handle->timeout_ms);
    if (ret != ESP_OK) {
        return ret;
    }

    return pm2008m_parse_frame(frame, out_data);
}

esp_err_t pm2008m_delete(pm2008m_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_master_bus_rm_device(handle->i2c_dev);
    if (ret == ESP_OK) {
        free(handle);
    }
    return ret;
}
