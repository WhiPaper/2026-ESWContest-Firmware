#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle for a PM2008M I2C device. */
typedef struct pm2008m_dev_t *pm2008m_handle_t;

/** Configuration for a PM2008M device. */
typedef struct {
    /** Timeout used for I2C probe and receive operations, in milliseconds. */
    uint32_t timeout_ms;
} pm2008m_config_t;

/** Particulate mass concentration values reported by the sensor. */
typedef struct {
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;
} pm2008m_mass_t;

/** Particle counts reported by the sensor. */
typedef struct {
    uint16_t particles_0_3;
    uint16_t particles_0_5;
    uint16_t particles_1_0;
    uint16_t particles_2_5;
    uint16_t particles_5_0;
    uint16_t particles_10_0;
} pm2008m_particles_t;

/** A validated PM2008M measurement frame. */
typedef struct {
    uint8_t status;
    uint16_t measurement_mode;
    uint16_t calibration;

    pm2008m_mass_t grimm;
    pm2008m_mass_t tsi;
    pm2008m_particles_t particles;
} pm2008m_data_t;

/**
 * @brief Probe and add a PM2008M device to an application-owned I2C bus.
 *
 * The component owns the returned device handle, but never owns the bus.
 */
esp_err_t pm2008m_new(i2c_master_bus_handle_t bus, const pm2008m_config_t *config,
                      pm2008m_handle_t *out_handle);

/**
 * @brief Receive, validate, and parse one 32-byte PM2008M measurement frame.
 */
esp_err_t pm2008m_read(pm2008m_handle_t handle, pm2008m_data_t *out_data);

/** @brief Remove the PM2008M device from its I2C bus and free its handle. */
esp_err_t pm2008m_delete(pm2008m_handle_t handle);

#ifdef __cplusplus
}
#endif
