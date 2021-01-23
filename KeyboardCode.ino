// Name:Brendan Farrell
// Modified by Benjamin Wright
// Date Started (Last Modified):4/2/2020 (4/29/2020)
// Description:Keyboard Encoder Communication. Based on Capstone Project member's initial code.
// The function that occurs may be different from the button that is pressed. This is because
// we were never able to make sure that the SKey values matched up to the key function on the
// keyboard since we were unable to test the code on the intended board for the keyboard.

#include <Wire.h>
#include "Keyboard.h"

// Addresses for the encoder, descriptions, and registors.
#define ENC_ADDR 0x39
#define Read_Addr 0x73
#define Write_Addr 0x72
#define HID_Desc 0x0001
#define Report_Desc 0x0002
#define Input_Rep 0x0003
#define Output_Rep 0x0004
#define CMD_Reg 0x0005
#define Data_Reg 0x0006

// Defines interrupt and reset pins
#define Key_Press_Interrupt_Pin 20
#define Encoder_Reset_Pin 22

// Defines Symbols and Caps Lock Skey values
#define Symbols_Lock_Skey_Val 41
#define Caps_Lock_Skey_Val 81

// Defines pins for the Symbols Lock and Caps Lock leds
#define Symbols_Lock_Indicator_Pin 10
#define Caps_Lock_Indicator_Pin 11

// If DebugEnabled = 1, print debug to serial monitor
// If DebugEnabled = 0, don't print debug to serial monitor
#define DebugEnabled 0

// Toggles for CapsLock and SymbolsLock
bool SymbolsEnabled = false;
bool CapsLockEnabled = false;

