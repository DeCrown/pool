
/*
* Project Name: POOL
*/
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <iarduino_RTC.h>
#include "Hash.h"

#define LICENSE_PERIOD 2592000
#define LICENSE_DOP_PERIOD 604800

//2592000 604800

// УСТРОЙСТВА

// 0 - 6.0, 1 - 6.1, 2 - 6.2, 3 - 6.3, 4 - 5.3

iarduino_RTC Timer(RTC_DS1302, D0, D1, D2); //RST, SCLK, DATA

MDNSResponder mdns;
const char* ssid = "POOL";
const char* password = "1111111111";

byte arduino_mac[] = { 0x2C, 0xF4, 0x32, 0x7A, 0x44, 0x50 };
IPAddress ip(192,168,1,178);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80);
 
#include <OneWire.h>
#include <DallasTemperature.h>

SoftwareSerial RS485(D6, D5); //RX, TX

OneWire oneWire(0); // вход датчиков 18b20
DallasTemperature DS18b20(&oneWire);

DeviceAddress SENSORS[] = {{},
  {0x28, 0x29, 0x42, 0x62, 0x4F, 0x20, 0x01, 0x32}, //T2 выход 1 
  {0x28, 0xCE, 0x8F, 0x6E, 0x06, 0x00, 0x00, 0x62}, //T3 выход 2
  {0x28, 0xFF, 0x68, 0x26, 0xC4, 0x17, 0x04, 0x30}, //T3 вход 3
  {0x28, 0xFF, 0xCD, 0x31, 0xC4, 0x17, 0x04, 0x6F}, //T2 вход 4
  {0x28, 0xFF, 0xA5, 0x26, 0xC4, 0x17, 0x04, 0xFE}, //T1 вход 5
  }; 

//константы

byte COUNT_POOL = 2;
byte COUNT_INPUTS = 5;
byte COUNT_OUTPUTS = 8;
byte COUNT_CONTROLLERS = 24;
byte COUNT_VALVES = 27;
byte COUNT_RELES = 10;
byte COUNT_PUMPS = 5;
int KOEF = 100;
int LICENSE = 23;
int POOL_ID = 1;

//все

int lastOUTPUTS[] = {0, 0, 0, 0, 0, 0, 0, 0};
int OUTPUTS[] = {0, 0, 0, 0, 0, 0, 0, 0};

int INPUTS[] = {0, 0, 0, 0, 0};

int MEMORY_START = 0;

#include "POOL_6.5_classes.h"

//скрипты

Step *steps1 = new Step[41] {
  {0, 0, 1, -1}, //открыть канализационную задвижку

  {3, 4, 0, -1},
  {3, 1, 0, -1},
  {3, 2, 0, -1},
  {3, 10, 0, -1},
  {3, 8, 0, -1},
  {3, 19, 0, -1},

  {1, 24, 0, -1},
  {1, 25, 0, -1},
  {2, 0, 0, -1},
  {2, 7, 0, -1},

  {1, 0, 1, -1},
  //{1, 1, 1, -1},
  
  {1, 12, 0, -1}, //31
  {1, 13, 0, -1}, //32
  {1, 15, 1, -1}, //34
  {1, 14, 1, -1}, //33
  {1, 8, 0, -1}, //21
  {1, 9, 0, -1}, //22
  {1, 4, 0, -1}, //11
  {1, 5, 0, -1}, //12
  {4, 0, 0, -1},

  {1, 11, 1, -1}, //24
  {1, 10, 1, -1}, //23
  {1, 14, 0, -1}, //33
  {1, 15, 0, -1}, //34
  {4, 0, 0, -1},

  {1, 7, 1, -1}, //14
  {1, 6, 1, -1}, //13
  {1, 10, 0, -1}, //23
  {1, 11, 0, -1}, //24
  {4, 0, 0, -1},
  
  {1, 13, 1, -1}, //32
  {1, 12, 1, -1}, //31
  {1, 9, 1, -1}, //22
  {1, 8, 1, -1}, //21
  {1, 6, 0, -1}, //13
  {1, 7, 0, -1}, //14
  {1, 5, 1, -1}, //12
  {1, 4, 1, -1}, //11

  {0, 0, 8, -1}, //закрыть канализационную задвижку
  {0, 0, 9, -1}, //промывка завершена

  };

