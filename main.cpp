#include "WindowsThemeColorApi.h"
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <versionhelpers.h>
#include <windows.h>

std::mutex mutex;
COLORREF dwmColor;
COLORREF accentColor;
volatile BOOL do_stop;

static DWORD get_rgb_from_index(unsigned int index) {
	if(index < 256) {
		return RGB(255, index, 0);
	}

	index -= 256;
	if(index < 256) {
		return RGB(255 - index, 255, 0);
	}

	index -= 256;
	if(index < 256) {
		return RGB(0, 255, index);
	}

	index -= 256;
	if(index < 256) {
		return RGB(0, 255 - index, 255);
	}

	index -= 256;
	if(index < 256) {
		return RGB(index, 0, 255);
	}

	index -= 256;
	if(index < 256) {
		return RGB(255, 0, 255 - index);
	}

	return 0;
}

BOOL WINAPI consoleHandler2(DWORD signal) {
	mutex.lock();

	printf("Closing\n");
	SetDwmColorizationColor(dwmColor);
	SetAccentColor(accentColor);
	SetAutoColorAccentAlgorithm(true);

	do_stop = TRUE;

	mutex.unlock();
	exit(0);

	return TRUE;
}
int main(int argc, char* argv[]) {
	do_stop = FALSE;

	if(!SetConsoleCtrlHandler(consoleHandler2, TRUE)) {
		printf("\nERROR: Could not set control handler");
		return 1;
	}

	InitWindowsThemeColorApi();

	dwmColor = GetDwmColorizationColor();
	accentColor = GetAccentColor();

	SetAutoColorAccentAlgorithm(false);

	unsigned int period = 39;
	unsigned int color_change_per_period = 10;

	if(IsWindows10OrGreater()) {
		period = 1000;
		color_change_per_period = 256;
	}

	for(int index = 1; index < argc; index++) {
		if(!strcmp(argv[index], "--period") && (index + 1) < argc) {
			period = strtoul(argv[index + 1], NULL, 0);
			index++;
		} else if(!strcmp(argv[index], "--speed") && index + 1 < argc) {
			color_change_per_period = strtoul(argv[index + 1], NULL, 0);
			index++;
		} else if(!strcmp(argv[index], "--help")) {
			printf("Usage: %s --period PERIOD --speed SPEED\n"
			       " --period PERIOD      Period between each color change in ms (default: 1s)\n"
			       " --speed SPEED        Color hue change per period (color hue max is 1536)\n"
			       "                      (default: 5)\n",
			       argv[0]);
			return 0;
		}
	}

	while(do_stop == FALSE) {
		static unsigned int index = 30;

		mutex.lock();

		if(!do_stop) {
			COLORREF color = get_rgb_from_index(index);
			index = (index + color_change_per_period) % (6 * 256);

			/*printf("Randomize colors to %d %d %d\n",
			       GetRValue(dwmcolor.clrColor),
			       GetGValue(dwmcolor.clrColor),
			       GetBValue(dwmcolor.clrColor));*/

			SetDwmColorizationColor(color);

			if(IsWindows10OrGreater()) {
				SetAccentColor(color);
			}
		}
		mutex.unlock();

		Sleep(period);
	}

	return 0;
}
