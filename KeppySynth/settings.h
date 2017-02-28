/*
Keppy's Synthesizer settings loading system
*/

struct evbuf_t{
	UINT   uDeviceID;
	UINT   uMsg;
	DWORD_PTR	dwParam1;
	DWORD_PTR	dwParam2;
	int exlen;
	unsigned char *sysexbuffer;
};

static struct evbuf_t evbuf[36864];
static UINT  evbwpoint = 0;
static UINT  evbrpoint = 0;
static volatile LONG evbcount = 0;
static UINT evbsysexpoint;

void crashmessage(LPCWSTR part) {
	TCHAR errormessage[MAX_PATH] = L"The driver encountered a serious error at this point: ";
	TCHAR clickokmsg[MAX_PATH] = L"\n\nPlease take a screenshot of this messagebox (ALT+PRINT) and send it to KaleidonKep99 through an issue on GitHub.\nClick OK to close the program.";
	lstrcat(errormessage, part);
	lstrcat(errormessage, clickokmsg);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
	std::cout << "(Error at \"" << part << "\") - Fatal error during the execution of the driver." << std::endl;
	MessageBox(NULL, errormessage, L"Keppy's Synthesizer - Fatal execution error", MB_ICONERROR | MB_SYSTEMMODAL);
	exit(0);
}

void DLLLoadError(LPCWSTR dll) {
	TCHAR errormessage[MAX_PATH] = L"There was an error while trying to load the DLL for the driver!\nFaulty/missing DLL: ";
	TCHAR clickokmsg[MAX_PATH] = L"\n\nClick OK to close the program.";
	lstrcat(errormessage, dll);
	lstrcat(errormessage, clickokmsg);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
	std::cout << "(Invalid DLL: " << dll << ") " << " - Fatal error during the loading process of the following DLL." << std::endl;
	MessageBox(NULL, errormessage, L"Keppy's Synthesizer - DLL load error", MB_ICONERROR | MB_SYSTEMMODAL);
	exit(0);
}

void DLLLoadError2(LPCWSTR dll) {
	TCHAR errormessage[MAX_PATH] = L"BASS VST hasn't been loaded because Microsoft Visual C++ 2010 is missing from your computer.\nIt's not mandatory, but you'll not be able to use LoudMax.";
	TCHAR clickokmsg[MAX_PATH] = L"\n\nClick OK to continue.";
	lstrcat(errormessage, clickokmsg);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
	std::cout << "(Invalid DLL: " << dll << ") " << " - Fatal error during the loading process of the following DLL." << std::endl;
	MessageBox(NULL, errormessage, L"Keppy's Synthesizer - DLL load error", MB_ICONASTERISK | MB_SYSTEMMODAL);
	exit(0);
}

void ResetSynth(int ischangingbuffermode){
	reset_synth = 1;
	if (ischangingbuffermode == 1) {
		evbwpoint = 0;
		evbrpoint = 0;
		evbcount = 0;
	}
	BASS_MIDI_StreamEvent(hStream, 0, MIDI_EVENT_SYSTEMEX, MIDI_SYSTEM_DEFAULT);
	reset_synth = 0;
}

void LoadSoundfont(int whichsf){
	if (noaudiodevices == 1) {
		// Do nothing
	}
	else {
		try {
			PrintToConsole(FOREGROUND_RED, whichsf, "Loading soundfont list...");
			TCHAR config[MAX_PATH];
			BASS_MIDI_FONT * mf;
			HKEY hKey;
			long lResult;
			DWORD dwType = REG_DWORD;
			DWORD dwSize = sizeof(DWORD);
			lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer\\Watchdog", 0, KEY_ALL_ACCESS, &hKey);
			FreeFonts(0);
			RegSetValueEx(hKey, L"currentsflist", 0, dwType, (LPBYTE)&whichsf, sizeof(whichsf));
			RegCloseKey(hKey);
			LoadFonts(0, sflistloadme[whichsf - 1]);
			BASS_MIDI_StreamLoadSamples(hStream);
			PrintToConsole(FOREGROUND_RED, whichsf, "Done.");
		}
		catch (...) {
			crashmessage(L"SFLoad");
		}
	}
}

