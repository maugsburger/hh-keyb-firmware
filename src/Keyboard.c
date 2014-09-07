/*
                   The HumbleHacker Keyboard Project
                 Copyright � 2008-2010, David Whetstone
              david DOT whetstone AT humblehacker DOT com

  This file is a part of the HumbleHacker Keyboard Firmware project.

  The HumbleHacker Keyboard Project is free software: you can
  redistribute it and/or modify it under the terms of the GNU General
  Public License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  The HumbleHacker Keyboard Project is distributed in the
  hope that it will be useful, but WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with The HumbleHacker Keyboard Firmware project.  If not, see
  <http://www.gnu.org/licenses/>.

  --------------------------------------------------------------------

  This code is based on the keyboard demonstration application by
  Denver Gingerich.

  Copyright 2008  Denver Gingerich (denver [at] ossguy [dot] com)

  --------------------------------------------------------------------

  Gingerich's keyboard demonstration application is based on the MyUSB
  Mouse demonstration application, written by Dean Camera.

             LUFA Library
     Copyright (C) Dean Camera, 2010.

  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
  --------------------------------------------------------------------
*/

/** \file
 *
 *  Main source file for the Keyboard demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include <assert.h>
#include "Keyboard.h"
#ifndef MATRIX_DISCOVERY_MODE
#include "keyboard_class.h"
#else
#include "matrix_discovery.h"
#endif
#include "dbg.h"


/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
  {
    .Config =
      {
        .InterfaceNumber              = 0,
        .ReportINEndpoint             =
            {
                .Address              = KEYBOARD_EPNUM,
                .Size                 = KEYBOARD_EPSIZE,
                .Banks                = 1,
            },
        .PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
        .PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
      },
    };

uint8_t PrevDBGHIDReportBuffer[sizeof(USB_DBGReport_Data_t)];

USB_ClassInfo_HID_Device_t DBG_HID_Interface =
  {
    .Config =
      {
        .InterfaceNumber              = 1,
        .ReportINEndpoint             =
            {
                .Address              = DBG_EPNUM,
                .Size                 = DBG_EPSIZE,
                .Banks                = 1,
            },
        .PrevReportINBuffer           = PrevDBGHIDReportBuffer,
        .PrevReportINBufferSize       = sizeof(PrevDBGHIDReportBuffer),
      },
    };

uint8_t g_num_lock, g_caps_lock, g_scrl_lock;

/* EEPROM Data */
//static KeyMap  EEMEM ee_persistent_map;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */

void PRG_Device_USBTask(void);

void PRG_Device_USBTask()
{
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

  Endpoint_SelectEndpoint(PRG_EPNUM);

  if (Endpoint_IsConfigured() && Endpoint_IsINReady() && Endpoint_IsReadWriteAllowed())
  {
    static char data[] = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
    Endpoint_Write_Stream_LE((void *)data, 64, NULL);
    // FIXME handle err

    Endpoint_ClearIN();
  }
}

int main(void)
{
  SetupHardware();
  sei();

#ifdef MATRIX_DISCOVERY_MODE
  for (;;)
  {
    HID_Device_USBTask(&Keyboard_HID_Interface);
    MatrixDiscovery__scan_matrix();
    USB_USBTask();
  }
#else
  for (;;)
  {
    if (USB_DeviceState != DEVICE_STATE_Suspended)
    {
      HID_Device_USBTask(&Keyboard_HID_Interface);
      HID_Device_USBTask(&DBG_HID_Interface);

      PRG_Device_USBTask();
    }
    else if (USB_Device_RemoteWakeupEnabled && Keyboard__key_is_down())
    {
      USB_Device_SendRemoteWakeup();
    }
    USB_USBTask();
  }
#endif
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware()
{
  /* Disable clock division */
  clock_prescale_set(clock_div_2);

  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  /* Hardware Initialization */
  LEDs_Init();
  USB_Init();

  /* Task init */
#ifndef MATRIX_DISCOVERY_MODE
  Keyboard__init();
#else
  MatrixDiscovery__init();
#endif

  g_num_lock = g_caps_lock = g_scrl_lock = 0;

#if defined(BOOTLOADER_TEST)
  uint8_t bootloader = eeprom_read_byte(&ee_bootloader);
  if (bootloader == 0xff) // eeprom has been reset
  {
    eeprom_write_byte(&ee_bootloader, FALSE);
  }
  else if (bootloader == TRUE)
  {
    asm("jmp 0xF000");
  }
#endif
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
  LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
  LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
  LEDs_SetAllLEDs(LEDMASK_USB_READY);

  if (!(HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface)))
    LEDs_SetAllLEDs(LEDMASK_USB_ERROR);

  if (!(HID_Device_ConfigureEndpoints(&DBG_HID_Interface)))
    LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
/* FIXME re-add programming endpoint
  if (!(Endpoint_ConfigureEndpoint(PRG_EPNUM, EP_TYPE_BULK, ENDPOINT_DIR_IN,
                                   PRG_EPSIZE, ENDPOINT_BANK_SINGLE)))
    LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
*/
  USB_Device_EnableSOFEvents();
}

/** Event handler for the library USB Unhandled Control Request event. */
void EVENT_USB_Device_UnhandledControlRequest(void)
{
  HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
  HID_Device_ProcessControlRequest(&DBG_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
  HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
  HID_Device_MillisecondElapsed(&DBG_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID  Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in] ReportType  Type of the report to create, either REPORT_ITEM_TYPE_In or REPORT_ITEM_TYPE_Feature
 *  \param[out] ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out] ReportSize  Number of bytes written in the report (or zero if no report is to be sent
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID,
                                         const uint8_t ReportType, void* ReportData, uint16_t* ReportSize)
{
  if (HIDInterfaceInfo == &Keyboard_HID_Interface)
  {
    USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;
  #ifndef MATRIX_DISCOVERY_MODE
    *ReportSize = Keyboard__get_report(KeyboardReport);
  #else
    *ReportSize = MatrixDiscovery__get_report(KeyboardReport);
  #endif
  }
  else if (HIDInterfaceInfo == &DBG_HID_Interface)
  {
    USB_DBGReport_Data_t* DBGReport = (USB_DBGReport_Data_t*)ReportData;
    *ReportSize = DBG__get_report(DBGReport);
  }

  return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either REPORT_ITEM_TYPE_Out or REPORT_ITEM_TYPE_Feature
 *  \param[in] ReportData  Pointer to a buffer where the created report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, const uint8_t ReportID,
                                          const uint8_t ReportType, const void* ReportData, const uint16_t ReportSize)
{
  uint8_t* LEDReport = (uint8_t*)ReportData;

  LEDs_ChangeLEDs(LED_CAPS_LOCK|LED_SCROLL_LOCK|LED_NUM_LOCK|
                  LED_COMPOSE|LED_KANA, *LEDReport);
}
