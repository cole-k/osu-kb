// Use: Pulling PIN_Z low presses the Z key,
// pulling PIN_X low presses the X key. If you have
// issues with detecting input or ghost inputs, configure POLL_DELAY
// (increasing helps with debouncing, just don't increase it too much).
// Also try enabling the Trinket to run at 16 MHz. Running at 8 MHz
// (the default) will draw about 3mA less current, though.
//
// Pressing X and Z for longer than CONFIG_DELAY will
// enter the configuration menu and the LEDs will blink thrice.
// If you press X, you'll be taken to the LED brightness configuration
// and if you press Z, you'll be taken to the LED mode configuration.
//
// In the LED brightness configuration, pressing X will cycle through
// various levels of brightness. Pressing Z will exit and save the
// brightness selected.
//
// In the LED mode configuration, pressing X will toggle between
// Always On and Keypress mode. The LEDs will turn on if in Always On
// mode; they will be off if in Keypress mode. Pressing Z will save
// the mode selected and exit.
//
// Any changes made in the configuration mode are retained upon restart.

// Keyboard.
#include <TrinketKeyboard.h>
// Enable 16MHz.
#include <avr/power.h>
// Write to and read from EEPROM.
#include <EEPROM.h>

#define PIN_Z 2
#define PIN_X 0
#define PIN_LEDS 1

// How long to wait before polling again (for debouncing).
#define POLL_DELAY   4

// How long to wait before entering config mode, in ms.
#define CONFIG_DELAY 4000
// How long to wait before accepting input again after changing
// brightness in config mode. This is high to make it easier to
// select a desired brightness.
#define CONFIG_POLL_DELAY 300
// How frequently to blink the LEDs.
#define CONFIG_LED_BLINK_RATE 1000

// LED Modes
#define KEYPRESS  0
#define ALWAYS_ON 1

// EEPROM Addresses
#define BRIGHTNESS_ADR 0
#define LEDMODE_ADR 1

// Previous state of the keyboard.
int prevState = 0;
// Count of how many times both keys have been held
// (this is used to enter config mode).
unsigned short ZXCount = 0;
uint8_t brightness = 0;
uint8_t ledMode = ALWAYS_ON;

void setup() {
  // Uncomment the line beneath to run at 16 MHz. Default is 8 MHz.
  // You will need to change the board to "Adafruit Trinket (ATtiny85 @ 16MHz)"
  // to run at 16 MHz.
  // if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  
  // Pin pulled LOW = active
  pinMode(PIN_Z, INPUT_PULLUP);
  pinMode(PIN_X, INPUT_PULLUP);

  pinMode(PIN_LEDS, OUTPUT);

  readValuesFromEEPROM();

  setupLEDs();
  TrinketKeyboard.begin();
}

void setupLEDs()
{
  if (ledMode == ALWAYS_ON)
  {
    analogWrite(PIN_LEDS, brightness);
  }
}

void readValuesFromEEPROM()
{
  brightness = EEPROM.read(BRIGHTNESS_ADR);
  ledMode = EEPROM.read(LEDMODE_ADR);
}

void blinkLEDs(int duration)
{
  digitalWrite(PIN_LEDS, HIGH);
  delay(duration);
  digitalWrite(PIN_LEDS, LOW);
  delay(duration);
}

void configureLEDBrightness()
{ 
  bool zPressed = false;
  bool xPressed = false;
  unsigned long lastCheck = millis();
  
  // Loop while z is not pressed.
  while (!zPressed)
  {
    // Only execute this every CONFIG_POLL_DELAY ms.
    // This debounces the inputs.
    if (millis() - lastCheck > CONFIG_POLL_DELAY)
    {
      zPressed = !digitalRead(PIN_Z);
      xPressed = !digitalRead(PIN_X);
      
      // Add to brightness if x is pressed.
      if (xPressed)
      {
        brightness += 16;
        
        // Set the LEDs to that brightness.
        analogWrite(PIN_LEDS, brightness);
      }

      // Update lastCheck.
      lastCheck = millis();
    }

    // Send empty keypress to keep connection to computer.
    TrinketKeyboard.pressKey(0, 0);
    
    // Avoid spamming the computer by waiting
    // POLL_DELAY between each cycle
    delay(POLL_DELAY);
  }
  
  // Save the brightness.
  EEPROM.write(BRIGHTNESS_ADR, brightness);
}

