# Experimental Fork

This is an experimental fork of the [ZMK Project](https://www.github.com/zmkfirmware/zmk).

## Shields

### twobytwo3

This is an experimental setup on two breadboards, each run by a Nice!Nano v2 with a 3.7V Lipo battery connected to BAT
and GND of the Nice!Nano, a button between GND and RST, and with a 2x2 diode matrix with rows GPIO 15 and 14, diodes
from rows to columns and columns GPIO 16 and 10 (left) or GPIO 10 and 16 (right). The right half is the master.

In addition, a Pimoroni Trackball module PIM447 is connected to the right half, to the toggleable 3V3 output of the
Nice!Nano2 and GND, with SDA connected to GPIO 2 and SCL to GPIO 3 (all in "Spark Fun Pro Micro" numbering). The INT pin
of the PIM447 remains unconnected.

The bluetooth name of all these breadboard tests is "Two By Two".

### yackboard-v1

This is the firmware of the Yackboard v1. Required patches:

- mouse emulation
- custom retro-tap behavior
- PIM447 patch (my own version) if a Pimoroni Trackball PIM447 is to be used

### Patches

This project was forked from the main ZMK repository (upstream) on 2023-10-25.

The mouse emulation of https://github.com/ftc/zmk/tree/mouse-ftc was adapted to Zephyr 3.2 in https://github.com/urob/zmk/tree/mouse-3.2
It was patched into the current branch on 2023-10-25. This patch is required whenever a pointing device is used.

The custom retro-tap behavior patch of https://github.com/nickconway/zmk/tree/retro-tap-binding was added on
2023-10-25. It is used only in the keymap of the yackboard.

The old PIM447 patch by cdc-mkb https://github.com/zmkfirmware/zmk/pull/961 was used, adapted to Zephyr 3.2 and heavily modified.

The following is its original `README.md` file.

## Zephyr™ Mechanical Keyboard (ZMK) Firmware

[![Discord](https://img.shields.io/discord/719497620560543766)](https://zmk.dev/community/discord/invite)
[![Build](https://github.com/zmkfirmware/zmk/workflows/Build/badge.svg)](https://github.com/zmkfirmware/zmk/actions)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](CODE_OF_CONDUCT.md)

[ZMK Firmware](https://zmk.dev/) is an open source ([MIT](LICENSE)) keyboard firmware built on the [Zephyr™ Project](https://www.zephyrproject.org/) Real Time Operating System (RTOS). ZMK's goal is to provide a modern, wireless, and powerful firmware free of licensing issues.

Check out the website to learn more: https://zmk.dev/.

You can also come join our [ZMK Discord Server](https://zmk.dev/community/discord/invite).

To review features, check out the [feature overview](https://zmk.dev/docs/). ZMK is under active development, and new features are listed with the [enhancement label](https://github.com/zmkfirmware/zmk/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement) in GitHub. Please feel free to add 👍 to the issue description of any requests to upvote the feature.
