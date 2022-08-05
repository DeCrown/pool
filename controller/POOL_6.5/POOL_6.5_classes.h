int NOW_INPUT = 0;

String OUTPUT_COMMANDS[] = {
  "#07S000", "#07S100", "#07S200", "#07S300", "#08S000", "#08S100", "#08S200", "#08S300"
};

String INPUT_COMMANDS[] {
  "$06S06", "$06S16", "$06S26", "$06S36", "$05S36"
};

String RELE_COMMANDS[] {
  "$07S06", "$07S16", "$07S26", "$07S36", "$08S06", "$08S16", "$08S26", "$08S36"
};

struct Answer {String answer; int success; };

// 0 - клапан слишком долго едет, 1 - температура слишком низкая, 2 - температура слишком высокая, 3 - термодатчик не отвечает, 4 - общие ошибки, ...
int ERROR_COUNT = 5;
int ERRORS[] = {0, 0, 0, 0, 0};

struct argTemp {
  byte in;
  byte out;
  byte hot_in;
  byte hot_out;
};

struct argInOut {
  byte iou;
  byte ion;
  byte icu;
  byte icn;
  byte oou;
  byte oon;
  byte ocu;
  byte ocn;
};

struct argPumps {
  byte p1;
  byte p2;
  byte p3;
  byte p4;
  byte p5;
};

class Valve {
  private:
    //value: 0 - закрыт, 1 - открыт, 2 - открывается, 3 - закрывается, 4 - стоит не в крайнем положении
    byte controller;
    byte input_ustr_open, input_num_open, input_ustr_close, input_num_close, output_ustr_open, output_num_open, output_ustr_close, output_num_close, value;
    int changing_start;
  public:
    String title;
    int id;
    
    Valve(int id_, String title_, byte cont, argInOut data)
    {
      title = title_;
      input_ustr_open = data.iou;
      input_num_open = data.ion;
      input_ustr_close = data.icu;
      input_num_close = data.icn;
      output_ustr_open = data.oou;
      output_num_open = data.oon;
      output_ustr_close = data.ocu;
      output_num_close = data.ocn;
      value = 4;
      controller = cont;
      id = id_;

      changing_start = millis();
    }
  
    void turn_open() {
      bitSet(OUTPUTS[output_ustr_open], output_num_open);
      bitClear(OUTPUTS[output_ustr_close], output_num_close);
      update_value();
    }

    void turn_close() {
      bitSet(OUTPUTS[output_ustr_close], output_num_close);
      bitClear(OUTPUTS[output_ustr_open], output_num_open);
      update_value();
    }

    void turn_stop() {
      bitClear(OUTPUTS[output_ustr_close], output_num_close);
      bitClear(OUTPUTS[output_ustr_open], output_num_open);
      update_value();
    }

    /*void set_value(int newvalue) {
      value = newvalue;
    }*/

    byte get_value() {
      return value;
    }

    String get_data() {
      return "[" + String(value) + ", " + String(controller) + "]";
    }

    bool accessed();

    /*bool too_long() {
      if ((value == 2 || value == 3) && millis() - changing_start > 60000) {
        return true;
      }
      else {
        return false;
      }
    }*/

    void update_value();

    /*void check() {
      if (input_ustr_close == NOW_INPUT && INPUTS[NOW_INPUT] & int(pow(2, input_num_close))) {
        turn_stop();
      }
    }*/
};

class Rele {
  private:
    byte controller, value, output_ustr, output_num;

  public:
    String title;
    
    Rele(String title_, int cont, byte ustr, byte num) {
      title = title_;
      controller = cont;
      value = 0;
      output_ustr = ustr;
      output_num = num;
    }

    void update_value() {
      if (OUTPUTS[output_ustr] & int(pow(2, output_num))) {
        value = 1;
      } else {
        value = 0;
      }
    }

    void turn_on() {
      bitSet(OUTPUTS[output_ustr], output_num);
      update_value();
    }

    void turn_off() {
      bitClear(OUTPUTS[output_ustr], output_num);
      update_value();
    }

    bool accessed();

    byte get_value() {
      return value;
    }

    String get_data() {
      return "[" + String(value) + ", " + String(controller) + "]";
    }
  
};

/*

0
режим

1
режим
сменен ли
включенный насос

2
режим
уставка
поправка

*/


class Controller {
  protected:
    int memory_start;
    int memory_length;
  
