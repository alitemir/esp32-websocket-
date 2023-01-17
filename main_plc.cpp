#include <ModbusMaster.h>
// #include <SoftwareSerial.h>
// SoftwareSerial mod; // RX=26 , TX =27
ModbusMaster node;

#define MAX485_DE      22
#define MAX485_RE_NEG  23

void preTransmission()            //Function for setting stste of Pins DE & RE of RS-485
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
   
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

int i = 0;
 uint8_t  result ;
 constexpr uint32_t hwSerialConfig = SERIAL_8E1;

void setup()
{
 
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  // use Serial (port 0); initialize Modbus communication baud rate
  // mod.begin(9600, SWSERIAL_8N1, 18, 19, false, 95, 11);
  // mod.setTimeout(500);
  Serial2.begin(9600);

  Serial.begin(9600);

  // communicate with Modbus slave ID 2 over Serial (port 0)
  node.begin(2, Serial2);
  node.preTransmission(preTransmission);         //Callback for configuring RS-485 Transreceiver correctly
  node.postTransmission(postTransmission);
}


void loop()
{
  
  node.writeSingleRegister(0x1000, i); // 
  delay(20);
  node.writeSingleRegister(0x1001, i+1);
  delay(20);
  node.writeSingleRegister(0x1002, i+2);//Writes value to 0x1000 holding register
delay(20);
  node.writeSingleCoil( 0x805, LOW);
 node.writeSingleCoil( 0x805, HIGH);
delay(20);

int r=node.getResponseBuffer(node.readHoldingRegisters(0x1000,2));
delay(100);
int w=node.getResponseBuffer(node.readHoldingRegisters(0x1002,2));
delay(100);

Serial.print(r);
Serial.print(" ");
Serial.println(w);

  i++;
delay(100);
}