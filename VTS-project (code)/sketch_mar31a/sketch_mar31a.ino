#include <AsyncDelay.h>
#include <LiquidCrystal.h>

#define SW_PIN                  A3  // define the Buttons Pin

#define LDR_PIN                 A6  // define the Ldr Pin

#define ULTRASONIC_ECHO_PIN     8   // define the Echo Pin of UltraSonic
#define ULTRASONIC_TRIG_PIN     7   // define the Trigger Pin of UltraSonic

#define GREEN_LED_PIN           A0  // define the Green Led Pin
#define YELLOW_LED_PIN          A1  // define the Yellow Led Pin

// define the Red Leds Pins
#define RED_LED1_PIN            13  
#define RED_LED2_PIN            10
#define RED_LED3_PIN            9

#define BUZ_PIN                 6 // define the Buzzer Pin

/*****************************************
 * Definitions Used in Program functions *
 *****************************************/
#define LIGHT_ENOUGH_TO_READ    (600UL) // define the Light that is enough to read a Book 

#define PERIOD_NEEDED           (60UL)  // define the time value, if (1UL) means time in Seconds
                                        // if (60UL) means time in Minutes
/*****************************************
 * LCD Configurations                    *
 *****************************************/
#define RS_PIN                12
#define EN_PIN                11
#define D4_PIN                5
#define D5_PIN                4
#define D6_PIN                3
#define D7_PIN                2

LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

/*******************************************
 * Async Delay Configuration               *
 *******************************************/
AsyncDelay time_Left;  // Async Time for the period that begins based on Any Event
AsyncDelay every_second; // Async Time for each second in a triggered event

// Async Timers for the Switches
AsyncDelay Wait_On_Switch; // wait on the single read
AsyncDelay SW_CorrectingMode; // for correcting the read
AsyncDelay SW_Checklongpress; // checking if the comming read is Long One or Not

AsyncDelay UltraSonicTriger_timeON; // this time is for triggering  the UltraSonic 

AsyncDelay flash_time; // time for the flash
AsyncDelay BuzOn_time; // specific for the Buzzer


// Periods for the Single Press on switch 1 or 2
unsigned long long Time_Periods[4] = {2, 10, 15, 30};
/**************************************************************
 *  Set_Period : for setting the period needed for an event   *
 *                                                            *
 * current_time : Calculating the Time in Seconds during a    *                                                    
 *                trigger of a specific event                 *
 **************************************************************/
unsigned long long int Set_Period = 0, current_time = 0;

/**************************************************************
 *  triggerd : flag is specific for checking if the UltraSonic*   
 *             is triggered and ready for a new read          *
 *                                                            *
 *  sw_wait_flag : flag for checking for a comming Long Press *
 *                 On Button 1                                *
 *                                                            *
 *  Operation_Code : coding the current running event         *
 *                                                            *
 *  Sw1_LongPress_flag : inform the function LDR_Value() that *                                                         
 *                       a new comming Long Press on Button 1 *
 *                       has happened                         *
 *                                                            *
 * Ultra_Flag : this flag informs the Check_Object_Position() *                     
 *              function that an UltraSonic Event is already  *
 *              running and can't trigger a new event till the*
 *              current one is finished                       *
 *                                                            *  
 **************************************************************/
unsigned char triggerd, sw_wait_flag, Operation_Code, Sw1_LongPress_flag, Ultra_Flag;
/**************************************************************
 *  GreenLed_State : is detecting the state of the Green Led  *
 *                    based on the current LDR value and the  *
 *                    current state of the Sw1_LongPress_flag *
 *                                                            *
 **************************************************************/
unsigned int GreenLed_State = 0;

