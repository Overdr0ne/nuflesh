// Keyboard - Version: Latest 
#include <Keyboard.h>

// Mouse - Version: Latest 
#include <Mouse.h>

// Define Pins

const int startEmulation = 2;      // switch to turn on and off mouse emulation
//const int toggleMouse = 2;    // input pin for the mouse left Button
const int joystickX = A1;         // joystick X axis
const int joystickY = A0;         // joystick Y axis

const int YELLOW = 0;
const int ORANGE = 1;
const int BLUE = 2;
const int RED = 3;
const int PURPLE = 4;
const int GREEN = 5;

const int N_KEYS = 6;
const int N_LAYERS = 4;

struct Kbd {
  int pin[N_KEYS];
  char outChar[N_LAYERS][N_KEYS];
  int curState[N_KEYS];
  int prevState[N_KEYS];
  bool send[N_KEYS];
};

struct Kbd kbd;

// parameters for reading the joystick:
int cursorSpeed = 10;               // output speed of X or Y movement
int responseDelay = 5;        // response delay of the mouse, in ms
int threshold = cursorSpeed/5;      // resting threshold
int lowerThreshold = cursorSpeed/2;      // resting threshold
int upperThreshold = 3*cursorSpeed/4;      // resting threshold
int center = cursorSpeed/2;         // resting position value

unsigned int dtime;
unsigned int startTime;
int kbdResponseTime = 5;

boolean mouseMode = false;    // whether or not to control the mouse
boolean kbdInit = true;
int lastModeToggle = LOW;        // previous switch state

void setup() {
  kbd.pin[YELLOW] = 3;
  kbd.pin[ORANGE] = 4;
  kbd.pin[BLUE] = 5;
  kbd.pin[RED] = 6;
  kbd.pin[PURPLE] = 7;
  kbd.pin[GREEN] = 8;
  kbd.outChar[0][ORANGE] = 'g';
  kbd.outChar[0][RED] = 'f';
  kbd.outChar[0][GREEN] = 'd';
  kbd.outChar[0][BLUE] = 's';
  kbd.outChar[0][PURPLE] = 'a';
  kbd.outChar[0][YELLOW] = KEY_LEFT_CTRL;
  kbd.outChar[1][ORANGE] = 't';
  kbd.outChar[1][RED] = 'r';
  kbd.outChar[1][GREEN] = 'e';
  kbd.outChar[1][BLUE] = 'w';
  kbd.outChar[1][PURPLE] = 'q';
  kbd.outChar[1][YELLOW] = KEY_LEFT_CTRL;
  kbd.outChar[2][ORANGE] = 'b';
  kbd.outChar[2][RED] = 'v';
  kbd.outChar[2][GREEN] = 'c';
  kbd.outChar[2][BLUE] = 'x';
  kbd.outChar[2][PURPLE] = 'z';
  kbd.outChar[2][YELLOW] = KEY_LEFT_CTRL;
  for(int i=0; i<N_KEYS; i++) {
    pinMode(kbd.pin[i], INPUT_PULLUP);
    kbd.send[i] = false;
    kbd.prevState[i] = HIGH;
  }
  
	pinMode(startEmulation, INPUT_PULLUP);   // the switch pin
//	pinMode(mouseLeftButton, INPUT_PULLUP);  // the left mouse button pin

  Keyboard.begin();
	Mouse.begin();  // take control of the mouse
}

//axis for joystick
int readAxis(int thisAxis, int offset) {
  // read the analog input
  int reading = analogRead(thisAxis);
 reading = reading + offset;

  // map the reading from the analog input range to the output range
  reading = map(reading, 0, 1023, 0, cursorSpeed);

  // if the output reading is outside the
  // rest position threshold,  use it
  int distance = reading - center;

  if (abs(distance) < lowerThreshold) {
    distance=0;
  }

	// return the distance for this axis
	return distance;
}

void doMouseMode() {
  // read and scale the two axes
	int xReading = readAxis(A1,10);
	int yReading = readAxis(A0,0);
	
  Mouse.move(xReading, yReading, 0); // (x, y, scroll mouse wheel)
  
	if (digitalRead(kbd.pin[RED]) == LOW) {
		// if the mouse is not pressed, press it
		if (!Mouse.isPressed(MOUSE_LEFT)) {
			Mouse.press(MOUSE_LEFT);
			delay(100); // delay to enable single and double-click
		}
	}
	else {
		// if the mouse is pressed, release it
		if (Mouse.isPressed(MOUSE_LEFT)) {
			Mouse.release(MOUSE_LEFT);
		}
	}

	if (digitalRead(kbd.pin[GREEN]) == LOW) {
		// if the mouse is not pressed, press it
		if (!Mouse.isPressed(MOUSE_RIGHT)) {
			Mouse.press(MOUSE_RIGHT);
			delay(100); // delay to enable single and double-click
		}
	}
	else {
		// if the mouse is pressed, release it
		if (Mouse.isPressed(MOUSE_RIGHT)) {
			Mouse.release(MOUSE_RIGHT);
		}
	}
}

void getKeypresses() {
  for(int j=0; j<N_KEYS; j++) {
    kbd.curState[j] = digitalRead(kbd.pin[j]);
    if((kbd.curState[j]==LOW) && 
    (kbd.curState[j]!=kbd.prevState[j])) {
      kbd.send[j] = true;
    }
    kbd.prevState[j] = kbd.curState[j];
  }
}

void sendKeys(int layer) {
  kbdInit = false;
  startTime = millis();
  for(int i=0; i<N_LAYERS; i++) {
    for(int j=0; j<N_KEYS; j++) {
      if(kbd.send[j]==true && (layer==i)) {
        Keyboard.press(kbd.outChar[i][j]);
        kbd.send[j] = false;
      }
    }
  }
  Keyboard.releaseAll();
}

int getLayer() {
  int layer = 0;
  int xReading = readAxis(A1,10);
  int yReading = readAxis(A0,0);
  
  if(abs(xReading) > abs(yReading)) {
    if(xReading > 0)
      layer=1;
    else if(xReading < 0)
      layer=2;
  }
  else {
    if(yReading > 0)
      layer=3;
    else if(yReading < 0)
      layer=4;
  }

  return layer;
}

void doKbdMode() {
  int layer = 0;
  
  layer = getLayer();
  
  getKeypresses();
	
	dtime = millis() - startTime;
	if(kbdInit || (dtime > kbdResponseTime)) {
	  sendKeys(layer);
	}
}

void loop() {
	// read the switch:
	int modeToggle = digitalRead(startEmulation);

	// if it's changed and it's high, toggle the mouse state:
	if (modeToggle != lastModeToggle) {
		if (modeToggle == LOW) {
			mouseMode = !mouseMode;
		}
	}
	// save switch state for next loop:
	lastModeToggle = modeToggle;

	// if the mouse control state is active, move the mouse:
	if (mouseMode) {
		doMouseMode();
		delay(responseDelay);
	}
	else {
	  doKbdMode();
	}
}