# STK500 Developer Board

<p align="center">
  <img src="Graphics/STK500_Developer_Board.png" alt="STK500 Developer Board" width="800">
</p>

## Overview

The **STK500 Developer Board** is a custom development platform based on the **ATmega2560 microcontroller**, created during the **PCB Design Workflow** course at the School of Electrical and Electronic Engineering (EIEE), Universidad del Valle.

The project was developed by the IEEE Student Branch CAS Technical Chapter with the objective of demonstrating the complete PCB design workflow and encouraging students to create their own custom electronic hardware solutions.

This repository contains the complete hardware design files, software examples, libraries, and documentation required to understand, use, and modify the board.

---

## Hardware Features

### Microcontroller
- ATmega2560 running at 16 MHz
- 256 KB Flash Memory
- 8 KB SRAM
- 4 KB EEPROM
- 4 USART interfaces
- SPI communication interface

### I/O Resources
- 50 Digital GPIOs
- 16 Analog Inputs
- 13 PWM Outputs

### Integrated Peripherals
- 4x3 Keypad
- 16x2 LCD Display
- LM35 Temperature Sensor
- Two onboard potentiometers
- Status LED
- 5V and 3.3V power outputs

### Power Supply Options
- USB connection
- External DC adapter

---

## Applications

The STK500 Developer Board is suitable for:

- Embedded Systems Education
- Robotics
- Human-Machine Interfaces (HMI)
- Rapid Prototyping
- Electronics Laboratories
- Microcontroller Programming Courses

---

## Repository Structure

```text
STK500_Dev_Board
│
├── Graphics/
│   └── Board images, renders and visual resources
│
├── Documentation/
│   └── User manuals and reference documents
│
├── Libraries/
│   ├── lcd.c
│   └── lcd.h
│
├── Scripts/
│   ├── BOARD_TEST_CODE/
│   └── LED_TEST_CODE/
│
├── Project_Files/
│   ├── Schematic files
│   ├── PCB layout files
│   └── Manufacturing resources
│
└── README.md
