/*
    AUTHOR : Alan Métivier : https://github.com/LesQuater/Mini_Auto_Brew.git
*/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>

/* 8 Relay : 5v in command for a 220V AC 10A or 30 DC 10A

           1 Valves
           1 pomp
           2 electric-resistance heated
*/


  /*            
   *    ---------
   *   _|_      |
   *            |
   * |\Malt/|   |
   * | \  / |   |      Cuve 1 : Sac malt
   * |  \/  |   |
   * |______|   |
   *  | E  |    |      E : Electrovanne
   * _|____|_   |
   * |      |   |
   * |  T   |-P--     Cuve 2 : - T : capteur de T°
   * |S    S|                  - S : 2 thermo Resistance
   * |______|                  - P : Pompe
   * 
   * PHASE 1 : <--------------------------------------
   *  R : ON                                         |
   *  Pompe : OFF                                    |
   *  Vanne : ON                                     |
   *  (L'eau est dans la cuve 2 et chauffe à 50°)    |
   *                                                 |
   *  ||Si|| T° = 50° ____                           |
   *                      |                          |
   *                      v                          | x 4 = 40 min
   * PHASE 2 :                                       |
   *  Pompe : ON                                     |
   *  Vanne : OFF                                    |
   *  R : OFF                                        |
   *  (L'eau est dans la cuve 1 et noye le malt)     |
   *                                                 |
   *  ||Si|| Temps = 10 min et boucle =/= 4 ---------|
   *  ||Sinon||  ____
   *                 |
   *                 v                 
   * PHASE 3 : <--------------------------------------
   *  R : ON                                         |
   *  Pompe : OFF                                    |
   *  Vanne : ON                                     |
   *  (L'eau est dans la cuve 2 et chauffe à 60°)    |
   *                                                 |
   *  ||Si|| T° = 60° ____                           |
   *                      |                          |
   *                      v                          | x 2 = 20 min
   * PHASE 4 :                                       |
   *  Pompe : ON                                     |
   *  Vanne : OFF                                    |
   *  R : OFF                                        |
   *  (L'eau est dans la cuve 1 et noye le malt)     |
   *                                                 |
   *  ||Si|| Temps = 10 min et boucle =/= 2 ---------|
   *  ||Sinon||  ____
   *                 |
   *                 v
   * PHASE 5 :
   *  R : ON                                         
   *  Pompe : OFF                                    
   *  Vanne : ON                                     
   *  (L'eau est dans la cuve 2 et chauffe à 70°)    
   *                                                 
   *  ||Si|| T° = 70° ____                           
   *                      |                          
   *                      v                          
   * PHASE 6 :                                       
   *  Pompe : ON                                     
   *  Vanne : OFF                                    
   *  R : OFF                                        
   *  (L'eau est dans la cuve 1 et noye le malt)     
   *                                                 
   *  ||Si|| Temps = 10 min ____                           
   *                            |                          
   *                            v    
   *    
   *  PHASE 7 :
   *   Vanne : ON
   *   R : ON
   *   Pompe : OFF
   *   
   *   ||Si|| Temps = 50 min ____
   *                             |
   *                             v
   *  PHASE 8 :
   *   Bip : ON
   *   delay(1000)
   *   Bip : OFF
   *    
   *   ||Si|| Temps = 10 min ___
   *                             |
   *                             v
   *  PHASE 9 : 
   *  Bip : ON
   *  
   *  FIN
   * 
  */

//  .. 2x 220V AC Resistance
const int heat1_PIN = 2;
int heat1_Value = 0;
const int heat2_PIN = 0;
int heat2_Value = 0;

//  .. 12 V DC Pomp for hot water Max ~ 8L/Min
const int pump_PIN = 7;
int pump_Value = 0;

//  .. 2x 12 V DC Solenoid valve ~ 1.3 l/min
const int valve1_PIN = 5;
int valve1_Value = 0;

//Buzzer
const int buz_PIn = 0;
int buz_Value = 0;

// 2x Temperature sensor : DS18B20
const byte T_PIN = 10;
float T_Value[2];
float T1_Value = 0;
float T2_Value = 0;

enum DS18B20_RCODES {
  READ_OK,  // Lecture ok
  NO_SENSOR_FOUND,  // Pas de capteur
  INVALID_ADDRESS,  // Adresse reçue invalide
  INVALID_SENSOR  // Capteur invalide (pas un DS18B20)
};

