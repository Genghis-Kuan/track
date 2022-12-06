//code use for KC868-A8S board

const char phone[] = "0210540840";  //replace for your mobile phone number

int data;
String message;
unsigned long int last;

void setup()
{

  Serial.begin(115200);
  Serial1.begin(115200);


  Serial.print("start test");
  //check();
  //send_sms("testing");
  GPS_ping();
}

void loop()
{
  check_sms_responce();
}

void check()
{
  Serial1.print("AT\r");  //Set text mode
  delay(1000);
}

void GPS_ping ()
{
  uint8_t answer = 1;
  bool RecNull = true;
  int i = 0;
  char RecMessage[200];
  char LatDD[3], LatMM[10], LogDD[4], LogMM[10], DdMmYy[7] , UTCTime[7];
  int DayMonthYear;

  Serial1.print("AT+CGPS=1,1\r");  //Set GPS Mode
  delay(1000);
  while (RecNull) {
    Serial1.print("AT+CGPSINFO\r");  //Set GPS Mode

    if (answer == 1) {
      answer = 0;
      while (Serial1.available() == 0);
      // this loop reads the data of the GPS
      do {
        // if there are data in the UART input buffer, reads it and checks for the asnwer
        if (Serial1.available() > 0) {
          RecMessage[i] = Serial1.read();
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
        Serial1.print("AT+CGPS=0\r");  //Set GPS Mode
      }
    }
    else {
      Serial.print("error \n");
      return false;
    }
    delay(2000);
  }
}


void send_sms ( String sms)
{
  Serial1.print("AT+CMGF=1\r");  //Set text mode
  delay(1000);
  Serial1.print("AT+CMGS=\"" + String(phone) + "\"\r"); //Send message
  delay(1000);
  Serial1.print(sms);//Text message
  Serial1.println((char)0x1A); //Ctrl+Z
}

void check_sms_responce()
{
  if (Serial1.available() > 0)
  {
    delay(60);
    message = "";
    while (Serial1.available())
    {
      message += (char)Serial1.read();
    }
    Serial.print(message);
  }
}
