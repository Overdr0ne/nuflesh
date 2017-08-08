#include <Arduino.h>
#include <Wire.h>
#include <Keyboard.h>
#include <Mouse.h>

// Define Pins
const int startEmulation = 2;      // switch to turn on and off mouse emulation
const int joystickX = A1;         // joystick X axis
const int joystickY = A0;         // joystick Y axis

const int YELLOW = 0;
const int ORANGE = 1;
const int BLUE = 2;
const int RED = 3;
const int PURPLE = 4;
const int GREEN = 5;

const int N_KEYS = 6;

struct Kbd {
	int pin[N_KEYS];
	char outChar[N_KEYS];
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
unsigned int lowerJoyThreshold = 40;
int center = cursorSpeed/2;         // resting position value
int xCenter,yCenter;
int position=-1;
int prevPosition=-2;
bool sendPos;

unsigned int dtime,dtimeCstick;
unsigned int startTime,startTimeCstick;
unsigned int kbdResponseTime = 5;
unsigned int cStickResponseTime = 1;

boolean mouseMode = false;    // whether or not to control the mouse
boolean kbdInit = true;
int lastModeToggle = LOW;        // previous switch state

void setup() {
  Serial.begin(9600);
	kbd.pin[YELLOW] = 3;
	kbd.pin[ORANGE] = 4;
	kbd.pin[BLUE] = 5;
	kbd.pin[RED] = 6;
	kbd.pin[PURPLE] = 7;
	kbd.pin[GREEN] = 8;
	kbd.outChar[ORANGE] = 'g';
	kbd.outChar[RED] = KEY_LEFT_SHIFT;
	kbd.outChar[GREEN] = KEY_LEFT_CTRL;
	kbd.outChar[BLUE] = KEY_LEFT_ALT;
	kbd.outChar[PURPLE] = KEY_LEFT_GUI;
	kbd.outChar[YELLOW] = KEY_ESC;
	for(int i=0; i<N_KEYS; i++) {
		pinMode(kbd.pin[i], INPUT_PULLUP);
		kbd.send[i] = false;
		kbd.prevState[i] = HIGH;
	}

	xCenter = analogRead(A1);
	yCenter = analogRead(A0);

	pinMode(startEmulation, INPUT_PULLUP);   // the switch pin
	//	pinMode(mouseLeftButton, INPUT_PULLUP);  // the left mouse button pin

	Keyboard.begin();
	Mouse.begin();  // take control of the mouse
}

//axis for joystick
int readMouseAxis(int thisAxis, int offset) {
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
	int xReading = readMouseAxis(A1,10);
	int yReading = readMouseAxis(A0,0);

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

void sendKeys() {
	kbdInit = false;
	startTime = millis();
	for(int j=0; j<N_KEYS; j++) {
		if(kbd.send[j]==true) {
			Keyboard.press(kbd.outChar[j]);
			kbd.send[j] = false;
		}
	}
	Keyboard.releaseAll();
}

int readJoyAxis(int thisAxis, int offset, int center)
{
	// read the analog input
	int reading = analogRead(thisAxis);
	reading = reading + offset;

	// if the output reading is outside the
	// rest position threshold,  use it
	unsigned int distance = reading - center;

	if (abs(distance) < lowerJoyThreshold) {
		distance=0;
	}

	// return the distance for this axis
	return distance;
}

void getPosition()
{
	int xReading = readJoyAxis(A1,30,xCenter);
	int yReading = readJoyAxis(A0,0,yCenter);

	if(xReading==0 && yReading==0) {
		position=-3;
		sendPos = false;
	}
	else
		sendPos = true;

	Serial.println("X:");
	Serial.println(xReading);
	Serial.println("Y:");
	Serial.println(yReading);
	//delay(100);

	if(abs(xReading) >= abs(yReading)) {
		if(xReading > 0)
			position=KEY_RIGHT_ARROW;
		else if(xReading < 0)
			position=KEY_LEFT_ARROW;
	}
	else if(abs(xReading) < abs(yReading)) {
		if(yReading > 0)
			position=KEY_DOWN_ARROW;
		else if(yReading < 0)
			position=KEY_UP_ARROW;
	}
}

void sendPosition()
{
	if(position==prevPosition)
		return;
	prevPosition = position;
	startTimeCstick = millis();
	if(sendPos)
		Keyboard.press(position);
	Keyboard.release(position);
}

void doKbdMode()
{
	HID_ hid;
	unsigned char test[3];
	test[0]='a';
	test[1]='b';
	test[2]='c';

	getKeypresses();
	getPosition();

	dtime = millis() - startTime;
	if(kbdInit || (dtime > kbdResponseTime)) {
		sendKeys();
	}
	dtimeCstick = millis() - startTimeCstick;
	if(kbdInit || (dtimeCstick > cStickResponseTime)) {
		sendPosition();
	}
	hid.SendReport(123,(void *)test,3);
}

void loop()
{
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