#include <HardwareSerial.h>
#include <string.h>
#include <stdio.h>

HardwareSerial GPS(2);

#define RX2 16
#define TX2 17
#define BUFFER_SIZE 512

volatile char rxBuffer[BUFFER_SIZE];
volatile int head = 0;
volatile int tail = 0;

char lineBuffer[120];
int lineIndex = 0;

// ===== datos =====
char fixTime[12], latitude[15], latNS[3];
char longitude[15], lonEW[3];
char fixQuality[2], numSats[4];
char HDOP[5], altitude[12];
char ageDGPS[10], speedKnots[10];

char imuHeading[6] = "0";
char imuRoll[6] = "0";
char imuPitch[6] = "0";
char imuYawRate[6] = "0";

bool nmeaReady = false;

// ================= INIT =================
void gpsInit() {
  GPS.begin(38400, SERIAL_8N1, RX2, TX2);
}

// ================= LOOP =================
void gpsUpdate() {
  readUART();
  processLines();
}

// ================= UART =================
void readUART() {
  while (GPS.available()) {
    char c = GPS.read();
    int next = (head + 1) % BUFFER_SIZE;

    if (next != tail) {
      rxBuffer[head] = c;
      head = next;
    }
  }
}

// ================= LINEAS =================
void processLines() {
  while (tail != head) {
    char c = rxBuffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;

    if (c == '\n') {
      lineBuffer[lineIndex] = '\0';
      parseLine(lineBuffer);
      lineIndex = 0;
    } else {
      if (lineIndex < sizeof(lineBuffer) - 1) {
        lineBuffer[lineIndex++] = c;
      }
    }
  }
}

// ================= PARSER =================
void parseLine(char *line) {

  if (strstr(line, "$GPGGA")) {
    sscanf(line, "$GPGGA,%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]",
           fixTime, latitude, latNS, longitude, lonEW,
           fixQuality, numSats, HDOP, altitude);

    nmeaReady = true;
  }

  else if (strstr(line, "$GPVTG")) {
    sscanf(line, "$GPVTG,%[^,],T,%*[^,],M,%[^,]",
           speedKnots, speedKnots);
  }
}

// ================= BUILD =================
char nmea[150];
const char* asciiHex = "0123456789ABCDEF";

bool gpsAvailable() {
  if (nmeaReady) {
    nmeaReady = false;
    return true;
  }
  return false;
}

char* gpsBuildNmea() {

  strcpy(nmea, "$PANDA,");

  strcat(nmea, fixTime); strcat(nmea, ",");
  strcat(nmea, latitude); strcat(nmea, ",");
  strcat(nmea, latNS); strcat(nmea, ",");
  strcat(nmea, longitude); strcat(nmea, ",");
  strcat(nmea, lonEW); strcat(nmea, ",");

  strcat(nmea, fixQuality); strcat(nmea, ",");
  strcat(nmea, numSats); strcat(nmea, ",");
  strcat(nmea, HDOP); strcat(nmea, ",");
  strcat(nmea, altitude); strcat(nmea, ",");
  strcat(nmea, ageDGPS); strcat(nmea, ",");

  strcat(nmea, speedKnots); strcat(nmea, ",");
  strcat(nmea, imuHeading); strcat(nmea, ",");
  strcat(nmea, imuRoll); strcat(nmea, ",");
  strcat(nmea, imuPitch); strcat(nmea, ",");
  strcat(nmea, imuYawRate);

  strcat(nmea, "*");

  calculateChecksum();

  strcat(nmea, "\r\n");

  return nmea;
}

// ================= CHECKSUM =================
void calculateChecksum() {
  int sum = 0;

  for (int i = 1; nmea[i] != '*' && nmea[i] != '\0'; i++) {
    sum ^= nmea[i];
  }

  char hex[3];
  hex[0] = asciiHex[(sum >> 4) & 0x0F];
  hex[1] = asciiHex[sum & 0x0F];
  hex[2] = '\0';

  strcat(nmea, hex);
}
