#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>






void MqttInit();
void MqttPublishData();
void onMqttMessage(char* topic, uint8_t* payload, size_t len);
void MqttReconnect();
void Mqtt_tick();
bool sendFaseMqttData(int8_t numBrd);
bool sendMqttJson(const char* topic, const char* data);