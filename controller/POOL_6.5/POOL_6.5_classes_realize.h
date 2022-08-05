//Valve

bool Valve::accessed() {
  if (CONTROLLERS[controller] -> get_mode() == 1) {
    return true;
  }
  else {
    return false;
  }
}

void Valve::update_value() {
  if (INPUTS[input_ustr_close] & int(pow(2, input_num_close))) {
    value = 0;
    if (OUTPUTS[output_ustr_close] & int(pow(2, output_num_close))) {
      turn_stop();
    }
    bitClear(ERRORS[0], id);
  } else if (INPUTS[input_ustr_open] & int(pow(2, input_num_open))) {
    value = 1;
    if (OUTPUTS[output_ustr_open] & int(pow(2, output_num_open))) {
      turn_stop();
    }
    bitClear(ERRORS[0], id);
  } else if (OUTPUTS[output_ustr_close] & int(pow(2, output_num_close))) {
    
    if (value == 0 || value == 1) {
      changing_start = millis();
    }
    
    if (millis() - changing_start > 60000 && CONTROLLERS[controller] -> get_mode() != 2) {
      turn_stop();
      
      bitSet(ERRORS[0], id);
    }
    
    value = 2;
    
  } else if (OUTPUTS[output_ustr_open] & int(pow(2, output_num_open))) {

    if (value == 0 || value == 1) {
      changing_start = millis();
    }
    
    if (millis() - changing_start > 60000 && CONTROLLERS[controller] -> get_mode() != 2) {
      turn_stop();
      
      bitSet(ERRORS[0], id);
    }
    
    value = 3;
    
  } else {
    
    if (value == 0 || value == 1) {
      changing_start = millis();
    }
    
    if (millis() - changing_start > 60000 && CONTROLLERS[controller] -> get_mode() != 2) {
      bitSet(ERRORS[0], id);
    }
    
    value = 4;
    
  }
}

//Rele

bool Rele::accessed() {
  if (CONTROLLERS[controller] -> get_mode() == 1) {
    return true;
  }
  else {
    return false;
  }
}

//Boiler

byte Boiler::get_value() {
  return VALVES[valve].get_value();
}

void Boiler::job(){
  
  if (in) {
    temp_in = round(DS18b20.getTempC(SENSORS[in]) * KOEF);

    if (temp_in > 0) {
      errors[0] = 0;
      bitClear(ERRORS[3], in);
    }
    else {
      errors[0] += 1;

      if (errors[0] > 2) {
        bitSet(ERRORS[3], in);
      }
    }
  }
  if (out) {
    temp_out = round(DS18b20.getTempC(SENSORS[out]) * KOEF);

    if (temp_out > 0) {
      errors[1] = 0;
      bitClear(ERRORS[3], out);
    }
    else {
      errors[1] += 1;

      if (errors[1] > 2) {
        bitSet(ERRORS[3], out);
      }
    }
  }
  if (hot_in) {
    temp_hot_in = round(DS18b20.getTempC(SENSORS[hot_in]) * KOEF);

    if (temp_hot_in > 0) {
      errors[2] = 0;
      bitClear(ERRORS[3], hot_in);
    }
    else {
      errors[2] += 1;

      if (errors[2] > 2) {
        bitSet(ERRORS[3], hot_in);
      }
    }
  }
  if (hot_out) {
    temp_hot_out = round(DS18b20.getTempC(SENSORS[hot_out]) * KOEF);

    if (temp_hot_out > 0) {
      errors[3] = 0;
      bitClear(ERRORS[3], hot_out);
    }
    else {
      errors[3] += 1;

      if (errors[3] > 2) {
        bitSet(ERRORS[3], hot_out);
      }
    }
  }
  
  if (mode_ == 0) {
    if (temp_in > 0) {
      if (temp_in + offset < ust - gist_down && VALVES[valve].get_value() == 0) {
        //вкл
        VALVES[valve].turn_open();
        Serial.println("Теплообменник " + title + " включен.");
      }
      if (temp_in + offset > ust + gist_up && VALVES[valve].get_value() == 1) {
        //выкл
        VALVES[valve].turn_close();
        Serial.println("Теплообменник " + title + " выключен.");
      }
      
      if (ust - gist_down - (temp_in + offset) > limit_min) {
        bitSet(ERRORS[1], id);
      }
      if (ust - gist_down - (temp_in + offset) < 10) {
        bitClear(ERRORS[1], id);
      }
      
      if (temp_in + offset - ust - gist_up > limit_max) {
        bitSet(ERRORS[2], id);
      }
      if (temp_in + offset - ust - gist_up < 10) {
        bitClear(ERRORS[2], id);
      }
    }
  }
  else {
    bitClear(ERRORS[1], id);
    bitClear(ERRORS[2], id);
  }
}

//PumpGroup

