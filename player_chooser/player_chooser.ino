#include <Adafruit_CircuitPlayground.h>

/***************************
 * This program is a first-player picker. For example, chooing the first player in a game.
 * Players place their fingers on the touch pads, press the Start (A) button, and wait for the spinner to stop on the chosen player.
 * The program takes into account if a player leaves in the middle of spinner.
 * 
 * Inputs
 *    Button A      - Start
 *    Button B      - Reset
 *    Player 1 Pads - A4/A5
 *    Player 2 Pads - A6/A7
 *    Player 3 Pads - A0/A1
 *    Player 4 Pads - A2/A3
 * 
 * Program observations
 * 
 *    The program uses a state machine to simplify logic
 * 
 *    Because the programs are procedural (no events), need to keep two separate timers:
 *      seconds   - main timer in seconds
 *      spinClock - pixel update timer in milliseconds
 * 
 * Resources
 *    https://www.arduino.cc/reference/en
 *    https://adafruit.github.io/Adafruit_CircuitPlayground/html/class_adafruit___circuit_playground.html
 *    https://cdn-learn.adafruit.com/assets/assets/000/047/156/original/circuit_playground_Adafruit_Circuit_Playground_Express_Pinout.png?1507829017
 * 
 */

const bool DEBUG = false; // writes out debug messages to serial monitor if true
uint8_t pixeln = 0; // the next pixel to light
unsigned long lastTime = 0; // keeps track of last millisecond time to calculate deltaTime
long seconds = 0; // seconds since application has started
long secondClock = 0; // keeps track of milliseconds and updates seconds if 1000 has passed

long spinClock = 0; // clock cycles for spinner
int spinSpeed = 50; // how fast to flip pixels (in ms)
const int SPIN_DURATION = 4; // how long spinner should run before stopping
int spinCount = SPIN_DURATION; // countdown timer for spinner

const int FINISHED_DURATION = 5; // how long result should stay displayed before resetting
int finishedCount = FINISHED_DURATION; // countdown timer until moving to reset state

int chosenPlayerIndex = -1; // the chosen player
bool cselected[] = {false, false, false, false}; // state of activated touch pads - player 1, player 2, player 3, player 4

int state = 0; // current application state

const int STATE_WAITING = 0;    // waiting for button A to be pressed to start the app 
const int STATE_RUNNING = 1;    // the application starts spinning and after SPIN_DURATION seconds moves to STATE_FINISHED
const int STATE_FINISHED = 2;   // a player is chosen and will keep the display on for FINISHED_DURATION seconds then moves to STATE_RESETTING
const int STATE_RESETTING = 3;  // resets the application state and moves to STATE_WAITING
const int PIXEL_BRIGHTNESS = 8; // pixels are bright! set to low brightness



class Timer {
    
  private:
    unsigned long _milliseconds = 0;
    unsigned long _lastTime = 0;
    unsigned long _accum = 0;
    unsigned long _counter = 0;
    
  public:
    Timer(const int milliseconds) {
      _milliseconds = milliseconds;
    }
  
    unsigned long loop() {
      unsigned long t = millis();
      _accum += t - _lastTime;
      _lastTime = t;
      if(_accum >= _milliseconds) {
        _counter += 1;
        _accum = 0;
        return true;
      }      
      return false;
    }
  
    unsigned long getEllapsed() {
      return _counter;
    }
    
    unsigned long msEllapsedRead() {
      return millis();
    }

    void millisecondsWrite(const unsigned long milliseconds) {
      _milliseconds = milliseconds;
    }

    const unsigned long millisecondsRead() {
      return _milliseconds;
    }

};

Timer _secondTimer(1000),   // Second timer
      _spinTimer(50);       // Spinner timer

/***************************
 * Initialize application
 */
void setup() {
  if(DEBUG) {
    Serial.begin(9600);
    Serial.println("Player Chooser");
  }
  randomSeed(analogRead(0));  // make sure we get a new seed each time we run
  CircuitPlayground.begin();  // initialize the cpx
  CircuitPlayground.setBrightness(PIXEL_BRIGHTNESS); 
  CircuitPlayground.speaker.enable(false); // turn off speaker - otherwise it starts automatically
}


/***************************
 * Updates application state based on button state
 */
void checkButtons() {
  if (CircuitPlayground.leftButton() && state == STATE_WAITING) {
    state = STATE_RUNNING;
  }
  if (CircuitPlayground.rightButton()) {
    state = STATE_RESETTING;
  }
}

/***************************
 * Sets the current state of the players participating
 */
