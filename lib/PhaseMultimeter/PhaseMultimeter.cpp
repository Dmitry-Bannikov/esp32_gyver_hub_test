#include <PhaseMultimeter.h>

void Meter::begin() {
    Serial2.begin(9600);
}

bool Meter::requestValue(float &value, commands command) {
    uint16_t regAddr = ((uint16_t)(command))*2;
    bool result = false;
    uint8_t buffer[8] = {1,4,0,0x00,0,2,0,0};
    buffer[3] = (uint8_t)regAddr;
    getCRC(buffer, sizeof(buffer));
    Serial2.write(buffer, sizeof(buffer));
    uint32_t tmr = millis();
    while(!Serial2.available() || millis() - tmr < 200);
    if (Serial2.available()==9) {
        uint8_t bufferRX[9];
        Serial2.read(bufferRX, sizeof(bufferRX));
        if (bufferRX[0] == 1 && bufferRX[1] == 4) {
            float value_read = *((float*)(bufferRX+3));
            value = reverseBytes(value_read);
            flushSerial();
            result = true;
        } 
    }
    return result;
    
}

float* Meter::getValueManual(uint16_t startAddr, uint8_t valNum) {
    //01 04 00 00 00 06 70 08
    float* data = new float[valNum];
    uint16_t regAddr = reverseBytes(startAddr);
    uint8_t buffer[8] = {1,4,0,0x00,0,2,0,0};
    *(uint16_t*)(buffer + 2) = regAddr;
    buffer[5] = valNum*2;
    getCRC(buffer, sizeof(buffer));
    Serial2.write(buffer, sizeof(buffer));
    uint32_t tmr = millis();
    while(!Serial2.available() || millis() - tmr < 200);
    if (Serial2.available() > 3) {
        uint8_t bufferRx[3];
        Serial2.read(bufferRx, 3);
        if (bufferRx[0] != 0x01 || bufferRx[1] != 0x04) return nullptr;
        uint8_t byteCnt = bufferRx[2];
        if (byteCnt != valNum*sizeof(float)) return nullptr;
        for (uint8_t i = 0; i < valNum; i++) {
            uint8_t value_buf[4];
            Serial2.read(value_buf, 4);
            data[i] = reverseBytes(*(float*)(value_buf));
        }
    }
    return data;
}



void Meter::getCRC(uint8_t* buffer, size_t size) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < size-2; i++) {
        crc ^= buffer[i];
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    // Инверсия битов
    buffer[size-2] = (uint8_t)(crc);
    buffer[size-1] = ((crc & 0xFF00) >> 8);
    crc = ((crc & 0xFF) << 8) | ((crc & 0xFF00) >> 8);
}

void Meter::reverseBytes(uint8_t* buffer, size_t size) {
    uint8_t *front = buffer;
    uint8_t *back = front + size - 1;
    while (front < back)
    {
        std::swap(*front, *back);
        ++front;
        --back;
    }
}

void Meter::flushSerial() {
    while (Serial2.available()) {
        uint8_t garbage = Serial2.read();
    }
}

Meter::~Meter() {

}



//