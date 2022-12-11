/*
   Project: Arduino GSM GPS Device Tracker
   Board: Maker Uno
   GSM: SIM7600E
   GPS: SIM7600E
   Library:
   - Adafruit FONA
*/

#include "Adafruit_FONA.h"

#define BUTTON  2
#define BUZZER  8
#define LED11   11
#define LED12   12
#define LED13   13

#define GSM_RX  4
#define GSM_TX  3
#define GSM_PWR 2
#define GSM_RST 20 // Dummy
#define GSM_BAUD 9600

char replybuffer[255];

Adafruit_FONA SIM7600 = Adafruit_FONA(GSM_RST);

//#define NOTE_G3  196
//#define NOTE_C4  262
//#define NOTE_E4  330
//#define NOTE_F4  349
//#define NOTE_G4  392
//#define NOTE_C5  523
//#define NOTE_D5  587
//#define NOTE_G5  784
//
//#define playStartMelody() playTone(melody1, melody1Dur, 2)
//#define playReadyMelody() playTone(melody2, melody2Dur, 5)
//#define playErrorMelody() playTone(melody3, melody3Dur, 2)
//#define playSmsMelody() playTone(melody4, melody4Dur, 5)
//#define playButtonMelody() playTone(melody5, melody5Dur, 4)
//
//int melody1[] = {NOTE_C4, NOTE_G4};
//int melody1Dur[] = {12, 8};
//int melody2[] = {NOTE_C4, NOTE_F4, NOTE_G4, NOTE_E4, NOTE_C5};
//int melody2Dur[] = {6, 16, 6, 6, 3};
//int melody3[] = {NOTE_C4, NOTE_G3};
//int melody3Dur[] = {6, 3};
//int melody4[] = {NOTE_G4, NOTE_C5, 0, NOTE_G4, NOTE_C5};
//int melody4Dur[] = {12, 12, 12, 12, 12};
//int melody5[] = {NOTE_C5, NOTE_G5, NOTE_C5, NOTE_D5};
//int melody5Dur[] = {6, 6, 6, 6};

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
char SIM7600InBuffer[64]; // For notifications from the FONA
char callerIDbuffer[32]; // We'll store the SMS sender number in here
char SMSbuffer[32]; // We'll store the SMS content in here
uint16_t SMSLength;
String SMSString = "";

boolean buttonEnable = false;
String SMSSendString = "";
char SMSSendBuffer[100];
float Lat = 0;
float Log = 0;

void setup()
{
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED11, OUTPUT);
  pinMode(LED12, OUTPUT);
  pinMode(LED13, OUTPUT);

  delay(2000);
  pinMode(GSM_PWR, OUTPUT);
  delay(1000);
  pinMode(GSM_PWR, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Interfacing SIM7600 GSM GPS Module with Maker UNO");
  Serial.println("Initializing... (May take a minute)");

  //  playStartMelody();
  delay(15000);

  // Make it slow so its easy to read!
  Serial1.begin(GSM_BAUD);
  if (!SIM7600.begin(*Serial1)) {
    Serial.println("Couldn't find SIM7600");
    digitalWrite(LED11, HIGH);
    //    playErrorMelody();
    while (1);
  }
  Serial.println(F("SIM7600 is OK"));

  // Print SIM card IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = SIM7600.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }

  Serial1.print("AT+CNMI=2,1\r\n");  // Set up the SIM800L to send a +CMTI notification when an SMS is received
  Serial.println("GSM is ready!");
  digitalWrite(LED12, HIGH);

  while (!GPSPositioning()); // Wait until GPS get signal
  Serial.println("GPS is ready!");

  digitalWrite(LED13, HIGH);
  //  playReadyMelody();
}

