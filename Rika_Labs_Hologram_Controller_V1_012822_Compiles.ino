//RIKA LABS Hologram Controller
//Version 0.1
//Jan 2022 by Erik M. Werstler

//Initial menu code from CuriousScientist: https://curiousscientist.tech/blog/20x4lcd-rotaryencoder-menu
//Heavily modified to include actions for holography and multiple menu states

#include <LiquidCrystal_I2C.h> //SDA [A4], SCL = [A5] for [Arduino]
LiquidCrystal_I2C lcd(0x27, 20, 4);

int TimerCounter = 0;   //Used to control the state of the Switch function in the Timer menu
int ControlCounter = 0; //Used to control the state of the Switch function in the Controls menu

int Controls_Reading = LOW;  //the reading of the Controls button
int Start_Reading = LOW;     //the reading of the Start button

//-------Time Values in milliseconds-------
unsigned long Start_Millis = 0;
unsigned long Current_Millis = 0;
unsigned long Start_PressedTime = 0;
unsigned long Controls_PressedTime = 0;

unsigned long Yellow_Millis = 0;  //holders for the current millis time for the red status light
unsigned long Red_Millis = 0;     //holder for the current millis time for the yellow status light

unsigned long Countdown_Timer_Refresh = 0;  //holder for resetting the values of the countdown times during countdown

unsigned long Exposure_Time = 15000;    //Time shutter is open for exposure, msec (sec/1000)
unsigned long Settle_Time = 300000;     //Time after start button is pressed before exposure occurs to allow environment to stabilize, msec (min/60000)
unsigned long Fan_Delay = 60000;        //Time prior to esposure when fan is turned off, msec (sec/1000)
unsigned long Pump_Delay = 20000;       //Time prior to exposure when water cooling pump is turned off, msec (sec/1000)

unsigned long InitExposure_Time = 0;    //Time shutter is open for exposure, msec (sec/1000)
unsigned long InitSettle_Time = 0;      //Time after start button is pressed before exposure occurs to allow environment to stabilize, msec (min/60000)
unsigned long InitFan_Delay = 0;        //Time prior to esposure when fan is turned off, msec (sec/1000)
unsigned long InitPump_Delay = 0;       //Time prior to exposure when water cooling pump is turned off, msec (sec/1000)

unsigned long Exposure_Time_Countdown = 0;    //Time remaining in the exposure countdown for display, msec
unsigned long Settle_Time_Countdown = 0;      //Time remaining for settling for countdown display, msec
unsigned long Exposure_Start_Millis = 0;      //millis value of start time for exposure countdown
unsigned long Settle_Start_Millis = 0;        //millis value of start time for settle countdown

//-------Integers for Min/Sec display------
int Exposure_TimeSec = 0;     //display value for exposure time in seconds
int Settle_TimeMin = 0;       //display value for settling time in minutes
int Settle_TimeSec = 0;       //display value for settling time in seconds
int Fan_DelaySec = 0;         //display value for fan delay time in seconds
int Pump_DelaySec = 0;        //display value for pump delay time in seconds

int Exposure_CountdownSec = 0;    //display value for exposure during countdown in seconds
int Settle_CountdownSec = 0;      //display value for settling time during exposure in seconds



//--------Booleans--------

//define Timers menu, used when Control button is NOT pressed
bool Exposure_Selected = false;   //set to true/false to change the value of menu item
bool Settle_Selected = false;
bool Fan_Selected = false;
bool Pump_Selected = false;
//all selectable inputs start as "not selected"

//define Controls menu, used when Control button IS pressed
bool Lights_Selected = false;
bool Alignment_Selected = false;
bool FanControl_Selected = false;
bool PumpControl_Selected = false;
//all selectable inputs start as "not selected"

//define relay outputs
bool Shutter_on = false;  //initial values at startup
bool Fan_on = true;
bool Pump_on = true;
bool Lights_on = true;
bool Green_on = false;
bool Yellow_on = false;
bool Red_on = false;

//define possible device states
int DeviceState = 0;  //used to control the Switch function within the loop to select multiple device states
// 0 = Timers menu
// 1 = Controls menu
// 2 = Countdown running
// 3 = Post-exposure
bool Countdown_Running = false;
bool Countdown_Finished = false;
bool Countdown_Reset = false;
bool DeviceState_Changed = true;

//define if Start button has been pressed
bool Start_Pressed = false;
int Start_LastState = LOW;
int Start_State = LOW;
unsigned long StartDelay = 1000;    //must push Start button for 1 second to start/cancel the countdown
unsigned long ControlsDelay = 50;   //50 msec for debounce on Controls button
int Controls_LastState = LOW;
int Controls_State = LOW;


//-------Input and Output Pins-------

//Arduino interrupt pins: 2, 3.
//Input pins
const int RotaryCLK = 2;        //CLK pin on the rotary encoder
const int RotaryDT = 4;         //DT pin on the rotary encoder
const int PushButton = 3;       //Button to enter/exit menu
const int StartButton = 5;      //Button to start/reset the timer
const int ControlsButton = 6;   //Button to toggle Controls menu

