#include <EEPROM.h>
#include <string.h>

#define OUTPUT_PIN 13

void eeprom_write(char * data, int data_len)
{
  int i = 0;
  for (i = 0; i < data_len; i++)
    EEPROM.write(i, data[i]);
}
void eeprom_read(char * data, int data_len)
{
  int i = 0;
  for (i = 0; i < data_len; i++)
    data[i] = EEPROM.read(i);
}

#define PUBLIC_ADDR -1
#define FORBIDDEN_ADDR 0

struct Data
{
  int addr;
  int power;
};

struct Data stored_data;

#define SEARCH_LEVEL_1  "Search 1"
#define SEARCH_LEVEL_2  "Search 2"
#define SEARCH_LEVEL_3  "Search 3"

// 0 - Idle
// 1 - Level 1  "Second"
// 2 - Level 2  "Minute"
// 3 - Level 3  "Hour"
char search_state = 0;
char search_delay = 0;  

void search_process(void)
{
  switch(search_state)
  {
    case 0: // Idle
      
    break;
    case 1: // Level 1
    delay(stored_data.addr%60 * 20);
    break;
    case 2: // Level 2
    delay((stored_data.addr/60)%60 * 20);
    break;
    case 3: // Level 3
    delay(stored_data.addr/60/60);
    break;
    default:
    search_state = 0;
    break;
  }
}


char _addr[10];
char _cmd[10];
char _key[10];
char _val[10];

// Command list
const char * power = "power";
// "addr:power=key,val"
const char * addr = "addr";
// "addr:power=key,val"
const char * search = "search";
// "addr:power=key,val"

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
  memcpy(_addr, point, sign-point);
  point = sign + 1;
  sign = strchr(point, '=');
  if (sign == 0)
    return;
  memcpy(_cmd, point, sign-point);
  point = sign + 1;
  sign = strchr(point, ',');
  if (sign == 0)
    return;
  memcpy(_key, point, sign-point);
  point = sign + 1;
  memcpy(_val, point, strlen(point));
}
int process(char * line)
{
  analyze(line);
  int tmp_addr = FORBIDDEN_ADDR;
  sscanf(_addr, "%i", &tmp_addr);
  if (memcmp(_cmd, power, strlen(power)) == 0)
  {
    if (_key[0] == '1') // set
    {
      int power = atoi(_val);
      if (power >= 0 && power <= 255)
        analogWrite(OUTPUT_PIN, power);
    }
    if (_key[0] == '2') // query
    {
    }
  }
  if (memcmp(_cmd, addr, strlen(addr)) == 0)
  {
    if (_key[0] == '0') // set
    {
      int addr = atoi(_val);
    }
    if (_key[0] == '2') // query
    {
    }
  }
  if (memcmp(_cmd, search, strlen(search)) == 0)
  {
    
  }
}

// global line
char gb_line[50];

void setup() {
  // put your setup code here, to run once:
  pinMode(OUTPUT_PIN, OUTPUT);
  Serial.begin(9600);
  // restore stored data
  eeprom_read((char *)&stored_data, sizeof(stored_data));
  analogWrite(OUTPUT_PIN, stored_data.power);
}

void loop() {
  memset(gb_line, 0, 50);
  // put your main code here, to run repeatedly:
  Serial.readBytesUntil('\r', gb_line, 49);
  process(gb_line);
  Serial.flush();
}
