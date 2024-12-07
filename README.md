# Unofficial fork of ZMK

This is an unofficial fork of ZMK. I was asked about my proof of principle setup of a split keyboard with a Nice!Nano v2
controller on either half, with a Nice!View display on the master side (here: left) and a Cirque Glidepoint track pad at
the SPI bus on the slave side (here: right).

The present branch consists of the stock ZMK firmware current as of December 6, 2024 with the following modifications:

- The [patch](https://github.com/petejohanson/zmk/tree/feat/pointers-move-scroll) of [pull request 2027](https://github.com/zmkfirmware/zmk/pull/2027) for pointer movements.

- The definition of a shield called *breadboard* at `./app/boards/shields/breadboard` as a proof of principle configuration.

In addition to this branch of ZMK, I need the following two ZMK modules out of tree (at separate paths):

- [https://github.com/badjeff/zmk-split-peripheral-input-relay](zmk-split-peripheral-input-relay) by *badjeff* because the Cirque track pad is on the peripheral side,

- [https://github.com/petejohanson/cirque-input-module](cirque-input-module) the actual driver by *Pete Johanson* for the Cirque track pad.

I build the firmware in the `./app` subdirectory
```
west build -p -d build/left  -b nice_nano_v2 -- -DSHIELD="breadboard_left nice_view"  -DZMK_EXTRA_MODULES="/global/path/to/cirque-input-module;/global/path/to/zmk-split-peripheral-input-relay"
west build -p -d build/right -b nice_nano_v2 -- -DSHIELD="breadboard_right nice_view" -DZMK_EXTRA_MODULES="/global/path/to/cirque-input-module;/global/path/to/zmk-split-peripheral-input-relay"
```

and upload the `./build/left/zephyr/zmk.ut2` firmware as usual.

The circuit is mounted on two breadboards. On the left half, the Nice!View is connected to D1=0.06 (CS), D2=0.17 (MOSI), D3=0.20 (SCK) as well as 3V3 output of the Nice!Nano v2 and GND. On the right half, the Cirque track pad is connected to D0=0.08 (SS), D2=0.17 (MOSI), D3=0.20 (SCK), D4=0.22 (MISO) and D5=0.24 (DR) as well as 3V3 and GND. With the SPI bus set up like this, both devices Nice!View and Cirque track pad can easily share the same SPI bus by simply using different chip select lines (called CS or SS above). The data ready (DR) line is used outside of the SPI specification. The 2x2 keyboard matrix of the breadboard setup has rows D18, D19 and columns D14, D15.

The following is the original README:

# Zephyr™ Mechanical Keyboard (ZMK) Firmware

[![Discord](https://img.shields.io/discord/719497620560543766)](https://zmk.dev/community/discord/invite)
[![Build](https://github.com/zmkfirmware/zmk/workflows/Build/badge.svg)](https://github.com/zmkfirmware/zmk/actions)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](CODE_OF_CONDUCT.md)

[ZMK Firmware](https://zmk.dev/) is an open source ([MIT](LICENSE)) keyboard firmware built on the [Zephyr™ Project](https://www.zephyrproject.org/) Real Time Operating System (RTOS). ZMK's goal is to provide a modern, wireless, and powerful firmware free of licensing issues.

Check out the website to learn more: https://zmk.dev/.

You can also come join our [ZMK Discord Server](https://zmk.dev/community/discord/invite).

To review features, check out the [feature overview](https://zmk.dev/docs/). ZMK is under active development, and new features are listed with the [enhancement label](https://github.com/zmkfirmware/zmk/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement) in GitHub. Please feel free to add 👍 to the issue description of any requests to upvote the feature.
