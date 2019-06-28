#include <OneWire.h>

/*
    AUTHOR : Alan Métivier : https://github.com/LesQuater/Mini_Auto_Brew.git
*/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>

/* 8 Relay : 5v in command for a 220V AC 10A or 30 DC 10A

           2 Valves
           1 pomp
           1 electric hob
           1 electric-resistance heated
*/

//  .. 220V AC Hob
const int hob_PIN = 0;
int hob_Value = 0;

//  .. 220V AC Resistance (from electric kettle)
const int kettle_PIN = 0;
int kettle_Value = 0;

//  .. 12 V DC Pomp for hot water Max ~ 8L/Min
const int pump_PIN = 0;
int pump_Value = 0;

//  .. 12 V DC Solenoid valve ~ 1.3 l/min
const int valve1_PIN = 0;
int valve1_Value = 0;
const int valve2_PIN = 0;
int valve2_Value = 0;

// Temperature sensor : DS18B20
const byte T_PIN = 0;
float T_Value[3];
float T1_Value = 0;
float T2_Value = 0;
float T3_Value = 0;

enum DS18B20_RCODES {
  READ_OK,  // Lecture ok
  NO_SENSOR_FOUND,  // Pas de capteur
  INVALID_ADDRESS,  // Adresse reçue invalide
  INVALID_SENSOR  // Capteur invalide (pas un DS18B20)
};

OneWire ds(T_PIN);


// LCD 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

// 3 Button
const int X_PIN = A0;
int X_Value = 0;
const int Y_PIN = A1;
int Y_Value = 0;
const int enter_PIN = 2;
int enter_Value = 0;
int mapX = 0;
int mapY = 0;

//Buzzer?
const int buz_PIn = 0;
int buz_Value = 0;

// Boolean
boolean START;
boolean STAGE1;
boolean STAGE2;
boolean STAGE3;
boolean STAGE4;
boolean STAGE5;
boolean STAGE6;

boolean level0;
boolean level1;
boolean level2;
boolean level3;
boolean level4;
boolean level5;
boolean level6;

// Int
int nbLiter;

int startRunWater_Time;
int washRunWater_Time;
int addRunWater_Time;
int heatGrain_Time;
int heatWort_Time;
int hop1_Time;
int hop2_Time;


void setup() {
  
  // OUTPUT  
  pinMode(hob_PIN, OUTPUT);
  
  pinMode(kettle_PIN, OUTPUT);
  
  pinMode(pump_PIN, OUTPUT);
  
  pinMode(valve1_PIN, OUTPUT);
  pinMode(valve2_PIN, OUTPUT);
  
  pinMode(buz_PIn, OUTPUT);
  
  // INPUT  
  pinMode(X_PIN,INPUT);
  pinMode(Y_PIN,INPUT);
  pinMode(enter_PIN,INPUT);
  

  
  
  // Initialize
  digitalWrite(hob_PIN,LOW);
  digitalWrite(kettle_PIN,LOW);
  digitalWrite(pump_PIN,LOW);
  digitalWrite(valve1_PIN,LOW);
  digitalWrite(valve2_PIN,LOW);
  digitalWrite(buz_PIn,LOW);
  
  START = true;
  STAGE1 = false;
  STAGE2 = false;
  STAGE3 = false;
  STAGE4 = false;
  STAGE5 = false;
  STAGE6 = false;
  
  level0 = true;
  level1 = false;
  level2 = false;
  level3 = false;
  level4 = false;
  level5 = false;
  level6 = false;
  
  nbLiter = 0;
  heatGrain_Time = 0;
  
  
  //LCD Init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Mini_Auto_Brew");
  Serial.begin(9600);
  delay(1000);
  
}

