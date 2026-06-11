#include "imu.h"
#include "config.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- IMU state — private to this file (file-scope `static`) ---
static Adafruit_MPU6050 myIMU;

// Storage for the filtered values
static float filteredAx = 0, filteredAy = 0, filteredAz = 0;
static float filteredGx = 0, filteredGy = 0, filteredGz = 0;

// Gyro bias offsets — measured at startup, subtracted from every reading.
// Initialized to 0.0 so they are safe no-ops when calibration is disabled.
static float gyroBiasX = 0.0f, gyroBiasY = 0.0f, gyroBiasZ = 0.0f;

#ifdef GYRO_CALIBRATION_ENABLED
static void calibrateGyro() {
  Serial.println("⏳ Gyro calibration starting — keep device still...");
  double sumX = 0, sumY = 0, sumZ = 0;
  sensors_event_t a, g, temp;
  for (int i = 0; i < GYRO_CALIBRATION_SAMPLES; i++) {
    myIMU.getEvent(&a, &g, &temp);
    sumX += g.gyro.x;
    sumY += g.gyro.y;
    sumZ += g.gyro.z;
    delay(ACCEL_SAMPLE_INTERVAL_MS);
  }
  gyroBiasX = sumX / GYRO_CALIBRATION_SAMPLES;
  gyroBiasY = sumY / GYRO_CALIBRATION_SAMPLES;
  gyroBiasZ = sumZ / GYRO_CALIBRATION_SAMPLES;
  Serial.printf(
      "✅ Gyro calibration complete. Bias: X=%.4f Y=%.4f Z=%.4f rad/s\n",
      gyroBiasX, gyroBiasY, gyroBiasZ);
}
#endif // GYRO_CALIBRATION_ENABLED

void imuBegin() {
  if (!myIMU.begin()) {
    Serial.println("❌ Failed to find MPU6050 chip");
    while (1)
      delay(100);
  } else {
    Serial.println("✅ IMU Acceleromter/Gyro enabled.");
  }
  myIMU.setAccelerometerRange(ACCEL_RANGE);
  myIMU.setGyroRange(GYRO_RANGE);
  myIMU.setFilterBandwidth(FILTER_BANDWIDTH);
  sensors_event_t a, g, temp;
  myIMU.getEvent(&a, &g, &temp);

  // Initialize filters with the first real reading so they don't start at zero
  filteredAx = a.acceleration.x;
  filteredAy = a.acceleration.y;
  filteredAz = a.acceleration.z;
}

void imuCalibrate() {
#ifdef GYRO_CALIBRATION_ENABLED
  // Do this late in setup() so that the device has time to be settled prior.
  // Doing it sooner might catch small movements from being plugged in.
  // To be safe, throw in an extra half-second of wait time for settling.
  delay(500);
  calibrateGyro();
#endif // GYRO_CALIBRATION_ENABLED
}

void imuUpdate() {
  static unsigned long lastAccelReadMs = 0;
  // Update Accelerometer readings at fixed interval
  if (millis() - lastAccelReadMs >= ACCEL_SAMPLE_INTERVAL_MS) {
    lastAccelReadMs = millis();
    sensors_event_t a, g, temp;
    myIMU.getEvent(&a, &g, &temp);

    // Apply Exponential Moving Average (Complementary Filter logic)
    filteredAx =
        (ACCEL_ALPHA * a.acceleration.x) + ((1.0 - ACCEL_ALPHA) * filteredAx);
    filteredAy =
        (ACCEL_ALPHA * a.acceleration.y) + ((1.0 - ACCEL_ALPHA) * filteredAy);
    filteredAz =
        (ACCEL_ALPHA * a.acceleration.z) + ((1.0 - ACCEL_ALPHA) * filteredAz);

    filteredGx = (GYRO_ALPHA * (g.gyro.x - gyroBiasX)) +
                 ((1.0 - GYRO_ALPHA) * filteredGx);
    filteredGy = (GYRO_ALPHA * (g.gyro.y - gyroBiasY)) +
                 ((1.0 - GYRO_ALPHA) * filteredGy);
    filteredGz = (GYRO_ALPHA * (g.gyro.z - gyroBiasZ)) +
                 ((1.0 - GYRO_ALPHA) * filteredGz);
  }
}

ImuProtocolUnits imuReadProtocolUnits() {
  ImuProtocolUnits u;
  // Convert accelerometer to milli-g (1g = 9.80665 m/s^2)
  u.gX = filteredAx * 1000.0 / 9.80665;
  u.gY = filteredAy * 1000.0 / 9.80665;
  u.gZ = filteredAz * 1000.0 / 9.80665;
  // Convert gyro to centi-deg/sec
  u.rX = filteredGx * 180.0 / M_PI * 100.0;
  u.rY = filteredGy * 180.0 / M_PI * 100.0;
  u.rZ = filteredGz * 180.0 / M_PI * 100.0;
  return u;
}