//Output pins
const int Shutter = 7;          //Output for shutter relay
const int Fan = 8;              //Output for fan relay
const int Pump = 9;             //Output for pump relay
const int Lights = 10;          //Output for overhead lights relay
const int Green = 11;           //Output for green status light relay
const int Yellow = 12;          //Output for yellow status light relay
const int Red = 13;             //Output for red status light relay

//Statuses for the rotary encoder
int CLKNow;
int CLKPrevious;

int DTNow;
int DTPrevious;


bool refreshLCD_Timers = true;          //refreshes values on Timers menu
bool refreshLCD_Controls = false;       //refreshes values on Control menu
bool refreshSelection_Timers = false;   //refreshes selection in the Timers menu   (> / X)
bool refreshSelection_Controls = false; //refreshes selection in the Controls menu (> / X)

void setup() 
{
  //Input pins
  pinMode(2, INPUT_PULLUP); //RotaryCLK
  pinMode(4, INPUT_PULLUP); //RotaryDT
  pinMode(3, INPUT_PULLUP); //Encoder Button
  pinMode(5, INPUT_PULLUP); //Start Button
  pinMode(6, INPUT_PULLUP); //Timers/Controls Toggle Button

  //Output pins
  pinMode(7, OUTPUT);  //Shutter
  pinMode(8, OUTPUT);  //Fan
  pinMode(9, OUTPUT);  //Pump
  pinMode(10, OUTPUT); //Lights
  pinMode(11, OUTPUT); //Status green
  pinMode(12, OUTPUT); //Status yellow
  pinMode(13, OUTPUT); //Status red

  //Set initial states of device------------------------------------
  digitalWrite(Shutter, LOW);       //shutter is closed
  digitalWrite(Fan, HIGH);          //fan is on
  digitalWrite(Pump, HIGH);         //pump is on
  digitalWrite(Lights, HIGH);       //lights are on
  digitalWrite(Green, LOW);         //green status light is off
  digitalWrite(Yellow, LOW);        //yellow status light is off
  digitalWrite(Red, LOW);           //red status light is off

  //Startup the LCD---------------------------------------
  lcd.begin(20,4);                      // initialize the lcd   
  
  //Display the splash screen on the LCD---------------------------------------
  lcd.setCursor(9,0);       //Defining positon to write from first row, 9th column.
  lcd.print("RIKA LABS");   //Print in text
  lcd.setCursor(0,1);       //Second row, first column
  lcd.print("Hologram Controller"); 
  lcd.setCursor(0,2);       //Third row, first column
  lcd.print("Version 1.0"); 

  //Run the startup lights flash routine---------------------
  digitalWrite(Red, HIGH);
  delay(1000);
  digitalWrite(Yellow, HIGH);
  delay(1000);
  digitalWrite(Green, HIGH);
  delay(2000);
  digitalWrite(Red, LOW);
  digitalWrite(Yellow, LOW);
  digitalWrite(Green, LOW);
  //delays used to show startup
  
  lcd.clear();    //clear the whole LCD
  
  //Store states of the rotary encoder
  CLKPrevious = digitalRead(RotaryCLK);
  DTPrevious = digitalRead(RotaryDT);


      
  attachInterrupt(digitalPinToInterrupt(RotaryCLK), rotate, CHANGE); //CLK pin is an interrupt pin
  attachInterrupt(digitalPinToInterrupt(PushButton), PushButtonEnc, FALLING); //PushButton pin is an interrupt pin

}

void PushButton_Timers()  //if the encoder button is pushed while in the Timers menu
{
  switch(TimerCounter)
  {
     case 0:
     Exposure_Selected = !Exposure_Selected;  //change the status of the variable to the opposite
     break;

     case 1:
     Settle_Selected = !Settle_Selected;
     break;

     case 2:
     Fan_Selected = !Fan_Selected;
     break;

     case 3:
     Pump_Selected = !Pump_Selected;
     break;
  } 
  
  refreshLCD_Timers = true; //Refresh LCD after changing the value of the menu
  refreshSelection_Timers = true; //refresh the selection ("X")
}

void PushButton_Controls()  //if the encoder button is pushed while in the Controls menu
{
  switch(ControlCounter)
  {
     case 0:
     Lights_Selected = !Lights_Selected;  //change the status of the variable to the opposite
     break;

     case 1:
     Alignment_Selected = !Alignment_Selected;
     break;

     case 2:
     FanControl_Selected = !FanControl_Selected;
     break;

     case 3:
     PumpControl_Selected = !PumpControl_Selected;
     break;
  } 
  
  refreshLCD_Controls= true; //Refresh LCD after changing the value of the menu
  refreshSelection_Controls = true; //refresh the selection ("X")
}

void rotate()
{  if(DeviceState == 0)       //if the device is in the Timers menu
    {
     rotate_Timers();         //perform the action corresponding to rotating the encoder in the Timers menu
    }
   else if(DeviceState == 1)  //if the device is in the Controls menu
    {
     rotate_Controls();       //perform the action corresponding to rotating the encoder in the Controls menu
    }
}

