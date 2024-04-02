/*
*Библиотека для общения с дисплеем
* Методы библиотеки:
*

*/


#pragma once
#include <Arduino.h>
#include <cstring>

enum commands {
    U_P1 = 0,
    U_P2,
    U_P3,
    I_P1,
    I_P2,
    I_P3,
    Papp_P1 = 12,
    Papp_P2,
    Papp_P3,
    CosFi_P1,
    CosFi_P2,
    CosFi_P3,
    L1L2U = 21,
    L2L3U,
    L3L1U,
    FREQ
};

class Meter
{
private:
	bool _inited = false;

	template <typename T>
	T reverseBytes(T &value)
	{
		uint8_t *front = reinterpret_cast<uint8_t *>(&value);
		uint8_t *back = front + sizeof(T) - 1;
		while (front < back)
		{
			std::swap(*front, *back);
			++front;
			--back;
		}
		return value;
	}
	void reverseBytes(uint8_t* buffer, size_t size);
	void flushSerial();
	void getCRC(uint8_t* buffer, size_t size);

public:

	Meter() = default;
	~Meter();

	void begin();
	bool requestValue(float &value, commands command);
	float* getValueManual(uint16_t startAddr, uint8_t wordsNum);
	void tick();

};

