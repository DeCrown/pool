int DecToInt(String a) {
  int ret = 0;
  int len = a.length();
  for (int i = 0; i < len; i++) {
    switch (a.charAt(i)) {
      case 'F':
        ret += pow(16, len - i - 1) * 15;
        break;
      case 'E':
        ret += pow(16, len - i - 1) * 14;
        break;
      case 'D':
        ret += pow(16, len - i - 1) * 13;
        break;
      case 'C':
        ret += pow(16, len - i - 1) * 12;
        break;
      case 'B':
        ret += pow(16, len - i - 1) * 11;
        break;
      case 'A':
        ret += pow(16, len - i - 1) * 10;
        break;
      default:
        ret += pow(16, len - i - 1) * a.substring(i, i + 1).toInt();
        break;
    }
  }
  return ret;
}

String IntToDec(int a) {
  String chars = "0123456789ABCDEF";
  int d1 = a / 16;
  int d2 = a % 16;
  String ret = chars.substring(d1, d1 + 1) + chars.substring(d2, d2 + 1);
  return ret;
}

Answer command(String code, int count = 0) {
  //Serial.println("отправлена команда (управление): " + code);
  
  RS485.print(code + static_cast <char> (13));
  String answer = "";
  int waitingAnswer = 1;
  int lastRequest = millis();

  while (waitingAnswer == 1) {
    if (RS485.available() > 0) {
      char symb = (char)RS485.read();
      if (symb == static_cast <char> (13)) {
        //Serial.println("принят ответ на команду");
        waitingAnswer = 2;
      }
      else {
        answer += symb;
      }
      delay(30);
    }

    if (abs(millis() - lastRequest) > 2000) {
      Serial.println("истекло время на ответ");
      answer = "?00";
      waitingAnswer = 0;
    }
  }

  if (waitingAnswer == 0) {
    if (count >= 2) {
      bitSet(ERRORS[4], 0);
      return {"", 0};
    }
    else {
      Serial.println("ошибка (01): перезапрос");
      return command(code, count + 1);
    }
  }
  
  if (waitingAnswer == 2) {
    bitClear(ERRORS[3], 0);
    return {answer, 1};
  }
}

int getRele(int rele_i) {
  Answer request = command(RELE_COMMANDS[rele_i] + static_cast <char> (120) + static_cast <char> (48) + static_cast <char> (68));
  
  if (request.success) {
    int symbsAllow = 0;
    if (request.answer.length() == 9) {
      if (request.answer.charAt(0) == '!') {
        symbsAllow += 1;
      }
      else if (request.answer.charAt(0) == '?') {
        //ошибка
        Serial.println("ошибка (02)");
        return false;
      }
      String allowChars = "0123456789ABCDEF";
      for (int ii = 0; ii < 8; ii++) {
        if (allowChars.indexOf(request.answer.charAt(ii + 1)) > -1) {
          symbsAllow += 1;
        }
      }
    }

    if (symbsAllow == 9) {
      //Serial.println(request.answer + " " + request.answer.substring(3, 5) + " " + DecToInt(request.answer.substring(3, 5)));
      return DecToInt(request.answer.substring(3, 5));
    }
    else {
      //ошибка
      Serial.println("ошибка (03)");
      return -1; 
    }
  }
  else {
    Serial.println("ошибка (01)");
    return -1;
  }
}

