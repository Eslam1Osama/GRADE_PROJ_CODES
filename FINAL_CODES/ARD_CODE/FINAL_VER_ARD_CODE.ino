/*=========================================================================*/
                          /**********   LIBRARYS   **********/   
/*for DHT22 sensor*/
#include <DHT.h>
//======================================
/*for DS18B20 sensor*/
#include <OneWire.h>
#include <DallasTemperature.h>
//======================================
/*for TDS sensor*/
#include "GravityTDS.h"
//======================================
/*for LCD*/
#include <LiquidCrystal.h>
LiquidCrystal lcd(26, 27, 25, 24, 23, 22);//(rs, en, d4, d5, d6, d7)

/*=========================================================================*/
/*for DHT22 sensor*/

//Constants
#define DHTPIN 29     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

//Variables
float h = 0;  //Stores humidity value
float t = 0; //Stores temperature value
/*====================================================================================*/
/*for ultrasonic sensor*/

// defines pins numbers
const int trigPin = 1;
const int echoPin = 2;

// defines variables
long duration = 0.0;
float distance = 0.0;
float actual_water_level = 0.0;
/*====================================================================================*/
/*for DS18B20 sensor*/

#define ONE_WIRE_BUS 6                 // Data wire is plugged into digital pin 6 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);         // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire);   // Pass oneWire reference to DallasTemperature library
float Celsius = 0.0;
/*====================================================================================*/
/*for TDS sensor*/

#define TdsSensorPin A1
GravityTDS gravityTds;
float tdsValue = 0.0;
/*====================================================================================*/
/*for PH sensor*/

float calibration_value = 21.34 - 0.14;   //the calibration value is defined, which can be modified as required to get an accurate pH value of solutions.
unsigned long int avgval;                 //Store the average value of the sensor feedback
int buffer_arr[10],temp = 0;
float ph_act = 0.0;
/*====================================================================================*/
// EC isolator

#define EC_Isolator A3  // 3906 PNP TYPE TRANSISTOR THIS is used to connect and disconnect the 3.3V wire 
#define EC_GND_Wire A2  // 2N2222 NPN TRANSISTOR THIS IS USED TO CONNECT AND DISCONNECT THE GND WIRE
/*====================================================================================*/
/*flags*/

int flag_error = 0;
float ph_acid_values[2] = {0 , 0};
float ph_base_values[2] = {0 , 0};
float ph_normal_values[2] = {0 , 0};
int i_normal = 0;
int i_acid = 0;
int i_base = 0;
int PH_normal = 0;
int PH_high = 0;
int PH_low = 0;
/*====================================================================================*/
/*for push button and count down timer*/

#define buttonpin 13 // chance to change it's pin from 32
int button_state = 0;
int num_sec = 12;
/*====================================================================================*/
//PH sensor

String PH_UP_PUMP =     "No current decision";
String PH_DOWN_PUMP =   "No current decision";
String PH_CONDITION =   "No current decision";

//PPM

String PPM_CONDITION =          "No current decision";
String PUMPS_A_B =              "No current decision";
String WATER_PUMP_UP_FOR_PPM =  "No current decision";

//AIR_TEMP

String AIR_TEMP_CONDITION =   "No current decision";
String FAN_FOR_AIR_TEMP =     "No current decision";

//WATER_LEVEL

String VALVE_CONDITION =                "No current decision";
String WATER_PUMP_UP_FOR_WATER_LEVEL =  "No current decision";
String WATER_LEVEL_CONDITION =          "No current decision";

//water_temp

String WATER_TEMP_CONDITION =   "No current decision";
String COOLER_CONDITION =       "No current decision";

//HUMDITIY

String HUMD_CONDITION =     "No current decision";
String FAN_FOR_HUMDITIY =   "No current decision";

/*====================================================================================*/
/*for actuators*/
// pins of the following is going to change
 
int nutrientA = 8;//IN1
int nutrientB = 9;//IN2
int acid = 10;//IN3
int base = 11;//IN4