void setup() {
  Serial.begin(57600); // Set the baud rate value to 57600
  lcd.begin(16, 2);
  lcd.print("Program Started");
  lcd.setCursor(0,1);
  lcd.print("Make an Event");
  
  /*************************************************************
   *                    Pin Configurations                     *
   *************************************************************/
  pinMode(SW_PIN, INPUT); // switch pin is input
  pinMode(LDR_PIN, INPUT); // LDR pin is input

  pinMode(ULTRASONIC_ECHO_PIN, INPUT); // UltraSonic ECHO pin is input
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT); // UltraSonic Trigger pin is input

  pinMode(BUZ_PIN, OUTPUT); // Buzzer Pin is OUTPUT

  // Led Pins are output
  pinMode(YELLOW_LED_PIN, OUTPUT); 
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED1_PIN, OUTPUT);
  pinMode(RED_LED2_PIN, OUTPUT);
  pinMode(RED_LED3_PIN, OUTPUT);

  /*************************************************************
   * Asynchronous Delay Set the delay Value                    *
   *************************************************************/
  time_Left.start(Set_Period, AsyncDelay::MILLIS);    // Initialize the Time for each event as 0
  every_second.start(1000, AsyncDelay::MILLIS);       // Initialize and Set the second Value as 1000 ms 
  Wait_On_Switch.start(50, AsyncDelay::MILLIS);       // Initialize the time for waiting on a read as 50 ms
  SW_CorrectingMode.start(2000, AsyncDelay::MILLIS);  // Initialize the time for Correcting the read as 2s
  SW_Checklongpress.start(1000, AsyncDelay::MILLIS);  // Initailize the time for checking the Long as 2s
  flash_time.start(50, AsyncDelay::MILLIS);           // Initailize the time for the flash as 50 ms 
  BuzOn_time.start(500, AsyncDelay::MILLIS);          // Initialize the time for the Buzzer as 500 ms
  UltraSonicTriger_timeON.start(10, AsyncDelay::MICROS);  // Initialize the time for the UltraSonic Trigger As 10 Microsecond


  /*************************************************************
   * Initialize the State of Buzzer                            *
   *************************************************************/
  digitalWrite(BUZ_PIN, HIGH); 
}

void loop() {
  Trig_UltraSonic();
  if (triggerd)
    Check_Object_Position();

  Check_Lights();
  
  Check_Pressed_Switch();

  Trace_Time_Left();

  trun_BuzOff();
}

/**************************************************************************
 * Function Name :  Check_Lights                                          *
 * Parameters    :  Void                                                  *
 * Return Type   :  void                                                  *
 * Functionality :  Checks The LDR value and Controls the Green Led State *
 *                  if the value of LDR is Not enough it Clear the Green  *
 *                  Led State, Otherwise it's Set.                        *
 *                  if a long Press has happened it Invert this Operation *
 **************************************************************************/
void Check_Lights(void) {
  if (Sw1_LongPress_flag) { // if long press is happened then we toggle the state of yellow led in next time
    GreenLed_State ^= 1;
    Sw1_LongPress_flag = 0;
  }
  if (LDR_Value() < LIGHT_ENOUGH_TO_READ) {
    digitalWrite(GREEN_LED_PIN, GreenLed_State);
  }
  else {
    digitalWrite(GREEN_LED_PIN, GreenLed_State ^ 0x01);
  }
}

/**************************************************************************
 * Function Name :  Check_Object_Position                                 *
 * Parameters    :  void                                                  *
 * Return Type   :  void                                                  *
 * Functionality :  Controls the event based on the UltraSonic, E.G. if   *
 *                  an object is a 10-cm close it starts an event but it  *                                                    
 *                  won't start a new UltraSonic event till the current   *
 *                  is Finished or A new event is triggered from Buttons  *
 **************************************************************************/
void Check_Object_Position(void) {
  static unsigned long long Last_Period_changed = 0;
  Last_Period_changed = Set_Period; 
  if (Distance_Measured() <= 10 && Ultra_Flag == 0) { 
    Set_Period = 10 * 1000 * PERIOD_NEEDED; // set the time of the ultrasonic event to 10 mins
    Ultra_Flag = 1; // Set the flag to not restart the period for the same Time
                    // that the object is close Until it's finished at least one time
    Last_Period_changed = Set_Period; // set the last period to the current one 
    time_Left.expire(); // expire the current running event to start the ultra sonic event
    Operation_Code = 8; // encode the operation 
    digitalWrite(YELLOW_LED_PIN, HIGH); // set the yellow led
    { // Serial Lines
      Serial_LineSeparator();
      Serial.print("New Time Period Started for ULTRASONIC : "); Serial.print((unsigned int)(Set_Period / 60000)); Serial.println("mins");
      Serial_LineSeparator();
    }
  }
  else if ((Last_Period_changed != 600000 && Ultra_Flag == 1) || time_Left.isExpired()) { // Period Changed Due To another Event
    Ultra_Flag = 0; /* return to Waiting Mode for new UltraSonic Event
                     this flag means that Ultrasonic is no longer Busy with the Last Event Happend
                     and Waiting for another Event to start A new 10 mins Period on setting the
                     Yellow Led */
  }
}

