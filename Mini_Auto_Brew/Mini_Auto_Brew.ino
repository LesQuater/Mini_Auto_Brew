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
   * |     S|                  - S : thermo Resistance
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
const int pump_PIN = 5;
int pump_Value = 0;

//  .. 2x 12 V DC Solenoid valve ~ 1.3 l/min
const int valve1_PIN = 7;
int valve1_Value = 0;

//Buzzer
const int buz_PIn = 8;
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
int PHASE9_Time;

int PHASE10_Time;


int boucle1;
int boucle2;
int boucle3;
int boucle4;
int nbBoucle1;
int nbBoucle2;
int nbBoucle3;
int nbBoucle4;

int palier1, palier2, palier3, palier4;


void setup() {

  // OUTPUT
  pinMode(heat1_PIN, OUTPUT);
  //pinMode(heat2_PIN, OUTPUT);

  pinMode(pump_PIN, OUTPUT);

  pinMode(valve1_PIN, OUTPUT);

  pinMode(buz_PIn, OUTPUT);

  // Initialize
  digitalWrite(heat1_PIN,LOW);
  //digitalWrite(heat2_PIN,LOW);

  digitalWrite(pump_PIN,LOW);

  digitalWrite(valve1_PIN,LOW);

  digitalWrite(buz_PIn,HIGH);
  delay(10000UL);
  digitalWrite(buz_PIn,LOW);

  boucle1 = 0;
  boucle2 = 0;
  boucle3 = 0;
  boucle4 = 0;
  nbBoucle1 = 1;
  nbBoucle2 = 8;
  nbBoucle3 = 4;
  nbBoucle4 = 1;

  PHASE = 0;
  PHASE2_Time = 10; //minutes : 10min*2boucle = 20min
  PHASE4_Time = 5; // 5*4 = 20min;
  PHASE6_Time = 5; // 5*4 = 20min

  PHASE8_Time = 5; // 2*5 = 10min
  PHASE9_Time = 30; // 30min ebullition
  PHASE10_Time = 5; // + 5 amertume

  palier1 = 56;
  palier2 = 66;
  palier3 = 68;
  palier4 = 76;


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
        if(T1_Value>palier1 && getTemperature(&T_Value[0], true) == READ_OK)
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
        delay(60000UL);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,LOW);

        delay(600000UL);

        if(boucle1<=nbBoucle1)
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
        if(T1_Value>palier2 && getTemperature(&T_Value[0], true) == READ_OK)
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
        delay(60000UL);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,LOW);
        //delay(1000*60*PHASE4_Time);
        delay(300000UL);
        if(boucle2<=nbBoucle2)
        {
          PHASE = 3;
        }
        else{
          PHASE = 7;
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
        if(T1_Value>palier3 && getTemperature(&T_Value[0], true) == READ_OK)
        {
          PHASE = 6;
        }
      break;
    case 6:
        boucle3 ++;
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
        delay(30000UL);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,LOW);
        //delay(1000*60*PHASE6_Time);
        delay(300000UL);
        if(boucle3<=nbBoucle3)
        {
          PHASE = 5;
        }
        else{
          PHASE = 7;
        }
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
        if(T1_Value>palier4 && getTemperature(&T_Value[0], true) == READ_OK)
        {
          PHASE = 8;
        }
      break;
    case 8:
        boucle4 ++;
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE8");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,LOW);
        //digitalWrite(heat2_PIN,LOW);
        digitalWrite(pump_PIN,HIGH);
        delay(30000UL);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,LOW);
        //delay(1000*60*PHASE8_Time);
        delay(300000UL);
        if(boucle4<=nbBoucle4)
        {
          PHASE = 7;
        }
        else{
          PHASE = 9;
        }
      break;
    case 9:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE9");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,HIGH);
        //digitalWrite(heat2_PIN,HIGH);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,HIGH);
        digitalWrite(buz_PIn,HIGH);
        delay(5000);
        digitalWrite(buz_PIn,LOW);
        //delay(1000*60*PHASE9_Time);
        delay(1800000UL);
          PHASE = 10;
      break;
    case 10:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE10");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,8);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,HIGH);
        //digitalWrite(heat2_PIN,HIGH);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,HIGH);
        digitalWrite(buz_PIn,HIGH);
        delay(5000);
        digitalWrite(buz_PIn,LOW);
        //delay(1000*60*PHASE10_Time);
        delay(300000UL);
          PHASE = 11;
      break;
    case 11:
        lcd.clear();
        lcd.setCursor(7,0);
        lcd.print("STAGE11");
        lcd.setCursor(7,1);
        lcd.print("FIN");
        lcd.setCursor(1,2);
        lcd.print("T :");
        lcd.setCursor(1,3);
        lcd.print(T1_Value);
        digitalWrite(heat1_PIN,LOW);
        //digitalWrite(heat2_PIN,LOW);
        digitalWrite(pump_PIN,LOW);
        digitalWrite(valve1_PIN,LOW);
        digitalWrite(buz_PIn,HIGH);
        delay(5000);
        digitalWrite(buz_PIn,LOW);
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
