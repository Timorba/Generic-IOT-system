#include <MKRGSM.h>
#include <ArduinoMqttClient.h>//this lib(ArduinoMqttClient by Arduino) must be installed via Libarary manager
#include <ArduinoJson.h>//this lib(Arduino_json by Arduino) must be installed via Libarary manager
#include <TimeLib.h>
#include "ArduinoLowPower.h"


//GSM  Authoriasition
const char pin[]      = "";
const char apn[]      = "";
const char login[]    = "";
const char password[] = "";

//Cloud Authoriasition
String TOKEN ;        // Endpoint token - you get (or specify) it during device provisioning
String APP_VERSION  ;  // Application version - you specify it during device provisioning
String mqtt_server = "mqtt.cloud.kaaiot.com";

//GSM Client Configuration
GSMClient net;
GPRS gprs;
GSM gsmAccess;
GSM_SMS sms;
MqttClient mqttClient(net);

//Cloud Configuration
String configurationUpdatesTopic;
String configurationRequestTopic;
String configurationResponceTopic;
String metricsPublishTopic;

//Variables
bool GotMsg;
bool isBusy;
bool dataRecieved;
unsigned int start_time;
unsigned int check_time;
int timeout = 30 * 60 * 1000;
int Errorindx ;
int sendagain;
byte currentGatewayIDHandle;

//arrays
float value[10][20];
long timestamp[10];
int Data_indicator[10][20];
int Gateway_indicator[9];
String sensortypes[10][20] = {"temperature", "humidity", "pressure", "Soil"};
String tokens[10] = {"token2", ""};
String  App_version[10] = {"c2tneiegul2q7quk38b0-dev1", ""};



//definitions
#pragma pack(1)
#define GATE_WAY_1 1   //define gateways
#define STACK_SIZE 10  //max num of msg to save (stack)
#define BUFF_SIZE 1024 //size of data per msg

//Data Structure
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
  check_time = millis();
  dataRecieved = false;
  GotMsg = true;
  currentGatewayIDHandle = 0;
  Errorindx = 0;
  delay(15000);
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Master is running...");
}


void loop() {

  while (millis() - check_time < 1200)
  {
    if (Serial1.available() > 0)
    {
      GotMsg = true;
      HandleGatewayMsg();
    }

    if (dataRecieved)
    {
      if (millis() - start_time > timeout)
      {
        SendMsgToCloud();
        start_time = millis();
        dataRecieved = false;
      }
    }
  }

  check_time = millis();

  if (!GotMsg)
  {
    isBusy = false;
    if (!isBusy)
    {
      gsmAccess.shutdown();
      LowPower.deepSleep(12000);
      isBusy = true;
    }
  }
}


void HandleGatewayMsg()
{
  header_S hdr;
  byte *hdr_ptr;
  hdr_ptr = (byte *)&hdr;
  Serial1.readBytes(hdr_ptr, sizeof(header_S));
  Serial.println("Msg recieved from gateway number: " + String(hdr.gatewayID) + " MsgType: " + String(hdr.msgType) );
  if ((currentGatewayIDHandle == 0) || (hdr.gatewayID == currentGatewayIDHandle))
  {
    switch (hdr.msgType)
    {
      case AskForSend:
        Serial.println("Recieved request from gateway number: " + String(hdr.gatewayID));
        currentGatewayIDHandle = hdr.gatewayID;
        header_S hdrSend;
        hdrSend.sendRecieve = 0;
        hdrSend.gatewayID = hdr.gatewayID;
        hdrSend.sensorID = 0;
        hdrSend.msgType = Ack;
        hdrSend.msgSize = 0;
        SendMsg(&hdrSend, NULL);
        Serial.println("Send ACK to gateway number: " + String(hdr.gatewayID));
        break;

      case Data:
        currentGatewayIDHandle = 0;
        switch (hdr.gatewayID)
        {
          case 1 :
            Gateway_indicator[0] = 1;
            switch (hdr.sensorID)
            {
              case 1 :
                {
                  Serial.println("Recieved data from gateway number: " + String(hdr.gatewayID) + " Sensor number: " + String(hdr.sensorID));
                  data_S data;
                  byte *data_ptr;
                  data_ptr = (byte *)&data;
                  Serial1.readBytes(data_ptr, sizeof(data_S));
                  value[0][0] = data.m_data1 ;
                  value[0][1] =  data.m_data2;
                  timestamp[0] = data.m_time;
                  Data_indicator[0][0] = 1;
                  Data_indicator[0][1] = 1;
                  Serial.print(String(sensortypes[0][0]) + " = ");
                  Serial.println(data.m_data1);
                  Serial.print(String(sensortypes[0][1]) + " = ");
                  Serial.println(data.m_data2);
                  Serial.print("timestamp = ");
                  Serial.println(data.m_time);
                  dataRecieved = true;

                  GotMsg = false;
                  if ((data.m_data1 != data.m_data1 | data.m_data2 != data.m_data2) == false )
                  {
                    pinMode(LED_BUILTIN, LOW);
                  }
                  break;
                }

              case 2 :
                {
                  // test for more sensors:
                  //                  Serial.println("Recieved data from gateway number: " + String(hdr.gatewayID) + " Sensor number: " + String(hdr.sensorID));
                  //                  data_S data;
                  //                  byte *data_ptr;
                  //                  data_ptr = (byte *)&data;
                  //                  Serial1.readBytes(data_ptr, sizeof(data_S));
                  //                  value[0][2] = data.m_data1;
                  //                  Serial.print(String(sensortypes[0][2]) + " = ");
                  //                  Serial.println(data.m_data1);
                  //                  Serial.print("timestamp = ");
                  //                  Serial.println(timestamp[0]);
                  //                  //Data_indicator[0][2]=1;
                  //                  dataRecieved = true;
                  break;
                }

                break;
            }

          case 2 :

            break;
        }
        break;


      case Error:
        Serial.println("We have an error on sensor number: " + String(hdr.sensorID) + "  on gateway number: " + String(hdr.gatewayID));
        digitalWrite(LED_BUILTIN, HIGH);
        Errorindx++;
        if (Errorindx == 5)
        {

          ConnectingtoGSM();
          Serial.println("Sending SMS after 5 errors in a row");
          char remoteNum[20] = "" ;
          String txtMsg = "You have problem with Sensor number : " + String(hdr.sensorID) + "  on Gateway : " + String(hdr.gatewayID);
          sms.beginSMS(remoteNum);
          sms.print(txtMsg);
          sms.endSMS();
          Errorindx = 0;
          gsmAccess.shutdown();
          Serial.println("GSM terminated");
        }
        break;
    }
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
    Serial.println("Sending data to the Gateway");
    Serial1.write((byte*)buff, sizeOfData);
  }
}

