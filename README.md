# Product Name

## Overview

### SKU:U213

This is a web control interface project based on M5Stack Unit PoE P4, developed using the ESP-IDF framework, implementing remote control and monitoring functions. The project provides a web-based user interface that allows users to control various functions of the device through the network, including RGB LED, button counter, and GPIO pin control.

The system implements real-time bidirectional communication through WebSocket protocol, supporting LED color control, hardware button counting synchronization, and GPIO pin mode and level control functions.

## IDF Build

### Tool Chains

[ESP-IDF  v5.4.2 ](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/index.html)

## Related Link

- [Document & Datasheet](https://docs.m5stack.com/en/unit/unit-poe-p4)

## Required Libraries:

- [esp-protocols/components/mdns at master · espressif/esp-protocols](https://github.com/espressif/esp-protocols/tree/master/components/mdns)

## License

- [M5Unit PoE-P4- MIT](LICENSE)

## Remaining steps(Editorial Staff Look,After following the steps, remember to delete all the content below)

1. Change [clang format check path](./.github/workflows/clang-format-check.yml#L42-L47).
2. Add License content to [LICENSE](/LICENSE).
3. Change link on line 78 of [bug-report.yml](./.github/ISSUE_TEMPLATE/bug-report.yml#L79).

```cpp
Example
# M5Unit-ENV

## Overview

### SKU:U001 & U001-B & U001-C

Contains M5Stack-**UNIT ENV** series related case programs.ENV is an environmental sensor with integrated SHT30 and QMP6988 internally to detect temperature, humidity, and atmospheric pressure data.

## Related Link

- [Document & Datasheet](https://docs.m5stack.com/en/unit/envIII)

## Required Libraries:

- [Adafruit_BMP280_Library](https://github.com/adafruit/Adafruit_BMP280_Library)

## License

- [M5Unit-ENV - MIT](LICENSE)
```