bool LoadSoundfontStartup() {
	if (noaudiodevices == 1) {
		return TRUE;
	}
	else {
		try {
			int done = 0;
			TCHAR modulename[MAX_PATH];
			TCHAR fullmodulename[MAX_PATH];
			GetModuleFileName(NULL, modulename, MAX_PATH);
			GetModuleFileName(NULL, fullmodulename, MAX_PATH);
			PathStripPath(modulename);

			for (int i = 0; i <= 15; ++i) {
				SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, listsloadme[i]);
				SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, sflistloadme[i]);
				_tcscat(sflistloadme[i], sfdirs[i]);
				_tcscat(listsloadme[i], listsanalyze[i]);
				std::wifstream file(listsloadme[i]);
				if (file) {
					TCHAR defaultstring[MAX_PATH];
					while (file.getline(defaultstring, sizeof(defaultstring) / sizeof(*defaultstring)))
					{
						if (_tcsicmp(modulename, defaultstring) && _tcsicmp(fullmodulename, defaultstring) == 0) {
							LoadSoundfont(i + 1);
							done = 1;
							PrintToConsole(FOREGROUND_RED, i, "Found it");
						}
					}
				}
			}

			if (done == 1) {
				return TRUE;
			}
			else {
				return FALSE;
			}
		}
		catch (...) {
			crashmessage(L"SFLoadStartup");
		}
	}
}

BOOL load_bassfuncs()
{
	try {
		TCHAR installpath[MAX_PATH] = { 0 };
		TCHAR bassencpath[MAX_PATH] = { 0 };
		TCHAR bassencpathalt[MAX_PATH] = { 0 };
		TCHAR bassmidipath[MAX_PATH] = { 0 };
		TCHAR bassmidipathalt[MAX_PATH] = { 0 };
		TCHAR basspath[MAX_PATH] = { 0 };
		TCHAR basspathalt[MAX_PATH] = { 0 };
		TCHAR bassvstpath[MAX_PATH] = { 0 };
		TCHAR bassvstpathalt[MAX_PATH] = { 0 };
		int installpathlength;

		GetModuleFileName(hinst, installpath, MAX_PATH);
		PathRemoveFileSpec(installpath);

		SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, basspathalt);
		SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, bassmidipathalt);
		SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, bassencpathalt);
		SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, bassvstpathalt);

#if defined(_WIN64)
		PathAppend(basspathalt, _T("\\Keppy's Synthesizer\\dlloverride\\64\\bass.dll"));
		PathAppend(bassmidipathalt, _T("\\Keppy's Synthesizer\\dlloverride\\64\\bassmidi.dll"));
		PathAppend(bassencpathalt, _T("\\Keppy's Synthesizer\\dlloverride\\64\\bassenc.dll"));
		PathAppend(bassvstpathalt, _T("\\Keppy's Synthesizer\\dlloverride\\64\\bass_vst.dll"));
#elif defined(_WIN32)
		PathAppend(basspathalt, _T("\\Keppy's Synthesizer\\dlloverride\\32\\bass.dll"));
		PathAppend(bassmidipathalt, _T("\\Keppy's Synthesizer\\dlloverride\\32\\bassmidi.dll"));
		PathAppend(bassencpathalt, _T("\\Keppy's Synthesizer\\dlloverride\\32\\bassenc.dll"));
		PathAppend(bassvstpathalt, _T("\\Keppy's Synthesizer\\dlloverride\\32\\bass_vst.dll"));