    int addr_mode;
    byte default_mode;
  public:
    byte mode_; //0 - авт, 1 - ручной, 2 - дистанция
    String title;
    byte type; //0 - пустой, 1 - группа насосов, 2 - теплообменник, 3 - контроллер клапана, 4 - озоногенератор, 5 - фильтр, 6 - бассейн, 7 - дозация, 8 - сценарий, 9 - лизенция
    int id;
    
    Controller(int id_, String title_, byte type_, byte default_mode_ = 0) {
      id = id_;
      title = title_;
      mode_ = default_mode_;
      type = type_;
      default_mode = default_mode_;
    }

    void save() {
      EEPROM.put(memory_start, mode_);
    
      if (EEPROM.commit()) {
        Serial.println("Изменен режим работы для контроллера " + title);
      }
      else {
        Serial.println("Ошибка изменения режима работы для контроллера " + title);
      }
    }

    void set_mode(byte mod) {
      if (mode_ != mod && mode_ != 3) {

        if (mod == 3) {
          mode_ = 3;
        }
        else {
          mode_ = mod;
          
          if (mode_ != 0 && mode_ != 1 && mode_ != 2) {
            mode_ = default_mode;
          }
  
          save();
        }
      }
    }

    int get_mode() {
      return mode_;
    }

    virtual void job() {}

    virtual int load() {
      memory_length = 1;
      memory_start = MEMORY_START;
      
      byte mod = 0;
      EEPROM.get(memory_start, mod);
      set_mode(mod);
      
      return memory_length;
    }

    virtual bool set_ust(int newust) {}

    virtual bool set_pump(byte newpump) {}

    virtual bool check_pump(byte newpump) {}

    virtual byte get_value() {}

    virtual void check() {}

    virtual String get_data() {
      return "[" + String(get_mode()) + "," + String(type) + "," + String(POOL_ID) + "]";
    }
    
    virtual int get_pool_temp() {}

    virtual bool go() {}

    virtual bool stop_go() {}

    virtual void next() {}

    virtual bool submit() {}

    virtual void set_period(int period_) {}

    virtual bool checklicense(String str) {}
};

class License : public Controller  {
  private:
    byte licensed; // 0 - продлена, 1 - ожидается продление, 2 - отключено
    int timer;
    int last_timer;
    int last_save;

    String secret;
    byte new_secret;
  public:
    License(int id_, String title_, String secret_ = "", byte new_ = 0) : Controller (id_, title_, 9, 0)
    {
      timer = 0;
      licensed = 0;
      last_timer = 0;
      last_save = 0;
      
      secret = secret_;
      new_secret = new_;
    }

    int load() {
      memory_length = 5;
      memory_start = MEMORY_START;

      EEPROM.get(memory_start + 1, timer);

      return memory_length;
    }

    void job() {
      int dif = millis() / 1000 - last_timer;
      
      if (dif > 0) {
        timer += dif;
      }
      
      last_timer = millis() / 1000;

      if (licensed == 0) {
        bitClear(ERRORS[4], 1);
        
        if (timer > LICENSE_PERIOD) {
          licensed = 1;
          bitSet(ERRORS[4], 1);
          timer = 0;

          EEPROM.put(memory_start + 1, timer);
          EEPROM.commit();
        
          last_save = timer;
        }
      }
      else if (licensed == 1) {
        if (timer > LICENSE_DOP_PERIOD) {
          licensed = 2;
          bitSet(ERRORS[4], 1);
        }
      }

      if (timer - last_save > 3600) {
        EEPROM.put(memory_start + 1, timer);
        EEPROM.commit();
        
        last_save = timer;
      }
    }

    bool checklicense(String str) {
      Timer.gettime();
      String generate_str = "";
      
      if (Timer.day < 10) {
        generate_str += "0";
      }
      generate_str += String(Timer.day);
      if (Timer.month < 10) {
        generate_str += "0";
      }
      generate_str += String(Timer.month);
      generate_str += String(Timer.year);
      
      String hash_str = sha1(generate_str + secret);
      
      if (str == hash_str) {
        licensed = 0;
        timer = 0;
        return true;
      }
      else {
        return false;
      }
    }

    byte get_value() {
      return licensed;
    }

    String get_data() {
      if (licensed == 0) {
        return "[0,9,0," + String(LICENSE_PERIOD - timer) + "]";
      }
      if (licensed == 1) {
        return "[0,9,1," + String(LICENSE_DOP_PERIOD - timer) + "]";
      }
      if (licensed == 2) {
        return "[0,9,2]";
      }
    }
};

