#pragma once
#include <Arduino.h>

// ============================================================================
// Telemetry module - application glue
//
// Turns the latest GNSS + IMU data into RaceBox packets, sends them over BLE,
// and prints periodic serial stats. Consumes the imu / gnss / ble / ubx_helpers
// module interfaces; owns no hardware itself.
// ============================================================================

// Start the stats-report timer. Call once in setup(), after the other modules
// are up.
void telemetryBegin();

// On each new GNSS epoch, count it and - when a client is connected - build and
// send an 88-byte RaceBox Data Message over BLE.
void telemetrySendPacketIfReady();

// Periodically print packet-rate and GNSS/IMU debug stats over serial.
void telemetryReport();
