# This directory is intended to be used as a component in an ESP-IDF test app.
idf_component_register(
    SRCS "test_pm2008m.c"
    INCLUDE_DIRS "../include" ".."
    REQUIRES unity esp_driver_i2c
)
