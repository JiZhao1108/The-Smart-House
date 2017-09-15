//ESD Final Project
//Creator: Raza Qazi and Ji Zhao
//Date: 25 April 2017
//Description: 
//1. C Code in main Simblee BLE processor.
//2. It handle all BLE command and sensor input flow
//3. Decodes and processes information and Control outputs accordingly.


///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////   Including Headers  ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#include <SimbleeBLE.h>
#include <SPI.h>
#include <myOLED.h>

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////   Preprocessor Directives  ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//Light, Moisture and Fire Sensor
#define lightPin 21                     //Power pin for Light Sensor
#define lightPower 24                   //Output pin for Light Sensor 
#define firePin 17                      //Output pin for Fire Sensor 
#define moisturePin 6                   //Output pin for Moisture Value
#define moisturePower 19                //Power pin for Moisture Sensor
#define echoPin 7                       //Output pin for Vicinity Sensor
#define vicinityPower 22                //Power pin for Vicinity Sensor
#define waterMotor 18                   //Water Motor Output
#define acFan 15                        //AC/Fan Motor Output
#define roomLight 12                    //Roomlight Output
#define streetLight 11                  //Streetlight Output
#define fireSprinkle 25                 //Water Sprinkler System Output
#define autoDoor 20                     //Door Motor Output
#define autoLock 23                     //Door Motor/Alarm Output

/* Changin SPI default pins in variants.h file
#define SPI_INTERFACE        NRF_SPI0
#define PIN_SPI_SS           (5u)
#define PIN_SPI_MOSI         (16u)
#define PIN_SPI_MISO         (3u)
#define PIN_SPI_SCK          (14u)
*/
//SPI Interface PINS to OLED Display // MOSI = pin 16 and SCK = pin 14
#define PIN_RESET 9  // Connect RST to pin 9 (req. for SPI and I2C)
#define PIN_DC    8  // Connect DC to pin 8 (required for SPI)
#define PIN_CS    10 // Connect CS to pin 10 (required for SPI)

////////////////////////////////////////////////////////////////////////////////////////
//Sensor Name and Pin Allocations
MicroOLED oled(PIN_RESET, PIN_DC, PIN_CS); //Declare OLED object inside OLED class (C++)

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// BLE message allocations /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

const char msg_pass            = '1';//To enter admin settings
const char msg_light           = 'a';//To control light sensor
const char msg_moisture        = 'b';//To control moisture sensor
const char msg_temp            = 'c';//To control temperature sensor

const char msg_vicinity        = 'd';//To control vicinity sensor
const char msg_print_data      = 'e';//To upload moisture & values for current month
const char msg_away            = 'f';//To upload temperature values for current month

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////State Flag allocations /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

boolean light_flag = LOW; //Flags to store state of light sensor
boolean moist_flag = LOW;//Flags to store state of moisture sensor
boolean temp_flag = LOW; //Flags to store state of temperature sensor
boolean vicinity_flag = LOW;//Flag to store state of vicinity sensor
boolean away_flag = LOW;//If we are at home or holidays//For security purposes
boolean pass_flag = LOW; //Set password flag low
boolean fire_flag = LOW; //Flag to store state of fire sensor

////////////////////////////////////////////////////////////////////////////
//////////////// Declaration of global variables and arrays ////////////////
////////////////////////////////////////////////////////////////////////////

boolean light_val_new, light_val_old, fire_val = 1;
float moist_val = 0;//Stores moisture value
float temp_val = 0; //Stores temperature value

//unsigned int press_val = 0; //Stores pressure value
//float hum_val = 0; //Stores humidity value

unsigned int moist_array[500]; //Moisture array for storage
unsigned int temp_array[500]; //Temperature array for storage
unsigned char moist_index = 0;//Pointer index for Moisture array
unsigned char temp_index = 0; //Pointer index for Temperature array
long vicinity_duration;
int vicinity_distance;

