#pragma once
#include <Arduino.h>

constexpr uint8_t lidar_mot_ctl_pin = 11;
constexpr uint8_t lidar_rx_pin = 12;
constexpr uint8_t lidar_tx_pin = 13;

// Encodeurs quadrature (un canal A/B par roue).
constexpr uint8_t encoderLeftA  = 15;
constexpr uint8_t encoderLeftB  = 16;
// TODO: pins provisoires, a confirmer avec le cablage reel de la roue droite.
constexpr uint8_t encoderRightA = 17;
constexpr uint8_t encoderRightB = 18;

#define BIN1 39
#define BIN2 40
#define BPWM 41

#define AIN1 37
#define AIN2 36
#define APWM 35

#define STBY 38