#endif

		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		PrintToConsole(FOREGROUND_RED, 1, "Allocating memory for BASS DLLs...");

		// BASS
		if (PathFileExists(basspathalt)) {
			if (!(bass = LoadLibrary(basspathalt))) {
				DLLLoadError(basspathalt);
				exit(0);
			}
			isoverrideenabled = 1;
		}
		else {
			lstrcat(basspath, installpath);
			lstrcat(basspath, L"\\bass.dll");
			if (!(bass = LoadLibrary(basspath))) {
				DLLLoadError(basspath);
				exit(0);
			}
		}

		// BASSMIDI
		if (PathFileExists(bassmidipathalt)) {
			if (!(bassmidi = LoadLibrary(bassmidipathalt))) {
				DLLLoadError(bassmidipathalt);
				exit(0);
			}
			isoverrideenabled = 1;
		}
		else {
			lstrcat(bassmidipath, installpath);
			lstrcat(bassmidipath, L"\\bassmidi.dll");
			if (!(bassmidi = LoadLibrary(bassmidipath))) {
				DLLLoadError(bassmidipath);
				exit(0);
			}
		}

		// BASSenc
		if (PathFileExists(bassencpathalt)) {
			if (!(bassenc = LoadLibrary(bassencpathalt))) {
				DLLLoadError(bassencpathalt);
				exit(0);
			}
			isoverrideenabled = 1;
		}
		else {
			lstrcat(bassencpath, installpath);
			lstrcat(bassencpath, L"\\bassenc.dll");
			if (!(bassenc = LoadLibrary(bassencpath))) {
				DLLLoadError(bassencpath);
				exit(0);
			}
		}

		// BASS_VST
		if (PathFileExists(bassvstpathalt)) {
			if (!(bass_vst = LoadLibrary(bassvstpathalt))) {
				isbassvstloaded = 0;
				DLLLoadError2(bassvstpath);
			}
			else {
				isbassvstloaded = 1;
			}
			isoverrideenabled = 1;
		}
		else {
			lstrcat(bassvstpath, installpath);
			lstrcat(bassvstpath, L"\\bass_vst.dll");
			if (!(bass_vst = LoadLibrary(bassvstpath))) {
				isbassvstloaded = 0;
				DLLLoadError2(bassvstpath);
			}
			else {
				isbassvstloaded = 1;
			}
		}

		PrintToConsole(FOREGROUND_RED, 1, "Done loading BASS DLLs.");

		/* "load" all the BASS functions that are to be used */
		PrintToConsole(FOREGROUND_RED, 1, "Loading BASS functions...");
		LOADBASSENCFUNCTION(BASS_Encode_Start);
		LOADBASSENCFUNCTION(BASS_Encode_Stop);
		LOADBASSFUNCTION(BASS_ChannelFlags);
		LOADBASSFUNCTION(BASS_ChannelGetAttribute);
		LOADBASSFUNCTION(BASS_ChannelSetDevice);
		LOADBASSFUNCTION(BASS_ChannelGetData);
		LOADBASSFUNCTION(BASS_ChannelGetLevel);
		LOADBASSFUNCTION(BASS_ChannelPlay);
		LOADBASSFUNCTION(BASS_ChannelStop);
		LOADBASSFUNCTION(BASS_ChannelRemoveFX);
		LOADBASSFUNCTION(BASS_ChannelSetAttribute);
		LOADBASSFUNCTION(BASS_ChannelSetFX);
		LOADBASSFUNCTION(BASS_ChannelUpdate);
		LOADBASSFUNCTION(BASS_ErrorGetCode);
		LOADBASSFUNCTION(BASS_Free);
		LOADBASSFUNCTION(BASS_GetInfo);
		LOADBASSFUNCTION(BASS_GetDeviceInfo);
		LOADBASSFUNCTION(BASS_Init);
		LOADBASSFUNCTION(BASS_PluginLoad);
		LOADBASSFUNCTION(BASS_SetConfig);
		LOADBASSFUNCTION(BASS_SetDevice);
		LOADBASSFUNCTION(BASS_SetVolume);
		LOADBASSFUNCTION(BASS_StreamFree);
		LOADBASSFUNCTION(BASS_Update);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontFree);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontInit);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontLoad);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamCreate);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamEvent);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamEvents);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamGetEvent);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamLoadSamples);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamSetFonts);
		LOADBASSMIDIFUNCTION(BASS_MIDI_InFree);
		LOADBASSMIDIFUNCTION(BASS_MIDI_InGetDeviceInfo);
		LOADBASSMIDIFUNCTION(BASS_MIDI_InInit);
		LOADBASSMIDIFUNCTION(BASS_MIDI_InStart);
		LOADBASSMIDIFUNCTION(BASS_MIDI_InStop);
		if (isbassvstloaded == 1) {
			LOADBASS_VSTFUNCTION(BASS_VST_ChannelSetDSP);
		}
		PrintToConsole(FOREGROUND_RED, 1, "BASS functions succesfully loaded.");
		return TRUE;
	}
	catch (...) {
		crashmessage(L"BASSDefLoad");
	}
}

