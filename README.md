# M5Unit-PoE-P4-UserDemo

## Overview

### SKU:U213

This is a web control interface project based on M5Stack Unit PoE P4, developed using the ESP-IDF framework, implementing remote control and monitoring functions. The project provides a web-based user interface that allows users to control various functions of the device through the network, including RGB LED, button counter, and GPIO pin control.

The system implements real-time bidirectional communication through WebSocket protocol, supporting LED color control, hardware button counting synchronization, and GPIO pin mode and level control functions.

## IDF Build, Flash, Run

### Tool Chains

[ESP-IDF  v5.4.2 ](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/index.html)

### Build

```
idf.py build
```

### Flash

```
idf.py -p PORT flash
```

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

1. Directly enter `poe-p4.local` in the browser to access the webpage.
2. Copy the IP address printed in the serial monitor and paste it into the browser to open the webpage.

## Related Link

- [Document & Datasheet](https://docs.m5stack.com/en/unit/unit-poe-p4)

## Required Libraries:

- [mdns](https://github.com/espressif/esp-protocols/tree/master/components/mdns)

## License

- [M5Unit PoE-P4- MIT](LICENSE)
