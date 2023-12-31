#include <Display.h>



Display::~Display()
{
	userSerial->end();
	delete(userSerial);
}

void Display::Test(int16_t value) {
	uint8_t buffer[8];
	buffer[0] = 0x5A;
	buffer[1] = 0xA5;
	buffer[2] = 0x05;
	buffer[3] = 0x82;
	buffer[4] = 0x50;
	buffer[5] = 0x00;
	buffer[6] = (uint8_t)(value >> 8);
	buffer[7] = (uint8_t)(value & 0xFF);
	userSerial->write(buffer, sizeof(buffer));
}

void Display::tick() {

	static uint32_t tmr = 0;
	if (millis() - tmr > 1000) {
		//Test(255);
		tmr = millis();
	}
}

bool Display::pollForDataRx() {
	uint32_t tmr = millis();
	while(millis() - tmr < 500) {
		if(userSerial->available()) 
			return true;
		yield();
	}
	return false;
}


void Display::writeAddedValues(const uint16_t addr) {
	if (!pointer || !_inited) return;
	uint8_t* addrPtr = new uint8_t[sizeof(addr)];
	convertData(addr, addrPtr);
	txbuf[0] = header1;
	txbuf[1] = header2;
	txbuf[2] = pointer + 3;
	txbuf[3] = 0x82;
	memcpy(txbuf + 4, addrPtr, 2);
	userSerial->write(txbuf, pointer + 6);
	pointer = 0;
	delete(addrPtr);
}

void Display::requestFrom(const uint16_t addr, const uint8_t words) {
	if (!_inited) return;
	uint8_t* addrPtr = new uint8_t[sizeof(addr)];
	convertData(addr, addrPtr);
	uint8_t tx_buf[50];
	tx_buf[0] = header1;
	tx_buf[1] = header2;
	tx_buf[2] = 0x04;
	tx_buf[3] = 0x83;
	memcpy(tx_buf + 4, addrPtr, 2);
	tx_buf[6] = words;
	userSerial->write(tx_buf, 7);
	delete(addrPtr);
}

void Display::begin(HardwareSerial *Ser, CallbackFunction callback) {
	userSerial = Ser;
	userSerial->begin(115200, SERIAL_8N1);
	//onDataReceived = callback;
	userSerial->setRxTimeout(10);
	userSerial->setRxFIFOFull(255);
	userSerial->onReceive(callback);
	_inited = true;
}

uint16_t Display::parseAddress(const uint8_t* buffer) {
	if (buffer[0] != 0x5A) return 0;
	uint16_t res = ((uint16_t)(buffer[4]<<8)|buffer[5]);
	return res;
}

void Display::sendRawData(const uint16_t addr, const uint8_t *data, uint8_t size) {
	if (!_inited) return;
	uint8_t tx_buf[255];
	uint8_t* addrPtr = new uint8_t[sizeof(addr)];
	convertData(addr, addrPtr);
	tx_buf[0] = header1;
	tx_buf[1] = header2;
	tx_buf[2] = sizeof(size) + 3;
	tx_buf[3] = 0x82;
	memcpy(tx_buf + 4, addrPtr, 2);
	memcpy(tx_buf + 6, data, size);
	userSerial->write(tx_buf, size + 6);
	delete(addrPtr);
}

//