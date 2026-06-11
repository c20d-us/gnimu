#include "gnss.h"
#include "config.h"

// --- GNSS state — private to this file ---
static SFE_UBLOX_GNSS myGNSS;
static HardwareSerial GPS_Serial(2);

static void resetGnssBaudRate() {
  Serial.println("Attempting to set Correct Baud Rate");
  GPS_Serial.begin(FACTORY_GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  delay(500);

  if (!myGNSS.begin(GPS_Serial)) {
    Serial.print("u-blox GNSS not detected at ");
    Serial.print(FACTORY_GNSS_BAUD);
    Serial.println(" baud.");
    Serial.print("u-blox GNSS not detected, Check documentation for factory "
                 "baud rate and/or check your wiring");
    while (1)
      delay(100);
  } else {
    Serial.print("GNSS detected at ");
    Serial.print(FACTORY_GNSS_BAUD);
    Serial.println(" baud!");
  }
  delay(500);

  // Now switch baud rate
  Serial.print("Setting baud rate to ");
  Serial.print(GNSS_BAUD);
  Serial.println("...");
  myGNSS.setSerialRate(GNSS_BAUD);
  Serial.print("Baud rate changed to ");
  Serial.println(GNSS_BAUD);

  GPS_Serial.end();
  delay(100);
  // Re-initialize the serial port at the new baud rate
  GPS_Serial.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  delay(500);

  if (!myGNSS.begin(GPS_Serial)) {
    Serial.print("GNSS not detected at ");
    Serial.print(GNSS_BAUD);
    Serial.println(" baud.");
    Serial.print("u-blox GNSS not detected, Check documentation for factory "
                 "baud rate and/or check your wiring");
    while (1)
      delay(100);
  }
  Serial.print("GNSS detected at ");
  Serial.print(GNSS_BAUD);
  Serial.println(" baud! Saving to Flash");
  myGNSS.saveConfiguration(); // Save to flash
  GPS_Serial.end();
}

void gnssBegin() {
  GPS_Serial.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  if (!myGNSS.begin(GPS_Serial)) {
    Serial.println("❌ GNSS not detected. Attempting to configure.");
    GPS_Serial.end();
    resetGnssBaudRate();
    GPS_Serial.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  }

  // Set GNSS output to PVT only
  myGNSS.setAutoPVT(true);
  myGNSS.setDynamicModel(GNSS_DYNAMIC_MODEL);
  // --- Configure GPS update rate to MAX_NAVIGATION_RATE Hz ---
  if (myGNSS.setNavigationFrequency(MAX_NAVIGATION_RATE)) {
    Serial.printf("✅ GNSS update rate set to %d Hz.\n", MAX_NAVIGATION_RATE);
  } else {
    Serial.println("❌ Failed to set GNSS update rate.");
  }

// --- GNSS Constellation Setup ---

// GPS
#ifdef ENABLE_GNSS_GPS
  if (myGNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_GPS)) {
    Serial.println("✅ GPS enabled.");
  } else {
    Serial.println("❌ Failed to enable GPS.");
  }
#else
  myGNSS.enableGNSS(false, SFE_UBLOX_GNSS_ID_GPS);
  Serial.println("🚫 GPS disabled.");
#endif

// Galileo
#ifdef ENABLE_GNSS_GALILEO
  if (myGNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_GALILEO)) {
    Serial.println("✅ Galileo enabled.");
  } else {
    Serial.println("❌ Failed to enable Galileo.");
  }
#else
  myGNSS.enableGNSS(false, SFE_UBLOX_GNSS_ID_GALILEO);
  Serial.println("🚫 Galileo disabled.");
#endif

// Optional: GLONASS
#ifdef ENABLE_GNSS_GLONASS
  if (myGNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_GLONASS)) {
    Serial.println("✅ GLONASS enabled.");
  } else {
    Serial.println("❌ Failed to enable GLONASS.");
  }
#else
  myGNSS.enableGNSS(false, SFE_UBLOX_GNSS_ID_GLONASS);
  Serial.println("🚫 GLONASS disabled.");
#endif

// Optional: BeiDou
#ifdef ENABLE_GNSS_BEIDOU
  if (myGNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_BEIDOU)) {
    Serial.println("✅ BEIDOU enabled.");
  } else {
    Serial.println("❌ Failed to enable BEIDOU.");
  }
#else
  myGNSS.enableGNSS(false, SFE_UBLOX_GNSS_ID_BEIDOU);
  Serial.println("🚫 BEIDOU disabled.");
#endif

// Optional: QZSS
#ifdef ENABLE_GNSS_QZSS
  if (myGNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_QZSS)) {
    Serial.println("✅ QZSS enabled.");
  } else {
    Serial.println("❌ Failed to enable QZSS.");
  }
#else
  myGNSS.enableGNSS(false, SFE_UBLOX_GNSS_ID_QZSS);
  Serial.println("🚫 QZSS disabled.");
#endif

// Optional: SBAS (satellite-based augmentation)
#ifdef ENABLE_GNSS_SBAS
  if (myGNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_SBAS)) {
    Serial.println("✅ SBAS enabled.");
  } else {
    Serial.println("❌ Failed to enable SBAS.");
  }
#else
  myGNSS.enableGNSS(false, SFE_UBLOX_GNSS_ID_SBAS);
  Serial.println("🚫 SBAS disabled.");
#endif
}

void gnssCheck() { myGNSS.checkUblox(); }

bool gnssHasNewEpoch() {
  if (!(myGNSS.getPVT() && myGNSS.packetUBXNAVPVT != nullptr))
    return false;

  static uint32_t lastITOW = 0;
  uint32_t currentITOW = myGNSS.packetUBXNAVPVT->data.iTOW;
  if (currentITOW == lastITOW)
    return false; // Same epoch — nothing new
  lastITOW = currentITOW;
  return true;
}

const UBX_NAV_PVT_data_t *gnssLatestPvt() {
  return myGNSS.packetUBXNAVPVT ? &myGNSS.packetUBXNAVPVT->data : nullptr;
}

bool gnssHeadingValid() { return myGNSS.getHeadVehValid(); }
