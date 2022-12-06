const char phone[] = "0210540840";  //replace for your mobile phone number

int data;
String message;
unsigned long int last;

void setup()
{

 Serial.begin(115200); 
 Serial1.begin(115200);
  
  
  Serial.print("start test");

  //send_sms("Don't Cry");
  GPS_ping();
}

void loop()
{
 check_sms_responce();
 delay(1000);
 //GPS_ping();
}

void GPS_ping ()
{
      Serial1.print("AT+CGPS=1,1\r");  //Set GPS Mode
      delay(1000);
      Serial1.print("AT+CGPSINFO\r"); //Send message
      delay(1000);
      Serial1.print("AT+CGPS=0\r"); //Send message
}


void send_sms ( String sms)
{
      Serial1.print("AT+CMGF=1\r");  //Set text mode
      delay(1000);
      Serial1.print("AT+CMGS=\""+String(phone)+"\"\r"); //Send message
      delay(1000);
      Serial1.print(sms);//Text message
      Serial1.println((char)0x1A); //Ctrl+Z
}

void check_sms_responce()
{
   if(Serial1.available()>0)
  {
    delay(60);
    message="";
    while(Serial1.available())
    {
      message+=(char)Serial1.read();
    }
    Serial.print(message);
  }
}