bool checkDInputs() {

  Answer request = command(INPUT_COMMANDS[NOW_INPUT] + static_cast <char> (120) + static_cast <char> (48) + static_cast <char> (68));
  
  if (request.success) {
    int symbsAllow = 0;
    if (request.answer.length() == 9) {
      if (request.answer.charAt(0) == '!') {
        symbsAllow += 1;
      }
      else if (request.answer.charAt(0) == '?') {
        //ошибка
        Serial.println("ошибка (02)");
        return false;
      }
      String allowChars = "0123456789ABCDEF";
      for (int ii = 0; ii < 8; ii++) {
        if (allowChars.indexOf(request.answer.charAt(ii + 1)) > -1) {
          symbsAllow += 1;
        }
      }
    }

    if (symbsAllow == 9) {
      int result = 65535 - DecToInt(request.answer.substring(3, 7));

      if (result != INPUTS[NOW_INPUT]) { //временно
        for (int ii = 0; ii < 16; ii++) {
          if (result & int(pow(2, ii)) && (INPUTS[NOW_INPUT] & int(pow(2, ii))) == 0) {
            Serial.println("Значение датчика [" + String(NOW_INPUT) + " - " + String(ii) + "] изменилось с 0 на 1.");
          }
          if ((result & int(pow(2, ii))) == 0 && INPUTS[NOW_INPUT] & int(pow(2, ii))) {
            Serial.println("Значение датчика [" + String(NOW_INPUT) + " - " + String(ii) + "] изменилось с 1 на 0.");
          }
        }
      }

      INPUTS[NOW_INPUT] = result;
    }
    else {
      //ошибка
      Serial.println("ошибка (03)");
      return false; 
    }
  }
  else {
    Serial.println("ошибка (01)");
    return false;
  }

  NOW_INPUT = (NOW_INPUT + 1) % COUNT_INPUTS;

  return true;
}

bool sendOutputs() {
  for (int i = 0; i < COUNT_OUTPUTS; i++) {
    if (OUTPUTS[i] != lastOUTPUTS[i]) {
      Answer result = command(OUTPUT_COMMANDS[i] + IntToDec(OUTPUTS[i]));
      Serial.println("отправлена команда (управление): " + OUTPUT_COMMANDS[i] + IntToDec(OUTPUTS[i]) + " | ответ: " + result.answer);
      if (result.success) {
        if (result.answer == ">") {
          lastOUTPUTS[i] = OUTPUTS[i];
        }
        else {
          //ошибка
          Serial.println("ошибка (05):" + result.answer);
          return false;
        }
      }
      else {
        Serial.println("ошибка (01)");
        return false;
      }
    }
  }
  
  return true;
}

int getParams(String argNm, String val, String paramName = "params") {
  int ret = -1;
  if (argNm == paramName && val) {
    int symbsAllow = 0;
    
    String allowChars = "0123456789";
    for (int j = 0; j < val.length(); j++) {
      if (allowChars.indexOf(val.charAt(j)) > -1) {
        symbsAllow += 1;
      }
    }

    if (symbsAllow < val.length()) {
      return -1;
    }
    
    ret = val.toInt();
  }
  return ret;
}

void sendAnswer(int success) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "[" + String(success) + "]");
}

String getData() {
  
  String json = "[[";

  //клапаны

  for (int i = 0; i < COUNT_VALVES - 1; i++) {
    json += String(VALVES[i].get_data()) +  ",";
  }
  
  json += String(VALVES[COUNT_VALVES - 1].get_data());
  json += "], [";

  //реле

  for (int i = 0; i < COUNT_RELES - 1; i++) {
    json += String(RELES[i].get_data()) +  ",";
  }
  
  json += String(RELES[COUNT_RELES - 1].get_data());
  json += "], [";

  //данные контроллеров

  for (int i = 0; i < COUNT_CONTROLLERS - 1; i++) {
    json += String(CONTROLLERS[i] -> get_data()) +  ",";
  }

  json += String(CONTROLLERS[COUNT_CONTROLLERS - 1] -> get_data()) +  "],[";

  for (int i = 0; i < ERROR_COUNT - 1; i++) {
    json += String(ERRORS[i]) +  ",";
  }

  json += String(ERRORS[ERROR_COUNT - 1]) +  "]]";

  return(json);
}