void appname() {
	HKEY hKey;
	long lResult;
	DWORD dwType = REG_SZ;
	DWORD dwSize = sizeof(DWORD);
	TCHAR modulename[MAX_PATH];
	TCHAR bitapp[MAX_PATH];
	ZeroMemory(modulename, MAX_PATH * sizeof(TCHAR));
	ZeroMemory(bitapp, MAX_PATH * sizeof(TCHAR));
	GetModuleFileNameEx(GetCurrentProcess(), NULL, modulename, MAX_PATH);
	lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer\\Watchdog", 0, KEY_ALL_ACCESS, &hKey);
#if defined(_WIN64)
	wcscpy(bitapp, L"64-bit");
#elif defined(_WIN32)
	wcscpy(bitapp, L"32-bit");
#endif
	RegSetValueEx(hKey, L"currentapp", 0, dwType, (LPBYTE)&modulename, sizeof(modulename));
	RegSetValueEx(hKey, L"bit", 0, dwType, (LPBYTE)&bitapp, sizeof(bitapp));
	RegCloseKey(hKey);
}

void load_settings()
{
	try {
		PrintToConsole(FOREGROUND_BLUE, 1, "Loading settings from registry...");
		HKEY hKey;
		long lResult;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer\\Settings", 0, KEY_ALL_ACCESS, &hKey);
		RegQueryValueEx(hKey, L"autopanic", NULL, &dwType, (LPBYTE)&autopanic, &dwSize);
		RegQueryValueEx(hKey, L"allhotkeys", NULL, &dwType, (LPBYTE)&allhotkeys, &dwSize);
		RegQueryValueEx(hKey, L"buflen", NULL, &dwType, (LPBYTE)&frames, &dwSize);
		RegQueryValueEx(hKey, L"cpu", NULL, &dwType, (LPBYTE)&maxcpu, &dwSize);
		RegQueryValueEx(hKey, L"defaultsflist", NULL, &dwType, (LPBYTE)&defaultsflist, &dwSize);
		RegQueryValueEx(hKey, L"fadeoutdisable", NULL, &dwType, (LPBYTE)&fadeoutdisable, &dwSize);
		RegQueryValueEx(hKey, L"defaultdev", NULL, &dwType, (LPBYTE)&defaultoutput, &dwSize);
		RegQueryValueEx(hKey, L"defaultmidiindev", NULL, &dwType, (LPBYTE)&defaultmidiindev, &dwSize);
		RegQueryValueEx(hKey, L"midiinenabled", NULL, &dwType, (LPBYTE)&midiinenabled, &dwSize);
		RegQueryValueEx(hKey, L"pitchshift", NULL, &dwType, (LPBYTE)&pitchshift, &dwSize);
		RegQueryValueEx(hKey, L"encmode", NULL, &dwType, (LPBYTE)&encmode, &dwSize);
		RegQueryValueEx(hKey, L"32bit", NULL, &dwType, (LPBYTE)&floatrendering, &dwSize);
		RegQueryValueEx(hKey, L"frequency", NULL, &dwType, (LPBYTE)&frequency, &dwSize);
		RegQueryValueEx(hKey, L"ignorenotes1", NULL, &dwType, (LPBYTE)&ignorenotes1, &dwSize);
		RegQueryValueEx(hKey, L"midivolumeoverride", NULL, &dwType, (LPBYTE)&midivolumeoverride, &dwSize);
		RegQueryValueEx(hKey, L"extra8lists", NULL, &dwType, (LPBYTE)&extra8lists, &dwSize);
		RegQueryValueEx(hKey, L"newevbuffvalue", NULL, &dwType, (LPBYTE)&newevbuffvalue, &dwSize);
		RegQueryValueEx(hKey, L"polyphony", NULL, &dwType, (LPBYTE)&midivoices, &dwSize);
		RegQueryValueEx(hKey, L"monorendering", NULL, &dwType, (LPBYTE)&monorendering, &dwSize);
		RegQueryValueEx(hKey, L"oldbuffersystem", NULL, &dwType, (LPBYTE)&oldbuffermode, &dwSize);
		RegQueryValueEx(hKey, L"preload", NULL, &dwType, (LPBYTE)&preload, &dwSize);
		RegQueryValueEx(hKey, L"rco", NULL, &dwType, (LPBYTE)&rco, &dwSize);
		RegQueryValueEx(hKey, L"sndbfvalue", NULL, &dwType, (LPBYTE)&newsndbfvalue, &dwSize);
		RegQueryValueEx(hKey, L"lovelign", NULL, &dwType, (LPBYTE)&lovel, &dwSize);
		RegQueryValueEx(hKey, L"hivelign", NULL, &dwType, (LPBYTE)&hivel, &dwSize);
		RegQueryValueEx(hKey, L"vmsemu", NULL, &dwType, (LPBYTE)&vmsemu, &dwSize);
		RegQueryValueEx(hKey, L"vms2emu", NULL, &dwType, (LPBYTE)&vms2emu, &dwSize);
		RegQueryValueEx(hKey, L"volume", NULL, &dwType, (LPBYTE)&volume, &dwSize);
		RegQueryValueEx(hKey, L"volumehotkeys", NULL, &dwType, (LPBYTE)&volumehotkeys, &dwSize);
		RegQueryValueEx(hKey, L"sysexignore", NULL, &dwType, (LPBYTE)&sysexignore, &dwSize);
		RegQueryValueEx(hKey, L"allnotesignore", NULL, &dwType, (LPBYTE)&allnotesignore, &dwSize);
		RegQueryValueEx(hKey, L"xaudiodisabled", NULL, &dwType, (LPBYTE)&xaudiodisabled, &dwSize);
		RegQueryValueEx(hKey, L"sinc", NULL, &dwType, (LPBYTE)&sinc, &dwSize);

		if (lovel < 1) { lovel = 1; }
		if (hivel > 127) { hivel = 127; }

		RegCloseKey(hKey);

		appname();

		frequencynew = frequency;

		sndbf = (float *)malloc(newsndbfvalue*sizeof(float));
		memset(evbuf, newevbuffvalue, sizeof(newevbuffvalue));

		sound_out_volume_float = (float)volume / 10000.0f;
		sound_out_volume_int = (int)(sound_out_volume_float * (float)0x1000);

		PrintToConsole(FOREGROUND_BLUE, 1, "Done loading settings from registry.");
	}
	catch (...) {
		crashmessage(L"RegSetLoad");
	}
}