////////////////////////////////////////////////////////////////////////////
////////////////////////// Fire Sensor Interrupt ///////////////////////////
////////////////////////////////////////////////////////////////////////////
void fire_ISR()
{
  fire_val = ! fire_val;
  if (fire_val == HIGH)
  {
    Serial.println("\n\rFIRE!!!");
    Serial.println("\n\rPlease contact:(303)441-3350\n\r");
    all_off();
    digitalWrite(fireSprinkle, HIGH);
    oled.begin(); //Initialize the display
    oled.clear(ALL); //Clear the display
    oled.clear(PAGE); //Clear RAM for current page
    oled.display(); //Show to changes
    oled.setCursor(00, 00); //Jump to location 0,0
    oled.print("Fire");
    oled.setCursor(00, 10);
    oled.print("Plz call:");
    oled.setCursor(00, 30);
    oled.print("3034413350");
    oled.display();
    oled.clear(PAGE);
    fire_flag == HIGH; //Flip fire_flag state
  }
}
////////////////////////////////////////////////////////////////////////////
////////////////////////// All Sensors Turned Off Function /////////////////
////////////////////////////////////////////////////////////////////////////
void all_off(void)
{
  digitalWrite(lightPower, LOW); //Turn off Power to all sensors
  digitalWrite(moisturePower, LOW);
  digitalWrite(vicinityPower, LOW);
  digitalWrite(waterMotor, LOW); // 18
  digitalWrite(acFan, LOW);
  digitalWrite(roomLight, LOW);
  digitalWrite(streetLight, LOW);
  digitalWrite(autoDoor, LOW);
  digitalWrite(autoLock, LOW);
  light_flag = LOW; //Reset all state flags
  moist_flag = LOW;
  temp_flag = LOW;
  vicinity_flag = LOW;
}

void oled_label(void) //Used after password is succesfully entered
{
  //Initilization
  oled.clear(PAGE);
  oled.clear(ALL);
  oled.setCursor(0, 0); // Set the text cursor to the upper-left of the screen.
  oled.print("T('C):"); // Print a const string
  oled.setCursor(0, 10);
  oled.print("M(%):");
  oled.setCursor(0, 20);
  oled.print("D/N:");
  oled.setCursor(0, 30);
  oled.print("Vic:");
  oled.display(); // Draw to the screen

}

void oled_label2(void) //used to refresh the dispaly continuously in between readings
{
  //Initilization
  oled.setCursor(0, 0); // Set the text cursor to the upper-left of the screen.
  oled.print("T('C):"); // Print a const string
  oled.setCursor(0, 10);
  oled.print("M(%):");
  oled.setCursor(0, 20);
  oled.print("D/N:");
  oled.setCursor(0, 30);
  oled.print("Vic:");
  oled.display(); // Draw to the screen

}
/* A 4 digit password function which had bugs and we couldnt complete it
unsigned char password(void)
{
  Serial.print("\n\rWe have Password Function\n\r");
  Serial.println(ble_rx);
  oled.clear(PAGE);
  oled.clear(ALL);
  oled.setCursor(0, 0);
  oled.print("Password: ");
  oled.display();
  oled.setCursor(0, 10);
  delay(5000);
  rx_flag = 0; //Reset rx flag
  Serial.print("\n\rWaiting for first char\n\r");
  while(rx_flag == 0)
  {
    }
  if (ble_rx == 'a')//First char received
  {
    rx_flag = 0;//Reset buffer flag
    Serial.print(ble_rx);
    oled.print(ble_rx);
    oled.display();
    while(rx_flag == 0);
    Serial.print("\n\rWaiting for second char\n\r");
    if (ble_rx == pass_set[1])
    {
      rx_flag = 0;
      Serial.print(ble_rx);
      oled.print(ble_rx);
      oled.display();
      while(rx_flag == 0);
      Serial.print("\n\rWaiting for third char\n\r");
      if (ble_rx == pass_set[2])
      {
        rx_flag = 0;
        Serial.print(ble_rx);
        oled.print(ble_rx);
        oled.display();
        while(rx_flag == 0);
        Serial.print("\n\rWaiting for fourth char\n\r");
        if (ble_rx == pass_set[3])
        {
          rx_flag = 0;
          Serial.print(ble_rx);
          oled.print(ble_rx);
          oled.display();
          delay(1000);
          oled.clear(ALL);
          oled.setCursor(0, 0);
          oled.print("SUCCESSFUL!!!");
          oled.display();
          delay(3000);
          oled.clear(ALL);
          return 1;
        }
      }
    }
  }
  oled.clear(ALL);
  oled.setCursor(0, 0);
  oled.print("Fail!!!");
  delay(3000);
  oled.clear(ALL);
  return 0;
}*/