class Boiler : public Controller  {
  private:
    int valve;
    byte in, out, hot_in, hot_out;
    int temp_in, temp_out, temp_hot_in, temp_hot_out;
    byte errors[4];
    int gist_up, gist_down;
    int ust;
    int offset, standart;
    int limit_min, limit_max;
  public:
    Boiler(int id_, String title_, int valve_, argTemp temp_, int offset_, int standart_, int gist_up_, int gist_down_, int limit_min_, int limit_max_) : Controller (id_, title_, 2)
    {
      valve = valve_;
      
      in = temp_.in;
      out = temp_.out;
      hot_in = temp_.hot_in;
      hot_out = temp_.hot_out;
      temp_in = -12700;
      temp_out = -12700;
      temp_hot_in = -12700;
      temp_hot_out = -12700;

      limit_min = limit_min_;
      limit_max = limit_max_;

      errors[0] = 0; errors[1] = 0; errors[2] = 0; errors[3] = 0;

      offset = offset_;
      standart = standart_;
      ust = standart;

      gist_up = gist_up_;
      gist_down = gist_down_;
    }

    int load() {
      memory_length = 29;
      memory_start = MEMORY_START;
      
      byte mod = 0;
      EEPROM.get(memory_start, mod);
      set_mode(mod);
      
      EEPROM.get(memory_start + 1, ust);
      if (ust < 1500 || ust > 3500) {
        ust = standart;
        EEPROM.put(memory_start + 1, ust);

        if (EEPROM.commit()) {
          Serial.println("Сброшено значение уставки темплообенника " + title);
        }
        else {
          Serial.println("Ошибка сброса значения уставки темплообенника " + title);
        }
      }

      return memory_length;
    }

    byte get_value();

    bool set_ust(int newust) {
      if (newust < 2650 || newust > 3250) {
        return false;
      }

      ust = newust;

      EEPROM.put(memory_start + 1, ust);
      
      if (EEPROM.commit()) {
        return true;
      }
      else {
        return false;
      }
    }

    void job();

    String get_data() {      
      return "[" + String(get_mode()) + ",2, " + String(valve) + "," + String(temp_in) + "," + String(temp_out) + "," + String(temp_hot_in) + "," + String(temp_hot_out) + "," + String(ust) + "]";
    }

    int get_pool_temp() {
      return temp_in + offset;
    }
};


class ValveController : public Controller  {
  protected:
    int valve;
  public:    
    ValveController(int id_, String title_, int valve_, int mode_ = 0) : Controller (id_, title_, 3, mode_)
    {
      valve = valve_;
    }
};

class Podpitka : public ValveController {
  byte input_ustr, input_num;
  int timer;
  int per;
  public:    
    Podpitka(int id_, String title_, int valve_, byte input_ustr_, byte input_num_, int period_, int mode_ = 0) : ValveController (id_, title_, valve_, mode_)
    {
      input_ustr = input_ustr_;
      input_num = input_num_;
      per = period_ * 1000;
    }

    void job();
};

class Donnik : public ValveController {
  int valve_relation;
  public:    
    Donnik(int id_, String title_, int valve_, int valve_relation_, int mode_ = 0) : ValveController (id_, title_, valve_, mode_)
    {
      valve_relation = valve_relation_;
    }

    void job();
};

class Ozon : public Controller  {
  private:
    int rele;
    byte mode_ustr, mode_num, input_ustr, input_num, value;
  public:
    Ozon(int id_, String title_, int rele_, byte mode_ustr_, byte mode_num_, byte input_ustr_, byte input_num_) : Controller (id_, title_, 4, 2)
    {
      rele = rele_;
      mode_ustr = mode_ustr_;
      mode_num = mode_num_;
      input_ustr = input_ustr_;
      input_num = input_num_;
    }

    void job() {
      if (INPUTS[mode_ustr] & int(pow(2, mode_num))) {
        set_mode(2);
      } else {
        if (mode_ == 2) {
          set_mode(1);
        }
      }

      if (INPUTS[input_ustr] & int(pow(2, input_num))) {
        value = 1;
      } else {
        value = 0;
      }
    }

    String get_data() {
      return "[" + String(get_mode()) + ",4, " + String(rele) + "," + String(value) + "]";
    }
};

class Filter : public Controller  {
  private:
    int v1, v2, v3, v4;
  public:
    Filter(int id_, String title_, int v1_, int v2_, int v3_, int v4_) : Controller (id_, title_, 5, 2)
    {
      v1 = v1_;
      v2 = v2_;
      v3 = v3_;
      v4 = v4_;
    }

    String get_data();
};

