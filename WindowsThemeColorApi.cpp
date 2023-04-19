#include "WindowsThemeColorApi.h"
#include <versionhelpers.h>

static HRESULT(WINAPI *DwmGetColorizationParameters)(DWMCOLORIZATIONPARAMS *color);
static HRESULT(WINAPI *DwmSetColorizationParameters)(DWMCOLORIZATIONPARAMS *color, UINT unknown);

static HRESULT(WINAPI *GetUserColorPreference)(IMMERSIVE_COLOR_PREFERENCE *pcpPreference, bool fForceReload);
static HRESULT(WINAPI *SetUserColorPreference)(const IMMERSIVE_COLOR_PREFERENCE *cpcpPreference, bool fForceCommit);

static const COLORREF predefinedColors[] = {
	0x0000B9FF, 0x005648E7, 0x00D77800, 0x00BC9900,
	0x0074757A, 0x00767676, 0x00008CFF, 0x002311E8,
	0x00B16300, 0x009A7D2D, 0x00585A5D, 0x00484A4C,
	0x000C63F7, 0x005E00EA, 0x00D88C8E, 0x00C3B700,
	0x008A7668, 0x007E7969, 0x001050CA, 0x005200C3,
	0x00D6696B, 0x00878303, 0x006B5C51, 0x0059544A,
	0x00013BDA, 0x008C00E3, 0x00B86487, 0x0094B200,
	0x00737C56, 0x00647C64, 0x005069EF, 0x007700BF,
	0x00A94D74, 0x00748501, 0x00606848, 0x00545E52,
	0x003834D1, 0x00B339C2, 0x00C246B1, 0x006ACC00,
	0x00058249, 0x00457584, 0x004343FF, 0x0089009A,
	0x00981788, 0x003E8910, 0x00107C10, 0x005F737E,
};

#define ENSURE_THROW(a, b) \
	if(!(a)) \
		throw b;

#define ENSURE_SUCCEEDED(a) \
	if((a) != 0) \
		throw a;

void InitWindowsThemeColorApi()
{
	HMODULE hDwmApi = LoadLibrary("dwmapi.dll");
	ENSURE_THROW(hDwmApi, GetLastError());

	DwmGetColorizationParameters = reinterpret_cast<decltype(DwmGetColorizationParameters)>(GetProcAddress(hDwmApi, (LPCSTR)127));
	ENSURE_THROW(DwmGetColorizationParameters, GetLastError());

	DwmSetColorizationParameters = reinterpret_cast<decltype(DwmSetColorizationParameters)>(GetProcAddress(hDwmApi, (LPCSTR)131));
	ENSURE_THROW(DwmSetColorizationParameters, GetLastError());

	if(IsWindows10OrGreater()) {
		HMODULE hUxTheme = LoadLibrary("uxtheme.dll");
		ENSURE_THROW(hUxTheme, GetLastError());

		GetUserColorPreference = reinterpret_cast<decltype(GetUserColorPreference)>(
		    GetProcAddress(hUxTheme, "GetUserColorPreference"));
		ENSURE_THROW(GetUserColorPreference, GetLastError());

		SetUserColorPreference = reinterpret_cast<decltype(SetUserColorPreference)>(
		    GetProcAddress(hUxTheme, (LPCSTR) 122));
		ENSURE_THROW(SetUserColorPreference, GetLastError());
	}
}

COLORREF GetDwmColorizationColor() {
	HRESULT hr;

	DWMCOLORIZATIONPARAMS dwmColor;
	hr = DwmGetColorizationParameters(&dwmColor);
	ENSURE_SUCCEEDED(hr);

	COLORREF retReversed = dwmColor.dwColor & 0x00FFFFFF;
	return RGB(GetBValue(retReversed), GetGValue(retReversed), GetRValue(retReversed));
}

