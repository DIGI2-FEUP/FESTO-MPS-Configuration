/*
This code was written by myself Hugo Moreira de Sousa Diogo as part of my Research Initiation Grant at DIGI2, SYSTEC. 
For any questions regarding the code, please contact me:
- Email: hugomsdiogo@gmail.com
- LinkedIn: https://www.linkedin.com/in/msousahugo/
- GitHub: https://github.com/Hugo-Diogo

The code is used to control a conveyor using an Arduino Mega 2560 PLC. 
The conveyor is controlled using MQTT protocol, thus it can be integrated with other systems in the factory. 
Additionally, the code reads the angle from an absolute encoder and calculates the velocity of the conveyor using a moving average filter. 
Finally, the code also reads a distance sensor to stop the conveyor if an object is detected.
By using MQTT comunications protocol we can read the velocity of the conveyor in real time and also turn on and off the conveyor remotely. 
In this project we use an MQTT broker installed in the computer called Mosquitto, but it can be installed in a server as well, thus allowing to control the conveyor from anywhere in the world.
You shall find more details about the code and the project in the README file.
I also show some suggestions for future work in the README file.

*/


#include <SPI.h>
#include <PubSubClient.h>
#include <Ethernet2.h>
#include <EEPROM.h>


/*
This variables helps simplifing the code for changing both the pins or the PLC. This code was written for Arduino Mega 2560, thus the pins are set according to this board and not the PLC in itself.
In fact, inside the PLC I have used was built a Arduino mega thus you can check on the internet which pins correspond to the PLC output pins and change the code accordingly. 
*/

#define R1_8 42
#define I0_5 57
#define DIST_THRESH 100


const int Cs = 3; // this pin enables or disables the encoder (active low)
int enableSerial = 0;
float prevAngle = 0.000;

//------------------------------------------------------------------------------------------------------------------------------settings 
/*
Here you can set the parameters for the velocity calculation. This code is ment to be used independntly of the roller size.
Additionally a moving average filter is implemented to smooth the velocity signal and you can change the parameters of this filter as well.
*/
const float radius = 0.02296; // With this variable you can set the radius of the roller (meters)
const int pi = 3.1416; // You can add more precision if needed
const int sampleInterval = 20; // Interval between samples in milliseconds
const int numberSamples = 50; // Number of samples for moving average

//------------------------------------------------------------------------------------------------------------------------------ethernet settings



byte mac[6]    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
IPAddress server(10,227,17,96);
byte ip[] = { 10,227,17,105};
char macstr[18];


//------------------------------------------------------------------------------------------------------------------------------functions

// Variáveis MQTT
EthernetClient ethClient;
PubSubClient client(ethClient); 

// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  // Convertendo o payload para char
  char message = (char)payload[0];

  // Veryfying received message
  if (message == '1') {
    digitalWrite(R1_8, HIGH);  // Turn on the conveyor
    Serial.println("conveyor ON.");
  } else if (message == '0') {
    digitalWrite(R1_8, LOW);   // Turn off the conveyor
    Serial.println("conveyor OFF.");
  }
}












///////////////////////////////////////////////////////////////////////////////////////////////////////
// Note that it is important to read the datasheet of the encoder to understand how to read the angle correctly. In this case, the encoder is a 14-bit absolute encoder, thus we need to read two bytes and combine them to get the angle value. Additionally, the angle value is between 0 and 2*pi, thus we need to convert it to radians.
double readAngle(){

  unsigned int angle1 = 0;
  unsigned int angle2 = 0;

  SPI.begin();
  digitalWrite(Cs, LOW);

  
  angle1 = SPI.transfer(0x7FFF);
  angle2 = SPI.transfer(0x3FFF);

  angle1 &= 0x3F;
  angle1 = angle1 << 8;
  angle1 |= angle2;

  digitalWrite(Cs, HIGH);
  SPI.end();

  return (double(angle1) * 2.0000 * pi) / double(pow(2, 14));
  
}







///////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Using a moving average filter of the angle difference to calculate the velocity of the conveyor.
*/

