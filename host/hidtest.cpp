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
	char cmd,key;
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	hid_device *handle;
	int i;
	char outKey;
	std::string keyStr;
	std::map<std::string,char> gestMap;
	xdo_t * xdo = xdo_new(":0.0");

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
		key = buf[1];
		printf("cmd: %i\n",cmd);
		printf("key: %c\n",key);

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

	hid_close(handle);

	/* Free static HIDAPI objects. */
	hid_exit();

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