// Each number from the encoder refers to either a number or letter/symbol.
// All redundant values serve to allow the keys without symbols to function even in symbol or caps mode
int SkeyHash[]   = {44,  88,  62,  139, 56,  34,  35,  59,  61,  43,  6,   22,  7,   21,  29,  27,   45,  26,  33,  17,  133, 30,  31,  32,  11,  74,  39,  9,   24,  8,   53,  57,  16,  58,  5,    13,  10,  20,  60,  51,  48,  63 };
int LetterHash[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '!', '?', ',', '.', '\'', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',  't', 'u', 'v', 'w', 'x', 'y', 'z'};
int CapsHash[]   = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '!', '?', ',', '.', '\'', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',  'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
int SymbolHash[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '!', '?', ',', '.', '\'', '^', ';', '(', '@', '#', '&', ':', '"', '=', '_', '[', ']', '>', '<', '{', '}', '~', '$', '\\', '%', '+', ')', '|', '/', '-', '*'};
      
void setup()
{
  // Sets up pin 20 to be used as an interrupt pin.
  pinMode(Key_Press_Interrupt_Pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(Key_Press_Interrupt_Pin), interrupt_handler, FALLING);

  // Sets up pin 22 to act as a switch to reset the encoder.
  // Reset pin is active low, so the PIN must be held high for normal operation.
  pinMode(Encoder_Reset_Pin, OUTPUT);
  digitalWrite(Encoder_Reset_Pin, HIGH);

  // Sets up pins 10 and 11 as the SymbolsLock and CapsLock led outputs (active high)
  pinMode(Symbols_Lock_Indicator_Pin, OUTPUT);
  pinMode(Caps_Lock_Indicator_Pin, OUTPUT);
  
  Serial.begin(115200);

  // Initializes keyboard
  Keyboard.begin();

  // Starts I2C communications.
  Wire.begin();
}

// Resets the encoder.
void ResetEncoder() {
  digitalWrite(Encoder_Reset_Pin, LOW);
  if (DebugEnabled)
    Serial.println("Reset Encoder");
  //Delay is needed for reset.
  delay(10);
  digitalWrite(Encoder_Reset_Pin, HIGH);
}

void Handshake() {
  //Even address
  Wire.beginTransmission(ENC_ADDR);
  Wire.write(Read_Addr);
  Wire.endTransmission();

  //Read command for HID input
  Wire.beginTransmission(ENC_ADDR);
  Wire.write(0x01);
  Wire.write(0x00);
  Wire.endTransmission();

  //Sends the input report command
  Wire.beginTransmission(ENC_ADDR);
  Wire.write(0x03);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.beginTransmission(ENC_ADDR);
  Wire.write(0x05);
  Wire.write(0x00);
  Wire.write(0x02);
  Wire.endTransmission();

  //Reads until a nack is sent by the encoder.
  Wire.beginTransmission(ENC_ADDR);
  Wire.read();
  Wire.endTransmission();
}

void interrupt_handler()
{
  Handshake();

  //Stores the bytes sent by the encoders.
  int returnVals[11];
  int numBytesToReceive = 0;
  int numBytesAvailable = 0;

  //Teensy requests 11 bytes from the encoder.
  Wire.requestFrom(ENC_ADDR, 11, true);
  numBytesAvailable = Wire.available();
  if (numBytesAvailable == 0)
  {
    return;
  }

  // Number of bytes to read
  numBytesToReceive = Wire.read();

  if (DebugEnabled) {
    Serial.print("Available:");
    Serial.println(numBytesAvailable);
    Serial.print("Transmitted:");
    Serial.println(numBytesToReceive);
  }

  //Records each byte into the returnVals string.
  returnVals[0] = numBytesToReceive;
  for (int i = 0; i < numBytesToReceive; i++)
  {
    returnVals[i] = Wire.read();
  }

  if (DebugEnabled) {
    Serial.print("[ ");
    for (int i = 0; i < numBytesToReceive; i++)
    {
      Serial.print(returnVals[i]);
      if (i != 10)
        Serial.print(", ");
    }
    Serial.println(" ]");
  }

  Wire.endTransmission();

  //Shows if any errors occurred.
  byte error = Wire.endTransmission(true);

  if (DebugEnabled) {
    Serial.print("Error Code:");
    Serial.println(error);
  }

  //Checks which SKey value was sent by the encoder and prints the key that was pressed.
  int SkeyVal = returnVals[4];
  int SkeyIndex = -1;

  // Loops through table of Skey values and finds the index of the value which matches SkeyVal
  for (unsigned int i = 0; i < sizeof(SkeyHash) / sizeof(SkeyHash[0]); i++) {
    if (SkeyHash[i] == SkeyVal) {
      SkeyIndex = i;
      continue;
    }
  }

  // Toggles symbols and caps lock
  if (SkeyVal == Symbols_Lock_Skey_Val){
    SymbolsEnabled = !SymbolsEnabled;
    digitalWrite(Symbols_Lock_Indicator_Pin, SymbolsEnabled);
  }
  else if (SkeyVal == Caps_Lock_Skey_Val){
    CapsLockEnabled = !CapsLockEnabled;
    digitalWrite(Caps_Lock_Indicator_Pin, CapsLockEnabled);
  }

  // Calls Keyboard.write and passes the same index of the Skey val in one of three arrays
  if (SymbolsEnabled && SkeyIndex != -1){
    Keyboard.write(SymbolHash[SkeyIndex]);
  }
  else if (CapsLockEnabled && SkeyIndex != -1){
    Keyboard.write(CapsHash[SkeyIndex]);
  }
  else if (SkeyIndex != -1){
    Keyboard.write(LetterHash[SkeyIndex]);
  }
  else{
    switch(SkeyVal){
      case 25:
        Keyboard.press(KEY_TAB);
        Keyboard.release(KEY_TAB);
        break;
      case 4:
        Keyboard.press(KEY_RETURN);
        Keyboard.release(KEY_RETURN);
        break;
      case 65:
        Keyboard.press(KEY_BACKSPACE);
        Keyboard.release(KEY_BACKSPACE);
        break;
      case 100:
        Keyboard.press(KEY_ESC);
        Keyboard.release(KEY_ESC);
        break;
      case 19:
        Keyboard.press(KEY_UP_ARROW);
        Keyboard.release(KEY_UP_ARROW);
        break;
      case 50:
        Keyboard.press(KEY_DOWN_ARROW);
        Keyboard.release(KEY_DOWN_ARROW);
        break;
      case 136:
        Keyboard.press(KEY_LEFT_ARROW);
        Keyboard.release(KEY_LEFT_ARROW);
        break;
      case 23:
        Keyboard.press(KEY_RIGHT_ARROW);
        Keyboard.release(KEY_RIGHT_ARROW);
        break;
    }
  }

  ResetEncoder();
}

void loop()
{
  //Nothing is needed.
}
