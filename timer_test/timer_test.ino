#include <Adafruit_CircuitPlayground.h>
/***************************
 * Spike program to test creating a spinner
 * 
 * Inputs
 *  Button A - slows down spinner
 *  Button B - speeds up spinner
 *  
 */
 
uint8_t pixeln = 0;
long deltaTime = 0;
unsigned long lastTime = 0;
int speed = 1000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Timer test!");
  CircuitPlayground.begin();
}

void tick() {
  CircuitPlayground.setPixelColor(pixeln++, CircuitPlayground.colorWheel(25 * pixeln));
  if (pixeln == 11) {
    pixeln = 0;
    CircuitPlayground.clearPixels();
  }
}

void loop() {
  unsigned long t = millis();
  deltaTime += t - lastTime;
  lastTime = t;

  if (CircuitPlayground.leftButton()) {
    speed+=5;
  }
  if (CircuitPlayground.rightButton()) {
    speed-=5;
  }
  delay(10);

  if(deltaTime >= speed) {
    tick();
    deltaTime = 0;
  }
}