Step *steps2 = new Step[35] {
  {0, 0, 0, -1}, //открыть канализационную задвижку

  {3, 3, 0, -1},
  {3, 11, 0, -1},
  {3, 9, 0, -1},
  {3, 20, 0, -1},

  {1, 26, 0, -1},
  {2, 1, 0, -1},
  {2, 8, 0, -1},
  
  {1, 20, 0, -1}, //51
  {1, 21, 0, -1}, //52
  {1, 23, 1, -1}, //54
  {1, 22, 1, -1}, //53
  {1, 16, 0, -1}, //41
  {1, 17, 0, -1}, //42

  {0, 0, 2, -1},
  
  {4, 0, 0, -1},

  {0, 0, 3, -1},

  {4, 0, 0, -1},

  {0, 0, 4, -1},

  {1, 19, 1, -1}, //44
  {1, 18, 1, -1}, //43
  {1, 22, 0, -1}, //53
  {1, 23, 0, -1}, //54

  {0, 0, 5, -1},
  
  {4, 0, 0, -1},

  {0, 0, 6, -1},

  {4, 0, 0, -1},

  {0, 0, 7, -1},
  
  {1, 21, 1, -1}, //52
  {1, 20, 1, -1}, //51
  {1, 18, 0, -1}, //43
  {1, 19, 0, -1}, //44
  {1, 17, 1, -1}, //42
  {1, 16, 1, -1}, //41

  {0, 0, 9, -1},

  };

//все

Controller *CONTROLLERS[] = {
  new Controller(0, "ОБЩИЙ", 0, 1), //id, название, тип контроллера, режим по умолчанию

  new Boiler(1, "Т1", 24, {5, 0, 0, 0}, 10, 2720, 0, 10, 100, 40), //id, название, клапан, датчики, поправка, уставка по умолчанию, гист. верхний, гист. нижний, мин. значение, макс. значение
  new Boiler(2, "Т2", 25, {4, 1, 0, 0}, 10, 2720, 0, 10, 100, 40),
  new Boiler(3, "Т3", 26, {3, 2, 0, 0}, 150, 3050, 0, 10, 100, 100),
  
  new Donnik(4, "ДОННИК 1", 0, 1, 1), //id, название, клапан, режим по умолчанию
  new ValveController(5, "ПОДПИТКА 1", 1, 2),
  new ValveController(6, "ДОННИК 2", 2, 2),
  new Podpitka(7, "ПОДПИТКА 2", 3, 3, 5, 10, 0), //id, название, клапан, устр. дат. долива, номер дат. долива, время (секунды), режим по умолчанию

  new Ozon(8, "ОГ 1", 0, 3, 15, 2, 12), //id, название, реле, устр. дат. ручной работы, номер дат. ручной работы, устр. дат. работы, номер дат. работы
  new Ozon(9, "ОГ 2", 1, 3, 15, 3, 3),

  new PumpGroup(10, "НАСОСЫ 1", 3, new byte[3] {2, 3, 4}, 2, 4, 3), //id, название, кол-во насосов, реле насосов, первый насос, устр. дат. ручной работы, номер дат. ручной работы
  new PumpGroup(11, "НАСОСЫ 2", 2, new byte[2] {5, 6}, 5, 4, 8),

  new Filter(12, "ФИЛЬТР 1", 4, 5, 6, 7), //id, название, клапан1, клапан2, клапан3, клапан4
  new Filter(13, "ФИЛЬТР 2", 8, 9, 10, 11),
  new Filter(14, "ФИЛЬТР 3", 12, 13, 14, 15),
  new Filter(15, "ФИЛЬТР 4", 16, 17, 18, 19),
  new Filter(16, "ФИЛЬТР 5", 20, 21, 22, 23),

  new Pool(17, "БАССЕЙН 1", 2), //id, название, теплообменник
  new Pool(18, "БАССЕЙН 2", 3),

  new Dosage(19, "ДОЗАЦИЯ 1", 7), //id, название, реле
  new Dosage(20, "ДОЗАЦИЯ 2", 8),

  new Script(21, "ПРОМЫВКА 1", 41, steps1, 600), //id, название, кол-во шагов, шаги
  new Script(22, "ПРОМЫВКА 2", 35, steps2, 600),

  new License(23, "ЛИЦЕНЗИЯ", "bbef7ec9e2", 1), //id, название, кодовая строка (bbef7ec9e2)
  };