void realtime_load_settings()
{
	try {
		HKEY hKey;
		long lResult;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		int vms2emutemp = vms2emu;
		int oldbuffermodetemp = oldbuffermode;
		int frequencyttemp = frequency;
		int potato;
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer\\Settings", 0, KEY_ALL_ACCESS, &hKey);
		RegQueryValueEx(hKey, L"frequency", NULL, &dwType, (LPBYTE)&frequency, &dwSize);
		RegQueryValueEx(hKey, L"autopanic", NULL, &dwType, (LPBYTE)&autopanic, &dwSize);
		RegQueryValueEx(hKey, L"allhotkeys", NULL, &dwType, (LPBYTE)&allhotkeys, &dwSize);
		RegQueryValueEx(hKey, L"cpu", NULL, &dwType, (LPBYTE)&maxcpu, &dwSize);
		RegQueryValueEx(hKey, L"pitchshift", NULL, &dwType, (LPBYTE)&pitchshift, &dwSize);
		RegQueryValueEx(hKey, L"midivolumeoverride", NULL, &dwType, (LPBYTE)&midivolumeoverride, &dwSize);
		RegQueryValueEx(hKey, L"nofx", NULL, &dwType, (LPBYTE)&nofx, &dwSize);
		RegQueryValueEx(hKey, L"fadeoutdisable", NULL, &dwType, (LPBYTE)&fadeoutdisable, &dwSize);
		RegQueryValueEx(hKey, L"noteoff", NULL, &dwType, (LPBYTE)&noteoff1, &dwSize);
		RegQueryValueEx(hKey, L"polyphony", NULL, &dwType, (LPBYTE)&midivoices, &dwSize);
		RegQueryValueEx(hKey, L"oldbuffersystem", NULL, &dwType, (LPBYTE)&oldbuffermode, &dwSize);
		if (oldbuffermodetemp != oldbuffermode) {
			ResetSynth(0);
		}
		RegQueryValueEx(hKey, L"ignorenotes1", NULL, &dwType, (LPBYTE)&ignorenotes1, &dwSize);
		RegQueryValueEx(hKey, L"preload", NULL, &dwType, (LPBYTE)&preload, &dwSize);
		RegQueryValueEx(hKey, L"rco", NULL, &dwType, (LPBYTE)&rco, &dwSize);
		RegQueryValueEx(hKey, L"sinc", NULL, &dwType, (LPBYTE)&sinc, &dwSize);
		RegQueryValueEx(hKey, L"sysresetignore", NULL, &dwType, (LPBYTE)&sysresetignore, &dwSize);
		RegQueryValueEx(hKey, L"volume", NULL, &dwType, (LPBYTE)&volume, &dwSize);
		RegQueryValueEx(hKey, L"lovelign", NULL, &dwType, (LPBYTE)&lovel, &dwSize);
		RegQueryValueEx(hKey, L"hivelign", NULL, &dwType, (LPBYTE)&hivel, &dwSize);
		RegQueryValueEx(hKey, L"volumehotkeys", NULL, &dwType, (LPBYTE)&volumehotkeys, &dwSize);
		RegQueryValueEx(hKey, L"volumemon", NULL, &dwType, (LPBYTE)&volumemon, &dwSize);
		RegQueryValueEx(hKey, L"vms2emu", NULL, &dwType, (LPBYTE)&vms2emu, &dwSize);
		RegQueryValueEx(hKey, L"sysexignore", NULL, &dwType, (LPBYTE)&sysexignore, &dwSize);
		RegQueryValueEx(hKey, L"allnotesignore", NULL, &dwType, (LPBYTE)&allnotesignore, &dwSize);
		RegQueryValueEx(hKey, L"potato", NULL, &dwType, (LPBYTE)&potato, &dwSize);
		if (vms2emutemp != vms2emu) {
			ResetSynth(1);
		}
		if (potato) {
			if (frequencyttemp != frequency) {
				BASS_ChannelSetAttribute(hStream, BASS_ATTRIB_FREQ, frequency);
			}
		}
		if (lovel < 1) { lovel = 1; }
		if (hivel > 127) { hivel = 127; }
		RegCloseKey(hKey);
		//cake
		if (xaudiodisabled == 1) {
			BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, volume);
		}
		else {
			sound_out_volume_float = (float)volume / 10000.0f;
			sound_out_volume_int = (int)(sound_out_volume_float * (float)0x1000);
		}
		// stuff
		BASS_ChannelSetAttribute(hStream, BASS_ATTRIB_MIDI_CPU, maxcpu);
		BASS_ChannelSetAttribute(hStream, BASS_ATTRIB_MIDI_KILL, fadeoutdisable);
		if (noteoff1) {
			BASS_ChannelFlags(hStream, BASS_MIDI_NOTEOFF1, BASS_MIDI_NOTEOFF1);
		}
		else {
			BASS_ChannelFlags(hStream, 0, BASS_MIDI_NOTEOFF1);
		}
		if (nofx) {
			BASS_ChannelFlags(hStream, BASS_MIDI_NOFX, BASS_MIDI_NOFX);
		}
		else {
			BASS_ChannelFlags(hStream, 0, BASS_MIDI_NOFX);
		}
		if (sysresetignore) {
			BASS_ChannelFlags(hStream, BASS_MIDI_NOSYSRESET, BASS_MIDI_NOSYSRESET);
		}
		else {
			BASS_ChannelFlags(hStream, 0, BASS_MIDI_NOSYSRESET);
		}
		if (sinc) {
			BASS_ChannelFlags(hStream, BASS_MIDI_SINCINTER, BASS_MIDI_SINCINTER);
		}
		else {
			BASS_ChannelFlags(hStream, 0, BASS_MIDI_SINCINTER);
		}
		appname();
	}
	catch (...) {
		crashmessage(L"RTSetLoad");
	}
}

