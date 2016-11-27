#include <string.h>

// Set ENERGIA = 1 to build for MSP430 launchpad
// Otherwise, set ENERGIA to 0 to build with Arduino board
#define ENERGIA 0

// MSP430 LaunchPad does not support eeprom

#if ENERGIA
#else
#include <EEPROM.h>
#endif

#if ENERGIA
#define LED_RUN_PIN       RED_LED
#define OUTPUT_PIN    GREEN_LED
#else
#define OUTPUT_PIN 13
#endif

#if ENERGIA
void led_toggle()
{
  static char led_state = 0;
  if (led_state == 0)
  {
    digitalWrite(LED_RUN_PIN, HIGH);
    led_state = 1;
  }
  else
  {
    digitalWrite(LED_RUN_PIN, LOW);
    led_state = 0;
  }
}
#endif

void eeprom_write(char * data, int data_len)
{
#if ENERGIA
#else
  int i = 0;
  for (i = 0; i < data_len; i++)
    EEPROM.write(i, data[i]);
#endif
}
void eeprom_read(char * data, int data_len)
{
#if ENERGIA
#else
  int i = 0;
  for (i = 0; i < data_len; i++)
    data[i] = EEPROM.read(i);
#endif
}

#define PUBLIC_ADDR -1
#define FORBIDDEN_ADDR 0

struct Data
{
  int addr;
  int power;
};

struct Data stored_data;

// <addr>:<command>=<key>,<value>\r
char _addr[10];
char _cmd[10];
char _key[10];
char _val[10];

// key
const char key_set = 1;
const char key_get = 2;
const char key_rep = 3;

// Command list
// <addr>:power=<key>,<value>\r
const char * power = "power";
// <addr>:address=<key>,<value>\r
const char * address = "address";
// <addr>:search=<key>,<value>\r
const char * search = "search";

#define INVALID_ADDRESS -1

// 0 - Idle
// 1 - Level 1  "Second"
// 2 - Level 2  "Minute"
// 3 - Level 3  "Hour"
char search_state = 0;
char search_delay_under_100ms = 0;
char search_delay_100ms_count = 0;
char search_report = 0;

void search_process(void)
{
  switch (search_state)
  {
  case 0: // Idle
    if (search_delay_100ms_count > 0)
    {
      search_delay_100ms_count--;
    }
    else if (search_delay_under_100ms > 0)
    {
      //delay(search_delay_under_100ms);
      search_delay_under_100ms = 0;
    }
    else if (search_report != 0)
    {
      char search_report_msg[32];
      memset(search_report_msg, 0, 32);
      sprintf(search_report_msg, "\r\n%i:%s=%i,%i\r\n",
      stored_data.addr, search, key_rep, stored_data.addr);
      Serial.write(search_report_msg);
      search_report = 0;
    }
    break;
  case 1: // Level 1
    search_delay_100ms_count = ((stored_data.addr % 60) * 20) / 100;
    search_delay_under_100ms = ((stored_data.addr % 60) * 20) % 100;

    delay(search_delay_under_100ms);
    search_delay_under_100ms = 0;

    if (search_delay_100ms_count == 0)
    {
      // trigger process now
      char search_report_msg[32];
      memset(search_report_msg, 0, 32);
      sprintf(search_report_msg, "\r\n%i:%s=%i,%i\r\n",
      stored_data.addr, search, key_rep, stored_data.addr);
      Serial.write(search_report_msg);
      search_report = 0;
    }
    else
    {
      search_report = 1;
    }
    search_state = 0; // switch back to idle process
    break;
  case 2: // Level 2
    search_delay_100ms_count = (((stored_data.addr / 60) % 60) * 20) / 100;
    search_delay_under_100ms = (((stored_data.addr / 60) % 60) * 20) % 100;

    delay(search_delay_under_100ms);
    search_delay_under_100ms = 0;
    if (search_delay_100ms_count == 0)
    {
      // trigger process now
      char search_report_msg[32];
      memset(search_report_msg, 0, 32);
      sprintf(search_report_msg, "\r\n%i:%s=%i,%i\r\n",
      stored_data.addr, search, key_rep, stored_data.addr);
      Serial.write(search_report_msg);
      search_report = 0;
    }
    else
    {
      search_report = 1;
    }
    search_state = 0; // switch back to idle process
    break;
  case 3: // Level 3
    search_delay_100ms_count = ((stored_data.addr / 60 / 60) * 20) / 100;
    search_delay_under_100ms = ((stored_data.addr / 60 / 60) * 20) % 100;

    delay(search_delay_under_100ms);
    search_delay_under_100ms = 0;
    if (search_delay_100ms_count == 0)
    {
      // trigger process now
      char search_report_msg[32];
      memset(search_report_msg, 0, 32);
      sprintf(search_report_msg, "\r\n%i:%s=%i,%i\r\n",
      stored_data.addr, search, key_rep, stored_data.addr);
      Serial.write(search_report_msg);
      search_report = 0;
    }
    else
    {
      search_report = 1;
    }
    search_state = 0; // switch back to idle process
    break;
  default:
    search_state = 0;
    break;
  }
}