void PushButtonEnc()             //if the encoder button is pressed
{
  if(DeviceState == 0)        //if the device is in the Timers menu
   {
    PushButton_Timers();      //perform the action corresponding to pushing the encoder button in the Timers menu
   }
  else if(DeviceState == 1)   //if the device is in the Controls menu
   {
    PushButton_Controls();    //perform the action corresponding to pushing the encoder button in the Controls menu
   }
}

void rotate_Timers()  //encoder rotated in Timers menu
{  
  //-----Exposure Time---------------------------------------------------------
  if(Exposure_Selected == true)
  {
  CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then the encoder is rotating in A direction and the value is INCREASED
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      Exposure_Time = (Exposure_Time + 1000);     //increase the exposure time by 1000 msec (1 second)
        if(Exposure_Time > 120000)
        {
          Exposure_Time = 120000;               //if exposure time exceeds 120000 msec (120 sec) reset to 120 sec
        }
    } 
    else //the encoder is rotating in the B direction and the value is DECREASED
     {
      if(Exposure_Time >= 1000)
      {
       Exposure_Time = (Exposure_Time - 1000);    //decrease the exposure time by 1 second
      }
         if(Exposure_Time < 1000)               //if the exposure time is less than 1000 msec and decreased
         {
          Exposure_Time = 1000;                 //reset to 1 second           
         }
      }
   }
  CLKPrevious = CLKNow;  // Store last CLK state
  }
  //---Settling Time---------------------------------------------------------------
  else if(Settle_Selected == true)
  {
    CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then the encoder is rotating in A direction and the value is INCREASED
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(Settle_Time <= 1740000) //do not go above 30 min settle time
      {
        Settle_Time = Settle_Time + 60000;  //increase by 1 min
      }
    } 
    else   //the encoder is rotating in the B direction and the value is DECREASED
    {
      if(Settle_Time > 60000) //do not go below 0 min
      {
        Settle_Time = (Settle_Time - 60000);  //decrease by 1 min
      }
      if(Settle_Time <= 60000)      //if the settle time is less than 1 min and decreased
      {
        Settle_Time = 0;                //set the settle time to zero min    
      }
    }    
  }
  CLKPrevious = CLKNow;  // Store last CLK state
  }
  //---Fan Delay Time---------------------------------------------------------------
  else if(Fan_Selected == true)
  {
    CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(Fan_Delay < 118000) //do not go above 120 seconds for fan delay
      {
        Fan_Delay = (Fan_Delay + 2000);  //increase the fan delay by 2 seconds
      }  
        if(Fan_Delay > 120000)
        {
          Fan_Delay = 120000;  
        }
    } 
    else 
    {
      if(Fan_Delay > 2000) //do not go below 0 sec
      {
        Fan_Delay = (Fan_Delay - 2000);  //decrease the fan delay by 2 seconds
      }
      if(Fan_Delay <= 2000)
      {
        Fan_Delay = 0;    //lowest fan delay can go is zero seconds      
      }
    }    
  }
  CLKPrevious = CLKNow;  // Store last CLK state
  }
  //---Pump Delay Time----------------------------------------------------------------
  else if(Pump_Selected == true)
  {
    CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(Pump_Delay < 118000) //do not go above 120 seconds for pump delay
      {
        Pump_Delay = (Pump_Delay + 2000);  //increase the fan delay by 2 seconds
      }
    } 
    else 
    {
      if(Pump_Delay > 2000) //do not go below 0
      {
       Pump_Delay = (Pump_Delay - 2000);  //decrease the fan delay by 2 seconds    
      }
      if(Pump_Delay <= 2000)
      {
        Pump_Delay = 0;      //lowest pump delay can go is zero seconds
      }
    }    
  }
  CLKPrevious = CLKNow;  // Store last CLK state
  }
  else //TIMER MENU COUNTER----------------------------------------------------------------------------
  {
  CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(TimerCounter < 3) //do not go above 3
      {
        TimerCounter++;  //increment the position downwards
      }
    }
      else
      {
        TimerCounter = 0;  //reset to the top
      }      
    } 
    else 
    {
      if(TimerCounter < 1) //do not go below 0
      {
         TimerCounter = 3;
      }
      else
      {
      // Encoder is rotating CW so increment upwards
         TimerCounter--;      
      }      
    }    
  CLKPrevious = CLKNow;  // Store last CLK state
  }

  //Refresh LCD after changing the counter's value
  refreshLCD_Timers = true;
}