/***************************************************************************
 * Function Name : Trace_Time_Left                                         *
 * Parameters    : void                                                    *
 * Return Type   : void                                                    *
 * Functionality : this function is Specific for calculating the time for  *
 *                 a running event                                         *
 *                 - if an event is running it calculates the time and     *                                             
 *                   trace it on the three led descendingly till the end of* 
 *                   the time detected for this event based on PWM         *   
 *                 - if an event cuts a running event the 3 leds are set   * 
 *                   for a new period and starts to calculate              *
 *                 - if a time is finished for a running event it set the  *
 *                   period to 0 and wait for a new one based on a trigger *
 ***************************************************************************/
void Trace_Time_Left(void) {
  static int LED_Flag = 0;
  static float Interval = 0, Led1_Light = 255, Led2_Light = 255, Led3_Light = 255, prev_Operation = 0;
  // if the operation is the same and the event time is not finished yet
  if (Operation_Code == prev_Operation && !time_Left.isExpired()) {
    Interval = (765.0 / (Set_Period / (1000.0))); // Devide the Current Running Time to 255 * 3
                                                  // that means get the the Value needed in each
                                                  // second to represent the left time on the 3 leds with PWM
    if (every_second.isExpired()) { // every second do the following
        current_time++;  // increase the timer every second to match the current event time passed 
        
        lcd.clear();
        lcd.setCursor(0, 0); // print the time left to end the current event (in seconds) on LCD
        lcd.print("Time Left: " + String((unsigned int)(Set_Period/1000 - current_time)));
        LCD_printOperation(); // print current operation on lcd
      { // Serial Lines
        Serial_LineSeparator();
        Serial.print("New Time Period Started for  : ");
        Serial.print((unsigned int)(Set_Period / 60000));
        Serial.println(" mins");
        Serial.print("Time Now == "); Serial.println(current_time / 60.0);
        Serial.print("Interval == "); Serial.println(Interval);
        Serial.print("Led 1 Value == "); Serial.println(Led1_Light);
        Serial.print("Led 2 Value == "); Serial.println(Led2_Light);
        Serial.print("Led 3 Value == "); Serial.println(Led3_Light);
        Serial_LineSeparator();
      }

      if (Led1_Light > 0 && LED_Flag == 0) { // stay decreasing the first red led untill it reaches 0 
        Led1_Light -= Interval; 
      }
      else if (Led1_Light <= 0 && LED_Flag == 0) { // if it reached 0 then rise the flag for the led 2 to start
        LED_Flag += 1;                             // decreasing its Lightness too
        Led1_Light = 0;                            
      }

      if (Led2_Light > 0 && LED_Flag == 1) {  // stay decreasing the second red led untill it reaches 0
        Led2_Light -= Interval;              
      }
      else if (Led2_Light <= 0 && LED_Flag == 1) { // if it reached 0 then rise the flag for the led 3 to start
        LED_Flag += 1;
        Led2_Light = 0;
      }

      if (Led3_Light > 0 && LED_Flag == 2) {   // stay decreasing the third red led untill it reaches 0
        Led3_Light -= Interval;
      }
      else if (Led3_Light <= 0 && LED_Flag == 2) { // if it reached 0 then rise the flag for the led 0 to ready
        LED_Flag = 0;                              // for e new comming event
        Led3_Light = 0;
      }
      analogWrite(RED_LED1_PIN, ((int)Led1_Light <= 0 ? 0 : (int)Led1_Light));
      analogWrite(RED_LED2_PIN, ((int)Led2_Light <= 0 ? 0 : (int)Led2_Light));
      analogWrite(RED_LED3_PIN, ((int)Led3_Light <= 0 ? 0 : (int)Led3_Light));
      every_second.repeat();
    }
  }
  // if another operation (Event) cuts the running operation but not 3(double Press on B1)
  // as it ends the timer and Clear the yellow led
  // operations 1, 3, 7 don't contain interval to start
  else if (time_Left.isExpired() && Operation_Code != 3 && Operation_Code != 0 && Operation_Code != 7 && Operation_Code != 1) {
    LED_Flag = 0; // force the time tracer to return back to the First RED Led
    
    current_time = 0; // reset the current value of time rerached (start counting from 0 again)
    every_second.expire();  // expire the current second
    time_Left.start(Set_Period, AsyncDelay::MILLIS); // Start A new Time
    
    digitalWrite(BUZ_PIN, HIGH); // turn on the buzzer for 300 ms 
    BuzOn_time.start(300, AsyncDelay::MILLIS);
    
    Led3_Light = 255, Led2_Light = 255, Led1_Light = 255; // Set all RED Leds
    analogWrite(RED_LED1_PIN, Led1_Light);
    analogWrite(RED_LED2_PIN, Led2_Light);
    analogWrite(RED_LED3_PIN, Led3_Light);
  }
  // the current event completes its detected time and wasn't cut by another event
  else if (prev_Operation == Operation_Code && time_Left.isExpired() && Operation_Code != 0) {

    LED_Flag = 0; // force the time tracer to return back to the First RED Led
    
    current_time = 0; // reset the current value of time rerached (start counting from 0 again)
    every_second.expire(); // end the current running second
    Set_Period = 0;        // No time needed as the event completed and no new event happened yet

    Operation_Code = 0;
    LCD_printOperation(); // print on Lcd
    
    // turn on the buzzer for 300 ms
    digitalWrite(BUZ_PIN, HIGH);
    BuzOn_time.start(300, AsyncDelay::MILLIS);
    
    digitalWrite(YELLOW_LED_PIN, LOW); // OFF the Yellow Led after the Setting Period Expires
    
    Led3_Light = 0, Led2_Light = 0, Led1_Light = 0; // off the trace leds and wait another period event
    analogWrite(RED_LED1_PIN, Led1_Light);
    analogWrite(RED_LED2_PIN, Led2_Light);
    analogWrite(RED_LED3_PIN, Led3_Light);
  }

  prev_Operation = Operation_Code; // the last Interval Value Without Change
}