void Panic() {
	//Panic system
	if (autopanic == 1) {
		if (currentcpuusage0 >= 100.0f && currentcpuusage0 < 150.0f) {
			int reduceby;
			if (currentcpuusage0 >= 100.0f && currentcpuusage0 < 110.0f) { reduceby = midivoices / 4; }
			else if (currentcpuusage0 >= 110.0f && currentcpuusage0 < 120.0f) { reduceby = midivoices / 8; }
			else if (currentcpuusage0 >= 120.0f && currentcpuusage0 < 130.0f) { reduceby = midivoices / 16; }
			else if (currentcpuusage0 >= 130.0f && currentcpuusage0 < 140.0f) { reduceby = midivoices / 32; }
			else if (currentcpuusage0 >= 140.0f && currentcpuusage0 < 150.0f) { reduceby = midivoices / 64; }
			BASS_ChannelSetAttribute(hStream, BASS_ATTRIB_MIDI_VOICES, reduceby);
		}
		else if (currentcpuusage0 >= 150.0f) {
			ResetSynth(0);
		}
		else {
			BASS_ChannelSetAttribute(hStream, BASS_ATTRIB_MIDI_VOICES, midivoices);
		}
	}
	else {
		BASS_ChannelSetAttribute(hStream, BASS_ATTRIB_MIDI_VOICES, midivoices);
	}
}