void rotate_Controls()
{  
  //---Lights Control--------------------------------------------------------------
  if(Lights_Selected == true)
 {
  CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(Lights_on == false); //if the lights are not on
      {
        Lights_on = true;  //turn them on
      }
    }
      else
      {
        Lights_on = false;  //otherwise if they were on, turn them off
      }      
  }
  CLKPrevious = CLKNow;  // Store last CLK state
 }  
  
  //---Shutter Control---------------------------------------------------------------
  else if(Alignment_Selected == true)
  {
    CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(Shutter_on == false) //if the shutter is closed
      {
        Shutter_on = true;  //open it
      }
      else
      {
        Shutter_on = false;  //otherwise if it was open, close it
      }      
    }
  }
  CLKPrevious = CLKNow;  // Store last CLK state
  }
  
  //---Fan Control---------------------------------------------------------------
  else if(FanControl_Selected == true)
  {
    CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(Fan_on == false) //if the fan is off
      {
        Fan_on = true;  //turn it on
      }
      else
      {
        Fan_on = false;  //otherwise if the fan was on, turn it off
      }      
    }   
  }
  CLKPrevious = CLKNow;  // Store last CLK state
  }
  
  //---Pump Control----------------------------------------------------------------
  else if(PumpControl_Selected == true)
  {
    CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(Pump_on == false)  //if the pump is off
      {
        Pump_on = true;  //turn it on
      }
      else
      {
        Pump_on = false;  //otherwise if the pump was on, turn it off
      }      
    }    
  }
  CLKPrevious = CLKNow;  // Store last CLK state
  }
  
  else //CONTROL MENU COUNTER----------------------------------------------------------------------------
  {
  CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin
  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1)
  {
    // If the DT state is different than the CLK state then
    // the encoder is rotating in A direction, so we increase
    if (digitalRead(RotaryDT) != CLKNow) 
    {
      if(ControlCounter < 3) //if the control cursor is not below the bottom of the menu
      {
        ControlCounter++;  //increment downwards
      }
      else
      {
        ControlCounter = 0;  //otherwise reset to the top of the menu
      }      
    } 
    else 
    {
      if(ControlCounter < 1) //if the control cursor is above the top of the menu
      {
         ControlCounter = 3; //reset to the bottom of the menu
      }
      else
      {
      // Encoder is rotating CW so increment upwards
         ControlCounter--;
      }      
    }    
  }
  CLKPrevious = CLKNow;  // Store last CLK state
  }

  //Refresh LCD after changing the counter's value
  refreshLCD_Controls= true;
}


void printLCD_Timers()
{
  //Print in menu names when in the Timer menu

  lcd.clear();        //clears the LCD
  
  lcd.setCursor(1,0); //1st line, 2nd block
  lcd.print("Exposure,       sec");
  lcd.setCursor(13,0);
  Exposure_TimeSec = (Exposure_Time / 1000);
  lcd.print(Exposure_TimeSec);

  lcd.setCursor(1,1); //2nd line, 2nd block
  lcd.print("Settling,       min");
  lcd.setCursor(13,1);
  Settle_TimeSec = (Settle_Time / 60000);
  lcd.print(Settle_TimeMin);

  lcd.setCursor(1,2); //3rd line, 2nd block
  lcd.print("Fan Delay,      sec");
  lcd.setCursor(13,2);
  Fan_DelaySec = (Fan_Delay / 1000);
  lcd.print(Fan_DelaySec);

  lcd.setCursor(1,3); //4th line, 2nd block
  lcd.print("Pump Delay,     sec");
  lcd.setCursor(13,3);
  Pump_DelaySec = (Pump_Delay / 1000);
  lcd.print(Pump_DelaySec);
}

void printLCD_Controls()
{
  //print in menu names when in the Controls menu

  lcd.clear();
  
  lcd.setCursor(1,0);
  lcd.print("Lights");
  if(Lights_on == true){
    lcd.setCursor(17,0);  //print the state of the lights
    lcd.print("ON");
  }
  if(Lights_on == false){
    lcd.setCursor(16,0);
    lcd.print("OFF");
  }
  
  lcd.setCursor(1,1);
  lcd.print("Shutter");
  if(Shutter_on == true){
    lcd.setCursor(15,1);  //print the state of the shutter
    lcd.print("OPEN");
  }
  if(Shutter_on == false){
    lcd.setCursor(13,1);
    lcd.print("CLOSED");
  }
  
  lcd.setCursor(1,2);
  lcd.print("Fan");
  if(Fan_on == true){
    lcd.setCursor(17,2);  //print the state of the fan
    lcd.print("ON");
  }
  if(Fan_on == false){
    lcd.setCursor(16,2);
    lcd.print("OFF");
  }

  lcd.setCursor(1,3);
  lcd.print("Pump");
  if(Pump_on == true){
    lcd.setCursor(17,2);  //print the state of the pump
    lcd.print("ON");
  }
  if(Pump_on == false){
    lcd.setCursor(16,2);
    lcd.print("OFF");
  }

}

void updateLCD_Timers()
{ 
  //update the values of the timers in seconds and minutes from the Millis values when in the Timers menu
   
  lcd.setCursor(13,0); //1st line, 10th block
  lcd.print("   "); //erase the content by printing space over it
  lcd.setCursor(13,0); //1st line, 10th block
  Exposure_TimeSec = (Exposure_Time / 1000);  //calculate the exposure time in sec from the Millis value
  lcd.print(Exposure_TimeSec); //display exposure time before "sec"

  lcd.setCursor(13,1);
  lcd.print("    ");
  lcd.setCursor(13,1);
  Settle_TimeMin = (Settle_Time / 60000);  //calculate the settle time in min from the Millis value
  lcd.print(Settle_TimeMin); //display settling time before "min"
  
  lcd.setCursor(13,2);
  lcd.print("    ");
  lcd.setCursor(13,2);
  Fan_DelaySec = (Fan_Delay / 1000);  //calculate the fan delay time in sec from the Millis value
  lcd.print(Fan_DelaySec); //display fan delay time before "sec"

  lcd.setCursor(13,3);
  lcd.print("    ");
  lcd.setCursor(13,3);
  Pump_DelaySec = (Pump_Delay / 1000);  //calculate the pump delay time in sec from the Millis value
  lcd.print(Pump_DelaySec); //display pump delay time before "sec"
}