/****************************************************************************
 * Function Name : Check_Pressed_Switch                                     *
 * Parameters    : void                                                     *
 * Return Type   : void                                                     *
 * Functionality : this Function is used to do the Operation of the buttons *
 *                 based on the current detected presses                    *
 ****************************************************************************/
void Check_Pressed_Switch(void) {
  int Switch_State = 0; // stores the state of the pressed switch
  
  // Index_Forward : access the array of Times for a single Press on Button 1
  // Index_Backward : access the array of Times for a single Press on Button 2
  // sw1_count_presses : stores how many times Button 1 was Pressed
  // sw2_count_presses : stores How many times Button 2 was Pressed
  static int Index_Forward = 0, Index_Backward = 3, sw1_count_presses = 0, sw2_count_presses = 0, SwBoth = 0; 
  
  
  if(!SW_CorrectingMode.isExpired()) { // 2s to get a new Button Event
      while(!Wait_On_Switch.isExpired()); // wait 50 ms to take a correct single press
      SW_Checklongpress.restart();       // restart the checking time for a comming long press 1s
      while(Sw_Value()){  // wait on the single press untill it is released
        Serial.println("SW : " + String(Sw_Value()));
        Switch_State = Sw_Value(); // get the state of the press
        while(!Wait_On_Switch.isExpired()); // wait 50 ms to get a correct read
        if(SW_Checklongpress.isExpired() && Switch_State == 1){ // if time of checking Long press was finished
          sw_wait_flag = 1;                       // and the state was 1 (B1), then rise a flag of long press 
          sw1_count_presses = 5;  // increase the times that B1 pressed to more than 3 presses
        }
        Wait_On_Switch.repeat(); 
      }
      SW_Checklongpress.expire(); // expire the wait time for the long press
      if (Switch_State == 1 && sw_wait_flag == 0) sw1_count_presses++; // if was no long press then increase (B1) presses by 1
      else if (Switch_State == 2) sw2_count_presses++; // if (B2) Pressed Increase presses by 1
      else if(Switch_State == 3) {
        SwBoth = 1;// rise the Both flag
                               /************************************************************************************/
        sw1_count_presses = 0; /* make A periority for the both as switch state can't be 3                         */
        sw2_count_presses = 0; /* unless the both are pressed together .. to remove the time between pressing      */
                               /* the both in the same time we clear the single or multople presses happend        */
                               /************************************************************************************/
      }
      Wait_On_Switch.repeat();
  }
  Wait_On_Switch.repeat();
  while(!Wait_On_Switch.isExpired()); 
   // if the time For taking the Correct Number of Presses was finished and a real press happened
   // then chack the Event must be taken Based on the n Presses 
  if (SW_CorrectingMode.isExpired() && (sw1_count_presses || sw2_count_presses || SwBoth)) {

    // if the press wasn't long then expire the current event
    // to start a new Button-triggered Event
    if(!sw_wait_flag){
      time_Left.expire();
      Set_Period = 0;
    }
    
    // if a press happened on Both Buttons
    if (SwBoth == 1) { 
      { // Serial Lines
        Serial.println("BOTH  BOTTUNS ARE PRESSED ########");
      }
      int flash_counts, Led_Value = 0; // flash the yellow led led
      for (flash_counts = 0; flash_counts < 12; ) {
        if (flash_time.isExpired()) {
          digitalWrite(YELLOW_LED_PIN, Led_Value);
          Led_Value ^= 0x00000001;
          flash_counts++;
          flash_time.restart();
        }
      }
      digitalWrite(YELLOW_LED_PIN, LOW);
      Operation_Code = 7; // code of the operation
    }
    else if (sw1_count_presses > 3) { // Long Press on Button 1
      { // Serial Lines
        Serial_LineSeparator();
        Serial.println("Loooong Press On Button (((((( 1 )))))))");
        Serial_LineSeparator();
      }
      Sw1_LongPress_flag = 1; // rise the flag of long press for the LDR Function to Invert its Control On the Green Led
      Operation_Code = 1; // code of the operation
    }
    else if (sw1_count_presses == 1) { // single press on Button 1
      { // Serial Lines
        Serial_LineSeparator();
        Serial.println("SINGLE Press On Button (((((( 1 )))))))");
        Serial_LineSeparator();
      }
      digitalWrite(YELLOW_LED_PIN, HIGH); // TARGET : set the Yellow Led for A while
      Set_Period = Time_Periods[Index_Forward++] * 1000 * PERIOD_NEEDED; // Set a new time Period Moving Forword 2, 10, 15, 30 mins
      if (Index_Forward > 3) Index_Forward = 0; // return to the first period again (2 mins)
      Operation_Code = 2; // code of the operation
    }
    else if (sw1_count_presses == 2) { // double presses on Button 1
      { // Serial Lines
        Serial_LineSeparator();
        Serial.println("DOUBLE Press On Button (((((( 1 )))))))");
        Serial_LineSeparator();
      }
      digitalWrite(YELLOW_LED_PIN, LOW);
      Operation_Code = 3; // code of the operation
    }
    else if (sw1_count_presses == 3) { // triple presses on Button 1
      { // Serial Lines
        Serial_LineSeparator();
        Serial.println("TRIPPLE Press On Button (((((( 1 )))))))");
        Serial_LineSeparator();
      }
      digitalWrite(YELLOW_LED_PIN, HIGH); // TARGET : set the Yellow Led for 30 mins
      Set_Period = 30 * 1000 * PERIOD_NEEDED; // New Period, 30 mins in MillieSeconds
      Operation_Code = 4; // code of the operation
    }
    else if (sw2_count_presses == 1) { // single press on Button 2
      { // Serial Lines
        Serial_LineSeparator();
        Serial.println("SINGLE Press On Button (((((( 2 )))))))");
        Serial_LineSeparator();
      }
      digitalWrite(YELLOW_LED_PIN, HIGH); // TARGET : set the Yellow Led for A while
      Set_Period = Time_Periods[Index_Backward--] * 1000 * PERIOD_NEEDED; // Set a new time Period Moving BackWard 30, 15, 10, 2 mins
      if (Index_Backward < 0) Index_Backward = 3; // return to the Last Period again (30 mins)
      Operation_Code = 5; // code of the operation
    }
    else if (sw2_count_presses >= 2) { // double presses on Button 2
      { // Serial Lines
        Serial_LineSeparator();
        Serial.println("DOUBLE Press On Button (((((( 2 )))))))");
        Serial_LineSeparator();
      }
      digitalWrite(YELLOW_LED_PIN, HIGH); // TARGET : set the Yellow Led for A while
      Set_Period = ((30 * 1000  * PERIOD_NEEDED) - (current_time * 1000)); // 30 mins - the current reached timer value
      Operation_Code = 6; // code of the operation
    }
 
       sw1_count_presses = 0; // Clear the Presses Happened for B1
       sw2_count_presses = 0; // Clear the Presses Happened for B2
       SwBoth = 0; // Clear the Both Flag
       sw_wait_flag = 0; // Clear The Flag of long Press 
    }
    if(SW_CorrectingMode.isExpired())
        SW_CorrectingMode.restart();
}