void PumpGroup::job() {
  if (INPUTS[input_ustr] & int(pow(2, input_num))) {
    set_mode(2);
  } else {
    if (mode_ == 2) {
      set_mode(0);
    }
  }
  
  if (mode_ == 0) {
    if ((Timer.day == 1 || Timer.day == 15) && Timer.Hours >= 11) {
      if (changed == 0) {
        pump_on = (pump_on - first_pump + 1) % count_pumps + first_pump;
        changed = 1;

        EEPROM.put(memory_start + 1, pump_on);
        EEPROM.put(memory_start + 2, changed);
          
        if (EEPROM.commit()) {
          Serial.println("Сохранены новые значения группы насосов " + title);
        }
        else {
          Serial.println("Ошибка сохранения значений группы насосов " + title);
        }
      }
    }
    else {
      if (changed) {
        changed = 0;

        EEPROM.put(memory_start + 2, changed);
        
        if (EEPROM.commit()) {
          Serial.println("Сохранены новые значения группы насосов " + title);
        }
        else {
          Serial.println("Ошибка сохранения значений группы насосов " + title);
        }
      }
    }

    for (int i = 0; i < count_pumps; i++) {
      if (pumps[i] == pump_on) {
        if (RELES[pumps[i]].get_value() == 0) {
          RELES[pumps[i]].turn_on();
          Serial.println("Насос " + RELES[pumps[i]].title + " включен.");
        }
      }
      else {
        if (RELES[pumps[i]].get_value() == 1) {
          RELES[pumps[i]].turn_off();
          Serial.println("Насос " + RELES[pumps[i]].title + " выключен.");
        }
      }
    }
  }
}

//Filter

String Filter::get_data() {
  byte value = 0;
  if (VALVES[v1].get_value() == 1 && VALVES[v2].get_value() == 1 && VALVES[v3].get_value() == 0 && VALVES[v4].get_value() == 0) {
    value = 1;
  } else if (VALVES[v1].get_value() == 0 && VALVES[v2].get_value() == 0 && VALVES[v3].get_value() == 1 && VALVES[v4].get_value() == 1) {
    value = 2;
  }
  
  return "[2,5," + String(value) + "]";
}

String Pool::get_data() {
  return "[2,6," + String(CONTROLLERS[boiler] -> get_pool_temp()) + "]";
}

String Dosage::get_data() {
  if (get_mode() == 3) {
    return "[3,7," + String(rele) + "]";
  }
  else {
    return "[" + String(RELES[rele].get_value()) + ",7," + String(rele) + "]";
  }
}

void Podpitka::job() {
  if (mode_ == 0) {
    if (INPUTS[input_ustr] & int(pow(2, input_num))) {
      timer = millis();
      VALVES[valve].turn_open();
    } else {
      if (abs(millis() - timer) >= per) {
        VALVES[valve].turn_close();
      }
    }
  }
}

void Donnik::job() {
  Timer.gettime();
  
  if (mode_ == 0) {
    if (Timer.Hours >= 23 || Timer.Hours < 6) {
      if (VALVES[valve_relation].get_value() == 1) {
        VALVES[valve].turn_open();
      } else if (VALVES[valve_relation].get_value() == 0) {
        VALVES[valve].turn_close();
      }
    }
    else {
      VALVES[valve].turn_close();
    }
  }
}

bool Script::stop_go() {
  if (value == 1) {
    for (int i = 0; i < steps_length; i++) {
      
      if (Steps[i].lastpos != -1) {
        if (Steps[i].type == 2) {
          
          if (Steps[i].lastpos == 0) {
            RELES[Steps[i].n].turn_off();
          }
          else if (Steps[i].lastpos == 1) {
            RELES[Steps[i].n].turn_on();
          }
          
        }
        else if (Steps[i].type == 3) {
          
          CONTROLLERS[Steps[i].n] -> mode_ = Steps[i].lastpos;
          
        }
      }
    }
    
    value = 0;
    Serial.println("Остановлен сценарий " + String(title));
    return true;
  }
  else {
    Serial.println("Ошибка остановки сценария " + String(title) + " (не запущен)");
    return false;
  }
}

bool Script::go() {
  if (value == 0) {
    value = 1;
    step_ = 0;
    Serial.println("Запущен сценарий " + String(title));
    return true;
  }
  else {
    Serial.println("Ошибка запуска сценария " + String(title) + " (уже запущен)");
    return false;
  }
}

void Script::job() {
  if (value == 1) {
    if (step_ < steps_length) {
      switch (Steps[step_].type) {
        case 1:
          if (VALVES[Steps[step_].n].get_value() == Steps[step_].pos) {
            next();
          }
          else {
            if (Steps[step_].pos == 0) {
              VALVES[Steps[step_].n].turn_close();
            }
            else if (Steps[step_].pos == 1) {
              VALVES[Steps[step_].n].turn_open();
            }
          }
          break;
        case 2:
          if (RELES[Steps[step_].n].get_value() == Steps[step_].pos) {
            next();
          }
          else {
            Steps[step_].lastpos = RELES[Steps[step_].n].get_value();
            if (Steps[step_].pos == 0) {
              RELES[Steps[step_].n].turn_off();
            }
            else if (Steps[step_].pos == 1) {
              RELES[Steps[step_].n].turn_on();
            }
          }
          break;
        case 3:
          if (CONTROLLERS[Steps[step_].n] -> mode_ == 3) {
            next();
          }
          else {
            Steps[step_].lastpos = CONTROLLERS[Steps[step_].n] -> mode_;
            CONTROLLERS[Steps[step_].n] -> mode_ = 3;
          }
          break;
        case 4:
          if (Steps[step_].n == 1) {
            if (abs(millis() - timer) >= per) {
              Steps[step_].n = 0;
              next();
            }
          }
          else {
            Steps[step_].n = 1;
            timer = millis();
          }
          break;
      }
    }
    else {
      stop_go();
    }
  }
}