void SendMsgToCloud()
{

  Serial.println("Sending data to the cloud");
  ConnectingtoGSM();
  Serial.print("Gateway indicator : ");
  Serial.println(Gateway_indicator[0]);
  int k = 0;
  while (k < 10)
  {
    if (Gateway_indicator[k] == 1 )
    {
      TOKEN = tokens[k];
      APP_VERSION = App_version[k];
      configurationUpdatesTopic = "kp1/" + APP_VERSION + "/cmx/" + TOKEN + "/config/json/status";
      configurationRequestTopic = "kp1/" + APP_VERSION + "/cmx/" + TOKEN + "/config/json/1";
      configurationResponceTopic = "kp1/" + APP_VERSION + "/cmx/" + TOKEN + "/config/json/1/status";
      metricsPublishTopic = "kp1/" + APP_VERSION + "/dcx/" + TOKEN + "/json";

      if (!mqttClient.connected())
      {
        reconnect();
      }
      mqttClient.poll();


      for (int i = 0; i < 10; i++)
      {
        for (int j = 0; j < 20; j++)
        {
          if (Data_indicator[i][j] == 1)
          {
            DynamicJsonDocument telemetry(128);
            telemetry.createNestedObject();
            telemetry[0]["timestamp"]    = timestamp[i];
            telemetry[0][sensortypes[i][j]] = value[i][j];
            String payload = telemetry.as<String>();
            mqttClient.beginMessage(metricsPublishTopic);
            mqttClient.print(payload.c_str());
            mqttClient.endMessage();
            Serial.println("Published on topic: " + metricsPublishTopic);
            Serial.println("payload:  " + payload);
          }
        }
      }
    }
    k++;
  }
  delay(2 * 1000);
  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < 20; j++)
    {
      Data_indicator[i][j] = 0;
    }
    Gateway_indicator[i] = 0;
  }
  gsmAccess.shutdown();
  Serial.println("GSM terminated");



}
void reconnect() {
  //  Serial.println("reconnect");

  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");

    char *client_id = "client-id-123ab";// some unique client id
    mqttClient.setId(client_id);

    if (!mqttClient.connect(mqtt_server.c_str(), 1883)) {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.connectError());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    } else {
      Serial.println("You're connected to the MQTT broker!");
      Serial.println();

      // set the message receive callback
      mqttClient.onMessage(onMqttMessage);

      subscribeToConfiguration();
    }
  }
}

void subscribeToConfiguration() {
  //Subscribe on configuration topic for receiving configuration updates
  mqttClient.subscribe(configurationUpdatesTopic);
  //  Serial.println("Subscribed on topic: " + configurationUpdatesTopic);

  //Subscribe on configuration topic response
  mqttClient.subscribe(configurationResponceTopic);
  //  Serial.println("Subscribed on topic: " + configurationResponceTopic);

  // Sending configuration rquest to the server
  DynamicJsonDocument configRequest(128);
  configRequest["observe"] = true;
  String payload = configRequest.as<String>();
  //  Serial.println("Request configuration topic: " + configurationRequestTopic);
  //  Serial.print("Request configuration payload =  ");Serial.println(payload);
  configRequest.clear();
  mqttClient.beginMessage(configurationRequestTopic);
  mqttClient.print(payload.c_str());
  mqttClient.endMessage();

}

void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  String topic = mqttClient.messageTopic();
  Serial.println("Received a message on topic '");
  Serial.print(topic);
  Serial.print("', length ");
  Serial.print(messageSize);

  //Getting payload
  byte buffer[messageSize];
  mqttClient.read(buffer, messageSize);
  Serial.print("  Payload: ");
  for (int i = 0 ; i < messageSize ; i++) {
    Serial.print((char)buffer[i]);
  }
  Serial.println();

  if (topic == configurationResponceTopic || topic == configurationUpdatesTopic) {
    Serial.println("Configuration received");

    //Getting brightness parameter from payload
    DynamicJsonDocument newConfig(1023);
    deserializeJson(newConfig, buffer);
    int brightness = newConfig.as<JsonVariant>()["config"].as<JsonVariant>()["brightness"].as<int>();
    Serial.print(" New brightness: "); Serial.println(String(brightness));

  }
}

void ConnectingtoGSM()
{
  bool connected = false;

  Serial.println("connecting to cellular network ...");

  //  After starting the modem with gsmAccess.begin()
  // attach to the GPRS network with the APN, login and password
  while (!connected) {
    if ((gsmAccess.begin(pin) == GSM_READY) &&
        (gprs.attachGPRS(apn, login, password) == GPRS_READY)) {
      connected = true;
    } else {
      Serial.print(".");
      delay(1000);
    }
  }

  Serial.println("GSM initialized");
}
