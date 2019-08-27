/*
 *  Created on: 01.01.2019
 *      Author: Georgi Angelov
 */

#ifndef HardwareSerial_h
#define HardwareSerial_h

#include "Stream.h"
#include "interface.h"

#define UART_STACK_SIZE (4 * 1024)
#define UART_RECEIVE_SIZE (1024)

class HardwareSerial : public Stream
{
private:
	TX_MUTEX *mutex;
	TX_EVENT_FLAGS_GROUP *event;
	TX_THREAD *task;
	UCHAR *stack;
	qapi_UART_Handle_t uart;
	qapi_UART_Port_Id_e id;
	uint32_t rx_ring_tail;
	uint32_t rx_ring_head;
	UCHAR rx_driver[1024];
	UCHAR rx_ring_buff[UART_RECEIVE_SIZE];

	void ctor();
	int save(uint8_t c);
	static void entry(void *THIS);
	static void receiveCallback(uint32_t num_bytes, void *THIS);
	static void transmitCallback(uint32_t num_bytes, void *THIS);

public:
	HardwareSerial(qapi_UART_Port_Id_e id);
	~HardwareSerial(){};
	void debug(void);
	void nodebug(void);
	void begin(unsigned long);
	void end(void);
	virtual int available(void);
	virtual int peek(void);
	virtual int read(void);
	virtual void flush(void){};
	virtual size_t write(uint8_t);
	using Print::write;
	operator bool() { return true; }
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;

#endif