double calculateVelocity() {

  double currentAngle = readAngle();
  double deltaAngle; // the angle is getting smaller and smaller so we calculate the difference the other way around
  double sum = 0;
  static double v[numberSamples] = {0};
  double angleAverage = 0;
  
  double timeSecond = double(sampleInterval) / 1000.00;
  
  



  if( currentAngle > prevAngle ){
    deltaAngle = prevAngle + (2.00000 * pi - currentAngle);
  }else{
    deltaAngle = prevAngle - currentAngle;
  }
  Serial.print("Angle: ");
  Serial.println(deltaAngle);


  bool zero = 0;
  if (enableSerial == 0){

    v[0] = deltaAngle;
    enableSerial++;

  }else if (enableSerial < numberSamples){

    v[enableSerial - 1] = deltaAngle;
    enableSerial++;


  }else{

    v[enableSerial % numberSamples] = deltaAngle;
    enableSerial++;

    for(int i = 0; i < numberSamples; i++){
        if(v[i] == 0 || v[i] >= 4) zero = 1;
        sum += v[i];
    }

    if(zero == 1) angleAverage = 0;
    else angleAverage =  sum / double(numberSamples);
  }
  
  
  prevAngle = currentAngle;
  
  if(angleAverage == 0) return 0;
  return (angleAverage / timeSecond) * radius;
}


//---------------------------------------------------------------------------------------------------------------------------setup

void setup() {
  Serial.begin(9600);

/*
The first part of this code was written by the company of the PLC and is use to find the MAC address of this PLC if it was not set before.
It first checks if there is a MAC address stored in the EEPROM memory, if not it generates a random one and saves it to the memory for future use.
*/
  if (EEPROM.read(1) == '#') {
    for (int i = 0; i < 6; i++) {
      mac[i] = EEPROM.read(i);
    }
  } else {
    randomSeed(analogRead(0));
    for (int i = 0; i < 6; i++) {
      mac[i] = random(0, 255);
      EEPROM.write(i, mac[i]);
    }
    EEPROM.write(1, '#');
  }
  snprintf(macstr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Start up networking
  Serial.print("DHCP (");
  Serial.print(macstr);
  Serial.print(")...");
  Ethernet.begin(mac, ip);

// Initializing the Ethernet conection and checking if the connection was successful
  Serial.println("Initializing Ethernet...");
  Serial.println(Ethernet.localIP());
  

//Trying to connect to the MQTT broker and checking if the connection was successful
Serial.print("Verifying connectivity with the broker...");
if (EthernetClient().connect(server, 1883)) {
    Serial.println("Broker reachable!");
    EthernetClient().stop();
} else {
    Serial.println("Broker NOT reachable.");
}

//-----------------------------------------------------pins and SPI
  pinMode(Cs, OUTPUT);
  digitalWrite(Cs, HIGH);
  SPI.beginTransaction(SPISettings(14000000,MSBFIRST, SPI_MODE1));
  pinMode(R1_8, OUTPUT);
  digitalWrite(R1_8, LOW);
  pinMode(8, OUTPUT);
  analogWrite(8, 0);


  
 




//------------------------------------------------------------client

  // Configuration of the MQTT client
  client.setServer(server, 1883);    // Configurando o servidor do MQTT (broker)
  client.setCallback(callback);      // Definindo a função callback para mensagens

  // Tentando conectar ao broker MQTT
  while (!client.connected()) {
    Serial.print("Conecting to MQTT broker...");
    if (client.connect("M-DuinoClient")) {
      Serial.println("Conected to MQTT broker");
      client.subscribe("motor/control");  // Subscribing to the topic "motor/control" to receive control messages
    } else {
      Serial.print("Error. conecting, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}



//--------------------------------------------------------------loop

void loop() {
  client.loop(); // This function is used to keep the MQTT connection alive and to check for incoming messages. It should be called as often as possible in the loop.

  
  double velocity = calculateVelocity();
  
  if(enableSerial >= numberSamples){
    Serial.print("Velocity: ");
    Serial.println(velocity);
    char v[10] ;
    dtostrf(velocity, 8, 2, v);
    client.publish("Velocity", v);
  }else{
    Serial.println("Loading...");
    client.publish("Velocity", "Loading");
  }
  delay(sampleInterval);


  float dist = analogRead(I0_5);
  if (dist > DIST_THRESH){
    // Object detected, stop the conveyor
    digitalWrite(R1_8, LOW);
    // Delay to ensure safety before restarting the conveyor
    delay(2000);
  }
  

}


