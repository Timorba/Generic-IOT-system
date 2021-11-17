#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <TimeLib.h>
#include "ArduinoLowPower.h"


Adafruit_AM2320 am2320 = Adafruit_AM2320();

//Variables
bool first_time;
bool RequestSent;
bool isBusy;
float value[20];
unsigned int start_time;
unsigned int send_again;
int timeout = 30 * 60 * 1000;
int timeToSendAgain = 2 * 1000;
int sleeptime;
long timestamp;
long t2; //t2 - the time data is sent
long t1 ; //t1 - the time we did the measurement


//Definitions
#define GATE_WAY_1 1   //define gateways
#define STACK_SIZE 10  //max num of msg to save (stack)
#define BUFF_SIZE 1024 //size of data per msg

//Data Structures
enum msg_type {AskForSend, Ack, Data, Error, Stop};

struct header_S
{
  byte sendRecieve;
  byte gatewayID;
  byte sensorID;
  msg_type msgType;
  long msgSize;
};

struct data_S
{
  float m_data1;
  float m_data2;
  long m_time;
};


void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  // hour, min, sec, day, month, year
  setTime( 9   , 57   , 0   , 13   , 10     , 2021); //Setting real date GMT -3hours for timestamp epoch approach
  pinMode(LED_BUILTIN, OUTPUT);
  start_time = millis();
  isBusy = true;
  first_time = true;
  while (!Serial) {
    delay(10); // hang out until serial port opens
  }
  am2320.begin();

}

void loop() {

  if (millis() - start_time > timeout)
  {
    t1 = millis();
    HandleSensorData();
    sendRequestToMaster();
    start_time = millis();
  }


  if (Serial1.available() > 0)
  {
    HandleMasterMsg();
  }


  if (RequestSent)
  {
    if (millis() - send_again > timeToSendAgain)
    {
      if (first_time)
      {
        delay(timeToSendAgain);
        Serial.println("Send request to the master again");
        sendRequestToMaster();
        first_time = false;
        send_again = millis();
      }
      else
      {
        Serial.println("Send request to the master again");
        sendRequestToMaster();
        send_again = millis();
      }
    }
  }


  if (!isBusy)//if the gateway is not Busy (isBusy='0') => sleep
  {
    sleeptime = (timeout - (t2 - t1) - 10);
    LowPower.deepSleep(sleeptime); //no interrupt->sleep
    isBusy = true;
  }

}


void HandleSensorData() {

  //sensor1:
  Serial.println("Getting data from the sensors");
  value[1] = am2320.readTemperature();
  value[2] = am2320.readHumidity();
  timestamp = now();
  Serial.print("temperature = " );
  Serial.println(value[1]);
  Serial.print("humidity = ");
  Serial.println(value[2]);
  Serial.print("timestamp= ");
  Serial.println(timestamp);


  digitalWrite(LED_BUILTIN, LOW);
  if (value[1] != value[1] | value[2] != value[2] )
  {
    Serial.println("You have an error on sensor number: 1");
    digitalWrite(LED_BUILTIN, HIGH);
    ErrorSender(1);
  }
  //Sensor2: test for more sensors
  //  value[3] = random(20, 90);
  //  Serial.print("pressure = " );
  //  Serial.println(value[3]);
}


void sendRequestToMaster()
{
  RequestSent = true;
  Serial.println("Sending request to the master");
  header_S hdrSend;
  hdrSend.sendRecieve = 1;
  hdrSend.gatewayID = GATE_WAY_1;
  hdrSend.sensorID = 0;
  hdrSend.msgType = AskForSend; // request
  hdrSend.msgSize = 0;
  SendMsg(&hdrSend, NULL); //send request msg
}


void HandleMasterMsg()
{
  header_S hdr;
  header_S hdrSend;
  byte *hdr_ptr;
  hdr_ptr = (byte *)&hdr;
  Serial.println("Recieved Msg from the master");
  Serial1.readBytes(hdr_ptr, sizeof(header_S));
  switch (hdr.msgType)
  {
    case Ack:
      Serial.println("Recieved ACK from the master");
      RequestSent = false;
      DataSender();
      t2 = millis();
      isBusy = false;
      break;
  }
}


void SendMsg(struct header_S* header, struct data_S* buff)
{
  //make header according to msgType and gatewayID
  // add data if needed
  // send message
  int sizeOfBuffer = header->msgSize; //size of the msg
  int sizeOfHeader = sizeof(header_S);

  // send haeder

  Serial1.write((byte*)header, sizeOfHeader);

  if (buff != NULL)
  {
    int sizeOfData = sizeof(data_S);
    Serial.println("Sending data to the master");
    Serial1.write((byte*)buff, sizeOfData);
  }
}

void DataSender()
{
  //sensor 1 :
  header_S hdrSend; //building header
  hdrSend.sendRecieve = 1;
  hdrSend.gatewayID = GATE_WAY_1;
  hdrSend.sensorID = 1;
  hdrSend.msgType = Data;
  hdrSend.msgSize = sizeof(data_S);
  data_S d; //building the data
  d.m_data1 = value[1];
  d.m_data2 = value[2];
  d.m_time = timestamp;
  SendMsg(&hdrSend, &d);

  delay(100);
  //senor 2 : //test for more sensors

  //  header_S hdrSend2; //building header
  //  hdrSend2.sendRecieve = 1;
  //  hdrSend2.gatewayID = GATE_WAY_1;
  //  hdrSend2.sensorID = 2;
  //  hdrSend2.msgType = Data;
  //  hdrSend2.msgSize = sizeof(data_S);
  //  data_S d2; //building the data
  //  d2.m_data1 = value[3];
  //  SendMsg(&hdrSend2, &d2);
}

void ErrorSender(byte sensorID)
{
  header_S hdrSend;
  hdrSend.sendRecieve = 1;
  hdrSend.gatewayID = GATE_WAY_1;
  hdrSend.sensorID = sensorID;
  hdrSend.msgType = Error; // Error
  hdrSend.msgSize = 0;
  SendMsg(&hdrSend, NULL); //send Error msg
}
