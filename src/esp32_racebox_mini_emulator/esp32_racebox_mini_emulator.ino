// This code is an evolution of work done by Anchit Chandra Sekhart
// https://github.com/anchit92/Open-Source-RaceBox-mini-Emulator
//
// Settings live in config.h
// Hardware & protocol logic lives in the ble, gnss, imu, and telemetry modules
#include "ble.h"
#include "gnss.h"
#include "imu.h"
#include "telemetry.h"

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 RaceBox Mini Emulator starting up...");

  gnssBegin();
  imuBegin();
  imuCalibrate();
  bleBegin();
  telemetryBegin();
}

void loop() {
  gnssCheck();
  imuUpdate();
  telemetryUpdateLed();
  telemetrySendPacketIfReady();
  telemetryReport();
  bleUpdate();
}