void loop() {
  
  X_Value = analogRead(X_PIN);
  Serial.println(X_Value);
  Y_Value = digitalRead(Y_PIN);
  Serial.println(Y_Value);
  enter_Value = digitalRead(enter_PIN);
  Serial.println(enter_Value);
  mapX = map(X_Value, 0, 1023, -512, 512);
  mapY = map(Y_Value, 0, 1023, -512, 512);
  
  /*if (getTemperature(&T_Value[0], true) != READ_OK) {
    Serial.println(F("Erreur de lecture du capteur 1"));
    return;
  }
  if (getTemperature(&T_Value[1], false) != READ_OK) {
    Serial.println(F("Erreur de lecture du capteur 2"));
    return;
  }
  if (getTemperature(&T_Value[2], false) != READ_OK) {
    Serial.println(F("Erreur de lecture du capteur 3"));
    return;
  }*/
  
  T1_Value = T_Value[0];
  T2_Value = T_Value[1];
  T3_Value = T_Value[2];
  
  
  
  // Start menu : enter the recipe. (Button)
  if(START)
  {
    
    
    if(enter_Value !=1 && level0)
    {
      lcd.clear();
      lcd.setCursor(7,0);
      lcd.print("START");
      level1 = true;
      level0 = false;
    }
    // LEVEL 1 : How many liter in fine
    else if(level1)
    { 
      lcd.setCursor(0,1);
      lcd.print("In liter, Max:5L");
      lcd.setCursor(0,2);
      lcd.print("H-M liter in fine ?");
      lcd.setCursor(0,3);
      lcd.print(nbLiter);
      if(mapX > 1000 && nbLiter<5)
      {
        nbLiter ++;
      }
      else if(mapX < 10 && nbLiter>0)
      {
        nbLiter --;
      }
      else if(enter_Value !=1)
      {
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("START");
        level2 = true;
        level1 = false;
      }
      
    }
    // LEVEL 2 : How many time with the grain
    else if(level2)
    {
      lcd.setCursor(0,1);
      lcd.print("In minutes");
      lcd.setCursor(0,2);
      lcd.print("Time with the grain ?");
      lcd.setCursor(0,3);
      lcd.print(heatGrain_Time);
      if(mapX > 1000)
      {
        heatGrain_Time ++;
      }
      else if(mapX < 10)
      {
        heatGrain_Time --;
      }
      else if(enter_Value !=1)
      {
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("START");
        level3 = true;
        level2 = false;
      }
      
    }
    // LEVEL 3 : How many times does the water will boil ?
    else if(level3)
    {
      lcd.setCursor(0,1);
      lcd.print("In minutes");
      lcd.setCursor(0,2);
      lcd.print("Boiling time ?");
      lcd.setCursor(0,3);
      lcd.print(heatWort_Time);
      if(mapX > 600)
      {
        heatWort_Time ++;
      }
      else if(mapX < 400)
      {
        heatWort_Time --;
      }
      else if(enter_Value !=1)
      {
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("START");
        level4 = true;
        level3 = false;
      }
      
    }
    // LEVEL 4 : How long (after the boiling level starts) will the first hops be added
    else if(level4)
    {
      lcd.setCursor(0,1);
      lcd.print("Min from boil start");
      lcd.setCursor(0,2);
      lcd.print("First houblon ?");
      lcd.setCursor(0,3);
      lcd.print(hop1_Time);
      if(mapX > 600)
      {
        hop1_Time ++;
      }
      else if(mapX < 400)
      {
        hop1_Time --;
      }
      else if(enter_Value !=1)
      {
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("START");
        level5 = true;
        level4 = false;
      }
      
    }
    // LEVEL 4 : How long (after the boiling level starts) will the second hops be added
    else if(level5)
    {
      lcd.setCursor(0,1);
      lcd.print("Min from boil start");
      lcd.setCursor(0,2);
      lcd.print("Second houblon ?");
      lcd.setCursor(0,3);
      lcd.print(hop2_Time);
      if(mapX > 600)
      {
        hop2_Time ++;
      }
      else if(mapX < 400)
      {
        hop2_Time --;
      }
      else if(enter_Value !=1)
      {
        level6 = true;
        level5 = false;
      }
      
    }
    else if(level6)
    {
      lcd.setCursor(0,1);
      lcd.print("Valided ?");
      lcd.setCursor(0,2);
      lcd.print("Enter:Y UP:N");
      if(mapX > 600)
      {
        level0=true;
        level6=false;
      }
      else if(enter_Value !=1)
      {
        STAGE1 = true;
        START = false;
        level0=true;
        level6=false;
      }
      
    }
    // Else : ERROR 
    else
    {
    }
    
  }
  // Stage 1 : Heated water. (Kettle, T°1)
  else if (STAGE1)
  {
    lcd.clear();
    lcd.setCursor(7,0);
    lcd.print("STAGE1");
    if(T1_Value<50)
    {
      
    }
    
  }
  // Stage 2 : Run water. (Valve1)
  else if (STAGE2)
  {
    lcd.clear();
    lcd.setCursor(7,0);
    lcd.print("STAGE2");
  }
  // Stage 3 : Heated the grain. (Hob)
  else if (STAGE3)
  {
    lcd.clear();
    lcd.setCursor(7,0);
    lcd.print("STAGE3");
  }
  // Stage 4 : Extract the grain. (Motor?)
  else if (STAGE4)
  {
    lcd.clear();
    lcd.setCursor(7,0);
    lcd.print("STAGE4");
  }
  // Stage 5 : Boil the wort. (Whirlpool with the pump, Hob, T°2)
  else if (STAGE5)
  {
    lcd.clear();
    lcd.setCursor(7,0);
    lcd.print("STAGE5");
  }
  // Stage 6 : Run the wort for cooling. (Valve2)
  else if (STAGE6)
  {
    lcd.clear();
    lcd.setCursor(7,0);
    lcd.print("STAGE6");
  }
  // ERROR
  else
  {
    lcd.clear();
    lcd.setCursor(7,0);
    lcd.print("Error stage");
  }

}

// From https://www.carnetdumaker.net/articles/mesurer-une-temperature-avec-un-capteur-1-wire-ds18b20-et-une-carte-arduino-genuino/
byte getTemperature(float *temperature, byte reset_search) {
  byte data[9], addr[8];
  if (reset_search) {
    ds.reset_search();
  }
  if (!ds.search(addr)) {
    return NO_SENSOR_FOUND;
  }
  if (OneWire::crc8(addr, 7) != addr[7]) {
    return INVALID_ADDRESS;
  }
  if (addr[0] != 0x28) {
    return INVALID_SENSOR;
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  delay(800);
  ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  for (byte i = 0; i < 9; i++) {
    data[i] = ds.read();
  }
  *temperature = (int16_t) ((data[1] << 8) | data[0]) * 0.0625; 
  return READ_OK;
}