void updateLCD_Controls()
{ 

if(Lights_on == true){
    lcd.setCursor(17,0);
    lcd.print("ON");
  }
  if(Lights_on == false){
    lcd.setCursor(16,0);
    lcd.print("OFF");
  }

if(Shutter_on == true){
    lcd.setCursor(15,1);
    lcd.print("OPEN");
  }
  if(Shutter_on == false){
    lcd.setCursor(13,1);
    lcd.print("CLOSED");
  }

if(Fan_on == true){
    lcd.setCursor(17,2);
    lcd.print("ON");
  }
  if(Fan_on == false){
    lcd.setCursor(16,2);
    lcd.print("OFF");
  }

if(Pump_on == true){
    lcd.setCursor(17,2);
    lcd.print("ON");
  }
  if(Pump_on == false){
    lcd.setCursor(16,2);
    lcd.print("OFF");
  }
}

void updateCursorPosition_Timers()
{
  //Clear display's ">" parts 
  lcd.setCursor(0,0); //1st line, 1st block
  lcd.print(" "); //erase by printing a space
  lcd.setCursor(0,1);
  lcd.print(" "); 
  lcd.setCursor(0,2);
  lcd.print(" "); 
  lcd.setCursor(0,3);
  lcd.print(" "); 
     
  //Place cursor to the new position
  switch(TimerCounter) //checks the value of the Timer counter (0, 1, 2 or 3)
  {
    case 0:
    lcd.setCursor(0,0); //1st line, 1st block
    lcd.print(">"); 
    break;
    //-------------------------------
    case 1:
    lcd.setCursor(0,1); //2nd line, 1st block
    lcd.print(">"); 
    break;
    //-------------------------------    
    case 2:
    lcd.setCursor(0,2); //3rd line, 1st block
    lcd.print(">"); 
    break;
    //-------------------------------    
    case 3:
    lcd.setCursor(0,3); //4th line, 1st block
    lcd.print(">"); 
    break;
  }
}

void updateCursorPosition_Controls()
{
  //Clear display's ">" parts 
  lcd.setCursor(0,0); //1st line, 1st block
  lcd.print(" "); //erase by printing a space
  lcd.setCursor(0,1);
  lcd.print(" "); 
  lcd.setCursor(0,2);
  lcd.print(" "); 
  lcd.setCursor(0,3);
  lcd.print(" "); 
     
  //Place cursor to the new position
  switch(ControlCounter) //checks the value of the Control counter (0, 1, 2 or 3)
  {
    case 0:
    lcd.setCursor(0,0); //1st line, 1st block
    lcd.print(">"); 
    break;

    case 1:
    lcd.setCursor(0,1); //2nd line, 1st block
    lcd.print(">"); 
    break;
  
    case 2:
    lcd.setCursor(0,2); //3rd line, 1st block
    lcd.print(">"); 
    break;
  
    case 3:
    lcd.setCursor(0,3); //4th line, 1st block
    lcd.print(">"); 
    break;
  }
}

void updateSelection_Timers()
{
  //When a menu is selected ">" becomes "X"

  if(Exposure_Selected == true)
  {
    lcd.setCursor(0,0); //1st line, 1st block
    lcd.print("X"); 
  }

   if(Settle_Selected == true)
  {
    lcd.setCursor(0,1); //2nd line, 1st block
    lcd.print("X"); 
  }

  if(Fan_Selected == true)
  {
    lcd.setCursor(0,2); //3rd line, 1st block
    lcd.print("X"); 
  }

  if(Pump_Selected == true)
  {
    lcd.setCursor(0,3); //4th line, 1st block
    lcd.print("X"); 
  }
}

void updateSelection_Controls()
{
  //When a menu is selected ">" becomes "X"

  if(Lights_Selected == true)
  {
    lcd.setCursor(0,0); //1st line, 1st block
    lcd.print("X"); 
  }

   if(Alignment_Selected == true)
  {
    lcd.setCursor(0,1); //2nd line, 1st block
    lcd.print("X"); 
  }

  if(FanControl_Selected == true)
  {
    lcd.setCursor(0,2); //3rd line, 1st block
    lcd.print("X"); 
  }

  if(PumpControl_Selected == true)
  {
    lcd.setCursor(0,3); //4th line, 1st block
    lcd.print("X"); 
  }
}

void printLCD_Countdown()
{
  lcd.clear();  //clear the whole LCD
  lcd.setCursor(2,0);
  lcd.print("Countdown Active");  //display the Countdown Active text at the top
  lcd.setCursor(0,1);
  lcd.print("Exposure Time");  //print in the exposure timer
  lcd.setCursor(17,1);
  lcd.print("sec");  //print in the units for exposure time (sec)
  lcd.setCursor(0,2);
  lcd.print("Settle Time");  //print in the settle timer text
  lcd.setCursor(17,2);
  lcd.print("sec");  //print in the units for the settle time (sec)
  lcd.setCursor(1,3);
  lcd.print("Fan:");  //print in the text for the fan
  lcd.setCursor(12,3);
  lcd.print("Pump:");  //print in the text for the pump
}

