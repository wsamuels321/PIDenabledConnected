/*---------------------
          And now for a little introduction!
     This is the mashup of various peoples code that enables my Delonghi pump espresso machine to talk with my phone.
     It is also PID enabled and can control the temperature of the temperature +/- 10degrees C, eventhough this thing uses F
     

      Got the max6675 library from Ladyada
                    https://github.com/adafruit/MAX6675-library
      found the PID with my roasting program, provided by  Brett Beauregard
                  https://github.com/johnmccombs/arduino-libraries/tree/master/PID_Beta6
      

*/
#include <SoftwareSerial.h>
#include <PID_Beta6.h>
#include <max6675.h>
//#include <EEPROM.h>
//
///--------------------todo, modularize, PID, test
//------pin setup
//----pump pin and heat pin
int pumppin = 4;
int heatPin = 3; //(has PWM capabilities)
//thermocouple pins
int thermoDO = 7;
int thermoCS = 6;
int thermoCLK = 5;
//b2th pin setup(software Serial)
int b2thRX = 11;
int b2thTX = 12;
//---------for temperature processing
double _tempF;
double output, input, _setpoint;
//-------defaults for autobrew(seconds)
int _PIBT = 5, _PI = 5, _bt = 15;
//-----setup some objects to be used
//thermocouple
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
//PID controller
PID espressoTime(&input, &output, &_setpoint, 1, 4, 1);
//imitation serial port for blootooth, tastes just like fruit tori!
SoftwareSerial blu2th(b2thRX, b2thTX);


//---initialize some pins and such
void setup() {
  // put your setup code here, to run once:
  //-----setup some pins
  pinMode(pumppin, OUTPUT);
  pinMode(heatPin, OUTPUT);
  /*
  digitalWrite(13, HIGH);
  digitalWrite(13, LOW);//-----------some debuggary*/
  Serial.begin(9600);
  blu2th.begin(9600);
  //initalize setpoint(get from Blu2th later   **the PID tends to overshoot when heating the element, subtract 10 from setpoint
  _setpoint = 85;
  //-------initalize PID
  espressoTime.SetInputLimits(0, 230);
  espressoTime.SetMode(AUTO);
  delay(500);

  //empty line
}
boolean resetBit = false;
void loop() {
  //Serial.println(output);                 //debuggary
  //Serial.println("---------------");      //debuggary
  //Serial.println(_tempF);                 //debuggary
  if (blu2th.available()) {
    readBlu2th();
    if (resetBit = false) { //first time its avalable, set resetBit to true
      resetBit = true;
    }
  }
  /* else if (!blu2th.available() && resetBit == true) //if the blu2th is not avalable, and it has been in the past (turn everything off

     blu2th.end();           //close connection
     Serial.end()
     resetBit = false;       //reset bit for next time*/                   //think about this one

  else {             //otherwise write buffer to bluetooth
    writeBlu2th();
  }

  heatControl();        //then do some work to the temp of the water tank


}


//--------------where PID has some fun
void heatControl() {
  input = _tempF;
  //---------failsafe block, dont boil-over now, or melt down for that matter
  if (input > 212.00) {
    analogWrite(heatPin, 0);
  }
  else {
    espressoTime.Compute();
    analogWrite(heatPin, output);
  }

}
//supply android with some information from arduino.
int count = 0;
String TempTX;
void writeBlu2th() {
  // char buffet[8] = "";           //buffer char array
  _tempF = upDateTemp();         //get temp from thermocouple
  TempTX = String(_tempF);
  // sprintf(buffet, "%5.3f", _tempF);     //format char array as ---.---
  blu2th.print(TempTX);               //send buffer to android
  //Serial.println(_tempF);

  delay(400);

  // Serial.println("---------------------------");
}
double upDateTemp()
{

  return thermocouple.readFarenheit();


}
//------------some bluetooth control

void readBlu2th()
{

  byte f;
  String recieved = "";
  f = blu2th.read();
  //-------------switch case for manual/auto
  switch (f)
  {
    //-------------case manual begin pump
    case (2):
      // Serial.println("initiate pump");
      digitalWrite(pumppin, HIGH);
      //  delay(100);

      break;
    //--default case, set to pumpin to low(so the pump dosnt keep going)
    case (1):
      digitalWrite(pumppin, LOW);
      //delay(500);
      /* blu2th.print("hello");
       blu2th.print(n);
       blu2th.flush();
       n++;*/
      break;
    //auto brew case------------------------------------------
    case (3):
      Serial.print("enter case 3");
      blu2th.print("startAB");
      delay(500);
      autoBrew();
      //after autobrew, goback to default, dont keep the pump going
      digitalWrite(pumppin, LOW);
      break;
    //---------if preferences screen has been changed
    case (4):
      blu2thInput();
      break;



    default:
      break;
  }
}
//  _PIBT(time for pump to saturate grinds)
//  _PI(time to wait for brewing)
//  _bt(time to pull a shot)



void autoBrew() {

  _PIBT = _PIBT * 1000;
  _PI = _PI * 1000;
  _bt = _bt * 1000;
  digitalWrite(pumppin, HIGH);  //start pump
  //Serial.println(Serial.available());
  delay(_PIBT);                //for _PIBT ms
  digitalWrite(pumppin, LOW);
  delay(_PI);
  digitalWrite(pumppin, HIGH);
  delay(_bt);
  blu2th.print("done");
  Serial.print("after shot terminated string");
  delay(500);// bufferTime
}
//sets _PIBT, _PI, _bt, and _setPoint  (byte by byte.)  Yea, I'm a noob,  I comment every line
void blu2thInput() {
  String b2Re;
  b2Re = blu2th.readStringUntil('>');      //store btstring from android
  String temp;                              //temporary string storage (not to be confused with temperature)
  int i = 0;                               //iterator for going char by char through the string
  char q;                                  //temporary char varaible for storing bytes into String temp
  int w = 1;                             //when a comma (',') is encountered, this controls the switch statment for breaking it up into variables
  int g = 0;                                //another temporary variable for converting from string to int for storage into variabls
  //Serial.println("--------------" + b2Re);     //debuggary
  while (b2Re.length() >= i)                    //iterate though bluetooth string
  { //open braket ;)

    q = b2Re[i];                              //store charactor from android string into q
    //------------------case a comma is read, convert to int and set variables accordingly (pibt, pi,bt, stpoint)
    if (q == ',') {               //if there is a comma, do some processing
      g = temp.toInt();                    //convert temp built string to int
      //Serial.println(g);
      switch (w) {                //switch based on what iteraton of string i.e: "1,2,3,4"
        case (1):
          //first case is PIBT
          _PIBT = g;
          Serial.println(_PIBT);
          w++;
          break;
        case (2):
          //second case is PI
          _PI = g;
          Serial.println(_PI);
          w++;
          break;
        //third case is BT
        case (3):
          _bt = g;
          Serial.println(_bt);
          w++;
          break;
        //forth case is temperature setpoint
        case (4):
          Serial.println(_setpoint);
          _setpoint = g - 10;
          break;
        default:
          break;
      }
      temp = "";   //if the string has been processed, start over
    }
    //----------end comma accorence
    //---otherwise add to temp string
    else {
      temp = temp + q;      //build string char by char
    }

    i++;     //iterate thorugh temp string charbychar
  }
}