////////////////////////////////////////////////////////
// BLE Receive data function gets called everytime it receives data through bluetooth
void SimbleeBLE_onReceive(char *data, int len)
{
  //ble_rx = data[0];
  // rx_flag = 1;
  action(data[0]);        //In Simblee data[0] stores the received character by default and send it to action function to execute proper codes
}

/////////////////////////////////////////////////////////
//BLE received message decode and processing function
void action (int now)
{
  //ble_rx = now;
  if (now == -1)
  {
    return;
  }

  if (now == msg_pass) // If correct password has been entered
  {
    pass_flag = !pass_flag; //Flip the pass flag
    if (pass_flag == HIGH) //If flag is HIGH i.e. user entering admin mode
    {
      oled.clear(ALL);
      oled.setCursor(00, 20);
      oled.print("Password");
      oled.setCursor(00, 30);
      oled.print("Accepted");
      oled.display();
      delay(2000);
      oled_label();
    }
    if (pass_flag == LOW) //if Flag is LOW i.e. user leaving admin mode
    {
      all_off();
      oled.clear(PAGE);
      oled.clear(ALL);
      oled.setCursor(0, 0);
      oled.print("Locked");
      oled.display();
      delay(3000);
      oled.clear(PAGE);
      oled.clear(ALL);
      oled.setCursor(00, 00);
      oled.print("Enter");
      oled.setCursor(00, 10);
      oled.print("Password");
      oled.begin();
      oled.display();
    }


  }

  if (now == msg_light) //If light message is received
  {
    light_flag = ! light_flag; //Flip the light flag
    oled_label2(); //Refresh display
    if (light_flag == LOW) //If flag is low
    {
      digitalWrite(lightPower, LOW); //Turn off the light sensor and its outputs
      digitalWrite(roomLight, LOW);
      digitalWrite(streetLight, LOW);
      oled.setCursor(30, 20);//clear oled screen for light sensor part
      oled.print("     ");
      oled.display();
    }
  }

  if ( now == msg_moisture)//If moisture message is received
  {
    moist_flag = ! moist_flag; //Flip the moisture flag
    oled_label2();//Refresh display
    if (moist_flag == LOW)//If flag is low
    {
      digitalWrite(moisturePower, LOW); //Turn off the moisture sensor and its outputs
      digitalWrite(waterMotor, LOW);
      oled.setCursor(36, 10);//clear oled screen for moisture sensor part
      oled.print("    ");
      oled.display();
    }
  }

  if ( now == msg_temp)//If temperature message is received
  {
    temp_flag = ! temp_flag;//Flip the temperature flag
    oled_label2();//Refresh display
    if (temp_flag == LOW)//If flag is low
    {
      digitalWrite(acFan, LOW);//Turn off the temperature sensor outputs
      oled.setCursor(36, 00);//clear oled screen for temperature sensor part
      oled.print("    ");
      oled.display();
    }
  }

  if (now == msg_vicinity)//If vicinity message is received
  {
    vicinity_flag = !vicinity_flag; //Flip the vicinity flag
    oled_label2();//Refresh display
    if (vicinity_flag == LOW)//If flag is low
    {
      digitalWrite(autoDoor, LOW);//Turn off the vicinity sensor and its outputs
      digitalWrite(autoLock, LOW);
      digitalWrite(vicinityPower, LOW);
      oled.setCursor(30, 30);//clear oled screen for vicinity sensor part
      oled.print("    ");
      oled.display();
    }
  }

  if (now == msg_away)//If away message is received

  {
    away_flag = !away_flag; //Flip the away flag
    oled_label2();//Refresh display
    if (away_flag == HIGH)//If flag is HIGH
    {
      oled.setCursor(00, 40);//Display it on the screen
      oled.print("Away");
      oled.display();
    }
    if (away_flag == LOW)//If Flag is LOW
    {
      oled.setCursor(00, 40);//Clear it from the screen
      oled.print("    ");
      oled.display();
    }

  }
  if (now == msg_print_data)//If upload message is received
  {
    oled_label2();//Refresh display
    oled.setCursor(30, 40);//Dsiplay "Print" on the screen
    oled.print("Print");
    oled.display();
    delay(2000);
    oled.setCursor(30, 40);
    oled.print("        ");
    oled.display();
    //Start sending data over UART
    unsigned int i;
    Serial.print('M');// Let Raspberry Pi know it will receive moisture data log
    Serial.print('S');//Array Start Bit
    for (i = 0; i < moist_index; i++)
    {
      Serial.print(moist_array[i]);
      Serial.print('Z'); //Byte Stop bit
    }
    Serial.print('E');//Array Stop Bit
    moist_index = 0; //Reset Moisture array pointer for overwrite
    delay(1000);
    Serial.print('T');//Let Raspberry Pi know it will receive temp data log
    Serial.print('S');//Array Start Bit
    for (i = 0; i < temp_index; i++)
    {
      Serial.print(temp_array[i]);
      Serial.print('Z'); //Byte Stop bit
    }
    Serial.print('E');//Array Stop Bit
    temp_index = 0; //Reset temperature array pointer for overwrite
  }
}