void configureLEDMode()
{ 
  bool zPressed = false;
  bool xPressed = false;
  unsigned long lastCheck = millis();
  
  // Loop while z is not pressed.
  while (!zPressed)
  {
    // Only execute this every CONFIG_POLL_DELAY ms.
    // This debounces the inputs.
    if (millis() - lastCheck > CONFIG_POLL_DELAY)
    {
      zPressed = !digitalRead(PIN_Z);
      xPressed = !digitalRead(PIN_X);
      
      // Toggle LED mode if x is pressed.
      if (xPressed)
      {
        ledMode = !ledMode;
        
        // Set the LEDs to indicate LED Mode.
        digitalWrite(PIN_LEDS, ledMode);
      }

      // Update lastCheck.
      lastCheck = millis();
    }

    // Send empty keypress to keep connection to computer.
    TrinketKeyboard.pressKey(0, 0);
    
    // Avoid spamming the computer by waiting
    // POLL_DELAY between each cycle
    delay(POLL_DELAY);
  }
  
    // Save the LED mode.
    EEPROM.write(LEDMODE_ADR, ledMode);
    // Reset the LEDs.
    analogWrite(PIN_LEDS, LOW);
    setupLEDs();
}

void configurationMenu()
{
  // Blink LEDs thrice to denote configuration mode.
  blinkLEDs(500);
  blinkLEDs(500);
  blinkLEDs(500);
  
  bool zPressed = false;
  bool xPressed = false;
  unsigned long lastCheck = millis();
  
  while (1)
  {
    // Only execute this every CONFIG_POLL_DELAY ms.
    // This debounces the inputs.
    if (millis() - lastCheck > CONFIG_POLL_DELAY)
    {
      bool zPressed = !digitalRead(PIN_Z);
      bool xPressed = !digitalRead(PIN_X);
      
      // Configure LED Brightness if x is pressed.
      if (xPressed)
      {
        configureLEDBrightness();
        return;
      }

      if (zPressed)
      {
        configureLEDMode();
        return;
      }

      // Update lastCheck.
      lastCheck = millis();
    }

    // Send empty keypress to keep connection to computer.
    TrinketKeyboard.pressKey(0, 0);
    
    // Avoid spamming the computer by waiting
    // POLL_DELAY between each cycle
    delay(POLL_DELAY);
  }
}

void keyFunction()
{
  bool zPressed = !digitalRead(PIN_Z);
  bool xPressed = !digitalRead(PIN_X);
  
  // Convert tuples of (xPressed, zPressed)
  // to a single integral value (think base decoding).
  int state = (2*zPressed) + xPressed;
  
  // Only do anything if the state has changed.
  if (state != prevState)
  {
    switch (state)
    {
      // Nothing pressed
      case 0:
        TrinketKeyboard.pressKey(0, 0);
        break;
      // x pressed.
      case 1:
        TrinketKeyboard.pressKey(0, KEYCODE_X);
        break;
      // z pressed.
      case 2:
        TrinketKeyboard.pressKey(0, KEYCODE_Z);
        break;
      // both pressed.
      case 3:
        TrinketKeyboard.pressKey(0, KEYCODE_X, KEYCODE_Z);
        break;
    }
    // If Z and X are no longer being pressed.
    if (state != 3)
    {
      // Reset config check.
      ZXCount = 0;
    }
    // If the mode is keypress, toggle the LEDs depending
    // on whether the keys are pressed.
    if (ledMode == KEYPRESS)
    {
      if (state == 0)
      {
        analogWrite(PIN_LEDS, LOW);
      }
      else
      {
        analogWrite(PIN_LEDS, brightness);
      }
    }
  }
  // If the state hasn't changed and both keys are pressed.
  else if (state == 3)
  {
    // Increment the counter.
    ZXCount += 1;
    
    // If both buttons have been pressed for longer than
    // the config wait, configure the LED brightness.
    if ((POLL_DELAY * ZXCount)  > CONFIG_DELAY)
    {
      TrinketKeyboard.pressKey(0, 0);
      configurationMenu();
      // Reset the count.
      ZXCount = 0;
    }
  }
  prevState = state;

}

void loop() {
  TrinketKeyboard.poll();
  // Debounce.
  delay(POLL_DELAY);
  keyFunction();
}
