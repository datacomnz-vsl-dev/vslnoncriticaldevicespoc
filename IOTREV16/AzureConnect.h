
#include <AzureIoTProtocol_MQTT.h>
#include <ArduinoJson.h>
#include <DNSServer.h>



void IoTHubFullSendReceive() {
  IOTHUB_CLIENT_STATUS Status;
  static unsigned int watchdog;
  while ((IoTHubClient_LL_GetSendStatus(iotHubClientHandle, &Status) == IOTHUB_CLIENT_OK) && (Status == IOTHUB_CLIENT_SEND_STATUS_BUSY))
  {
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    ThreadAPI_Sleep(100);
    watchdog++;
    yield(); ////////////////////////////////////////////////need to deal with this better. possibly leave routine and return later///////////////////////////////////////
    if (watchdog > 100)
    {
      watchdog = 0;
      Serial.println (F("Communication with iotHub Failed!"));
      ESP.restart();
    }
  }
}


void initTime() {

  static unsigned int watchdog;

  time_t epochTime;

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  while (true) {
    epochTime = time(NULL);

    if (epochTime == 0) {
      Serial.println(F("Fetching NTP epoch time failed! Waiting to retry"));
      delay(1000);
      watchdog++;
    } else {
      Serial.print(F("Fetched NTP epoch time is: "));
      Serial.println(epochTime);
      break;
    }
    if (watchdog >= 20)
    {
      //ESP.restart();
      break;
    }
  }
}

void readMessage(unsigned int messageId, char *payload) {
  StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
  JsonObject &object = jsonBuffer.createObject();
  object[PinReporting.messageID] = messageId;
  if (PinReporting.D0State) {
    object[PinReporting.D0Name] = String(LastPinStatus.D0Status);
  }
  if (PinReporting.D1State) {
    object[PinReporting.D1Name] = String(LastPinStatus.D1Status);
  }
  if (PinReporting.D2State) {
    object[PinReporting.D2Name] = String(LastPinStatus.D2Status);
  }
  if (PinReporting.A0State) {
    object[PinReporting.A0Name] = String(LastPinStatus.A0Status);
  }

  object.printTo(payload, MESSAGE_MAX_LEN); //inserting data
}

void readMessageAC(unsigned int messageId, char *payload) {
  StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
  JsonObject &object = jsonBuffer.createObject();
  object[PinReporting.messageID] = messageId;

  Serial.print("AC: ");
  double A0Value = calcIrms() * PinReporting.A0Scale;
  Serial.println(A0Value);
  object["AC"] = A0Value;

  object.printTo(payload, MESSAGE_MAX_LEN); //inserting data
}

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback) {
  if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
  {
    Serial.println(F("Message sent to IoT Hub"));
  }
  else
  {
    Serial.println(F("Failed message to IoT Hub"));
  }
}


static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *buffer) {
  IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)buffer, strlen(buffer));
  if (messageHandle == NULL) {
    Serial.println(F("Unable to create a new IoTHubMessage."));
  } else {
    Serial.printf("Sending message: %s.\r\n", buffer);
    if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
    {
      Serial.println(F("Failed to hand message to IoTHubClient"));
    }
    else
    {
      Serial.println(F("IoTHubClient available for delivery"));
    }
    IoTHubMessage_Destroy(messageHandle);
  }
}




//callback to handle mqtt messages





char PinStatusChange() {

  struct PinStatus ThisPinStatus;
  memset(&ThisPinStatus, 0, sizeof ThisPinStatus);

  if (PinReporting.D0State) {
    ThisPinStatus.D0Status = digitalRead(D0);
  }
  else {
    ThisPinStatus.D0Status = false;
  }
  if (PinReporting.D1State) {
    ThisPinStatus.D1Status = digitalRead(D1);
  }
  else {
    ThisPinStatus.D1Status = false;
  }
  if (PinReporting.D2State) {
    ThisPinStatus.D2Status = digitalRead(D2);
  }
  else {
    ThisPinStatus.D2Status = false;
  }
  if (PinReporting.A0State == 2) {
    Serial.print("AC: ");
    double A0Value = calcIrms() * PinReporting.A0Scale;
    Serial.println(A0Value);
    if (A0Value > PinReporting.A0Threshold) {
      ThisPinStatus.A0Status = true;
    } else {
      ThisPinStatus.A0Status = false;
    }
  }
  else if (PinReporting.A0State == 1) {
    Serial.print("DC: ");
    double A0Value = analogRead(A0) * PinReporting.A0Scale;
    Serial.println(A0Value);
    if (A0Value > PinReporting.A0Threshold) {
      ThisPinStatus.A0Status = true;
    } else {
      ThisPinStatus.A0Status = false;
    }
  } else {
    ThisPinStatus.A0Status = false;
  }
  if (memcmp(&ThisPinStatus, &LastPinStatus, sizeof(ThisPinStatus)) == 0) {
    return (0);
  } else {
    memset(&LastPinStatus, 0, sizeof LastPinStatus);
    LastPinStatus = ThisPinStatus;
    return (1);
  }
}

void ICACHE_RAM_ATTR onTimerISR(){
      WiFiCount++;
      iotcount++;
      ipcount++;
      timer1_write(5000000);//1s
}