void setup()
{
  //Set Up SimbleeBLE parameters
  SimbleeBLE.deviceName = "ESD Final"; //Name of our BLE
  SimbleeBLE.advertisementData = "ESD S17";
  SimbleeBLE.advertisementInterval = MILLISECONDS(300); //Advertises in intervals of 300ms each
  SimbleeBLE.txPowerLevel = 0;  // (-20dbM to +4 dBm) //Tranmit power level set to average

  // Start the BLE stack
  SimbleeBLE.begin();

  //Configure Simblee ports and its GPIOs to Input or Output
  pinMode(moisturePin, INPUT);//Input data from moisture sensor
  pinMode(moisturePower, OUTPUT); //Powers the moisture sensor
  pinMode(lightPin, INPUT); //Input data from light sensor
  pinMode(lightPower, OUTPUT); //Powers the lighte sensor
  pinMode(firePin, INPUT); //Interrupt from Fire Sensor
  pinMode(trigPin, OUTPUT);//Ultrasound transmit
  pinMode(echoPin, INPUT); //Ultrasound receive
  pinMode(vicinityPower, OUTPUT);//Ultrasound Power
  pinMode(waterMotor, OUTPUT);
  pinMode(acFan, OUTPUT);
  pinMode(roomLight, OUTPUT);
  pinMode(streetLight, OUTPUT);
  pinMode(fireSprinkle, OUTPUT);
  pinMode(autoDoor, OUTPUT);
  pinMode(autoLock, OUTPUT);

  //Start UART Communication at 9600 baud rate
  Serial.begin(9600);

  //Initialize Interrrupt for Fire Sensor
  attachInterrupt(firePin, fire_ISR, RISING); //when firePin goes HIGH, call fire_ISR

  //Start OLED
  oled.begin();
  oled.clear(ALL);//Complete Reset
  oled.display();
  //Initilization
  oled.clear(PAGE);
  oled.clear(ALL);
  oled.setCursor(00, 00);
  oled.print("Enter");
  oled.setCursor(00, 10);
  oled.print("Password");
  oled.begin();
  oled.display();
}

