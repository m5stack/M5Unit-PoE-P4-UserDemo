# M5Unit-PoE-P4-UserDemo

## Overview

### Unit PoE-P4 and Unit PoE-P4X

This is a web control interface project for M5Stack Unit PoE-P4 (SKU: U213) and Unit PoE-P4X, developed using the ESP-IDF framework. The project provides a web-based user interface that allows users to control various functions of the device through the network, including RGB LED, button counter, and GPIO pin control.

The system implements real-time bidirectional communication through WebSocket protocol, supporting LED color control, hardware button counting synchronization, aGPIO pin mode and level control functions.

## IDF Build, Flash, Run

### Requirements

- ESP-IDF v6.0.3
- Target: ESP32-P4 (`esp32p4`)
- Toolchain: riscv32-esp-elf (installed with ESP-IDF v6.0.3)

Both product profiles are built with ESP-IDF **v6.0.3** (target `esp32p4`) and are selected with the `PRODUCT` CMake variable. Variant settings live in
`sdkconfig.defaults.p4` / `sdkconfig.defaults.p4x`, and each profile keeps its own
`sdkconfig.p4` / `sdkconfig.p4x`. ESP32-P4 revisions below 3.0 and revisions 3.0 or newer are
mutually incompatible, so build the profile that matches your hardware.

### Unit PoE-P4X

Build:

```powershell
idf.py -B build_p4x -DPRODUCT=p4x build
```

Flash and monitor:

```powershell
idf.py -B build_p4x -DPRODUCT=p4x -p PORT flash monitor
```

The P4X profile uses the mDNS hostname `poe-p4x.local` and prints `POE_P4X_DEMO_READY` after the HTTP/WebSocket server starts.

### Unit PoE-P4

Build:

```powershell
idf.py -B build_p4 -DPRODUCT=p4 build
```

Flash and monitor:

```powershell
idf.py -B build_p4 -DPRODUCT=p4 -p PORT flash monitor
```

### Notes

- Always keep `-B` and `-DPRODUCT` together. `idf.py flash` without `-B` flashes the default
  `build/` directory, which may contain the other profile's firmware.
- Do not share one build directory between the two profiles.
- `sdkconfig.<profile>` is generated from the defaults on the first build; delete it to re-apply
  changed defaults.
- The IDE Build button runs `idf.py -B build build`, i.e. the default profile (`p4`, `build/`).

### Run

First, connect an Ethernet cable to the device’s Ethernet port, then connect the power supply. After the device is powered on, it will automatically connect to the Ethernet network. Now open the serial monitor; once the Ethernet connection is successful, the IP address information will be printed as follow.

```
I (511) app_webserver: Initializing SPIFFS
I (581) esp_eth.netif.netif_glue: 60:55:f9:fb:0a:3d
I (581) esp_eth.netif.netif_glue: ethernet attached to netif
I (2881) main: Ethernet Started
I (2881) app_webserver: Starting webserver...
I (2881) main: Ethernet Link Up
I (2881) main: Ethernet HW Addr 60:55:f9:fb:0a:3d
W (2881) httpd_uri: httpd_register_uri_handler: no slots left for registering handler
I (2891) main_task: Returned from app_main()
I (7411) esp_netif_handlers: eth ip: 192.168.20.198, mask: 255.255.255.0, gw: 192.168.20.1
I (7411) main: Ethernet Got IP Address
I (7411) main: ~~~~~~~~~~~
I (7411) main: ETHIP:192.168.20.198
I (7411) main: ETHMASK:255.255.255.0
I (7411) main: ETHGW:192.168.20.1
I (7421) main: ~~~~~~~~~~~
I (7421) mdns_mem: mDNS task will be created from internal RAM
I (7431) MDNS: mdns hostname set to: poe-p4
```

### How to Access the Device Control Webpage

First, make sure that the computer or mobile phone you are using is on the same local area network (LAN) as the Ethernet connection. There are two methods:

1. Enter `poe-p4.local` for Unit PoE-P4 or `poe-p4x.local` for Unit PoE-P4X.
2. Copy the IP address printed in the serial monitor and paste it into the browser to open the webpage.

## Related Link

- [Document & Datasheet](https://docs.m5stack.com/en/products/sku/U213)

## Required Libraries:

- [mdns 1.14.1](https://github.com/espressif/esp-protocols/tree/master/components/mdns)
- [cjson 1.7.19~2](https://github.com/espressif/idf-extra-components/tree/master/cjson)
- [ip101 1.1.0](https://github.com/espressif/esp-eth-drivers/tree/master/ip101)

## License

- [M5Unit PoE-P4 - MIT](LICENSE)

