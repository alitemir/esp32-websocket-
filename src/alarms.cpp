#include <Arduino.h>
#include <ModbusMaster.h>
#include "main.h"

// extern ModbusMaster node;

static void readAlarms()
{
  // int result = node.readHoldingRegisters(ALARM_ADDR_BEGIN, 16);
  // if (result == node.ku8MBSuccess)
  // {
  //   for (int j = 0; j < 16; j += 2)
  //   {
  //     int hour = node.getResponseBuffer(j);
  //     int minute = node.getResponseBuffer(j + 1);

  //     Serial.printf("%02d:%02d\n", hour, minute);
  //   }
  // }
  // else
  // {
  //   Serial.println("Alarm read error");
  // }
  // // Serial.println();
}

static void readAlarmStatus()
{
  // int result = node.readHoldingRegisters(ALARM_SETTINGS_ADDR_BEGIN, 8);
  // if (result == node.ku8MBSuccess)
  // {
  //   for (int j = 0; j < 8; j++)
  //   {
  //     int alarmStatus = node.getResponseBuffer(j);
  //     Serial.printf("%d", alarmStatus);
  //   }
  // }
  // else
  // {
  //   Serial.println("Alarm status read error");
  // }
  // // Serial.println("");
}