void loop()
{
  char* bufPtr = SIM7600InBuffer; // Handy buffer pointer

  if (SIM7600.available()) { // Any data available from the SIM800L
    int slot = 0; // This will be the slot number of the SMS
    int charCount = 0;

    // Read the notification into fonaInBuffer
    do {
      *bufPtr = SIM7600.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (SIM7600.available()) && (++charCount < (sizeof(SIM7600InBuffer) - 1)));

    // Add a terminal NULL to the notification string
    *bufPtr = 0;

    // Scan the notification string for an SMS received notification.
    // If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(SIM7600InBuffer, "+CMTI: \"SM\",%d", &slot)) {
      //      playSmsMelody();
      Serial.print("slot: "); Serial.println(slot);

      // Retrieve SMS sender address/phone number.
      if (!SIM7600.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print("FROM: "); Serial.println(callerIDbuffer);

      if (!SIM7600.readSMS(slot, SMSbuffer, 250, &SMSLength)) { // pass in buffer and max len!
        Serial.println("Failed!");
      }
      else {
        SMSString = String(SMSbuffer);
        Serial.print("SMS: "); Serial.println(SMSString);
      }

      // Compare SMS string
      if (SMSString == "LOCATION") {
        buttonEnable = true;

        Serial.print("Requesting device location...");
        GPSPositioning();

        delay(100);

        SMSSendString = "Google Maps:\nhttp://www.google.com/maps/place/" + String(Lat, 6) + "," + String(Log, 6);
        Serial.println(SMSSendString);

        SMSSendString.toCharArray(SMSSendBuffer, 100);
        // Send SMS for status
        if (!SIM7600.sendSMS(callerIDbuffer, SMSSendBuffer)) {
          Serial.println("Failed");
        }
        else {
          Serial.println(F("Sent!"));
          SMSSendString = "";
        }
      }
      else {
        Serial.print("Invalid command.");
        //        playErrorMelody();
      }

      // Delete the original msg after it is processed
      // otherwise, we will fill up all the slots
      // and then we won't be able to receive SMS anymore
      while (1) {
        boolean deleteSMSDone = SIM7600.deleteSMS(slot);
        if (deleteSMSDone == true) {
          Serial.println("OK!");
          break;
        }
        else {
          Serial.println("Couldn't delete, try again.");
        }
      }
    }
  }

  if (digitalRead(BUTTON) == LOW && buttonEnable == true) {
//    buttonEnable = false;
//    playButtonMelody();

    Serial.print("Requesting device location...");
    GPSPositioning();

    delay(100);

    SMSSendString = "Google Maps:\nhttp://www.google.com/maps/place/" + String(Lat, 6) + "," + String(Log, 6);
    Serial.println(SMSSendString);

    SMSSendString.toCharArray(SMSSendBuffer, 100);
    // Send SMS for status
    if (!SIM7600.sendSMS(callerIDbuffer, SMSSendBuffer)) {
      Serial.println("Failed");
    }
    else {
      Serial.println(F("Sent!"));
      SMSSendString = "";
    }
  }
}

//void playTone(int *melody, int *melodyDur, int notesLength)
//{
//  for (int i = 0; i < notesLength; i++) {
//    int noteDuration = 1000 / melodyDur[i];
//    tone(BUZZER, melody[i], noteDuration);
//    delay(noteDuration);
//    noTone(BUZZER);
//  }
//}

uint8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout)
{
  uint8_t x = 0, answer = 0;
  char response[100];
  unsigned long previous;

  memset(response, '\0', 100); // Initialize the string

  delay(100);

  while (Serial1.available() > 0) { // Clean the input buffer
    Serial1.read();
  }

  Serial1.println(ATcommand);    // Send the AT command

  x = 0;
  previous = millis();

  // This loop waits for the answer
  do {
    if (Serial1.available() != 0) {
      // if there are data in the UART input buffer, reads it and checks for the answer
      response[x] = Serial1->read();
      Serial.print(response[x]);
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer) != NULL) {
        answer = 1;
      }
    }
    // Waits for the asnwer with time out
  } while ((answer == 0) && ((millis() - previous) < timeout));

  // Serial1->print("\n");
  return answer;
}