void updateEnabledPlayers() {
  cselected[0] = CircuitPlayground.readCap(3) > 100 || CircuitPlayground.readCap(2) > 100;
  cselected[1] = CircuitPlayground.readCap(0) > 100 || CircuitPlayground.readCap(1) > 100;
  cselected[2] = CircuitPlayground.readCap(6) > 100 || CircuitPlayground.readCap(12) > 100;
  cselected[3] = CircuitPlayground.readCap(9) > 100 || CircuitPlayground.readCap(10) > 100;
}

/***************************
 * Get the pixel to light up based on the chosen player
 */
const int getPlayerPixelIndex(const int playerIndex) {
  switch(playerIndex) {
    case 0:
      return 1;
    case 1:
      return 3;
    case 2:
      return 6;
    case 3:
      return 8;
  }
  return -1;
}

/***************************
 * Returns the current number of players participating
 */
const int playerCount() {
  updateEnabledPlayers();
  return cselected[0] + cselected[1] + cselected[2] + cselected[3];
}

/***************************
 * Turns the previous pixel off, the current pixel on and increments to the next pixel
 */
void updatePixels() {
  // Turn off previous pixel
  if(pixeln == 0) {
    CircuitPlayground.setPixelColor(9, 0, 0, 0);
  } else {
    CircuitPlayground.setPixelColor(pixeln-1, 0, 0, 0);
  }
  // Set current pixel
  CircuitPlayground.setPixelColor(pixeln++, CircuitPlayground.colorWheel(25 * pixeln));
  
  // Makes sure next pixel circles back around
  if (pixeln == 10) pixeln = 0;
}

/***************************
 * Chooses a player and cycles through until all players anyone leaves while the app is running
 */
void tryChoosePlayer() {
  // Only try to choose a player if we're done spinning
  if(spinCount > 0) return;

  if(playerCount() == 0) {
    state = STATE_RESETTING;
    return;
  }

  if(chosenPlayerIndex == -1) {
    // Make our initial winner but may get changed depending on who's still playing
    chosenPlayerIndex = random(3); // might change this to calculate a random value a bunch during the spinning?
  } else {
    // Make sure the player is still playing
    while(cselected[chosenPlayerIndex] != 1 && playerCount() > 0) {
      chosenPlayerIndex = (chosenPlayerIndex + 1) % 4;
    }
    // Get the pixel to display
    int pix = getPlayerPixelIndex(chosenPlayerIndex);
    int curPixel = (pixeln-1) % 10; // Offset due to updatePixels incrementing pixeln before it exits
    
    // Only display the pixel if it's the currently lit one - otherwise let the cycle continue until it gets back here
    if(pix == curPixel) {
      state = STATE_FINISHED;
      return;
    }
  }
}

/***************************
 * Resets all variable back to start state to the app can run again
 */
void resetState() {
  state = STATE_WAITING;
  pixeln = 0;
  spinCount = SPIN_DURATION;
  finishedCount = FINISHED_DURATION;
  _spinTimer.millisecondsWrite(spinSpeed);
  chosenPlayerIndex = -1;
  CircuitPlayground.clearPixels();
}

/***************************
 * if DEBUG is true, writes out comments to Serial port
 */
void writeDebug() {
  Serial.print("------------------------ Time: "); Serial.println(seconds);
  Serial.print("state: "); Serial.println(state);
  
//  Serial.print("cselected[0]: "); Serial.println(cselected[0]);
//  Serial.print("cselected[1]: "); Serial.println(cselected[1]);
//  Serial.print("cselected[2]: "); Serial.println(cselected[2]);
//  Serial.print("cselected[3]: "); Serial.println(cselected[3]);

  Serial.print("pixeln: "); Serial.println(pixeln);
  Serial.print("chosenPlayerIndex: "); Serial.println(chosenPlayerIndex);
  Serial.print("spinCount: "); Serial.println(spinCount);
  Serial.print("finishedCount: "); Serial.println(finishedCount);
  
}

/***************************
 * Main event loop using state machine
 */
void loop() {
  bool tick = _secondTimer.loop();
  bool spin = _spinTimer.loop();
  
  checkButtons();
  
  if(DEBUG && tick) writeDebug();

  switch(state) {
    case STATE_WAITING:
      // waiting - noop
      break;
    case STATE_RUNNING:
      updateEnabledPlayers();
      if(spin) {
        updatePixels();
      }
      tryChoosePlayer();
      if(tick) {
        spinCount--;
        _spinTimer.millisecondsWrite(_spinTimer.millisecondsRead() + 20);
      }
      break;
    case STATE_FINISHED:
      // keep pixel on so player can see last result until reset button is pressed or 
      // if more than FINISHED_DURATION seconds passses
      if(tick) finishedCount--;
      if(finishedCount == 0) state = STATE_RESETTING;
      break;
    case STATE_RESETTING:
      resetState();
      break;
  }
  
  delay(10);
}
