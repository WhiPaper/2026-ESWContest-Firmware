# PM2008M I2C component

ESP-IDF v6.0 or newer용 PM2008M 측정 드라이버입니다. UART는 지원하지 않으며,
애플리케이션이 만든 I2C master bus에 PM2008M device를 추가해 사용합니다.

## 지원 범위

- 센서 연결 확인 및 device handle 생성/삭제
- 32바이트 측정 프레임 수신
- Header, length, XOR checksum 검증
- GRIMM/TSI PM1.0, PM2.5, PM10 파싱
- 0.3, 0.5, 1.0, 2.5, 5.0, 10.0 um 입자 개수 파싱
- 센서 status, measurement mode, calibration 값 파싱

측정 모드 변경, 보정값 설정, 자동 polling task는 제공하지 않습니다.

## 하드웨어 연결

| PM2008M | 설명 |
| --- | --- |
| VCC | 5 V |
| GND | ESP32와 공통 접지 |
| SDA | I2C SDA |
| SCL | I2C SCL |
| CTL | LOW (I2C mode) |

센서 전원은 5 V를 사용하지만 ESP32 GPIO는 5 V tolerant가 아닙니다.
SDA/SCL의 pull-up 전압이 5 V인 모듈은 ESP32에 직접 연결하지 말고 3.3 V
레벨 시프터를 사용하십시오. 외부 pull-up은 3.3 V에 연결하는 것을 권장합니다.

## I2C 설정

- 7-bit device address: `0x28`
- SCL frequency: `100000 Hz` 이하
- address length: `I2C_ADDR_BIT_LEN_7`
- 애플리케이션이 bus 생성 및 삭제를 담당
- 컴포넌트가 PM2008M device handle 생성 및 삭제를 담당

## 사용 예

```c
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "pm2008m.h"

#define PM_SDA GPIO_NUM_21
#define PM_SCL GPIO_NUM_22

void app_main(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT_NUM_0,
        .sda_io_num = PM_SDA,
        .scl_io_num = PM_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));

    const pm2008m_config_t config = { .timeout_ms = 1000 };
    pm2008m_handle_t sensor;
    ESP_ERROR_CHECK(pm2008m_new(bus, &config, &sensor));

    pm2008m_data_t data;
    ESP_ERROR_CHECK(pm2008m_read(sensor, &data));

    // data.status, data.measurement_mode
    // data.grimm.pm1_0 / pm2_5 / pm10
    // data.tsi.pm1_0 / pm2_5 / pm10
    // data.particles.particles_0_3 ... particles_10_0

    ESP_ERROR_CHECK(pm2008m_delete(sensor));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus));
}
```

일반적인 애플리케이션에서는 `pm2008m_read()`를 애플리케이션 task 또는
타이머에서 필요한 주기로 호출하십시오. 컴포넌트 내부에는 FreeRTOS task나
지연 기반 초기화가 없습니다.

## API

### `pm2008m_new(bus, config, &handle)`

주소 `0x28`을 probe한 뒤 100 kHz, 7-bit 주소 설정으로 device handle을
생성합니다. `config->timeout_ms`는 probe와 수신에 모두 사용됩니다.

### `pm2008m_read(handle, &data)`

센서에서 정확히 32바이트를 수신하고 검증한 뒤 `pm2008m_data_t`에 기록합니다.
성공하면 `ESP_OK`를 반환합니다.

### `pm2008m_delete(handle)`

컴포넌트가 생성한 I2C device handle만 삭제합니다. I2C bus는 삭제하지 않습니다.
반드시 bus 삭제보다 먼저 호출하십시오.

## 오류 처리

- `ESP_ERR_INVALID_ARG`: NULL 인자
- `ESP_ERR_NOT_FOUND`: probe에서 센서를 찾지 못함
- `ESP_ERR_TIMEOUT`: 버스 고착, pull-up, 배선 또는 timeout 문제
- `ESP_ERR_INVALID_RESPONSE`: header 또는 length 불일치
- `ESP_ERR_INVALID_CRC`: XOR checksum 불일치
- 그 밖의 I2C 오류: ESP-IDF 원래 `esp_err_t`를 그대로 반환

반환된 status 값은 해석하지 않고 raw 값으로 보존합니다. 알려진 상태값은
`0x01` 측정 종료, `0x02` 측정 중, `0x07` Alarm, `0x80` 데이터 안정입니다.

## 컴포넌트 추가

프로젝트의 `components/pm2008m`에 이 디렉터리를 배치하면 ESP-IDF가 자동으로
컴포넌트를 검색합니다. 소스에서는 `#include "pm2008m.h"`를 사용하십시오.