void printCountdownTimers()
{
  if(Exposure_Start_Millis == 0)  //if this is the first cycle of the countdown timers
   {
    Exposure_CountdownSec = Exposure_TimeSec;  //print the initial value of the exposure time
    Settle_CountdownSec = (Settle_Time / 60000);  //print the initial value of the settle time
   }
  if(((Start_Millis + Settle_Time) <= Current_Millis) && (Current_Millis < (Start_Millis + Settle_Time + Exposure_Time)))  //if an exposure is being performed
   {
    Exposure_CountdownSec = ((Exposure_Time - (millis() - Exposure_Start_Millis)) / 1000);  //calculate the remaining exposure time in seconds
    lcd.setCursor(14,1);
    lcd.print("   ");  //clear the space
    lcd.setCursor(14,1);
    if(Exposure_CountdownSec >=0)
     {
      lcd.print(Exposure_CountdownSec);  //print the value of the exposure countdown in sec
     }
    else
     {
      lcd.print("0");  //unless the exposure countdown has reached zero
     }
   }
  if((Start_Millis <= Current_Millis) && (Current_Millis < (Start_Millis + Settle_Time + Exposure_Time)))  //if the device is in the settle time
  {
    Settle_CountdownSec = ((Settle_Time - (millis() - Start_Millis)) / 1000);  //calculate the remaining settle time in seconds
    lcd.setCursor(12,2);
    lcd.print("    ");  //clear the space
    lcd.setCursor(12,2);
    if(Settle_CountdownSec >= 0)
     {
      lcd.print(Settle_CountdownSec);   //print the value of the settle countdown in sec
     }
    else
     {
      lcd.print("0");  //unless the settle countdown has reached zero
     }
  }
  
     //print the values of the pump and fan
  if(Fan_on == true)
  {
    lcd.setCursor(6,3);
    lcd.print("ON");
  }
  if(Fan_on == false)
  {
    lcd.setCursor(6,3);
    lcd.print("OFF");
  }
  if(Pump_on == true)
  {
    lcd.setCursor(16,3);
    lcd.print("ON");
  }
  if(Pump_on == false)
  {
    lcd.setCursor(16,3);
    lcd.print("OFF");    
  }
}

void printLCD_PostExposure()
{
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("Success!");
  lcd.setCursor(1,1);
  lcd.print("Exposure Complete!");
  lcd.setCursor(0,2);
  lcd.print("Exposure Time");
  lcd.setCursor(14,2);
  lcd.print(Exposure_TimeSec);
  lcd.setCursor(17,2);
  lcd.print("sec");
  lcd.setCursor(0,3);
  lcd.print("Press START to Reset");
}