int level_up_pump = 12;

/* for LABEL getting from random forest on resparry pi */
char Label ;

/*====================================================================================*/

void setup() 
{
  //LCD setup
  lcd.begin(16,2);

  //DHT22 sensor setup
  dht.begin();

  //ULTRASONIC sensor setup
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  //TDS sensor setup
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization 

  //DS18B20 sensor setup
  sensors.begin();

  //for Isolation between TDS sensor and PH sensor (NOTE that none of them work at the same time)
  pinMode(EC_Isolator, OUTPUT);
  pinMode(EC_GND_Wire, OUTPUT);
  digitalWrite(EC_Isolator, HIGH);
  digitalWrite(EC_GND_Wire, LOW); 

  //for push button
  pinMode(buttonpin , INPUT);

  //for dosing pumps "all of them are active low"
  pinMode(nutrientA , OUTPUT);  
  pinMode(nutrientB , OUTPUT);
  pinMode(acid , OUTPUT);
  pinMode(base , OUTPUT);
  digitalWrite(nutrientA , HIGH);
  digitalWrite(nutrientB , HIGH);
  digitalWrite(acid , HIGH);
  digitalWrite(base , HIGH);

  //for water level up pump
  pinMode(level_up_pump , OUTPUT);  

}

/*====================================================================================*/