void loop()
{
  if (pass_flag == HIGH)//If passoword is active
  {
    if (temp_flag == HIGH) //if temperature flag is HIGH
    {
      temp_val = Simblee_temperature(CELSIUS); //get the value for temperature
      Serial.print((unsigned int)temp_val); //Print it on screen
      temp_array[temp_index] = (unsigned int)temp_val;//Store it in array
      Serial.print("'C\n\r");
      temp_index++;//Increment temperature array pointer
      if ((unsigned int)temp_val > 28)//If temperature is more than 28 'C, turn on AC
      {
        //Serial.println("AC is ON");
        digitalWrite(acFan, HIGH); 
      }
      if ((unsigned int)temp_val < 29)//If temperature is less than 28 'C, turn off AC
      {
        digitalWrite(acFan, LOW);
      }
      oled.setCursor(36, 00);//Display temperature valules on OLED after every second
      oled.print((unsigned int)temp_val);
      oled.display();
      delay(1000);
    }

    if (moist_flag == HIGH) //if moisture sensor is ON
    {
      digitalWrite(moisturePower, HIGH); //Turn the moisture sensor ON/OFF
      float moist_val1 = analogRead(moisturePin); //Read the moisture value
      moist_val = (unsigned int)moist_val1 / 10; //Calculate percentage of moisture in soil //1000=100%
      Serial.print(msg_moisture); //Print/Send the msg_moisture character on UART
      Serial.print((unsigned int)moist_val); //Print the integer value of moisture on UART
      moist_array[moist_index] = (unsigned int) moist_val; //Store moisture value in array by typecasting it into integer

      if ((unsigned int)moist_val < 51)//If moisture is less than 50%, turn on water motor
      {
        //Serial.println("MOTOR is ON");
        digitalWrite(waterMotor, HIGH);
      }
      if ((unsigned int)moist_val > 50)//If moisture is more than 50%, turn off water motor
      {
        // Serial.println("MOTOR is OFF");
        digitalWrite(waterMotor, LOW);
      }
      oled.setCursor(36, 10); //Display new moisture value on OLED every one second
      oled.print((unsigned int)moist_val);
      oled.display();
      delay(1000);
      moist_index++;//Increment moisture array pointer
    }

    if (light_flag == HIGH)//If light sensor is ON
    {
      digitalWrite(lightPower, HIGH); //Turn light sensor ON/OFF
      light_val_new = digitalRead(lightPin); //Read digital  output from the sensor
      if (light_val_new  != light_val_old) //If value has changed
      {
        Serial.print(msg_light); //Send value on UART only if it has changed between DAY/NIGHT
        oled.setCursor(30, 20);
        if (light_val_new == HIGH) //If it is day, turn on roomlights and turn off street lights
        {
          Serial.print(1);//DAY
          oled.print("DAY  "); //Print DAY on OLED screen
          digitalWrite(roomLight, HIGH);
          digitalWrite(streetLight, LOW);
        }
        else                     //If it is night, turn off roomlights and turn on street lights
        {
          Serial.print(0);//NIGHT
          oled.print("NIGHT"); //Print NIGHT on OLED screen
          digitalWrite(roomLight, LOW);
          digitalWrite(streetLight, HIGH);
        }
        light_val_old = light_val_new; //Update old variable
      }
      oled.display(); //Display and wait one second
      delay(1000);
    }

    if (vicinity_flag == HIGH) //If vicinity flag is HIGH
    { 
      digitalWrite(vicinityPower, HIGH); //Turn on vicinity sensor
      vicinity_distance = digitalRead(echoPin); //Measure output from sensor
      Serial.println(vicinity_distance);
      //Serial.println("inch");
      oled.setCursor(30, 30);
      if (vicinity_distance == 0) //if someone is near
      {
        oled.print("Near "); //Print near on OLED
        Serial.println("NEAR");
      }
      if (vicinity_distance == 1)
      {
        oled.print("Far  ");//Print far on OLED
        Serial.println("Far");
      }
      oled.display(); Display changes and check over 1 second
      delay(1000);
      if (vicinity_flag == HIGH && vicinity_distance == 0 && away_flag == HIGH) //if you are not in away mode
      {
        digitalWrite(autoDoor, HIGH); //Auto Door open if someone is near
        digitalWrite(autoLock, LOW);
        //Serial.print("a");
      }
      if (vicinity_flag == HIGH && vicinity_distance == 1 && away_flag == HIGH)//if you are not in away mode
      {
        digitalWrite(autoDoor, LOW);
        digitalWrite(autoLock, LOW);
        //Serial.print("b");
      }
      if (vicinity_flag == HIGH && vicinity_distance == 0 && away_flag == LOW)//if you are in away mode
      {
        digitalWrite(autoDoor, LOW);
        digitalWrite(autoLock, HIGH);
        //Serial.print("c");
      }
      if (vicinity_flag == HIGH && vicinity_distance == 1 && away_flag == LOW)//if you are in away mode
      {
        digitalWrite(autoDoor, LOW);
        digitalWrite(autoLock, LOW); //Auto Lock and alarm open if someone is near
        //Serial.print("d");
      }
    }
  }
}