void loop() 
{ 
  
//set states of the outputs
  if(Shutter_on == true)
  {
    digitalWrite(Shutter, HIGH);
  }
    else if(Shutter_on == false)
    {
      digitalWrite(Shutter, LOW);
    }
  if(Fan_on == true)
  {
    digitalWrite(Fan, HIGH);
  }
    else if(Fan_on == false)
    {
      digitalWrite(Fan, LOW);
    }
  if(Pump_on == true)
  {
    digitalWrite(Pump, HIGH);
  }
    else if(Pump_on == false)
    {
      digitalWrite(Pump, LOW);
    }
  if(Lights_on == true)
  {
    digitalWrite(Lights, HIGH);
  }
    else if(Lights_on == false)
    {
      digitalWrite(Lights, LOW);
    }
  if(Green_on == true)
  {
    digitalWrite(Green, HIGH);
  }
    else if(Green_on == false)
    {
      digitalWrite(Green, LOW);
    }
  if(Yellow_on == true)
  {
    digitalWrite(Yellow, HIGH);
  }
    else if(Yellow_on == false)
    {
      digitalWrite(Yellow, LOW);
    }
  if(Red_on == true)
  {
    digitalWrite(Red, HIGH);
  }
    else if(Red_on == false)
    {
      digitalWrite(Red, LOW);
    }


//detect pressing of the Start/Cancel button, set start state
   Start_Reading = digitalRead(StartButton);  //read the Start button pin
   if(Start_Reading != Start_LastState)  //if the input value has changed
   {  
    Start_PressedTime = millis();  //record the millis value of the press time
   }
   if((millis() - StartDelay) > StartDelay)  //if the button has been pushed long enough
   {
    if(Start_Reading != Start_State)  //and if the previous reading is not the same as the current reading
    {
      Start_State = Start_Reading;  //set the button state to the current reading
      if(Start_State == LOW && Start_LastState == HIGH)  //if the button went from HIGH to LOW
      {
        //if the button is released do nothing
      }
      else if(Start_State == HIGH && Countdown_Running == false)  //if the button is pressed and the countdown is not running
      {
        if(Countdown_Finished == false)
         {
          Start_Pressed = true;  //start the countdown
          DeviceState = 2;
         }
      }
      else if(Start_State == HIGH && Countdown_Running == true)  //reset to the timers menu if the countdown is running
      {
       DeviceState = 0;           //return to the Timers menu
       Countdown_Reset = false;    //reset device states and timers
       Countdown_Running = false;
       Start_Millis = 0;
       Yellow_on = false;
       Red_on = false;
       Exposure_Start_Millis = 0;
       Settle_Start_Millis = 0;
       Countdown_Timer_Refresh = 0;
       refreshLCD_Timers = true;
       DeviceState_Changed = true;
      }
      else if(Start_State == HIGH && Countdown_Finished == true)  //reset to the initial state if the countdown has finished
      {
       Start_Pressed = false;
       Countdown_Reset = true;
      }
      Start_LastState = Start_State;  //reset the state of the Start trigger
    }
   } 

//detect if Controls button is pressed to toggle between the timers and controls menu
   if(DeviceState == 0 || DeviceState == 1)  //if the device is in the timers or controls menu
 {
   Controls_Reading = digitalRead(ControlsButton);  //read the Start button pin
   if(Controls_Reading != Controls_LastState)  //if the input value has changed
   {  
    Controls_PressedTime = millis();  //record the millis value of the press time
   }
   if((millis() - ControlsDelay) > ControlsDelay)  //if the controls button has been pushed longer than the delay
   {
    if(Controls_Reading != Controls_State)  //and if the controls reading does not match the previous reading
    {
      Controls_State = Controls_Reading;  //set the controls state to the reading state
      if(Controls_State == LOW && Controls_LastState == HIGH)  //if the controls state went from LOW to HIGH the button was released
      {
        //if the button is released do nothing
      }
       else if(Controls_State == HIGH && DeviceState == 0)
      {
        DeviceState = 1;  //set the device to the Timers menu
        DeviceState_Changed = true;  //and log that the device state was changed
      }
       else if(Controls_State == HIGH && DeviceState == 1)
      {
         DeviceState = 0;  //set the device to the Controls menu
         DeviceState_Changed = true;  //and log that the device state was changed
      }
      Controls_LastState = Controls_State;  //reset the last controls state to the current state
      }
    }
   }
     
  switch (DeviceState)  //controls which device state is currently active
 {

  case 0:  //Timers menu

    if(Start_Pressed == true)  //if the "start countdown" button is pressed
     {
      DeviceState = 2;  // start the countdown state
     }
    else{
    if(DeviceState_Changed == true)  //if the device state has been changed (from controls to timers)
    {
      printLCD_Timers();  //print the timers menu
      DeviceState_Changed == false;  //reset the device state changed status to reflect the new menu choice
    }
    if(refreshLCD_Timers == true) //If the timer menu needs to be refreshed
    {
      updateLCD_Timers(); //Run the update timers protocol to update VALUES
  
         //if one of the Timers menus variables is selected for editing
      if(Exposure_Selected == true || Settle_Selected == true || Fan_Selected == true || Pump_Selected == true)
      {
       // do nothing
      }
      else
      {
        updateCursorPosition_Timers(); //update the position of the cursor in the timers menu
      }
      
      refreshLCD_Timers = false; //reset the variable - wait for a new trigger
    }
  
    if(refreshSelection_Timers == true) //if the selection is changed
     {
      updateSelection_Timers(); //update the selection on the LCD
      refreshSelection_Timers = false; // reset the variable - wait for a new trigger
     }
    } 
    break;
 

  case 1:  //Controls menu 

    if(Start_Pressed == true)  //start countdown as above
    {
      DeviceState = 2;
    }
    else{
    if(DeviceState_Changed == true)
    {
      printLCD_Controls();  //if device state has changed from timers to controls, print the controls menu
      DeviceState_Changed = false;  
    }
    
    if(refreshLCD_Controls == true) //If the timer menu needs to be refreshed
    {
      updateLCD_Controls(); //Run the update timers protocol
  
         //if one of the Timers menus variables is selected for editing
      if(Exposure_Selected == true || Settle_Selected == true || Fan_Selected == true || Pump_Selected == true)
      {
       // do nothing
      }
      else  //if the device is editing a particular control
      {
        updateCursorPosition_Controls(); //update the position
      }
      
      refreshLCD_Controls= false; //reset the variable - wait for a new trigger
    }
  
    if(refreshSelection_Controls == true) //if the selection is changed
     {
      updateSelection_Controls(); //update the selection on the LCD
      refreshSelection_Controls = false; // reset the variable - wait for a new trigger
     }
    } 
    break;

  case 2:  //Countdown Active

  if(Start_Pressed == true)
  {
    Start_Pressed = false;
    Countdown_Running = true;
  }
  if(Start_Millis == 0)  //if the countdown has just been started
  {
    printLCD_Countdown();  //refresh the LCD with the countdown 
    Start_Millis = millis();  //record the start time
    Yellow_on = true;  //turn on the yellow light to show start of countdown
    Settle_Start_Millis = millis();  //record the start of the settle time
  }
  else
  {
    Current_Millis = millis();  //record the current time
    
    if(Current_Millis >= (Countdown_Timer_Refresh + 500))  //if more than 0.5 sec has elapsed since the last update of the timer values
    {
      printCountdownTimers();  //print in the current timer values
      Countdown_Timer_Refresh = Current_Millis;  //and reset the refresh timer
    }
    
    if(Shutter_on == true)  //if the exposure has started
      {
            if(Exposure_Start_Millis == 0)  //and the exposure timer has not been started
            {
              Exposure_Start_Millis = millis();  //record the millis time of the start of exposure
            }
      }
      
    if(((Start_Millis + (Settle_Time - 15000)) <= Current_Millis) && ((Start_Millis + Settle_Time) >= Current_Millis)) //if the countdown is in the final 15 seconds of settle time prior to exposure
    {
      if(Current_Millis > (Yellow_Millis + 250))  //toggle the yellow light on and off for 250 msec each (fast blink) to indicate final countdown
      {
        Yellow_on = !Yellow_on;
        Yellow_Millis = millis();
      }
    }
    
    if(((Start_Millis + (Settle_Time - 15000)) > Current_Millis) && ((Start_Millis + Settle_Time) > Current_Millis))  //if the countdown is in the settle time before the final 15 seconds
    {
    if(Current_Millis > (Yellow_Millis +1000))  //toggle the yellow light on and off for 1000 msec each (slow blink) to indicate countdown running
     {
        Yellow_on = !Yellow_on;
        Yellow_Millis = millis();
     }
    }
    
    if((Start_Millis + Settle_Time) <= Current_Millis) //if the current time is after the completion of the settle time
    {
      if((Start_Millis + Settle_Time + Exposure_Time) <= Current_Millis)  //if the exposure is complete, proceed to post-exposure state
      Shutter_on = false;  //close the shutter
      Fan_on = true;  //turn the fan on
      Pump_on = true;  //turn the pump on
      Yellow_on = false;  //turn off the yellow light if it is on
      Red_on = false;  //turn off the red light if it is on
      Start_Millis = 0;  //reset the start time to zero to prepare for the next exposure
      Countdown_Finished = true;  //set the countdown finished state to true
      DeviceState = 3;  //and set the device to the post-exposure state.
    }
    
    else if(((Start_Millis + Settle_Time) <= Current_Millis) && (Current_Millis < (Start_Millis + Settle_Time + Exposure_Time)))  //if the settle time has elapsed and it is time to perform the exposure
      {
        Shutter_on = true;  //open the shutter
        Fan_on = false;  //make sure fan is off
        Pump_on = false;  //make sure pump is off
        Red_on = true;  //start with the red light on
        Yellow_on = false; //turn off the yellow light if it is on
        if(Current_Millis >= (Red_Millis + 500))  //toggle the red light on and off for 500 msec each to indicate exposure in progress
        {
          Red_on = !Red_on;
          Red_Millis = millis();
        }
      }
      
    else if((Start_Millis <= Current_Millis) && (Current_Millis < (Start_Millis + Settle_Time + Exposure_Time)))  //if the current time is during the settle time
      {
        Shutter_on = false;  //make sure the shutter is closed
        if(Current_Millis >= ((Start_Millis + Settle_Time) - Fan_Delay))  //if the current time is past the start of the fan delay countdown time
        {
         Fan_on = false;  //turn off the fan
        }
        if(Current_Millis >= ((Start_Millis + Settle_Time) - Pump_Delay))  //if the current time is past the start of the pump delay countdown time
        {
         Pump_on = false;  //turn off the pump
        }
      }
  }
  break;

  case 3:  //Post-exposure state

  if(Countdown_Reset == false)  //if the device has not been reset yet
  {
   if(Countdown_Finished == true)  //if the countdown has just been completed
   {
    Countdown_Running = false;          //the countdown is no longer running
    Yellow_on = false;                  //turn off the yellow light if it is on
    Red_on = false;                     //turn off the red light if it is on
    Green_on = true;                    //turn on the green light to indicate exposure completed
    printLCD_PostExposure();            //print the information about the exposure just completed
    Countdown_Finished = false;         //set the trigger so that printing is not repeated
    Exposure_Time = InitExposure_Time;  //reset the exposure time to the initial value
    Settle_Time = InitSettle_Time;      //reset the settle time to the initial value
    Fan_on = true;                      //turn on the pump
    Pump_on = true;                     //turn on the fan
    Start_Millis = 0;                   //reset so the initial state is triggered when a new countdown is started
    Exposure_Start_Millis = 0;          //reset the exposure start time
    Settle_Start_Millis = 0;            //reset the settle start time
    Countdown_Timer_Refresh = 0;        //reset the counter refresh timer
    }
   }
  if(Countdown_Reset == true)  //if the countdown is reset
   {
    DeviceState = 0;             //return to the timers menu
    Yellow_on = false;  
    Red_on = false;  
    Green_on = false;             //turn off all the lights if they are on
    Countdown_Finished = false;   //set the countdown finished trigger to the initial state
    Countdown_Reset = false;      //set the countdown reset trigger to the initial state
    refreshLCD_Timers = true;     //print the timers menu
    DeviceState_Changed = true;   //set the state change trigger to true
   }
   break;
 }
 
}