void analyze(char * line)
{
  char * sign, * point;

  // clean old data
  memset(_addr, 0, 10);
  memset(_cmd, 0, 10);
  memset(_key, 0, 10);
  memset(_val, 0, 10);

  if (strlen(line) > 40)
    return;

  point = line;
  sign = strchr(point, ':');
  if (sign == 0)
    return;
  memcpy(_addr, point, sign - point);
  point = sign + 1;
  sign = strchr(point, '=');
  if (sign == 0)
    return;
  memcpy(_cmd, point, sign - point);
  point = sign + 1;
  sign = strchr(point, ',');
  if (sign == 0)
    return;
  memcpy(_key, point, sign - point);
  point = sign + 1;
  memcpy(_val, point, strlen(point));

  //  Serial.write(_addr);
  //  Serial.write(_cmd);
  //  Serial.write(_key);
  //  Serial.write(_val);
}
int process(char * line)
{
  analyze(line);
  int tmp_addr = atoi(_addr);
  if (tmp_addr != 0 && tmp_addr != stored_data.addr)
    return INVALID_ADDRESS;
  if (memcmp(_cmd, power, strlen(power)) == 0)
  {
    switch (atoi(_key))
    {
    case key_set:
      {
        int power = atoi(_val);
        if (power >= 0 && power <= 255)
          analogWrite(OUTPUT_PIN, power);
        break;
      }
    case key_get:
      break;
    case key_rep:
      break;
    }
  }
  if (memcmp(_cmd, address, strlen(address)) == 0)
  {
    switch (atoi(_key))
    {
    case key_set:
      {
        int set_addr = atoi(_val);
        if (set_addr > 0)
        {
          stored_data.addr = set_addr;
          eeprom_write((char *)&stored_data, sizeof(stored_data));
        }
        break;
      }
    case key_get:
      {
        char search_report_msg[32];
        memset(search_report_msg, 0, 32);
        sprintf(search_report_msg, "\r\n%i:%s=%i,%i\r\n",
        stored_data.addr, address, key_rep, stored_data.addr);
        Serial.write(search_report_msg);
        break;
      }
    case key_rep:
      break;
    }
  }
  if (memcmp(_cmd, search, strlen(search)) == 0)
  {
    // process key words
    switch (atoi(_key))
    {
    case key_set:
      search_state = atoi(_val);
      break;
    case key_get:
      break;
    case key_rep:
      break;
    }
    // trigger search process intermedily
    search_process();
  }
  return 0;
}

// global line
char gb_line[50];
int gb_line_index = 0;

void setup() {
  // put your setup code here, to run once:

#if ENERGIA
  pinMode(LED_RUN_PIN, OUTPUT);
#endif
  pinMode(OUTPUT_PIN, OUTPUT);

  Serial.begin(9600);
  // restore stored data
  eeprom_read((char *)&stored_data, sizeof(stored_data));
  analogWrite(OUTPUT_PIN, stored_data.power);
  memset(gb_line, 0, 50);
}
unsigned long process_time = 0;

void loop() {
  if (Serial.available())
  {
    char tmp_char = Serial.read();
    if (tmp_char == '\r')
    {
      process(gb_line);
      Serial.flush();
      memset(gb_line, 0, 50);
      gb_line_index = 0;
    }
    else if (tmp_char == 0x7F || tmp_char == 0x08) // Backspace Delete
    {
      if (gb_line_index > 0)
        gb_line_index--;
    }
    else
    {
      gb_line[gb_line_index++] = tmp_char;
      if (gb_line_index > 49)
        gb_line_index = 0;
    }

    // echo
    //    Serial.write(tmp_char);
  }

  if (millis() - process_time > 100)
  {
    // run search progress
    search_process();

    // run LED
#if ENERGIA
    led_toggle();
#endif
    // update process time
    process_time = millis();
  }
}

