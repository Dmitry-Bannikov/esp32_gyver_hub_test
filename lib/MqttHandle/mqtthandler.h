#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>






void MqttInit();
void MqttPublishData();
void OnMqttMessage(char* topic, uint8_t* payload, unsigned int len);
void MqttReconnect();
void Mqtt_tick();
bool sendFaseMqttData(int8_t numBrd);
bool sendMqttJson(const char* topic, const char* data);
void SubscribeTopics();