void setup() {

  Serial.begin(115200);
  RS485.begin(19200);
  Timer.begin();
  Timer.settime(0, 0, 0, 0, 0, 0, 0);

  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
   
  Serial.println("");

  // Wait for connection
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 60) {
    delay(1000);
    Serial.print(i); Serial.print(" ");
    i++;
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

   //+++++++++++++++++++++++ START REFRESH ++++++++++++++++++++

  server.on("/", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", getData());
  });

  server.on("/settime", [](){

    //settime(секунды [, минуты [, часы [, день [, месяц [, год [, день недели]]]]]])
    int newdate[7] = {-1, -1, -1, -1, -1, -1, -1};

    int params;
    
    for (int i = 0; i < server.args(); i++) {
      params = getParams(server.argName(i), server.arg(i), "second");
      if (params > -1) {
        newdate[0] = params;
      }

      params = getParams(server.argName(i), server.arg(i), "minute");
      if (params > -1) {
        newdate[1] = params;
      }

      params = getParams(server.argName(i), server.arg(i), "hour");
      if (params > -1) {
        newdate[2] = params;
      }
      
      params = getParams(server.argName(i), server.arg(i), "day");
      if (params > -1) {
        newdate[3] = params;
      }

      params = getParams(server.argName(i), server.arg(i), "month");
      if (params > -1) {
        newdate[4] = params;
      }

      params = getParams(server.argName(i), server.arg(i), "year");
      if (params > -1) {
        newdate[5] = params;
      }

      params = getParams(server.argName(i), server.arg(i), "weekday");
      if (params > -1) {
        newdate[6] = params;
      }
    }

    Timer.settime(newdate[0], newdate[1], newdate[2], newdate[3], newdate[4], newdate[5], newdate[6]);

    Serial.print("Установлена новая дата: ");
    Serial.println(Timer.gettime("d-m-Y, H:i:s, D"));

    sendAnswer(1);
  });

  server.on("/check-license", [](){    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i) == "hash" && server.arg(i)) {
        String hash_ = server.arg(i);
  
        Serial.println("→ Команда от клиента: /check-license, hash = " + hash_);
  
        if (CONTROLLERS[LICENSE] -> checklicense(hash_)) {
          Serial.println("Продлен срок действия ПО");
          success = 1;
        }
        else {
          Serial.println("Ошибка продления срока действия ПО");
        }
  
        break;
      }
    }

    sendAnswer(success);
  });

  server.on("/ustavka", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {
      int params = getParams(server.argName(i), server.arg(i));

      if (params == -1) {
        continue;
      }

      int boiler = params % 100;
      int val = params / 100;

      if (boiler >= COUNT_CONTROLLERS || val < 2650 || val > 3250) {
        continue;
      }

      if (CONTROLLERS[boiler] -> type != 2) {
        continue;
      }

      Serial.println("→ Команда от клиента: /ustavka, boiler = " + String(CONTROLLERS[boiler] -> title) + ", val = " + String(val));

      if (CONTROLLERS[boiler] -> set_ust(val)) {
        Serial.println("Сохранено новое значение уставки " + String(val) + " для теплообменника " + String(CONTROLLERS[boiler] -> title));
        success = 1;
      }
      else {
        Serial.println("Ошибка сохранения значения уставки " + String(val) + " для теплообменника " + String(CONTROLLERS[boiler] -> title));
      }

      break;
    }

    sendAnswer(success);
  });

  server.on("/set-valve", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {         
      int params = getParams(server.argName(i), server.arg(i));

      if (params == -1) {
        continue;
      }

      int valve = params / 10;
      int com = params % 10;

      if ((com != 0 && com != 1 && com != 2) || valve >= COUNT_VALVES) {
        continue;
      }

      Serial.println("→ Команда от клиента: /set-valve, valve = " + String(VALVES[valve].title) + ", com = " + String(com));

      if (VALVES[valve].accessed() == 0) {
        Serial.println("Клапан " + String(VALVES[valve].title) + " в автоматическом режиме, операция отклонена");
        continue;
      }

      if (VALVES[valve].accessed() == 2) {
        Serial.println("Клапан " + String(VALVES[valve].title) + " в дистанционном режиме, операция отклонена");
        continue;
      }
      
      if (com == 1) {
        VALVES[valve].turn_open();
        Serial.println("Клапану " + String(VALVES[valve].title) + " отправлена команда на открытие");
      }
      else if (com == 0) {
        VALVES[valve].turn_close();
        Serial.println("Клапану " + String(VALVES[valve].title) + " отправлена команда на закрытие");
      }
      else {
        VALVES[valve].turn_stop();
        Serial.println("Клапану " + String(VALVES[valve].title) + " отправлена команда на остановку");
      }

      success = 1;
    }

    sendAnswer(success);
  });

  server.on("/set-rele", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {         
      int params = getParams(server.argName(i), server.arg(i));

      if (params == -1) {
        continue;
      }

      int pump = params / 10;
      int mode_ = params % 10;

      if ((mode_ != 0 && mode_ != 1) || pump >= COUNT_RELES) {
        continue;
      }

      Serial.println("→ Команда от клиента: /set-rele, pump = " + String(RELES[pump].title) + ", mode = " + String(mode_));

      if (RELES[pump].accessed() == 0) {
        Serial.println("Насос " + String(RELES[pump].title) + " в автоматическом режиме, операция отклонена");
        continue;
      }

      if (RELES[pump].accessed() == 2) {
        Serial.println("Насос " + String(RELES[pump].title) + " в дистанционном режиме, операция отклонена");
        continue;
      }
      
      if (mode_) {
        RELES[pump].turn_on();
        //Serial.println("Насос " + String(pump) + " включен");
      }
      else {
        RELES[pump].turn_off();
        //Serial.println("Насос " + String(pump) + " выключен");
      }

      success = 1;
    }

    sendAnswer(success);
  });

  server.on("/choose-pump-on", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {
      int params = getParams(server.argName(i), server.arg(i));

      if (params == -1) {
        continue;
      }

      int pump_list = params / 10;
      int pump = params % 10;

      if (pump_list >= COUNT_CONTROLLERS || pump >= COUNT_RELES) {
        continue;
      }

      if (CONTROLLERS[pump_list] -> type != 1) {
        continue;
      }

      Serial.println("→ Команда от клиента: /choose-pump-on, pump_list = " + String(CONTROLLERS[pump_list] -> title) + ", pump = " + String(RELES[pump].title));

      if (CONTROLLERS[pump_list] -> set_pump(pump)) {
        Serial.println("Установлено значение включенного насоса в группе насосов " + CONTROLLERS[pump_list] -> title);
        success = 1;
      }
      else {
        Serial.println("Ошибка установки значения включенного насоса в группе насосов " + CONTROLLERS[pump_list] -> title);
      }
    }

    sendAnswer(success);
  });

  server.on("/set-mode", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {
      int params = getParams(server.argName(i), server.arg(i));

      if (params == -1) {
        continue;
      }

      int mode_ = params % 10;
      int cont = params / 10;

      Serial.println("→ Команда от клиента: /set-mode, cont = " + String(CONTROLLERS[cont] -> title) + ", mode = " + String(mode_));

      if ((mode_ != 0 && mode_ != 1) || cont >= COUNT_CONTROLLERS) {
        continue;
      }

      if (CONTROLLERS[cont] -> get_mode() == 2) {
        Serial.println("Контроллер " + String(CONTROLLERS[cont] -> title) + " в дистанционном режиме, операция отклонена");
        continue;
      }

      CONTROLLERS[cont] -> set_mode(mode_);
      
      Serial.println("Изменен режим работы контроллера");

      success = 1;
    
      break;
    }

    sendAnswer(success);
  });

  server.on("/script", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {
      int params = getParams(server.argName(i), server.arg(i));

      if (params == -1) {
        continue;
      }

      int cont = params / 1000;
      int time_ = (params % 1000) / 10;
      int mode_ = params % 10;

      Serial.println("→ Команда от клиента: /script, cont = " + String(CONTROLLERS[cont] -> title) + ", mode = " + String(mode_) + ", time = " + String(time_));

      if ((mode_ != 0 && mode_ != 1) || cont >= COUNT_CONTROLLERS || time_ > 20 || time_ < 1) {
        continue;
      }

      if (CONTROLLERS[cont] -> get_mode() == 2) {
        Serial.println("Контроллер " + String(CONTROLLERS[cont] -> title) + " в дистанционном режиме, операция отклонена");
        continue;
      }

      if (mode_ == 1) {
        CONTROLLERS[cont] -> set_period(time_);
        if (CONTROLLERS[cont] -> go()) {
          success = 1;
        }
      }
      else {
        if (CONTROLLERS[cont] -> stop_go()) {
          success = 1;
        }
      }
    
      break;
    }

    sendAnswer(success);
  });

  server.on("/next-step", [](){
    if (CONTROLLERS[LICENSE] -> get_value() == 2) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "[2," + String(POOL_ID) + "]");
      return false;
    }
    
    int success = 0;
    
    for (int i = 0; i < server.args(); i++) {
      int params = getParams(server.argName(i), server.arg(i));

      if (params == -1) {
        continue;
      }

      int cont = params;

      Serial.println("→ Команда от клиента: /next-step, cont = " + String(CONTROLLERS[cont] -> title));

      if (cont >= COUNT_CONTROLLERS) {
        continue;
      }

      if (CONTROLLERS[cont] -> get_mode() == 2) {
        Serial.println("Контроллер " + String(CONTROLLERS[cont] -> title) + " в дистанционном режиме, операция отклонена");
        continue;
      }

      if (CONTROLLERS[cont] -> submit()) {
        Serial.println("Переход на следующий шаг в сценарии " + String(CONTROLLERS[cont] -> title));
        success = 1;
      }
      else {
        Serial.println("Ошибка перехода на следующий шаг в сценарии " + String(CONTROLLERS[cont] -> title));
      }
    
      break;
    }

    sendAnswer(success);
  });
   
   //+++++++++++++++++++++++ END REFRESH ++++++++++++++++++++
      
  server.begin();
  Serial.println("HTTP server started");  
  DS18b20.begin();
  DS18b20.setResolution(SENSORS[1], 12); 
  DS18b20.setResolution(SENSORS[2], 12); 
  DS18b20.setResolution(SENSORS[3], 12); 
  DS18b20.setResolution(SENSORS[4], 12);

  EEPROM.begin(512);
  delay(10);

  int count = 0;
  for (int i = 0; i < COUNT_OUTPUTS; i++) {
    int result = getRele(i);
    Serial.println("Данные реле №" + String(i) + ": " + String(result));
    if (result >= 0) {
      lastOUTPUTS[i] = result;
      OUTPUTS[i] = result;
      count ++;
    }
    else {
      //ошибка
      Serial.println("ошибка (01)");
    }
  }

  Serial.println("Состояние " + String(count) + " реле из " + String(COUNT_OUTPUTS) + " загружено");

  MEMORY_START = 0;
  for (int i = 0; i < COUNT_CONTROLLERS; i++) {
    MEMORY_START += CONTROLLERS[i] -> load();
  }

  for (int i = 0; i < COUNT_INPUTS; i++) {
    checkDInputs();
  }

  Serial.println("Загрузка данных завершена");
}

void loop() {

  DS18b20.requestTemperatures();

  //загрузка данных следующего адама
  
  checkDInputs();

  //обработка - обработка датчиков клапанов и вызов функции job в каждом контроллере

  for (int i = 0; i < COUNT_VALVES; i++) {
    VALVES[i].update_value();
  }
  for (int i = 0; i < COUNT_RELES; i++) {
    RELES[i].update_value();
  }
  for (int i = 0; i < COUNT_CONTROLLERS; i++) {
    CONTROLLERS[i] -> job();
  }

  //обработка запросов клиента

  server.handleClient();

  //отправка команд

  sendOutputs();

  delay (20);
  
}
