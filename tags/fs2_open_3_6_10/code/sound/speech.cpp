/*
 * Code created by Thomas Whittaker (RT) for a FreeSpace 2 source code project
 *
 * You may not sell or otherwise commercially exploit the source or things you 
 * created based on the source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/sound/speech.cpp $
 * $Revision: 1.25 $
 * $Date: 2006-04-20 06:32:30 $
 * $Author: Goober5000 $
 *
 * Platform specific text-to-speech functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.24  2006/01/21 02:22:05  wmcoolmon
 * Scripting updates; Special scripting image list; Better operator meta; Orientation type; Wing type; Texture type. Fix for MSVC7 compiling.
 *
 * Revision 1.23  2006/01/20 07:10:33  Goober5000
 * reordered #include files to quash Microsoft warnings
 * --Goober5000
 *
 * Revision 1.22  2005/07/13 03:35:29  Goober5000
 * remove PreProcDefine #includes in FS2
 * --Goober5000
 *
 * Revision 1.21  2005/07/02 19:39:23  taylor
 * fix compiler warnings introduced by recent stub changes (defining an empty stub as something is wrong, it should be nothing)
 * assuming that GetStatus thing wasn't tested since it would never have worked
 *
 * Revision 1.20  2005/06/30 02:35:00  Goober5000
 * added functions to check if speech is playing
 * --Goober5000
 *
 * Revision 1.19  2005/04/19 06:29:28  taylor
 * try not to buffer output to the speech dev so that the text doesn't just show up some time in the future
 *
 * Revision 1.18  2005/03/29 06:36:37  wmcoolmon
 * Added some comments to #defines to make reading easier
 *
 * Revision 1.17  2005/03/27 08:12:53  wmcoolmon
 * Bizarre combo of fopen() and close()
 *
 * Revision 1.16  2005/02/02 10:36:23  taylor
 * merge with Linux/OSX tree - p0202
 *
 *
 * $NoKeywords: $
 */



#ifndef FS2_SPEECH
#ifdef _WIN32
#if NDEBUG
	#pragma message( "WARNING: You have not compiled speech into this build (use FS2_SPEECH)" )
#endif // NDEBUG
#endif // _WIN32
#else // to end-of-file ...


#ifdef LAUNCHER
#include "stdafx.h"
#endif	//LAUNCHER

#ifdef _WIN32
	#include <windows.h>
	#include <sapi.h>
	#include <sphelper.h>

	ISpVoice *Voice_device;
#elif defined(SCP_UNIX)
	#include <fcntl.h>
//	#include <stdio.h>

	int speech_dev = -1;
//	FILE *speech_dev = NULL;
#else 
	#pragma error( "ERROR: Unknown platform, speech (FS2_SPEECH) is not supported" )
#endif	//_WIN32

#include "globalincs/pstypes.h"
#include "speech.h"


bool Speech_init = false;

bool speech_init()
{
#ifdef _WIN32
    HRESULT hr = CoCreateInstance(
		CLSID_SpVoice, 
		NULL, 
		CLSCTX_ALL, 
		IID_ISpVoice, 
		(void **)&Voice_device);

	Speech_init = SUCCEEDED(hr);
#else

	speech_dev = open("/dev/speech", O_WRONLY | O_DIRECT);
//	speech_dev = fopen("/dev/speech", "w");

	if (speech_dev == -1) {
//	if (speech_dev == NULL) {
		mprintf(("Couldn't open '/dev/speech', turning text-to-speech off...\n"));
		return false;
	}

	Speech_init = true;
#endif

	return Speech_init;
}

void speech_deinit()
{
	if(Speech_init == false) return;

#ifdef _WIN32
	Voice_device->Release();
#else
	close(speech_dev);
//	fclose(speech_dev);
#endif
}

bool speech_play(char *text)
{
	if(Speech_init == false) return true;
	if(text == NULL) return false;

#ifdef _WIN32
	int len = strlen(text);
	unsigned short Conversion_buffer[MAX_SPEECH_CHAR_LEN];

	if(len > (MAX_SPEECH_CHAR_LEN - 1)) {
		len = MAX_SPEECH_CHAR_LEN - 1;
	}

	int count = 0;
	for(int i = 0; i < len; i++) {
		if(text[i] == '$') {
			i++;
			continue;
		}

		Conversion_buffer[count] = (unsigned short) text[i];
		count++;
	}

	Conversion_buffer[count] = '\0';

	speech_stop();
	return SUCCEEDED(Voice_device->Speak(Conversion_buffer, SPF_ASYNC, NULL));
#else
	int len = strlen(text);
	char Conversion_buffer[MAX_SPEECH_CHAR_LEN];

	if(len > (MAX_SPEECH_CHAR_LEN - 1)) {
		len = MAX_SPEECH_CHAR_LEN - 1;
	}

	int count = 0;
	for(int i = 0; i < len; i++) {
		if(text[i] == '$') {
			i++;
			continue;
		}

		Conversion_buffer[count] = text[i];
		count++;
	}

	Conversion_buffer[count] = '\0';

	if ( write(speech_dev, Conversion_buffer, count) == -1 )
		return false;
//	if (fwrite(Conversion_buffer, count, 1, speech_dev))
//		fflush(speech_dev);
//	else
//		return false;

	return true;
#endif	//_WIN32
}

bool speech_pause()
{
	if(Speech_init == false) return true;
#ifdef _WIN32
	return SUCCEEDED(Voice_device->Pause());
#else
	STUB_FUNCTION;

	return true;
#endif
}

bool speech_resume()
{
	if(Speech_init == false) return true;
#ifdef _WIN32
	return SUCCEEDED(Voice_device->Resume());
#else
	STUB_FUNCTION;

	return true;
#endif
}

bool speech_stop()
{
	if(Speech_init == false) return true;
#ifdef _WIN32
    return SUCCEEDED(Voice_device->Speak( NULL, SPF_PURGEBEFORESPEAK, NULL ));
#else
	STUB_FUNCTION;

	return true;
#endif
}

bool speech_set_volume(unsigned short volume)
{
#ifdef _WIN32
    return SUCCEEDED(Voice_device->SetVolume(volume));
#else
	STUB_FUNCTION;

	return true;
#endif
}

bool speech_set_voice(int voice)
{
#ifdef _WIN32
	HRESULT                             hr;
	CComPtr<ISpObjectToken>             cpVoiceToken;
	CComPtr<IEnumSpObjectTokens>        cpEnum;
	ULONG                               num_voices = 0;

	//Enumerate the available voices 
	hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);

	if(FAILED(hr)) return false;

    hr = cpEnum->GetCount(&num_voices);

	if(FAILED(hr)) return false;

	int count = 0;
	// Obtain a list of available voice tokens, set the voice to the token, and call Speak
	while (num_voices -- )
	{
		cpVoiceToken.Release();
		
		hr = cpEnum->Next( 1, &cpVoiceToken, NULL );

		if(FAILED(hr)) {
			return false;
		}

		if(count == voice) {
			return SUCCEEDED(Voice_device->SetVoice(cpVoiceToken));
		}

		count++;
	}
	return false;
#else
	STUB_FUNCTION;

	return true;
#endif
}

// Goober5000
bool speech_is_speaking()
{
#ifdef _WIN32
	HRESULT			hr;
	SPVOICESTATUS	pStatus;

	hr = Voice_device->GetStatus(&pStatus, NULL);
	if (FAILED(hr)) return false;

	return (pStatus.dwRunningState == SPRS_IS_SPEAKING);
#else
	STUB_FUNCTION;

	return false;
#endif
}

#endif // FS2_SPEECH