/**************************************************************************
 * Function Name :  Sw_Value                                              *
 * Parameters    :  void                                                  *
 * Return Type   :  int (the state of the Button Pin)                     *
 * Functionality :  is used to detect the current pressed button          *
 **************************************************************************/
int Sw_Value(void) {
  int Switch_State = 0;

  if (analogRead(SW_PIN) >= 750 && analogRead(SW_PIN) <= 780) {
    Switch_State = 3; // pressing A, B together 760 - 768
  }
  else if (analogRead(SW_PIN) >= 320 && analogRead(SW_PIN) <= 360) {
    Switch_State = 2; // pressing A             330 - 342
  }
  else if (analogRead(SW_PIN) >= 715 && analogRead(SW_PIN) <= 735) {
    Switch_State = 1; // pressing B             725 - 735
  }

  return Switch_State;
}

/**************************************************************************
 * Function Name : Trig_UltraSonic                                        *
 * Parameters    : void                                                   *
 * Return Type   : void                                                   *
 * Functionality : used for triggering the UltraSonic                     *
 *                                                                        *
 **************************************************************************/
void Trig_UltraSonic(void) {
  triggerd = 0;
  if (UltraSonicTriger_timeON.isExpired() && digitalRead(ULTRASONIC_TRIG_PIN) == LOW) {
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    UltraSonicTriger_timeON.start(10, AsyncDelay::MICROS);
  }
  else if (UltraSonicTriger_timeON.isExpired() && digitalRead(ULTRASONIC_TRIG_PIN) == HIGH) {
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    UltraSonicTriger_timeON.start(5, AsyncDelay::MICROS);
    triggerd = 1;
  }
}

