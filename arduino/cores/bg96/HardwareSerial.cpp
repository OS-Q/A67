/*
 HardwareSerial.cpp
 Copyright (c) 2018 Georgi Angelov. All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "HardwareSerial.h"

#define DEBUG_UART
//DBG

#define UART_RX_EVENT_MASK (0x01)
#define UART_TX_EVENT_MASK (0x10)

void HardwareSerial::debug(void)
{
	debug_enable(uart);
}

void HardwareSerial::nodebug(void)
{
	debug_disable();
}

int HardwareSerial::save(uint8_t c)
{
	size_t saved = 0;
	ENTER_CRITICAL();
	uint32_t i = (uint32_t)(rx_ring_head + 1) % sizeof(rx_ring_buff);
	if (i != rx_ring_tail)
	{
		rx_ring_buff[rx_ring_head] = c;
		rx_ring_head = i;
		saved = 1;
	}
	EXIT_CRITICAL();
	return saved; // full
}

void HardwareSerial::receiveCallback(uint32_t num_bytes, void *p)
{
	if (p)
	{
		HardwareSerial *THIS = (HardwareSerial *)p;
		for (uint32_t i = 0; i < num_bytes; i++)
			if (THIS->save(THIS->rx_driver[i]))
				break; // full
		tx_event_flags_set(THIS->event, UART_RX_EVENT_MASK, TX_OR);
	}
}

void HardwareSerial::transmitCallback(uint32_t num_bytes, void *p)
{
	if (p)
	{
		HardwareSerial *THIS = (HardwareSerial *)p;
		tx_event_flags_set(THIS->event, UART_TX_EVENT_MASK, TX_OR);
	}
}

void HardwareSerial::entry(void *p)
{
	if (NULL == p)
		return;
	HardwareSerial *THIS = (HardwareSerial *)p;
	qapi_Timer_Sleep(10, QAPI_TIMER_UNIT_MSEC, 1);
	while (1)
	{
		qapi_Timer_Sleep(1, QAPI_TIMER_UNIT_TICK, 1);
		if (THIS->uart)
		{
			if (0 == qapi_UART_Receive(THIS->uart, THIS->rx_driver, 1024, THIS))
			{
				ULONG sig_event;
				tx_event_flags_get(THIS->event, UART_RX_EVENT_MASK, TX_OR_CLEAR, &sig_event, TX_WAIT_FOREVER);
			}
		}
	}
}

HardwareSerial::HardwareSerial(qapi_UART_Port_Id_e port)
{
	mutex = NULL;
	event = NULL;
	task = NULL;
	stack = NULL;
	uart = NULL;
	rx_ring_tail = 0;
	rx_ring_head = 0;
	id = port;
}

void HardwareSerial::ctor()
{
	static int once = 0;
	if (once)
		return;
	stack = (UCHAR *)malloc(UART_STACK_SIZE);
	int r;
	if ((r = txm_module_object_allocate(&mutex, sizeof(TX_MUTEX))))
	{
		DEBUG_UART("[SERIAL][%d] MUTEX txm_module_object_allocate: %d\n", id, r);
		abort();
	}
	if ((r = tx_mutex_create(mutex, "hardware_serial_mutex", TX_INHERIT)))
	{
		DEBUG_UART("[SERIAL][%d] tx_mutex_create: %d\n", id, r);
		abort();
	}
	if ((r = txm_module_object_allocate(&event, sizeof(TX_EVENT_FLAGS_GROUP))))
	{
		DEBUG_UART("[SERIAL][%d] EVENT txm_module_object_allocate: %d\n", id, r);
		abort();
	}
	if ((r = tx_event_flags_create(event, "hardware_serial_event")))
	{
		DEBUG_UART("[SERIAL][%d] tx_event_flags_create: %d\n", id, r);
		abort();
	}
	if ((r = txm_module_object_allocate(&task, sizeof(TX_THREAD))))
	{
		DEBUG_UART("[SERIAL][%d] THREAD txm_module_object_allocate = %d\n", id, r);
		abort();
	}
	if ((r = tx_thread_create(task, "hardware_serial_thread", entry, this, stack, UART_STACK_SIZE, PRIO, PRIO, TX_NO_TIME_SLICE, TX_AUTO_START)))
	{
		DEBUG_UART("[SERIAL][%d] THREAD tx_thread_create = %d\n", id, r);
		abort();
	}
	once = 1;
}

void HardwareSerial::begin(unsigned long baudrate)
{
	int r;
	ctor();
	if ((r = tx_event_flags_set(event, 0, TX_AND)))
	{
		DEBUG_UART("[SERIAL][%d] tx_event_flags_set = %d\n", id, r);
		abort();
	}
	rx_ring_tail = 0;
	rx_ring_head = 0;
	if (uart)
		return; // allready open
	qapi_UART_Open_Config_t cfg;
	memset(&cfg, 0, sizeof(qapi_UART_Open_Config_t));
	cfg.baud_Rate = baudrate;
	cfg.enable_Flow_Ctrl = QAPI_FCTL_OFF_E;
	cfg.bits_Per_Char = QAPI_UART_8_BITS_PER_CHAR_E;
	cfg.num_Stop_Bits = QAPI_UART_1_0_STOP_BITS_E;
	cfg.parity_Mode = QAPI_UART_NO_PARITY_E;
	cfg.rx_CB_ISR = (qapi_UART_Callback_Fn_t)&receiveCallback;
	cfg.tx_CB_ISR = (qapi_UART_Callback_Fn_t)&transmitCallback;
	if ((r = qapi_UART_Open(&uart, id, &cfg)))
	{
		DEBUG_UART("[SERIAL][%d] qapi_UART_Open = %d\n", id, r);
		abort();
	}
	if ((r = qapi_UART_Power_On(uart)))
	{
		DEBUG_UART("[SERIAL][%d] qapi_UART_Power_On = %d\n", id, r);
		abort();
	}
}

void HardwareSerial::end()
{
	if (uart)
	{
		qapi_UART_Power_Off(uart);
		qapi_UART_Close(uart);
		uart = NULL;
	}
}

int HardwareSerial::available(void)
{
	if (NULL == uart)
		return -1;
	return (uint32_t)(sizeof(rx_ring_buff) + rx_ring_head - rx_ring_tail) % sizeof(rx_ring_buff);
}

int HardwareSerial::peek(void)
{
	if (NULL == uart || rx_ring_head == rx_ring_tail)
		return -1;
	return rx_ring_buff[rx_ring_tail];
}

int HardwareSerial::read(void)
{
	if (NULL == uart || rx_ring_head == rx_ring_tail)
		return -1;
	ENTER_CRITICAL();
	uint8_t r = rx_ring_buff[rx_ring_tail];
	rx_ring_tail = (uint32_t)(rx_ring_tail + 1) % sizeof(rx_ring_buff);
	EXIT_CRITICAL();
	return r;
}

size_t HardwareSerial::write(uint8_t c)
{
	if (NULL == uart)
		return 0;
	if (tx_mutex_get(mutex, TX_WAIT_FOREVER))
		return 0;
	int w = 0;
	if (0 == qapi_UART_Transmit(uart, &c, 1, this))
	{
		ULONG sig_event;
		tx_event_flags_get(event, UART_TX_EVENT_MASK, TX_OR_CLEAR, &sig_event, TX_WAIT_FOREVER);
		w = 1; // one char is transmited
	}
	tx_mutex_put(mutex);
	return w;
}