void SetDwmColorizationColor(COLORREF color) {
	HRESULT hr;

	DWMCOLORIZATIONPARAMS dwmColor;
	hr = DwmGetColorizationParameters(&dwmColor);
	ENSURE_SUCCEEDED(hr);

	DWORD dwNewColor = (((0xC4) << 24) | ((GetRValue(color)) << 16) | ((GetGValue(color)) << 8) | (GetBValue(color)));
	dwmColor.dwColor = dwNewColor;
	dwmColor.dwAfterglow = dwNewColor;

	hr = DwmSetColorizationParameters(&dwmColor, 0);
	ENSURE_SUCCEEDED(hr);
}

COLORREF GetAccentColor()
{
	HRESULT hr;

	if(!GetUserColorPreference)
		return 0;

	IMMERSIVE_COLOR_PREFERENCE immersiveColorPref;
	hr = GetUserColorPreference(&immersiveColorPref, 0);
	ENSURE_SUCCEEDED(hr);

	return immersiveColorPref.color2 & 0x00FFFFFF;
}

void SetAccentColor(COLORREF color, bool newAccentAlgorithmWorkaround)
{
	HRESULT hr;

	if(!GetUserColorPreference)
		return;

	IMMERSIVE_COLOR_PREFERENCE immersiveColorPref;
	hr = GetUserColorPreference(&immersiveColorPref, 0);
	ENSURE_SUCCEEDED(hr);

	color &= 0x00FFFFFF;

	if(newAccentAlgorithmWorkaround)
	{
		// This is a hack to make NewAutoColorAccentAlgorithm actually work:
		// if the color is one of the predefined colors, change it slightly.

		for(int i = 0; i < _countof(predefinedColors); i++)
		{
			if(color == predefinedColors[i])
			{
				color = RGB(GetRValue(color), GetGValue(color), GetBValue(color) + 1);
				break;
			}
		}
	}

	immersiveColorPref.color2 = color;

	hr = SetUserColorPreference(&immersiveColorPref, 0);
	ENSURE_SUCCEEDED(hr);
}

bool IsNewAutoColorAccentAlgorithm() {
	DWORD dwBufferSize = sizeof(DWORD);
	DWORD nResult = 0;
	LONG nError = RegGetValueA(HKEY_CURRENT_USER,
	                           "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
	                           "UseNewAutoColorAccentAlgorithm",
	                           RRF_RT_REG_DWORD,
	                           NULL,
	                           &nResult,
	                           &dwBufferSize);

	if(nError == ERROR_FILE_NOT_FOUND || nError == ERROR_PATH_NOT_FOUND) {
		return true;
	}

	ENSURE_SUCCEEDED(nError);

	bool newAutoColorAccentAlgorithm = nResult != 0;

	if(!newAutoColorAccentAlgorithm)
	{
		// For predefined colors, the new algorithm is always used.

		COLORREF accentColor = GetAccentColor();

		for(size_t i = 0; i < _countof(predefinedColors); i++) {
			if(accentColor == predefinedColors[i])
			{
				newAutoColorAccentAlgorithm = true;
				break;
			}
		}
	}

	return newAutoColorAccentAlgorithm;
}

void SetAutoColorAccentAlgorithm(bool bNewAlgorithm)
{
	DWORD dwError;

	HKEY hKey;
	dwError = RegCreateKey(HKEY_CURRENT_USER,
	                       "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
	                       &hKey);

	ENSURE_SUCCEEDED(dwError);

	if(bNewAlgorithm)
	{
		dwError = RegDeleteValue(hKey, "UseNewAutoColorAccentAlgorithm");
		if(dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_PATH_NOT_FOUND) {
			ENSURE_SUCCEEDED(dwError);
		}
	}
	else
	{
		DWORD value = 0;
		dwError = RegSetValueExA(hKey,
		                         "UseNewAutoColorAccentAlgorithm",
		                         0,
		                         REG_DWORD,
		                         (const BYTE*) &value,
		                         sizeof(value));
		ENSURE_SUCCEEDED(dwError);
	}

	RegCloseKey(hKey);
}