void loop() 
{
  /* meaning from this check because if there is an error from 1 of sensors 
     stop getting readings from sensors until fixing this error */
  if(flag_error == 0)
  {
    // isolate TDS sensor to get readings of PH sensor
    digitalWrite(EC_Isolator,HIGH); 
    digitalWrite(EC_GND_Wire, LOW);
    
    PH_sensor(); // first we get readings of PH sensor

    // isolate PH sensor to get readings of TDS sensor
    digitalWrite(EC_Isolator,LOW); 
    digitalWrite(EC_GND_Wire, HIGH);
    
    delay(1000); // wait for 1 sec. to be able to get TDS sensor readings

   
    waterproof_sensor(); // getting readings of DS18B20 sensor
    TDS_sensor(); // getting readings of TDS sensor
    temperature_and_humidity_sensor(); // getting readings of DHT22 sensor
    ultrasonic_sensor(); // getting readings of ULTRASONIC sensor
    delay(100);// Wait for 100 msec
  }
  
//===================================================================================================
  
  /*checking if there is a problem with any of sensors*/
   if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
   {
     ph_act = -1;
     PH_CONDITION = "Check PH sensor";
  
   }
   if((tdsValue >= 1000) || (tdsValue <= 10))// for TDS sensor
   {
     tdsValue = -2;
     PPM_CONDITION = "Check TDS sensor";
   }
   if ((isnan(h)) || (t == 0))// for DHT22 sensor
   {
     t = -3;
     AIR_TEMP_CONDITION = "Check DHT22_T";

     h = -5;
     HUMD_CONDITION = "Check DHT22_H";
   }
   if((distance == 0) || (distance >= 26))// for ultrasonic sensor
   {
     actual_water_level  = -4;
     WATER_LEVEL_CONDITION = "Check Ultrasonic";
   }
   if((Celsius <= -100) || (Celsius >= 50))// for Water_temp sensor
   {
     Celsius = -6;
     WATER_TEMP_CONDITION = "Check DS18B20";
   }
   
//===================================================================================================
   
   /*for PH normal which means not adding acid or base*/
   if((PH_normal == 1) && (ph_act != -1))
   {
    ph_normal_values[1] = ph_act;
    while(i_normal == 0)
    {
      if(((ph_normal_values[0] - ph_normal_values[1] <= 0.3) && (ph_normal_values[0] - ph_normal_values[1] >= 0)) || ((ph_normal_values[1] - ph_normal_values[0] <= 0.3) && (ph_normal_values[1] - ph_normal_values[0] >= 0)))
      {
        ph_normal_values[0] = ph_act;
        i_normal = 1;
        PH_normal = 0;
      }
      else
      {
        digitalWrite(EC_Isolator,HIGH); 
        digitalWrite(EC_GND_Wire, LOW);
        PH_sensor(); // first we get readings of PH sensor
        digitalWrite(EC_Isolator,LOW); 
        digitalWrite(EC_GND_Wire, HIGH);
        delay(1000);
      
        if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
        {
          ph_act = -1;
          PH_CONDITION = "Check PH sensor";   
        }
        if(ph_act != -1)
        {
          ph_normal_values[1] = ph_act;
          i_normal = 0;
        }
        else
        {
          i_normal = 1;
        }
      }
      
    }
   }
   i_normal = 0;
   
//===================================================================================================
   
   /*for PH low which means adding base*/
   if((PH_low == 1) && (ph_act != -1))
   {
    ph_base_values[1] = ph_act;
    while(i_base == 0)
    {
      if((ph_base_values[1] - ph_base_values[0] <= 0.5) && (ph_base_values[1] - ph_base_values[0] >= 0))
      {
        ph_base_values[0] = ph_act;
        i_base = 1;
        PH_low = 0 ;
      }
      else
      {
        digitalWrite(EC_Isolator,HIGH); 
        digitalWrite(EC_GND_Wire, LOW);
        PH_sensor(); // first we get readings of PH sensor
        digitalWrite(EC_Isolator,LOW); 
        digitalWrite(EC_GND_Wire, HIGH);
        delay(1000);
      
        if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
        {
          ph_act = -1;
          PH_CONDITION = "Check PH sensor";   
        }
        if(ph_act != -1)
        {
          ph_base_values[1] = ph_act;
          i_base = 0;
        }
        else
        {
          i_base = 1;
        }
      }
      
    }
   }
   i_base = 0;
   
//===================================================================================================
   
   /*for PH high which means adding acid*/
   if((PH_high == 1) && (ph_act != -1))
   {
    ph_acid_values[1] = ph_act;
    while(i_acid == 0)
    {
      if((ph_acid_values[0] - ph_acid_values[1] <= 0.6) && (ph_acid_values[0] - ph_acid_values[1] >= 0))
      {
        ph_acid_values[0] = ph_act;
        i_acid = 1;
        PH_high = 0;
      }
      else
      {
        digitalWrite(EC_Isolator,HIGH); 
        digitalWrite(EC_GND_Wire, LOW);
        PH_sensor(); // first we get readings of PH sensor
        digitalWrite(EC_Isolator,LOW); 
        digitalWrite(EC_GND_Wire, HIGH);
        delay(1000);
      
        if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
        {
          ph_act = -1;
          PH_CONDITION = "Check PH sensor";   
        }
        if(ph_act != -1)
        {
          ph_acid_values[1] = ph_act;
          i_acid = 0;
        }
        else
        {
          i_acid = 1;
        }
      }
      
    }
   }
   i_acid = 0;

   if((ph_act == -1) || (tdsValue == -2) || (t == -3) || (actual_water_level == -4) || (h == -5) || (Celsius == -6))
    {
      flag_error = 1; 
    }
    
//===================================================================================================
    
    if(flag_error == 0)// which means nothing wrong with any of sensors
   {
     /*for PH value*/
     if ((ph_act >= 5.5) && (ph_act <= 7.0))// normal 
     {
      PH_CONDITION = "PH is normal";
      PH_DOWN_PUMP = "PH down pump off";
      PH_UP_PUMP = "PH up pump off";
      ph_normal_values[0] = ph_act;
      PH_normal = 1;
     }
     else if ((ph_act < 5.5) && (ph_act > 0))// low
     {
       PH_CONDITION = "PH is low";
       PH_DOWN_PUMP = "PH down pump off";
       PH_UP_PUMP = "PH up pump on";
       ph_base_values[0] = ph_act;
       PH_low = 1;
     }
     else if ((ph_act > 7.0) && (ph_act < 10))// high
     {
       PH_CONDITION = "PH is high";
       PH_DOWN_PUMP = "PH down pump on";
       PH_UP_PUMP = "PH up pump off";
       ph_acid_values[0] = ph_act;
       PH_high = 1;
     }

     /*for TDS value*/
     if ((tdsValue >= 500) && (tdsValue <= 600))// normal 
     {
      PPM_CONDITION = "PPM is normal";
      PUMPS_A_B = "A&B pumps off";
      WATER_PUMP_UP_FOR_PPM = "Water UpPump off";
     }
     else if ((tdsValue < 500)&& (tdsValue > 0))// low
     {
       PPM_CONDITION = "PPM is low";
       PUMPS_A_B = "A&B pumps on";
       WATER_PUMP_UP_FOR_PPM = "Water UpPump off";
     }
     else if ((tdsValue > 600) && (tdsValue < 1000))// high
     {
       PPM_CONDITION = "PPM is high";
       PUMPS_A_B = "A&B pumps off";
       WATER_PUMP_UP_FOR_PPM = "Water UpPump on";
     }

     /*for Air temperature value*/
     if ((t >= 20) && (t <= 25))// normal 
     {
      AIR_TEMP_CONDITION = "Air_T is normal";
      FAN_FOR_HUMDITIY = "Fan is off";
     }
     else if ((t < 20)&& (t > 0))// low
     {
       AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
     }
     else if ((t > 25) && (t < 40))// high
     {
       AIR_TEMP_CONDITION = "Air_T is high";
       FAN_FOR_AIR_TEMP = "Fan is on";
     }
     /*for water level*/
     if ((actual_water_level >= 10) && (actual_water_level <= 18.1))// normal 
     {
       VALVE_CONDITION = "Valve off";
       WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump off";
       WATER_LEVEL_CONDITION = "Water Lev normal";
     }
     else if (actual_water_level < 10)// low
     {
       VALVE_CONDITION = "Valve off";
       WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump on";
       WATER_LEVEL_CONDITION = "Water Lev low";
     }
     else if ((actual_water_level > 18.1) && (actual_water_level < 26))// high
     {
       VALVE_CONDITION = "Valve on";
       WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump off";
       WATER_LEVEL_CONDITION = "Water Lev high";
     }
  
     /*DHT22 (HUMIDITY) sensor*/
     if ((h <= 70) && (h > 0))// normal
     {
       HUMD_CONDITION = "humidity normal";
       FAN_FOR_HUMDITIY = "Fan is off"; 
     }
     else if (h > 70)// high
     {
       HUMD_CONDITION = "humidity high";
       FAN_FOR_HUMDITIY = "Fan is on"; 
     }
  
     /*water temeprature sensor*/
     if ((Celsius <= 26) && (Celsius > 0))// normal
     {
       WATER_TEMP_CONDITION = "wat_temp normal";
       COOLER_CONDITION = "Cooler off";
     }
     else if (Celsius > 26)// high
     {
       WATER_TEMP_CONDITION = "wat_temp high";
       COOLER_CONDITION = "Cooler on";
     }
//===================================================================================================
     //for PH sensor
     lcd.setCursor(0,0); 
     lcd.print("PH = ");          
     lcd.print(ph_act , 2); 
     lcd.setCursor(0,1);           
     lcd.print(PH_UP_PUMP); // PH_CONDITION = "PH - pump on" or "PH + pump ↓"
     delay(3000);
     lcd.clear();
    
     lcd.setCursor(0,0); 
     lcd.print("PH = ");          
     lcd.print(ph_act , 2); 
     lcd.setCursor(0,1);           
     lcd.print(PH_DOWN_PUMP); // PH_CONDITION = "PH + pump ↑" or "PH + pump ↓"
     delay(3000);
     lcd.clear();
    
     //for TDS sensor
     lcd.setCursor(0,0); 
     lcd.print("TDS = ");          
     lcd.print(tdsValue , 2); 
     lcd.print(" PPM"); 
     lcd.setCursor(0,1);           
     lcd.print(PUMPS_A_B);//PUMPS_A_B = "PPM A&B PUMPS ↑" or "PPM A&B PUMPS ↓"
     delay(3000);
     lcd.clear();
    
     lcd.setCursor(0,0);
     lcd.print("TDS = ");           
     lcd.print(tdsValue , 2);
     lcd.print(" PPM");  
     lcd.setCursor(0,1);           
     lcd.print(WATER_PUMP_UP_FOR_PPM);// WATER_PUMP_UP = "water PUMP ↑" or "water PUMP ↓"
     delay(3000);
     lcd.clear();
  
     //for air temperature sensor
     lcd.setCursor(0,0); 
     lcd.print("Air_temp= ");          
     lcd.print(t , 2);
     lcd.print("C"); 
     lcd.setCursor(0,1);           
     lcd.print(FAN_FOR_AIR_TEMP);// HUMD_CONDITION = "fan ↓" or "fan ↑"
     delay(3000);
     lcd.clear();
  
     //for water level sensor
     lcd.setCursor(0,0);  
     lcd.print("Dist = ");        
     lcd.print(distance , 2);
     lcd.print(" cm"); 
     lcd.setCursor(0,1);           
     lcd.print(WATER_PUMP_UP_FOR_WATER_LEVEL);// WATER_PUMP_UP = "water PUMP ↓" or "water PUMP ↑"
     delay(3000);
     lcd.clear();
    
     lcd.setCursor(0,0);          
     lcd.print("Dist = ");        
     lcd.print(distance , 2);
     lcd.print(" cm"); 
     lcd.setCursor(0,1);           
     lcd.print(VALVE_CONDITION);//VALVE_CONDITION = "valve ↓" or "valve ↑"
     delay(3000);
     lcd.clear();
  
     //for humdidty sensor
     lcd.setCursor(0,0); 
     lcd.print("hum = ");          
     lcd.print(h , 2); 
     lcd.print(" %"); 
     lcd.setCursor(0,1);           
     lcd.print(FAN_FOR_HUMDITIY);// AIR_TEMP_CONDITION = "fan ↓" or "fan ↑" 
     delay(3000);
     lcd.clear();
    
     //for water temperature sensor
     lcd.setCursor(0,0);
     lcd.print("wat_temp= ");            
     lcd.print(Celsius , 2); 
     lcd.print("C");  
     lcd.setCursor(0,1);           
     lcd.print(COOLER_CONDITION);// WATER_TEMP_CONDITION = "Chiller ↓" or "Chiller ↑"
     delay(3000);
     lcd.clear();
     
//===================================================================================================
    
     /*for real action*/
     if(PH_UP_PUMP == "PH up pump on")
     {
      digitalWrite(base , LOW);
      delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
      digitalWrite(base , HIGH);
     }
     if(PH_DOWN_PUMP == "PH down pump on")
     {
      digitalWrite(acid , LOW);
      delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
      digitalWrite(acid , HIGH);
     }
     if(PUMPS_A_B == "A&B pumps on")
     {
      for(int i = 0 ; i < 3 ; i++)
       {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
         digitalWrite(nutrientA , LOW);
         delay(2000);
         digitalWrite(nutrientA , HIGH);
         delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

         digitalWrite(nutrientB , LOW);
         delay(2000);
         digitalWrite(nutrientB , HIGH);
         delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
       }
     }
     if(WATER_PUMP_UP_FOR_WATER_LEVEL == "Water UpPump on")
     {
      digitalWrite(level_up_pump , HIGH);
      delay(3000);// open it for increasing water level by adding 130.5 mL
      digitalWrite(level_up_pump , LOW);
     }
   }
//===================================================================================================
   if(flag_error == 1)
   {
      //PH sensor
      PH_DOWN_PUMP = "PH down pump off";
      PH_UP_PUMP = "PH up pump off";
      digitalWrite(acid , HIGH);
      digitalWrite(base , HIGH);

      //PPM
      PUMPS_A_B = "A&B pumps off";
      WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
      digitalWrite(nutrientA , HIGH);
      digitalWrite(nutrientB , HIGH);
      
      //AIR_TEMP
      FAN_FOR_AIR_TEMP = "Fan is off";
      
      //WATER_LEVEL
      VALVE_CONDITION = "Valve off";
      WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump off";
      WATER_LEVEL_CONDITION = "Check Ultrsonic";
      digitalWrite(level_up_pump , LOW);// relay of water level pump is active high
      //water_temp
      COOLER_CONDITION = "Cooler off";
      
      //HUMDITIY
      FAN_FOR_HUMDITIY = "Fan is off";
   }

   if(ph_act == -1)
   {
    lcd.setCursor(0,0);          
    lcd.print(PH_CONDITION);  
    delay(3000);
    lcd.clear();
   }
   if(tdsValue == -2)
   {
    lcd.setCursor(0,0);          
    lcd.print(PPM_CONDITION); 
    delay(3000);
    lcd.clear();
   }
   if(t == -3)
   {
    lcd.setCursor(0,0); 
    lcd.print(AIR_TEMP_CONDITION);           
    delay(3000);
    lcd.clear();
   }
   if(distance == -4)
   {
    lcd.setCursor(0,0); 
    lcd.print(WATER_LEVEL_CONDITION);           
    delay(3000);
    lcd.clear();
   }
   if(h == -5)
   {
    lcd.setCursor(0,0); 
    lcd.print(HUMD_CONDITION);          
    delay(3000);
    lcd.clear();
   }
   if(Celsius == -6)
   {
    lcd.setCursor(0,0); 
    lcd.print(WATER_TEMP_CONDITION);           
    delay(3000);
    lcd.clear();
   }
//===================================================================================================
   while(num_sec > 0)
    {
     if(countDigits(num_sec) == 2)
     {
      lcd.setCursor(0,0);
      lcd.print("Countdown:");
      lcd.setCursor(0,1);
      lcd.print("00");
      lcd.print(":");
      lcd.print(num_sec);
      delay(1000);
      lcd.clear();
     }
     else if((countDigits(num_sec) == 1) || (countDigits(num_sec) == 0))
     {
      lcd.setCursor(0,0);
      lcd.print("Countdown:");
      lcd.setCursor(0,1);
      lcd.print("00");
      lcd.print(":");
      lcd.print("0");
      lcd.print(num_sec);
      delay(1000);
      lcd.clear();
     }
    button_state = digitalRead(buttonpin);
    if(button_state == 1)
    {
      flag_error = 0;
      delay(100);
      lcd.setCursor(0,0);
      lcd.print("button pressed");
      delay(100);
      lcd.clear();
    }
    num_sec--;
    }
   num_sec = 12;
}

uint8_t countDigits(int num)
{
  uint8_t count = 0;
  while(num)
  {
    num = num / 10;
    count++;
  }
  return count;
}

void temperature_and_humidity_sensor()
{
//Read data and store it to variables h (humidity) and t (temperature)
    // Reading temperature or humidity takes about 250 milliseconds!
    h = dht.readHumidity();
    t = dht.readTemperature();
}

void ultrasonic_sensor()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.0343 / 2;
  actual_water_level = 26 - distance; /*note that actual_water_level is the actual water level and we need to figure it*/
}

void PH_sensor()
{
  temp = random(660 , 700);
  ph_act = temp / 100.0;
}

void waterproof_sensor()
{
  sensors.requestTemperatures();// Send the command to get temperatures

  Celsius = sensors.getTempCByIndex(0);        //<==
}

void TDS_sensor()
{
  long int tds = random(19900 , 20200);
  tdsValue = tds / 100.0;
}
