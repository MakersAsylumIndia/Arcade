/* 
===============================================================================
This code is part of the ARCADE project for the "Electronics and 
Microcontrollers" skill of INNOVATION SCHOOL at MAKERS ASYLUM.

Copyright : 2025 Maker's Asylum | makersasylum.com
License : MIT

Summary of what this code does:
  Project: Buzzer Test
  Description:
    This simple program demonstrates basic digital output control 
    on an Arduino board. It repeatedly turns a Buzzer connected to 
    digital pin 3 ON and OFF at a fixed interval, creating a beeping audio 
    effect.

   Hardware Setup:
    - Buzzer is connected between +5V and collector of the NPN transistor
    - Digital pin D3 connects to base of the transistor

   Operation:
    - The Buzzer turns ON for 500 milliseconds & then OFF for 500 milliseconds.
    - This cycle repeats continuously, confirming that the microcontroller 
      and digital output pin are functioning correctly.
===============================================================================
*/

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(3, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(3, HIGH);   // turn the Buzzer on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(3, LOW);    // turn the Buzzer off by making the voltage LOW
  delay(500);                       // wait for a second
}