bool GPSPositioning()
{
  uint8_t answer = 0;
  bool RecNull = true;
  int i = 0;
  char RecMessage[200];
  char LatDD[3], LatMM[10], LogDD[4], LogMM[10], DdMmYy[7] , UTCTime[7];
  int DayMonthYear;
  Lat = 0;
  Log = 0;

  memset(RecMessage, '\0', 200); // Initialize the string
  memset(LatDD, '\0', 3); // Initialize the string
  memset(LatMM, '\0', 10); // Initialize the string
  memset(LogDD, '\0', 4); // Initialize the string
  memset(LogMM, '\0', 10); // Initialize the string
  memset(DdMmYy, '\0', 7); // Initialize the string
  memset(UTCTime, '\0', 7); // Initialize the string

  Serial.print("Start GPS session...\n");
  sendATcommand("AT+CGPS=1,1", "OK", 1000); // start GPS session, standalone mode

  delay(2000);

  while (RecNull) {
    answer = sendATcommand("AT+CGPSINFO", "+CGPSINFO: ", 1000); // start GPS session, standalone mode

    if (answer == 1) {
      answer = 0;
      while (Serial1.available() == 0);
      // this loop reads the data of the GPS
      do {
        // if there are data in the UART input buffer, reads it and checks for the asnwer
        if (Serial1.available() > 0) {
          RecMessage[i] = Serial1->read();
          i++;
          // check if the desired answer (OK) is in the response of the module
          if (strstr(RecMessage, "OK") != NULL) {
            answer = 1;
          }
        }
      } while (answer == 0);   // Waits for the asnwer with time out

      RecMessage[i] = '\0';
      Serial.print(RecMessage);
      Serial.print("\n");

      if (strstr(RecMessage, ",,,,,,,,") != NULL) {
        memset(RecMessage, '\0', 200);    // Initialize the string
        i = 0;
        answer = 0;
        delay(1000);
      }
      else {
        RecNull = false;
        sendATcommand("AT+CGPS=0", "OK:", 1000);
      }
    }
    else {
      Serial.print("error \n");
      return false;
    }
    delay(2000);
  }

  strncpy(LatDD, RecMessage, 2);
  LatDD[2] = '\0';

  strncpy(LatMM, RecMessage + 2, 9);
  LatMM[9] = '\0';

  Lat = atoi(LatDD) + (atof(LatMM) / 60);
  if (RecMessage[12] == 'N') {
    Serial.print("Latitude is ");
    Serial.print(Lat);
    Serial.print(" N\n");
  }
  else if (RecMessage[12] == 'S') {
    Serial.print("Latitude is ");
    Serial.print(Lat);
    Serial.print(" S\n");
  }
  else {
    return false;
  }

  strncpy(LogDD, RecMessage + 14, 3);
  LogDD[3] = '\0';

  strncpy(LogMM, RecMessage + 17, 9);
  LogMM[9] = '\0';

  Log = atoi(LogDD) + (atof(LogMM) / 60);
  if (RecMessage[27] == 'E') {
    Serial.print("Longitude is ");
    Serial.print(Log);
    Serial.print(" E\n");
  }
  else if (RecMessage[27] == 'W') {
    Serial.print("Latitude is ");
    Serial.print(Lat);
    Serial.print(" W\n");
  }
  else {
    return false;
  }

  strncpy(DdMmYy, RecMessage + 29, 6);
  DdMmYy[6] = '\0';
  Serial.print("Day Month Year is ");
  Serial.print(DdMmYy);
  Serial.print("\n");

  strncpy(UTCTime, RecMessage + 36, 6);
  UTCTime[6] = '\0';
  Serial.print("UTC time is ");
  Serial.print(UTCTime);
  Serial.print("\n");

  return true;
}