class PumpGroup : public Controller  {
  private:
    byte count_pumps, first_pump;
    byte *pumps;
    byte input_ustr, input_num;
    
    byte changed;
    byte pump_on;
  public:
    PumpGroup(int id_, String title_, byte count, byte *pumps_, byte first_pump_, byte input_ustr_, byte input_num_) : Controller (id_, title_, 1)
    {
      count_pumps = count;

      pumps = pumps_;

      first_pump = first_pump_;
      input_ustr = input_ustr_;
      input_num = input_num_;
    }

    int load() {
      memory_length = 3;
      memory_start = MEMORY_START;
      
      byte mod = 0;
      EEPROM.get(memory_start, mod);
      set_mode(mod);
      
      EEPROM.get(memory_start + 1, changed);
      if (changed != 0 && changed != 1) {
        changed = 0;
        EEPROM.put(memory_start + 1, changed);

        if (EEPROM.commit()) {
          Serial.println("Сброшено значение счетчика включения группы насосов " + title);
        }
        else {
          Serial.println("Ошибка сброса значения счетчика включения группы насосов " + title);
        }
      }
      
      EEPROM.get(memory_start + 2, pump_on);
      if (!check_pump(pump_on)) {
        pump_on = first_pump;
        
        EEPROM.put(memory_start + 2, pump_on);
        if (EEPROM.commit()) {
          Serial.println("Сброшено значение включенного насоса в группе насосов " + title);
        }
        else {
          Serial.println("Ошибка сброса значения включенного насоса в группе насосов " + title);
        }
      }

      return memory_length;
    }

    bool set_pump(byte newpump) {
      if (check_pump(pump_on)) {
        pump_on = newpump;
        
        EEPROM.put(memory_start + 2, pump_on);
        if (EEPROM.commit()) {
          return true;
        }
        else {
          return false;
        }
      }
      else {
        return false;
      }
    }

    bool check_pump(byte newpump) {
      for (int i = 0; i < count_pumps; i++) {
        if (pumps[i] == newpump) {
          return true;
        }
      }
      return false;
    }

    byte get_value() {
      return pump_on;
    }

    void job();
};

class Pool : public Controller  {
  private:
    int boiler;
  public:
    Pool(int id_, String title_, int boiler_) : Controller (id_, title_, 6, 2)
    {
      boiler = boiler_;
    }

    String get_data();
};

class Dosage : public Controller  {
  private:
    int rele;
  public:
    Dosage(int id_, String title_, int rele_) : Controller (id_, title_, 7, 1)
    {
      rele = rele_;
    }

    String get_data();
};

struct Step {byte type; int n; int pos; int lastpos; };

class Script : public Controller  {
  private:
    int step_;
    int value;
    // 1 - клапан, 2 - реле, 3 - режим работы, 4 - пауза
    int steps_length;
    Step *Steps;
    int timer, per;
  public:  
    Script(int id_, String title_, int steps_l, Step *steps_, int period_ = 600) : Controller (id_, title_, 8, 1)
    {
      Steps = steps_;
      steps_length = steps_l;
      value = 0;
      step_ = 0;
      per = period_ * 1000;
    }

    void set_period(int period_) {
      per = period_ * 60000;
    }

    bool go();

    bool stop_go();

    bool submit() {
      if (Steps[step_].type == 0) {
        next();
        return true;
      }
      else {
        return false;
      }
    }

    void next() {
      step_ += 1;
    }

    void job();

    String get_data() {
      if (value == 1) {
        switch(Steps[step_].type) {
          case 0:
            return "[" + String(get_mode()) + ",8,1," + String(step_) + "," + String(steps_length) + ",0," + String(Steps[step_].pos) + "]";
            break;
          case 1:
            return "[" + String(get_mode()) + ",8,1," + String(step_) + "," + String(steps_length) + ",1," + String(Steps[step_].n) + "]";
            break;
          case 2:
            return "[" + String(get_mode()) + ",8,1," + String(step_) + "," + String(steps_length) + ",2," + String(Steps[step_].n) + "]";
            break;
          case 3:
            return "[" + String(get_mode()) + ",8,1," + String(step_) + "," + String(steps_length) + ",3," + String(Steps[step_].n) + "]";
            break;
          case 4:
            return "[" + String(get_mode()) + ",8,1," + String(step_) + "," + String(steps_length) + ",4," + String(per - abs(millis() - timer)) + "]";
            break;
        }
      }
      else {
        return "[" + String(get_mode()) + ",8,0]";
      }
    }
};
