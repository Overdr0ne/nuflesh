#include <Arduino.h>
#include <Wire.h>
#include <Keyboard.h>
#include <Mouse.h>

const int POS_CMD = 1;
const int KEY_DN_CMD = 2;
const int KEY_UP_CMD = 3;

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
	int cmd;
};

struct Kbd kbd;

// parameters for reading the joystick:
int cursorSpeed = 10;               // output speed of X or Y movement
int responseDelay = 5;        // response delay of the mouse, in ms
int threshold = cursorSpeed/5;      // resting threshold
int lowerThreshold = cursorSpeed/2;      // resting threshold
int upperThreshold = 3*cursorSpeed/4;      // resting threshold
unsigned int lowerJoyThreshold = 60;
int center = cursorSpeed/2;         // resting position value
int xCenter,yCenter;
char position=-1;
char prevPosition=-2;
bool sendPos;

unsigned int dtime,dtimeCstick;
unsigned int startTime,startTimeCstick;
unsigned int kbdResponseTime = 5;
unsigned int cStickResponseTime = 1;

boolean mouseMode = false;    // whether or not to control the mouse
boolean kbdInit = true;
int lastModeToggle = LOW;        // previous switch state

const int SHIFT_L = 0;
const int CTRL_L = 1;
const int ALT_L = 2;
const int GUI_L = 3;
const int ESC = 4;
const int ENTER = 5;

void setup() {
	Serial.begin(9600);
	kbd.pin[YELLOW] = 3;
	kbd.pin[ORANGE] = 4;
	kbd.pin[BLUE] = 5;
	kbd.pin[RED] = 6;
	kbd.pin[PURPLE] = 7;
	kbd.pin[GREEN] = 8;
	//kbd.outChar[ORANGE] = 'g';
	//kbd.outChar[RED] = KEY_LEFT_SHIFT;
	//kbd.outChar[GREEN] = KEY_LEFT_CTRL;
	//kbd.outChar[BLUE] = KEY_LEFT_ALT;
	//kbd.outChar[PURPLE] = KEY_LEFT_GUI;
	//kbd.outChar[YELLOW] = KEY_ESC;
	kbd.outChar[ORANGE] = ENTER;
	kbd.outChar[RED] = SHIFT_L;
	kbd.outChar[GREEN] = CTRL_L;
	kbd.outChar[BLUE] = ALT_L;
	kbd.outChar[PURPLE] = GUI_L;
	kbd.outChar[YELLOW] = ESC;
	for(int i=0; i<N_KEYS; i++) {
		pinMode(kbd.pin[i], INPUT_PULLUP);
		kbd.send[i] = false;
		kbd.prevState[i] = HIGH;
	}

	xCenter = analogRead(A1);
	yCenter = analogRead(A0);

	pinMode(startEmulation, INPUT_PULLUP);   // the switch pin
	//	pinMode(mouseLeftButton, INPUT_PULLUP);  // the left mouse button pin

	//Keyboard.begin();
	//Mouse.begin();  // take control of the mouse
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

	//Mouse.move(xReading, yReading, 0); // (x, y, scroll mouse wheel)

	if (digitalRead(kbd.pin[RED]) == LOW) {
		// if the mouse is not pressed, press it
		//if (!Mouse.isPressed(MOUSE_LEFT)) {
			//Mouse.press(MOUSE_LEFT);
			//delay(100); // delay to enable single and double-click
		//}
	}
	else {
		// if the mouse is pressed, release it
		//if (Mouse.isPressed(MOUSE_LEFT)) {
			//Mouse.release(MOUSE_LEFT);
		//}
	}

	if (digitalRead(kbd.pin[GREEN]) == LOW) {
		// if the mouse is not pressed, press it
		//if (!Mouse.isPressed(MOUSE_RIGHT)) {
			//Mouse.press(MOUSE_RIGHT);
			//delay(100); // delay to enable single and double-click
		//}
	}
	else {
		// if the mouse is pressed, release it
		//if (Mouse.isPressed(MOUSE_RIGHT)) {
			//Mouse.release(MOUSE_RIGHT);
		//}
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
			//Keyboard.press(kbd.outChar[j]);
			kbd.send[j] = false;
		}
	}
	//Keyboard.releaseAll();
}

void getMods() {
	for(int j=0; j<N_KEYS; j++) {
		kbd.curState[j] = digitalRead(kbd.pin[j]);
		if(kbd.curState[j]!=kbd.prevState[j]) {
			if(kbd.curState[j]==LOW) {
				kbd.cmd = KEY_DN_CMD;
			}
			if(kbd.curState[j]==HIGH) {
				kbd.cmd = KEY_UP_CMD;
			}
			kbd.send[j] = true;
		}
		kbd.prevState[j] = kbd.curState[j];
	}
}

void sendMods() {
	HID_ hid;
	hid.begin();
	char sendBuf[1];
	kbdInit = false;
	startTime = millis();
	for(int j=0; j<N_KEYS; j++) {
		if(kbd.send[j]==true) {
			//Serial.println("Modifier: ");
			//Serial.println(kbd.cmd);
			//Serial.println(sendBuf[0]);
			sendBuf[0] = kbd.outChar[j];
			hid.SendReport(kbd.cmd,sendBuf,1);
			kbd.send[j] = false;
		}
	}
}

int readJoyAxis(int thisAxis, int offset, int center)
{
	// read the analog input
	int reading = analogRead(thisAxis);
	//Serial.println("raw:");
	//Serial.println(reading);
	reading = reading + offset;

	// if the output reading is outside the
	// rest position threshold,  use it
	unsigned int distance = reading - center;

	// return the distance for this axis
	return distance;
}

void getPosition()
{
	int xReading = readJoyAxis(A1,0,xCenter);
	xReading -= 40;
	if (abs(xReading) < 60) {
		xReading=0;
	}

	int yReading = readJoyAxis(A0,0,yCenter);
	if (abs(yReading) < 60) {
		yReading=0;
	}

	//if(xReading==0 && yReading==0) {
		//position=-3;
		//sendPos = false;
	//}
	//else
		//sendPos = true;

	//Serial.println("X:");
	//Serial.println(xReading);
	//Serial.println("Y:");
	//Serial.println(yReading);
	//delay(10);

	if(xReading==0 && yReading==0) {
		position = 0;
		sendPos = true;
	}
	else if(abs(xReading) >= abs(yReading)) {
		if(xReading > 0)
			position='r';
		else if(xReading < 0)
			position='l';
	}
	else if(abs(xReading) < abs(yReading)) {
		if(yReading > 0)
			position='d';
		else if(yReading < 0)
			position='u';
	}
}

void sendPosition()
{
	HID_ hid;
	hid.begin();
	char sendBuf[1];
	if(position==prevPosition)
		return;
	prevPosition = position;
	startTimeCstick = millis();
	sendBuf[0] = position;
	if(sendPos)
		hid.SendReport(POS_CMD,sendBuf,1);
}

void doGestMode()
{
	getMods();
	getPosition();

	dtime = millis() - startTime;
	if(kbdInit || (dtime > kbdResponseTime)) {
		sendMods();
	}
	dtimeCstick = millis() - startTimeCstick;
	if(kbdInit || (dtimeCstick > cStickResponseTime)) {
		sendPosition();
	}
}

void doKbdMode()
{
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
		doGestMode();
	}
}