void WatchdogCheck()
{
	try {
		HKEY hKey;
		long lResult;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		DWORD zero = 0;
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer\\Watchdog", 0, KEY_ALL_ACCESS, &hKey);

		for (int i = 0; i <= 15; ++i) {
			RegQueryValueEx(hKey, rnames[i], NULL, &dwType, (LPBYTE)&rvalues[i], &dwSize);
			if (rvalues[i] == 1) {
				LoadSoundfont(i + 1);
				RegSetValueEx(hKey, rnames[i], 0, dwType, (LPBYTE)&zero, sizeof(zero));
			}
		}

		RegCloseKey(hKey);
	}
	catch (...) {
		crashmessage(L"WDCheck");
	}
}

void CheckVolume() {
	HKEY hKey;
	long lResult;
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD level, left, right;
	lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer", 0, KEY_ALL_ACCESS, &hKey);
	if (volumemon == 1) {
		if (xaudiodisabled == 1)
		{
			level = BASS_ChannelGetLevel(hStream);
			left = LOWORD(level); // the left level
			right = HIWORD(level); // the right level
			RegSetValueEx(hKey, L"leftvol", 0, dwType, (LPBYTE)&left, sizeof(left));
			RegSetValueEx(hKey, L"rightvol", 0, dwType, (LPBYTE)&right, sizeof(right));
		}
	}
	RegCloseKey(hKey);
}

void debug_info() {
	try {
		HKEY hKey;
		long lResult;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		DWORD level, left, right;
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer", 0, KEY_ALL_ACCESS, &hKey);
		float currentvoices0;
		int tempo;
		BASS_ChannelGetAttribute(hStream, BASS_ATTRIB_MIDI_VOICES_ACTIVE, &currentvoices0);
		BASS_ChannelGetAttribute(hStream, BASS_ATTRIB_CPU, &currentcpuusage0);

	    currentvoicesint0 = int(currentvoices0);
		int currentcpuusageint0 = int(currentcpuusage0);

		// Things
		RegSetValueEx(hKey, L"currentvoices0", 0, dwType, (LPBYTE)&currentvoicesint0, sizeof(currentvoicesint0));
		RegSetValueEx(hKey, L"currentcpuusage0", 0, dwType, (LPBYTE)&currentcpuusageint0, sizeof(currentcpuusageint0));

		RegCloseKey(hKey);
	}
	catch (...) {
		crashmessage(L"DebugRead");
	}
}

void mixervoid() {
	try {
		HKEY hKey;
		long lResult;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Keppy's Synthesizer\\Channels", 0, KEY_ALL_ACCESS, &hKey);

		for (int i = 0; i <= 15; ++i) {
			RegQueryValueEx(hKey, cnames[i], NULL, &dwType, (LPBYTE)&cvalues[i], &dwSize);
			if (midivolumeoverride == 1) {
				BASS_MIDI_StreamEvent(hStream, i, MIDI_EVENT_VOLUME, cvalues[i]);
			}
			else {
				if (cvalues[i] != tcvalues[i]) {
					BASS_MIDI_StreamEvent(hStream, i, MIDI_EVENT_VOLUME, cvalues[i]);
					tcvalues[i] = cvalues[i];
				}
			}
			RegQueryValueEx(hKey, pitchshiftname[i], NULL, &dwType, (LPBYTE)&pitchshiftchan[i], &dwSize);

		}

		RegCloseKey(hKey);
	}
	catch (...) {
		crashmessage(L"MixerCheck");
	}
}

