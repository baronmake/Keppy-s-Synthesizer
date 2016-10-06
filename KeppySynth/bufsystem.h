/*
Keppy's Synthesizer buffer system
*/

static BYTE gs_part_to_ch[16];
static BYTE drum_channels[16];
static const char sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const char sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const char sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static void ResetDrumChannels()
{
	static const BYTE part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

	unsigned i;

	memset(drum_channels, 0, sizeof(drum_channels));
	drum_channels[9] = 1;

	memcpy(gs_part_to_ch, part_to_ch, sizeof(gs_part_to_ch));

	if (hStream)
	{
		for (i = 0; i < 16; ++i)
		{
			BASS_MIDI_StreamEvent(hStream, i, MIDI_EVENT_DRUMS, drum_channels[i]);
		}
	}
}

int bmsyn_buf_check(void){
	int retval;
	int retvalemu;
	EnterCriticalSection(&mim_section);
	retval = (evbrpoint != evbwpoint) ? ~0 : 0;
	retvalemu = evbcount;
	LeaveCriticalSection(&mim_section);
	if (vms2emu == 1) {
		return retvalemu;
	}
	else {
		return retval;
	}
}

bool depends() {
	if (vms2emu == 1) {
		return InterlockedDecrement(&evbcount);
	}
	else {
		return bmsyn_buf_check();
	}
}

int bmsyn_play_some_data(void){
	UINT uDeviceID;
	UINT uMsg;
	DWORD_PTR	dwParam1;
	DWORD_PTR   dwParam2;

	UINT evbpoint;
	int exlen;
	unsigned char *sysexbuffer;

	if (!bmsyn_buf_check()){
		return ~0;
	}
	do{
		EnterCriticalSection(&mim_section);
		evbpoint = evbrpoint;

		if (++evbrpoint >= newevbuffvalue) {
			evbrpoint -= newevbuffvalue;
		}

		uDeviceID = evbuf[evbpoint].uDeviceID;
		uMsg = evbuf[evbpoint].uMsg;
		dwParam1 = evbuf[evbpoint].dwParam1;
		dwParam2 = evbuf[evbpoint].dwParam2;
		exlen = evbuf[evbpoint].exlen;
		sysexbuffer = evbuf[evbpoint].sysexbuffer;

		LeaveCriticalSection(&mim_section);
		switch (uMsg) {
		case MODM_DATA:
			dwParam2 = dwParam1 & 0xF0;
			exlen = (dwParam2 >= 0xF8 && dwParam2 <= 0xFF) ? 1 : ((dwParam2 == 0xC0 || dwParam2 == 0xD0) ? 2 : 3);
			BASS_MIDI_StreamEvents(hStream, BASS_MIDI_EVENTS_RAW, &dwParam1, exlen);
			break;
		case MODM_LONGDATA:
			if (!sysexignore) {
				BASS_MIDI_StreamEvents(hStream, BASS_MIDI_EVENTS_RAW, sysexbuffer, exlen);
				if ((exlen == _countof(sysex_gm_reset) && !memcmp(sysexbuffer, sysex_gm_reset, _countof(sysex_gm_reset))) ||
					(exlen == _countof(sysex_gs_reset) && !memcmp(sysexbuffer, sysex_gs_reset, _countof(sysex_gs_reset))) ||
					(exlen == _countof(sysex_xg_reset) && !memcmp(sysexbuffer, sysex_xg_reset, _countof(sysex_xg_reset)))) {
					ResetDrumChannels();
				}
				else if (exlen == 11 &&
					sysexbuffer[0] == (char)0xF0 && sysexbuffer[1] == 0x41 && sysexbuffer[3] == 0x42 &&
					sysexbuffer[4] == 0x12 && sysexbuffer[5] == 0x40 && (sysexbuffer[6] & 0xF0) == 0x10 &&
					sysexbuffer[10] == (char)0xF7)
				{
					if (sysexbuffer[7] == 2)
					{
						// GS MIDI channel to part assign
						gs_part_to_ch[sysexbuffer[6] & 15] = sysexbuffer[8];
					}
					else if (sysexbuffer[7] == 0x15)
					{
						// GS part to rhythm allocation
						unsigned int drum_channel = gs_part_to_ch[sysexbuffer[6] & 15];
						if (drum_channel < 16)
						{
							drum_channels[drum_channel] = sysexbuffer[8];
						}
					}
				}
			}
			free(sysexbuffer);
			break;
		}
	} while (depends());
	return 0;
}

bool modmdata(UINT evbpoint, UINT uMsg, UINT uDeviceID, DWORD_PTR dwParam1, DWORD_PTR dwParam2, int exlen, unsigned char *sysexbuffer) {
	EnterCriticalSection(&mim_section);
	evbpoint = evbwpoint;
	if (++evbwpoint >= newevbuffvalue)
		evbwpoint -= newevbuffvalue;
	evbuf[evbpoint].uDeviceID = !!uDeviceID;
	evbuf[evbpoint].uMsg = uMsg;
	evbuf[evbpoint].dwParam1 = dwParam1;
	evbuf[evbpoint].dwParam2 = dwParam2;
	evbuf[evbpoint].exlen = exlen;
	evbuf[evbpoint].sysexbuffer = sysexbuffer;
	LeaveCriticalSection(&mim_section);
	if (vms2emu == 1) {
		if (InterlockedIncrement(&evbcount) >= newevbuffvalue) {
			do
			{
				Sleep(1);
			} while (evbcount >= newevbuffvalue);
		}
	}
	return MMSYSERR_NOERROR;
}

bool longmodmdata(MIDIHDR *IIMidiHdr, UINT uDeviceID, DWORD_PTR dwParam1, DWORD_PTR dwParam2, int exlen, unsigned char *sysexbuffer) {
	IIMidiHdr = (MIDIHDR *)dwParam1;
	if (!(IIMidiHdr->dwFlags & MHDR_PREPARED)) return MIDIERR_UNPREPARED;
	IIMidiHdr->dwFlags &= ~MHDR_DONE;
	IIMidiHdr->dwFlags |= MHDR_INQUEUE;
	exlen = (int)IIMidiHdr->dwBufferLength;
	if (NULL == (sysexbuffer = (unsigned char *)malloc(exlen * sizeof(char)))) {
		return MMSYSERR_NOMEM;
	}
	else {
		memcpy(sysexbuffer, IIMidiHdr->lpData, exlen);
	}
	IIMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	IIMidiHdr->dwFlags |= MHDR_DONE;
}

void AudioRender() {
	decoded = BASS_ChannelGetData(hStream, sndbf, BASS_DATA_FLOAT + newsndbfvalue * sizeof(float));
	if (encmode == 1) {

	}
	else if (encmode == 0) {
		for (unsigned i = 0, j = decoded / sizeof(float); i < j; i++) {
			sndbf[i] *= sound_out_volume_float;
		}
		sound_driver->write_frame(sndbf, decoded / sizeof(float), true);
	}
}