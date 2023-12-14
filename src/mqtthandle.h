#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <data.h>
//#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>



#define USE_ORTEA

#ifdef USE_ORTEA
const char *mqtt_broker = "ortea.ru";//185.64.76.226
const char *mqtt_clientId = "esp32mqtt_client";
const char *mqtt_username = "device_stab";
const char *mqtt_password = "2#r]V\\r]+(Dw@WnAd5Kq";
const int mqtt_port = 8880;
#else
const char *mqtt_broker = "m6.wqtt.ru";
const char *topicToServer = "stab/toserver";
const char *topicToStab = "stab/tostab";
const char *mqtt_clientId = "esp32_stab_manager";
const char *mqtt_username = "u_J94WNP";
const char *mqtt_password = "TvTRzLsh";
const int mqtt_port = 15164;
#endif
WiFiClient espClient;
PubSubClient mqttClient(espClient);



void MqttInit();
void MqttPublishData();
void onMqttMessage(char* topic, uint8_t* payload, size_t len);
void MqttReconnect();
void Mqtt_tick();
bool createFaseMqttData(char fase);
 


void MqttInit() {
    mqttClient.setServer(mqtt_broker, mqtt_port);
    mqttClient.setCallback(onMqttMessage);
    MqttReconnect();
    //mqttClient.subscribe(topicToServer);
    //mqttClient.subscribe(topicToStab);
    String topic1 = "stab/tostab/alarm1";
    String topic2 = "stab/toserver/alarm1";
    mqttClient.subscribe(topic1.c_str());
    mqttClient.subscribe(topic2.c_str());
}

void MqttReconnect() {
    static uint32_t tmr = 0;
    static uint32_t period = 500;
    if (millis() - tmr <= period) return;
    if (!mqttClient.connected())
    {
        mqttConnected = false;
        if (mqttClient.connect(mqtt_clientId, mqtt_username, mqtt_password)){
            period = 10000;
            mqttConnected = true;
        } else {
            period = 60000;
            mqttConnected = false;
            //Serial.print("\n Mqtt failed, reason: ");
            //Serial.println(mqttClient.state());  
        }
    } else {
        mqttConnected = true;
    }
    tmr = millis();
}

void MqttPublishData() {
    static uint32_t tmr = 0;
    if (millis() < tmr + 1000) return;

    createFaseMqttData('A');
    createFaseMqttData('B');
    createFaseMqttData('C');

    //======================Старые===================================//
    /*
    String inV = String(board[activeBoard].mainData.Uin);
    mqttClient.publish("stab/toserver/uin", inV.c_str());

    String outV = String(board[activeBoard].mainData.Uout);
    mqttClient.publish("stab/toserver/uout", outV.c_str());

    String outC = String(board[activeBoard].mainData.Current, 1);
    mqttClient.publish("stab/toserver/cout", outC.c_str());

    String outP = String((board[activeBoard].mainData.Power/1000), 1);
    mqttClient.publish("stab/toserver/pout", outP.c_str());

    String alarm1 = String(board[activeBoard].addSets.Switches[SW_ALARM]);
    mqttClient.publish("stab/toserver/alarm1", alarm1.c_str());
    */
    tmr = millis();
}

void onMqttMessage(char* topic, uint8_t* payload, size_t len) {
    String topicStr = String(topic);
    if (topicStr == "stab/tostab/alarm1") {
        //Serial.println("\n Алярм");
        if (String((char*)payload) == "1") {
            board[activeBoard].sendCommand(SW_ALARM, 1);
        } else {
            board[activeBoard].sendCommand(SW_ALARM, 0);
        } 
    }

}

void Mqtt_tick() {
    mqttClient.loop();
    MqttReconnect();
    MqttPublishData();
}

bool createFaseMqttData(char fase) {
    int8_t board_number = -1;
    for (uint8_t i = 0; i < board.size(); i++) {
        if(board[i].getLiteral() == String(fase)) { //ищем номер платы с такой буквой
            board_number = i;
            break;
        }
    }
    if (board_number == -1) return false;
    String topic = "stab_brd/";
    topic += WiFi.macAddress();
    topic += "/data/fase";
    topic += String(fase);
    String data = board[board_number].getJsonData();
    mqttClient.publish(topic.c_str(), data.c_str());
    return true;
}



//-----------------------------------------------------------------------------------//