void ReloadSFList(DWORD whichsflist){
	try {
		if (xaudiodisabled == 1) {
			BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, 0);
		}
		else {
			sound_out_volume_float = 0.0f / 10000.0f;
		}
		ResetSynth(0);
		Sleep(100);
		LoadSoundfont(whichsflist);
		if (xaudiodisabled == 1) {
			BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, volume);
		}
		else {
			sound_out_volume_float = (float)volume / 10000.0f;
		}
	}
	catch (...) {
		crashmessage(L"ReloadSFList");
	}
}

void keybindings()
{
	try {
		if (allhotkeys == 1) {
			if (extra8lists == 1) {
				BOOL ControlPressed = (GetAsyncKeyState(VK_CONTROL) & (1 << 15));
				if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x31) & 0x8000) {
					ReloadSFList(1);
					return;
				}
				else if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x32) & 0x8000) {
					ReloadSFList(2);
					return;
				}
				else if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x33) & 0x8000) {
					ReloadSFList(3);
					return;
				}
				else if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x34) & 0x8000) {
					ReloadSFList(4);
					return;
				}
				else if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x35) & 0x8000) {
					ReloadSFList(5);
					return;
				}
				else if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x36) & 0x8000) {
					ReloadSFList(6);
					return;
				}
				else if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x37) & 0x8000) {
					ReloadSFList(7);
					return;
				}
				else if (!ControlPressed && GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x38) & 0x8000) {
					ReloadSFList(8);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x31) & 0x8000) {
					ReloadSFList(9);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x32) & 0x8000) {
					ReloadSFList(10);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x33) & 0x8000) {
					ReloadSFList(11);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x34) & 0x8000) {
					ReloadSFList(12);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x35) & 0x8000) {
					ReloadSFList(13);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x36) & 0x8000) {
					ReloadSFList(14);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x37) & 0x8000) {
					ReloadSFList(15);
					return;
				}
				else if (GetAsyncKeyState(VK_CONTROL) & GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x38) & 0x8000) {
					ReloadSFList(16);
					return;
				}
			}
			else {
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x31) & 0x8000) {
					ReloadSFList(1);
					return;
				}
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x32) & 0x8000) {
					ReloadSFList(2);
					return;
				}
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x33) & 0x8000) {
					ReloadSFList(3);
					return;
				}
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x34) & 0x8000) {
					ReloadSFList(4);
					return;
				}
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x35) & 0x8000) {
					ReloadSFList(5);
					return;
				}
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x36) & 0x8000) {
					ReloadSFList(6);
					return;
				}
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x37) & 0x8000) {
					ReloadSFList(7);
					return;
				}
				if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x38) & 0x8000) {
					ReloadSFList(8);
					return;
				}
				if (debugmode == 1) {
					if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(VK_CANCEL) & 0x8000) {
						COORD topLeft = { 0, 0 };
						CONSOLE_SCREEN_BUFFER_INFO screen;
						DWORD written;

						GetConsoleScreenBufferInfo(hConsole, &screen);
						FillConsoleOutputCharacterA(
							hConsole, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
							);
						FillConsoleOutputAttribute(
							hConsole, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
							screen.dwSize.X * screen.dwSize.Y, topLeft, &written
							);
						SetConsoleCursorPosition(hConsole, topLeft);
					}
				}
			}
			if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x39) & 0x8000) {
				TCHAR configuratorapp[MAX_PATH];
				BOOL run = TRUE;
				if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_SYSTEMX86, NULL, 0, configuratorapp)))
				{
					PathAppend(configuratorapp, _T("\\keppysynth\\KeppySynthMixerWindow.exe"));
					ShellExecute(NULL, L"open", configuratorapp, NULL, NULL, SW_SHOWNORMAL);
					Sleep(10);
					return;
				}
			}
			else if (GetAsyncKeyState(VK_MENU) & GetAsyncKeyState(0x30) & 0x8000) {
				TCHAR configuratorapp[MAX_PATH];
				BOOL run = TRUE;
				if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_SYSTEMX86, NULL, 0, configuratorapp)))
				{
					PathAppend(configuratorapp, _T("\\keppysynth\\KeppySynthDebugWindow.exe"));
					ShellExecute(NULL, L"open", configuratorapp, NULL, NULL, SW_SHOWNORMAL);
					Sleep(10);
					return;
				}
			}
			if (GetAsyncKeyState(VK_INSERT) & 1) {
				ResetSynth(0);
			}
			else {
				// Nothing lel
			}
		}
	}
	catch (...) {
		crashmessage(L"HotKeysCheck");
	}
}