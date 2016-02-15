#include <Wire.h>
#include <MySensor.h>
#include <SPI.h>
#include <BH1750.h>
#include <avr/wdt.h>
#include <SimpleTimer.h>
#include <DHT.h>  

 //#define NDEBUG                        // enable local debugging information

#define SKETCH_NAME "Outer light and temp"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"
#define NODE_ID 21 //or AUTO to let controller assign     


/******************************************************************************************************/
/*                               				Сенсоры												  */
/******************************************************************************************************/

#define CHILD_ID_LIGHT				  60  //Освещенность
#define CHILD_ID_TEMP				  0  //Температура
#define CHILD_ID_HUM				  80  //Влажность

#define REBOOT_CHILD_ID                       100
#define RECHECK_SENSOR_VALUES                 101 

/******************************************************************************************************/
/*                               				IO												                                    */
/******************************************************************************************************/

#define HUMIDITY_SENSOR_DIGITAL_PIN 5


/*****************************************************************************************************/
/*                               				Common settings									      */
/******************************************************************************************************/
#define RADIO_RESET_DELAY_TIME 20 //Задержка между сообщениями
#define MESSAGE_ACK_RETRY_COUNT 5  //количество попыток отсылки сообщения с запросом подтверждения

boolean gotAck=false; //подтверждение от гейта о получении сообщения 
int iCount = MESSAGE_ACK_RETRY_COUNT;

boolean boolRecheckSensorValues = false;

uint16_t lastlux;

float lastTemp;
float lastHum;
boolean metric = true; 

SimpleTimer checkLUX; 
SimpleTimer checkTempHum; 
int LUXTimerID;
int TempHumTimerID;

BH1750 lightSensor;
DHT dht;

MySensor gw;

MyMessage msgLightLevel(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
MyMessage msgHumidity(CHILD_ID_HUM, V_HUM);
MyMessage msgTemperature(CHILD_ID_TEMP, V_TEMP);

void setup() {

	gw.begin(incomingMessage, NODE_ID, false);


  	gw.wait(RADIO_RESET_DELAY_TIME);
  	gw.sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);



    gw.wait(RADIO_RESET_DELAY_TIME); 
    gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);   

    gw.wait(RADIO_RESET_DELAY_TIME); 
    gw.present(CHILD_ID_HUM, S_HUM);  

    gw.wait(RADIO_RESET_DELAY_TIME); 
    gw.present(CHILD_ID_TEMP, S_TEMP);      

      lightSensor.begin();

  	  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 


      LUXTimerID = checkLUX.setInterval(120000, checkLightLevel);
      TempHumTimerID = checkTempHum.setInterval(150000, checkTH);

        	  //Enable watchdog timer
  				wdt_enable(WDTO_8S);

}

void loop() {

checkLUX.run();
checkTempHum.run();

  gw.process();

if (boolRecheckSensorValues)
{
  checkLUX.disable(LUXTimerID);
  checkTempHum.disable(TempHumTimerID);

	checkLightLevel();
	checkTH();	

  checkLUX.enable(LUXTimerID);
  checkTempHum.enable(TempHumTimerID);	
  boolRecheckSensorValues = false;
}

    //reset watchdog timer
    wdt_reset();    

}


void incomingMessage(const MyMessage &message) {

  if (message.isAck())
  {
    gotAck = true;
    return;
  }

    if ( message.sensor == REBOOT_CHILD_ID && message.getBool() == true && strlen(message.getString())>0 ) {
             wdt_enable(WDTO_30MS);
              while(1) {};

     }
     


   

    if ( message.sensor == RECHECK_SENSOR_VALUES && strlen(message.getString())>0) {
         
         if (message.getBool() == true)
         {
            boolRecheckSensorValues = true;


         }

     }

        return;      
} 




void checkLightLevel()
{

  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
  Serial.println(lux);
  if (lux != lastlux) {


            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   gw.send(msgLightLevel.set(lux), true);
                    gw.wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                gotAck = false;


      lastlux = lux;
  }

          #ifdef NDEBUG
          Serial.print ("Light level is: ");
          Serial.print (lux);   
          Serial.println (" LUX");
          #endif 

}


void checkTH()
{

  delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
          #ifdef NDEBUG  	
      		Serial.println("Failed reading temperature from DHT");
      	  #endif		
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }


            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   gw.send(msgTemperature.set(temperature,1), true);
                    gw.wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                gotAck = false;

          #ifdef NDEBUG      	
		    Serial.print("T: ");
		    Serial.println(temperature);
		  #endif  	
  }
  
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
          #ifdef NDEBUG    	
      		Serial.println("Failed reading humidity from DHT");
      	  #endif	
  } else if (humidity != lastHum) {
      lastHum = humidity;


            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   gw.send(msgHumidity.set(humidity,1), true);
                    gw.wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                gotAck = false;

          #ifdef NDEBUG       	
		      Serial.print("H: ");
		      Serial.println(humidity);
		  #endif    	
  }

}