OneWire ds(T_PIN);


// LCD 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

int PHASE;

// Int
int nbLiter;

int PHASE2_Time;
int PHASE4_Time;
int PHASE6_Time;
int PHASE7_Time;
int PHASE8_Time;

int boucle1;
int boucle2;


void setup() {
  
  // OUTPUT  
  pinMode(heat1_PIN, OUTPUT);
  //pinMode(heat2_PIN, OUTPUT);
  
  pinMode(pump_PIN, OUTPUT);
  
  pinMode(valve1_PIN, OUTPUT);
    
  //pinMode(buz_PIn, OUTPUT);
  
  // Initialize
  digitalWrite(heat1_PIN,LOW);
  //digitalWrite(heat2_PIN,LOW);
  
  digitalWrite(pump_PIN,LOW);
  
  digitalWrite(valve1_PIN,LOW);
  
 // digitalWrite(buz_PIn,LOW);

  boucle1 = 0;
  boucle2 = 0;

  PHASE = 0;
  PHASE2_Time = 1000;
  PHASE4_Time = 1000;
  PHASE6_Time = 1000;
  PHASE7_Time = 1000;
  PHASE8_Time = 1000;
  
  //LCD Init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Mini_Auto_Brew");
  Serial.begin(9600);
  delay(2000);
}

void loop() {
  
  if (getTemperature(&T_Value[0], true) != READ_OK) {
    Serial.println(F("Erreur de lecture du capteur 1"));
    return;
  }
  
  T1_Value = T_Value[0];
  Serial.println(T1_Value);
  
  switch(PHASE){
    case 0:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("START");
        delay(10000);
        PHASE = 1;
      break;
    case 1:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE1");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,HIGH);
        //digitalWrite(heat2_PIN,HIGH);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,HIGH);
        if(T1_Value>50 && getTemperature(&T_Value[0], true) == READ_OK)
        {
          PHASE = 2;
        }
      break;
    case 2:
        boucle1 ++;
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE2");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,LOW);
        //digitalWrite(heat2_PIN,LOW);
        digitalWrite(pump_PIN,HIGH);
        digitalWrite(valve1_PIN,LOW);
        delay(PHASE2_Time);
        if(boucle1<4)
        {
          PHASE = 1;
        }
        else{
          PHASE = 3;
        }
      break;
    case 3:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE3");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,HIGH);
        //digitalWrite(heat2_PIN,HIGH);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,HIGH);
        if(T1_Value>60 && getTemperature(&T_Value[0], true) == READ_OK)
        {
          PHASE = 4;
        }
      break;
    case 4:
        boucle2 ++;
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE4");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,LOW);
        //digitalWrite(heat2_PIN,LOW);
        digitalWrite(pump_PIN,HIGH);
        digitalWrite(valve1_PIN,LOW);
        delay(PHASE4_Time);
        if(boucle1<2)
        {
          PHASE = 2;
        }
        else{
          PHASE = 5;
        }
      break;
    case 5:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE5");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,HIGH);
        //digitalWrite(heat2_PIN,HIGH);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,HIGH);
        if(T1_Value>70 && getTemperature(&T_Value[0], true) == READ_OK)
        {
          PHASE = 6;
        }
      break;
    case 6:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE6");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,LOW);
        //digitalWrite(heat2_PIN,LOW);
        digitalWrite(pump_PIN,HIGH);
        digitalWrite(valve1_PIN,LOW);
        delay(PHASE6_Time);
          PHASE = 7;
      break;
    case 7:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE7");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,HIGH);
        //digitalWrite(heat2_PIN,HIGH);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,HIGH);
        delay(PHASE7_Time);
          PHASE = 8;
      break;
    case 8:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE8");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,HIGH);
        //digitalWrite(heat2_PIN,HIGH);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,HIGH);
        //digitalWrite(buz_PIn,HIGH);
        //delay(5000);
        //digitalWrite(buz_PIn,LOW);
        delay(PHASE8_Time);
          PHASE = 9;
      break;
    case 9:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE9");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,LOW);
        //digitalWrite(heat2_PIN,LOW);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,LOW);
        //digitalWrite(buz_PIn,HIGH);
      break;
     default :
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("ERROR");
        digitalWrite(heat1_PIN,LOW);
        //digitalWrite(heat2_PIN,LOW);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,LOW);
        //digitalWrite(buz_PIn,HIGH);
      break;
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
