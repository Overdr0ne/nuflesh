#include <iostream>
#include <sstream>
#include <fstream>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <string>
#include "hidapi.h"
extern "C" {
#include <xdo.h>
}

// Headers needed for sleeping.
#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

void load_gestDict(std::map<std::string,char> &gestMap)
{
	std::ifstream gestDict;
	std::string gestStr;
	char sym;
	gestDict.open("gest-dict");

	while(gestDict >> gestStr >> sym) {
		gestMap[gestStr] = sym;
		std::cout << "gestStr: " << gestStr << ' ' << gestMap[gestStr] << std::endl;
	}

}

int main(int argc, char* argv[])
{
	std::map<std::string,std::string> gestDict;
	std::string gestStr;
	int res;
	unsigned char buf[256];
	char sendBuf[1];
	char cmd,key,xDist,yDist;
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	hid_device *handle;
	int i;
	char outKey;
	std::string keyStr;
	std::map<std::string,char> gestMap;
	const int POS_CMD = 1;
	const int KEY_DN_CMD = 2;
	const int KEY_UP_CMD = 3;
	const int MOUSE_MV_CMD = 4;
	const int MOUSE_DN_CMD = 5;
	const int MOUSE_UP_CMD = 6;
	const int SHIFT_L = 0;
	const int CTRL_L = 1;
	const int ALT_L = 2;
	const int GUI_L = 3;
	const int ESC = 4;
	const int ENTER = 5;
	char modBuf[32];
	xdo_t * xdo = xdo_new(":0.0");
	int button;

#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif

	struct hid_device_info *devs, *cur_dev;

	gestStr = "";

	load_gestDict(gestMap);

	if (hid_init())
		return -1;

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	handle = hid_open(0x1b4f, 0x9206, NULL);
	if (!handle) {
		printf("unable to open device\n");
 		return 1;
	}

	while(1)
	{
		res = 0;
		res = hid_read(handle, buf, sizeof(buf));
		if (res < 0)
			printf("Unable to read()\n");
		printf("Data read:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02hhx ", buf[i]);
		printf("\n");
		cmd = buf[0];
		printf("cmd: %i\n",cmd);
		printf("key: %c\n",key);

		if(cmd==POS_CMD) {
			key = buf[1];
			if(key==0) {
				std::cout << "string ended" << std::endl;
				std::cout << "string: " << gestStr << std::endl;
				std::cout << "outchar: " << gestMap[gestStr] << std::endl;
				keyStr = gestMap[gestStr];
				xdo_send_keysequence_window(xdo, CURRENTWINDOW, keyStr.c_str(), 0);
				gestStr = "";
			}
			else {
				gestStr += key;
			}
		}
		else if(cmd==KEY_DN_CMD || cmd==KEY_UP_CMD) {
			key = buf[1];
			switch(key) {
				case SHIFT_L:
					snprintf(modBuf,sizeof(modBuf),"Shift_L");
					break;
				case CTRL_L:
					snprintf(modBuf,sizeof(modBuf),"Control_L");
					break;
				case ALT_L:
					snprintf(modBuf,sizeof(modBuf),"Alt_L");
					break;
				case GUI_L:
					snprintf(modBuf,sizeof(modBuf),"Gui_L");
					break;
				case ESC:
					snprintf(modBuf,sizeof(modBuf),"Esc");
					break;
				case ENTER:
					snprintf(modBuf,sizeof(modBuf),"Enter");
					break;
				default:
					std::cerr << "ERROR: unrecognized modifier" << std::endl;
			}
			if(cmd==KEY_DN_CMD) {
				std::cout << "key down: " << (int)key << std::endl;
				xdo_send_keysequence_window_down(xdo, CURRENTWINDOW, modBuf, 0);
			}
			else if(cmd==KEY_UP_CMD) {
				std::cout << "key up: " << (int)key << std::endl;
				xdo_send_keysequence_window_up(xdo, CURRENTWINDOW, modBuf, 0);
			}
		}
		else if(cmd==MOUSE_MV_CMD) {
			xDist = buf[1];
			yDist = buf[2];
			printf("xDist: %i\n",xDist);
			printf("yDist: %i\n",yDist);

			xdo_move_mouse_relative(xdo, xDist, yDist);
		}
		else if(cmd==MOUSE_DN_CMD) {
			button = (int)buf[1];
			std::cout << "mouse btn: " << button << std::endl;
			xdo_mouse_down(xdo, CURRENTWINDOW,button);
		}
		else if(cmd==MOUSE_UP_CMD) {
			button = (int)buf[1];
			std::cout << "mouse btn: " << button << std::endl;
			xdo_mouse_up(xdo, CURRENTWINDOW,button);
		}
		else
			std::cerr << "ERROR: unrecognized command" << std::endl;
	}

	hid_close(handle);

	/* Free static HIDAPI objects. */
	hid_exit();

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