/**************************************************************************
 * Function Name : Distance_Measured                                      *
 * Parameters    : void                                                   * 
 * Return Type   : int (Distance of the object)                           *
 * Functionality : gets the object's position accordding to the ultrasonic*
 **************************************************************************/
int Distance_Measured(void) {
  int t = 0;
  t = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
  int Distance = t / 57;
  return (int)Distance;
}

/**************************************************************************
 * Function Name :  LDR_Value                                             *
 * Parameters    :  void                                                  *
 * Return Type   :  int (LDR read)                                        *
 * Functionality :  Measures the current Light                            *
 **************************************************************************/
int LDR_Value(void) {
  return analogRead(LDR_PIN);
}

/**************************************************************************
 * Function Name :  Serial_LineSeparator                                  *
 * Parameters    :  void                                                  *
 * Return Type   :  void                                                  *
 * Functionality :  prints a serial Line of '*'                           *
 *                                                                        *
 **************************************************************************/
void Serial_LineSeparator(void) {
  Serial.println();
  Serial.println("************************************************************");
}

/**************************************************************************
 * Function Name :  trun_BuzOff                                           *
 * Parameters    :  void                                                  *
 * Return Type   :  void                                                  *
 * Functionality :  turn off the buzzer                                   *
 *                                                                        *
 **************************************************************************/
void trun_BuzOff(void) {
  if (digitalRead(BUZ_PIN) == HIGH && BuzOn_time.isExpired()) {
    digitalWrite(BUZ_PIN, LOW);
  }
}
/**************************************************************************
 * Function Name :  LCD_printOperation                                    *
 * Parameters    :  void                                                  *
 * Return Type   :  void                                                  *
 * Functionality :  print a message on LCD Based on the Current           *
 *                  Operation                                             *
 **************************************************************************/
void LCD_printOperation(void){
  
  if(Operation_Code == 0){
    lcd.setCursor(0,0); // set the cursor to the first row, first column
    lcd.print("Event Finished");
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Do another Event");
  }
  else if(Operation_Code == 1){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Long Press B1");
  }
  else if(Operation_Code == 2){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Single Press B1");
  }
  else if(Operation_Code == 3){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Double Press B1");
  }
  else if(Operation_Code == 4){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Tripple Press B1");
  }
  else if(Operation_Code == 5){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Single Press B2");
  }
  else if(Operation_Code == 6){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Double Press B2");
  }
  else if(Operation_Code == 7){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("Yellow Led Flash");
  }
  else if(Operation_Code == 8){
    lcd.setCursor(0,1); // set the cursor to the second row, first column
    lcd.print("UltraSonic Event");
  }
}