Valve VALVES[] = {
  Valve(0, "ДОННИК 1", 4, {4, 12, 4, 13, 0, 0, 0, 1}), //0
  Valve(1, "ПОДПИТКА 1", 5, {4, 15, 4, 14, 0, 2, 0, 3}),
  Valve(2, "ДОННИК 2", 6, {0, 0, 0, 1, 0, 4, 0, 5}),
  Valve(3, "ПОДПИТКА 2", 7, {0, 2, 0, 3, 0, 6, 0, 7}),
  
  Valve(4, "K11", 0, {0, 4, 0, 5, 1, 0, 1, 1}), //4
  Valve(5, "K12", 0, {0, 6, 0, 7, 1, 2, 1, 3}),
  Valve(6, "K13", 0, {0, 8, 0, 9, 1, 4, 1, 5}),
  Valve(7, "K14", 0, {0, 10, 0, 11, 1, 6, 1, 7}),

  Valve(8, "K21", 0, {0, 12, 0, 13, 2, 0, 2, 1}), //8
  Valve(9, "K22", 0, {0, 14, 0, 15, 2, 2, 2, 3}),
  Valve(10, "K23", 0, {1, 0, 1, 1, 2, 4, 2, 5}),
  Valve(11, "K24", 0, {1, 2, 1, 3, 2, 6, 2, 7}),

  Valve(12, "K31", 0, {1, 4, 1, 5, 3, 0, 3, 1}), //12
  Valve(13, "K32", 0, {1, 6, 1, 7, 3, 2, 3, 3}),
  Valve(14, "K33", 0, {1, 8, 1, 9, 3, 4, 3, 5}),
  Valve(15, "K34", 0, {1, 10, 1, 11, 3, 6, 3, 7}),

  Valve(16, "K41", 0, {1, 12, 1, 13, 4, 6, 4, 7}), //16
  Valve(17, "K42", 0, {1, 14, 1, 15, 4, 2, 4, 3}),
  Valve(18, "K43", 0, {2, 0, 2, 1, 4, 4, 4, 5}),
  Valve(19, "K44", 0, {2, 2, 2, 3, 4, 0, 4, 1}),

  Valve(20, "K51", 0, {2, 4, 2, 5, 5, 0, 5, 1}), //20
  Valve(21, "K52", 0, {2, 6, 2, 7, 5, 2, 5, 3}),
  Valve(22, "K53", 0, {2, 8, 2, 9, 5, 4, 5, 5}),
  Valve(23, "K54", 0, {2, 10, 2, 11, 5, 6, 5, 7}),

  Valve(24, "T1", 1, {3, 10, 4, 10, 6, 0, 6, 1}), //24
  Valve(25, "T2", 2, {4, 5, 4, 0, 6, 2, 6, 3}), //25
  Valve(26, "T3", 3, {3, 4, 2, 13, 6, 4, 6, 5}) //26
  };

Rele RELES[] = {
  Rele("ОГ 1", 8, 6, 6), //0
  Rele("ОГ 2", 9, 6, 7),
  
  Rele("НАСОС 1", 10, 7, 0), //2
  Rele("НАСОС 2", 10, 7, 1),
  Rele("НАСОС 3", 10, 7, 2),
  Rele("НАСОС 4", 11, 7, 4),
  Rele("НАСОС 5", 11, 7, 5),

  Rele("ДОЗАТОР 1", 0, 7, 3), //7
  Rele("ДОЗАТОР 2", 0, 7, 6),

  Rele("РЕЗЕРВ 0", 0, 7, 7) //9
  };

#include "POOL_6.5.h"

#include "POOL_6.5_classes_realize.h"
