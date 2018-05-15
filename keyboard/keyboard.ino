// Use: Pulling PIN_Z low presses the Z key,
// pulling PIN_X low presses the X key. If you have
// issues with detecting input or ghost inputs, configure POLL_DELAY
// (increasing helps with debouncing, just don't increase it too much).
// Also try enabling the Trinket to run at 16 MHz. Running at 8 MHz
// (the default) will draw about 3mA less current, though.
//
// Pressing X and Z for longer than CONFIG_DELAY will
// enter brightness config mode, and the lights will blink thrice.
// In this mode, you can change the brightness of the lights by
// pressing X and exit by pressing Z.
//
// Pressing Z for longer than CONFIG_DELAY will enter LED mode
// config mode, and the lights will blink twice. In this mode,
// you can change the LED mode by pressing X. The LED mode being always on
// is denoted by the LEDs being on. The LED mode being keypress
// is denoted by the LEDs being off. Press Z to exit.

// Keyboard.
#include <TrinketKeyboard.h>
// Enable 16MHz.
#include <avr/power.h>
// Write to and read from EEPROM.
#include <EEPROM.h>

#define PIN_Z 2
#define PIN_X 0
#define PIN_LEDS 1

// How long to wait before polling again (prevent debouncing).
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

// EEPROM Address
#define BRIGHTNESS_ADR 0
#define LEDMODE_ADR 1

// Previous state of the keyboard.
int prevState = 0;
// Count of how many times both keys have been held
// (this is used to enter config mode).
unsigned short ZXCount = 0;
unsigned short ZCount = 0;
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
  // Blink the LEDs thrice to signify entering config mode.
  blinkLEDs(500);
  blinkLEDs(500);
  blinkLEDs(500);
  
  bool z = false;
  bool x = false;
  while (1)
  {
    delay(CONFIG_POLL_DELAY);
    bool z = !digitalRead(PIN_Z);
    bool x = !digitalRead(PIN_X);
    // Add to brightness if x is pressed.
    if (x)
    {
      brightness += 16;
    }
    // Write 0 if the LEDState is off to blink.
    analogWrite(PIN_LEDS, brightness);
    // Exit if z is pressed.
    if (z)
    {
      // Save the brightness.
      EEPROM.write(BRIGHTNESS_ADR, brightness);
      TrinketKeyboard.begin();
      return;
    }
  }
}

void configureLEDMode()
{
  // Blink the LEDs twice to signify entering config mode.
  blinkLEDs(500);
  blinkLEDs(500);
  
  bool z = false;
  bool x = false;
  while (1)
  {
    delay(CONFIG_POLL_DELAY);
    bool z = !digitalRead(PIN_Z);
    bool x = !digitalRead(PIN_X);
    // Toggle LED mode if x is pressed.
    if (x)
    {
      ledMode = !ledMode;
    }
    digitalWrite(PIN_LEDS, ledMode);
    // Exit if z is pressed.
    if (z)
    {
      // Save the LED mode.
      EEPROM.write(LEDMODE_ADR, ledMode);
      // Reset the LEDs.
      analogWrite(PIN_LEDS, LOW);
      setupLEDs();
      TrinketKeyboard.begin();
      return;
    }
  }
}

void keyFunction()
{
  bool z = !digitalRead(PIN_Z);
  bool x = !digitalRead(PIN_X);
  // compress so that it can be used in a switch.
  int state = (2*z) + x;
  if (state != prevState)
  {
    switch (state)
    {
      case 0:
        TrinketKeyboard.pressKey(0, 0);
        break;
      case 1:
        TrinketKeyboard.pressKey(0, KEYCODE_X);
        break;
      case 2:
        TrinketKeyboard.pressKey(0, KEYCODE_Z);
        break;
      case 3:
        TrinketKeyboard.pressKey(0, KEYCODE_X, KEYCODE_Z);
        break;
    }
    if (state != 3)
    {
      // reset led brightness config check.
      ZXCount = 0;
    }
    if (state != 2)
    {
      // reset led mode config check.
      ZCount = 0;
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
  } else if (state == 3)
  {
    // If both buttons are pressed, continue to increment the counter.
    ZXCount += 1;
    // If both buttons have been pressed for longer than
    // the config wait, configure the LED brightness.
    if ((POLL_DELAY * ZXCount)  > CONFIG_DELAY)
    {
      TrinketKeyboard.pressKey(0, 0, 0);
      configureLEDBrightness();
      // Reset the count.
      ZXCount = 0;
    }
  } else if (state == 2)
  {
    // If z is pressed, continue to increment the counter.
    ZCount += 1;
    // If z is pressed for longer than the
    // the config wait, configure the LED mode.
    if ((POLL_DELAY * ZCount)  > CONFIG_DELAY)
    {
      TrinketKeyboard.pressKey(0, 0, 0);
      configureLEDMode();
      // Reset the count.
      ZCount = 0;
    }
  }
  prevState = state;

}

void loop() {
  TrinketKeyboard.poll();
  // prevent debouncing
  delay(POLL_DELAY);
  keyFunction();
}
