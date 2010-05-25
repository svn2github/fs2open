// TabSound.cpp : implementation file
//

#include "stdafx.h"
#include "launcher.h"
#include "TabSound.h"

#include <mmsystem.h>
#include <vector>
#include <string>
#include <regstr.h>

#include "win32func.h"
#include "settings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int NUM_SND_MODES = 4;

char *sound_card_modes[NUM_SND_MODES] =
{
	"DirectSound",
	"EAX",
	"Aureal A3D",
	"No Sound"
}; 

// OpenAL stuff
typedef struct ALCdevice_struct ALCdevice;
typedef char ALCchar;
typedef int ALCenum;
typedef char ALCboolean;
#define ALC_FALSE		0
#define ALC_TRUE		1
#define ALC_DEFAULT_DEVICE_SPECIFIER	0x1004
#define ALC_DEVICE_SPECIFIER			0x1005
#define ALC_ALL_DEVICES_SPECIFIER		0x1013
#define ALC_CAPTURE_DEVICE_SPECIFIER			0x310
#define ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER	0x311
HINSTANCE hOAL;
const ALCchar* (*alcGetString)( ALCdevice *device, ALCenum param ) = NULL;
ALCboolean     (*alcIsExtensionPresent)( ALCdevice *device, const ALCchar *extname ) = NULL;

std::vector<std::string> OpenAL_playback_devices;
std::vector<std::string> OpenAL_capture_devices;


/////////////////////////////////////////////////////////////////////////////
// CTabSound dialog

CTabSound::CTabSound(CWnd* pParent /*=NULL*/)
	: CDialog(CTabSound::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabSound)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CTabSound::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTabSound)
	DDX_Control(pDX, IDC_JOYSTICK, m_joystick_list);
	DDX_Control(pDX, IDC_SOUND_CARD, m_sound_api_list);
	DDX_Control(pDX, IDC_SOUND_CAPTURE_CARD, m_capture_api_list);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTabSound, CDialog)
	//{{AFX_MSG_MAP(CTabSound)
	ON_BN_CLICKED(IDC_CALIBRATE, OnCalibrate)
	ON_CBN_SELCHANGE(IDC_SOUND_CARD, OnSelChangeSoundDevice)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabSound message handlers

void CTabSound::OnApply()
{
	const int CHECKED = 1; 

	// Sound settings
	int index = m_sound_api_list.GetCurSel();

	char string[256];
	m_sound_api_list.GetLBText(index, string);

	if ( Settings::is_new_sound_build() ) {
		// preferred devices
		reg_set_sz(Settings::reg_path, "Sound\\PlaybackDevice", string);

		index = m_capture_api_list.GetCurSel();
		m_capture_api_list.GetLBText(index, string);

		reg_set_sz(Settings::reg_path, "Sound\\CaptureDevice", string);

		// EFX
		int efx = (((CButton *) GetDlgItem(IDC_EFX))->GetCheck() == CHECKED) ? 1 : 0;
		reg_set_dword(Settings::reg_path, "Sound\\EnableEFX", efx);

		// SampleRate
		char  sample_rate_text[50];
		int   sample_rate_value;
		GetDlgItem(IDC_SAMPLE_RATE)->GetWindowText(sample_rate_text, 50);

		if (strlen(sample_rate_text) > 0) {
			sample_rate_value = atoi(sample_rate_text);
			reg_set_dword(Settings::reg_path, "Sound\\SampleRate", sample_rate_value);
		}
	} else if ( Settings::is_openal_build() ) {
		reg_set_sz(Settings::reg_path, "SoundDeviceOAL", string);
	} else {
		reg_set_sz(Settings::reg_path, "Soundcard", string);
	}


	// Joystick settings
	int ff = (((CButton *) GetDlgItem(IDC_FORCE_FREEDBACK))->GetCheck() == CHECKED) ? 1 : 0;
	int dh = (((CButton *) GetDlgItem(IDC_DIR_HIT))->GetCheck() == CHECKED) ? 1 : 0;
		
	reg_set_dword(Settings::reg_path, "EnableJoystickFF", ff);
	reg_set_dword(Settings::reg_path, "EnableHitEffect", dh);

	// Set joystick
	index = m_joystick_list.GetCurSel();

	if (index < 0)
		index = 0;

    int enum_id = m_joystick_list.GetItemData(index);

	reg_set_dword(Settings::reg_path, "CurrentJoystick", enum_id);
}

void CTabSound::SetupOpenALPlayback()
{
	const char *default_device = (const char*) alcGetString( NULL, ALC_DEFAULT_DEVICE_SPECIFIER );

	if (default_device) {
		OpenAL_playback_devices.push_back(default_device);
	}

	if ( alcIsExtensionPresent(NULL, (const ALCchar*)"ALC_ENUMERATION_EXT") == ALC_TRUE ) {
		const char *all_devices = NULL;

		if ( alcIsExtensionPresent(NULL, (const ALCchar*)"ALC_ENUMERATE_ALL_EXT") == ALC_TRUE ) {
			all_devices = (const char*) alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
		} else {
			all_devices = (const char*) alcGetString(NULL, ALC_DEVICE_SPECIFIER);
		}

		char *pos = (char*)all_devices;

		while (*pos && (strlen(pos) > 0)) {
			// see if this is the default device, which is already in the list
			if (default_device && !strcmp(default_device, pos)) {
				pos += (strlen(pos) + 1);
				continue;
			}

			OpenAL_playback_devices.push_back(pos);

			pos += (strlen(pos) + 1);
		}
	}

	if ( !Settings::is_new_sound_build() ) {
		OpenAL_playback_devices.push_back("no sound");
	}
}

void CTabSound::SetupOpenALCapture()
{
	const char *default_device = (const char*) alcGetString( NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER );

	if (default_device) {
		OpenAL_capture_devices.push_back(default_device);
	}

	if ( alcIsExtensionPresent(NULL, (const ALCchar*)"ALC_ENUMERATION_EXT") == ALC_TRUE ) {
		const char *all_devices = (const char*) alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);

		char *pos = (char*)all_devices;

		while (*pos && (strlen(pos) > 0)) {
			// see if this is the default device, which is already in the list
			if (default_device && !strcmp(default_device, pos)) {
				pos += (strlen(pos) + 1);
				continue;
			}

			OpenAL_capture_devices.push_back(pos);

			pos += (strlen(pos) + 1);
		}
	}
}

void CTabSound::SetupOpenAL()
{
	if ( OpenAL_playback_devices.empty() ) {
		if ( (hOAL = LoadLibrary("OpenAL32.dll")) == 0 ) {
			MessageBox("Build needs OpenAL but an OpenAL DLL cannot be found!", "Error", MB_OK);
			return;
		}

		alcGetString = (const ALCchar* (*)(ALCdevice *device, ALCenum param)) GetProcAddress( hOAL, "alcGetString" );

		if (alcGetString == NULL) {
			FreeLibrary( hOAL );
			MessageBox("Unable to acquire alcGetString pointer!", "Error", MB_OK);
			return;
		}

		alcIsExtensionPresent = (ALCboolean (*)(ALCdevice *device, const ALCchar *extname)) GetProcAddress( hOAL, "alcIsExtensionPresent" );

		if (alcIsExtensionPresent == NULL) {
			FreeLibrary( hOAL );
			MessageBox("Unable to acquire alcIsExtensionPresent pointer!", "Error", MB_OK);
			return;
		}

		SetupOpenALPlayback();

		if ( Settings::is_new_sound_build() ) {
			SetupOpenALCapture();
		}

		FreeLibrary( hOAL );

		alcGetString = NULL;
		alcIsExtensionPresent = NULL;
	}

	// playback devices
	m_sound_api_list.ResetContent();

	unsigned int i;
	int selection = 0;

	for (i = 0; i < OpenAL_playback_devices.size(); i++) {
		m_sound_api_list.InsertString(i, OpenAL_playback_devices[i].c_str());

		if ( !Settings::is_new_sound_build() ) {
			if (  !stricmp(OpenAL_playback_devices[i].c_str(), "Generic Software") ) {
				GetDlgItem(IDC_OAL_WARN_STATIC)->ShowWindow(false);
				selection = i;
			}
		}
	}

	m_sound_api_list.SetCurSel(selection);


	// capture devices
	m_capture_api_list.ResetContent();
	selection = 0;

	for (i = 0; i < OpenAL_capture_devices.size(); i++) {
		m_capture_api_list.InsertString(i, OpenAL_capture_devices[i].c_str());
	}

	m_capture_api_list.SetCurSel(selection);
}

BOOL CTabSound::OnInitDialog() 
{
	int i, list_index;
	JOYCAPS jc;
	JOYINFO ji;
	char joy_name[256];
	TCHAR szKey[256];
	TCHAR szValue[256];
	UCHAR szOEMKey[256];
	HKEY hKey;
	DWORD dwcb;
	LONG lr;

	CDialog::OnInitDialog();

	for (i = 0; i < NUM_SND_MODES; i++)
		m_sound_api_list.InsertString(i, sound_card_modes[i]);

	m_sound_api_list.SetCurSel(0);
	m_capture_api_list.SetCurSel(0);

	GetDlgItem(IDC_OAL_WARN_STATIC)->ShowWindow(false);
	GetDlgItem(IDC_EFX)->ShowWindow(false);
	GetDlgItem(IDC_SAMPLE_RATE_STATIC)->ShowWindow(false);
	GetDlgItem(IDC_SAMPLE_RATE)->ShowWindow(false);
	GetDlgItem(IDC_CAPTURE_STATIC)->ShowWindow(false);
	GetDlgItem(IDC_SOUND_CAPTURE_CARD)->ShowWindow(false);

	// Joystick
	list_index = m_joystick_list.AddString("No Joystick");
	m_joystick_list.SetItemData(list_index, 99999);

	int num_sticks = joyGetNumDevs();

	for (i = 0; i < num_sticks; i++) {
		memset( &jc, 0, sizeof(JOYCAPS) );
		memset( joy_name, 0, sizeof(joy_name) );

		MMRESULT mr = joyGetDevCaps( i, &jc, sizeof(JOYCAPS) );

		// make sure that our device/driver is good
		if (mr != JOYERR_NOERROR)
			continue;

		// if the joystick is unplugged or otherwise not available then just ignore it
		if (joyGetPos(i, &ji) != JOYERR_NOERROR)
			continue;

		// copy basic device name, in case getting the OEM name from the registry fails
		strcpy( joy_name, jc.szPname );

		// now try to grab the full OEM name for this joystick ...

		sprintf(szKey, "%s\\%s\\%s", REGSTR_PATH_JOYCONFIG, jc.szRegKey, REGSTR_KEY_JOYCURR);
		lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPTSTR) &szKey, 0, KEY_QUERY_VALUE, &hKey);

		if (lr != ERROR_SUCCESS)
			goto Done;

		dwcb = sizeof(szOEMKey);
		// NOTE: normal joystick values are 0 based, but it's 1 based in the registry, hence the "+1"
		sprintf(szValue, "Joystick%d%s", i+1, REGSTR_VAL_JOYOEMNAME);
		lr = RegQueryValueEx(hKey, szValue, 0, 0, (LPBYTE) &szOEMKey, (LPDWORD) &dwcb);
		RegCloseKey(hKey);

		if (lr != ERROR_SUCCESS)
			goto Done;

		sprintf(szKey, "%s\\%s", REGSTR_PATH_JOYOEM, szOEMKey);
		lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey);

		if (lr != ERROR_SUCCESS)
			goto Done;

		// grab the OEM name
		dwcb = sizeof(szValue);
		lr = RegQueryValueEx(hKey, REGSTR_VAL_JOYOEMNAME, 0, 0, (LPBYTE) joy_name, (LPDWORD) &dwcb);
		RegCloseKey(hKey);

Done:
		list_index = m_joystick_list.AddString(joy_name);
		m_joystick_list.SetItemData(list_index, i);
	}


	m_joystick_list.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTabSound::LoadSettings()
{
	DWORD ff, dh, efx, sample_rate;

	OpenAL_playback_devices.clear();
	OpenAL_capture_devices.clear();

	GetDlgItem(IDC_OAL_WARN_STATIC)->ShowWindow(false);
	GetDlgItem(IDC_EFX)->ShowWindow(false);
	GetDlgItem(IDC_SAMPLE_RATE_STATIC)->ShowWindow(false);
	GetDlgItem(IDC_SAMPLE_RATE)->ShowWindow(false);
	GetDlgItem(IDC_CAPTURE_STATIC)->ShowWindow(false);
	GetDlgItem(IDC_SOUND_CAPTURE_CARD)->ShowWindow(false);

	if ( Settings::is_new_sound_build() ) {
		GetDlgItem(IDC_EAX_STATIC)->ShowWindow(false);
		GetDlgItem(IDC_CAPTURE_STATIC)->ShowWindow(true);
		GetDlgItem(IDC_SOUND_CAPTURE_CARD)->ShowWindow(true);
		GetDlgItem(IDC_EFX)->ShowWindow(true);
		GetDlgItem(IDC_SAMPLE_RATE_STATIC)->ShowWindow(true);
		GetDlgItem(IDC_SAMPLE_RATE)->ShowWindow(true);

		GetDlgItem(IDC_SOUND_STATIC)->SetWindowText("Preferred Playback Device");
		((CEdit *) GetDlgItem(IDC_SAMPLE_RATE))->SetLimitText(5);
	} else if ( Settings::is_openal_build() ) {
		GetDlgItem(IDC_OAL_WARN_STATIC)->ShowWindow(true);
		GetDlgItem(IDC_EAX_STATIC)->ShowWindow(false);
		GetDlgItem(IDC_SOUND_STATIC)->SetWindowText("Currently selected OpenAL Sound Device");
	} else {
		GetDlgItem(IDC_EAX_STATIC)->ShowWindow(true);
		GetDlgItem(IDC_SOUND_STATIC)->SetWindowText("Currently selected Sound Card");
	}

	if ( Settings::is_openal_build() ) {
		SetupOpenAL();
	}

	reg_get_dword(Settings::reg_path, "EnableJoystickFF", &ff);
	reg_get_dword(Settings::reg_path, "EnableHitEffect", &dh);

	((CButton *) GetDlgItem(IDC_FORCE_FREEDBACK))->SetCheck(ff ? 1 : 0);
	((CButton *) GetDlgItem(IDC_DIR_HIT))->SetCheck(dh ? 1 : 0);

	char local_port_text[256];

	if ( Settings::is_new_sound_build() ) {
		unsigned int i;

		reg_get_sz(Settings::reg_path, "Sound\\PlaybackDevice", local_port_text, 256);

		for (i = 0; i < OpenAL_playback_devices.size(); i++) {
			if ( !strcmp(OpenAL_playback_devices[i].c_str(), local_port_text) ) {
				m_sound_api_list.SetCurSel(i);
				break;
			}
		}

		reg_get_sz(Settings::reg_path, "Sound\\CaptureDevice", local_port_text, 256);

		for (i = 0; i < OpenAL_capture_devices.size(); i++) {
			if ( !strcmp(OpenAL_capture_devices[i].c_str(), local_port_text) ) {
				m_capture_api_list.SetCurSel(i);
				break;
			}
		}

		reg_get_dword(Settings::reg_path, "Sound\\EnableEFX", &efx);
		((CButton *) GetDlgItem(IDC_EFX))->SetCheck(efx ? 1 : 0);

		if ( reg_get_dword(Settings::reg_path, "Sound\\SampleRate", &sample_rate) == true ) {
			sprintf(local_port_text, "%d", sample_rate);
			GetDlgItem(IDC_EFX)->SetWindowText(local_port_text);
		}
	} else if ( Settings::is_openal_build() ) {
		reg_get_sz(Settings::reg_path, "SoundDeviceOAL", local_port_text, 256);

		for (unsigned int i = 0; i < OpenAL_playback_devices.size(); i++) {
			if ( !stricmp(OpenAL_playback_devices[i].c_str(), local_port_text) ) {
				m_sound_api_list.SetCurSel(i);

				// if not software device then show warning message
				if ( stricmp(OpenAL_playback_devices[i].c_str(), "no sound") && 
						stricmp(OpenAL_playback_devices[i].c_str(), "Generic Software") )
				{
					GetDlgItem(IDC_OAL_WARN_STATIC)->ShowWindow(true);
				}

				break;
			}
		}
	} else {
		reg_get_sz(Settings::reg_path, "SoundCard", local_port_text, 256);

		for(int i = 0; i < NUM_SND_MODES; i++)
		{
			if(stricmp(sound_card_modes[i], local_port_text) == 0)
			{
				m_sound_api_list.SetCurSel(i);
				break;;
			}
		}
	}

	// Get joystick
	DWORD enum_id = 0;
	reg_get_dword(Settings::reg_path, "CurrentJoystick", &enum_id);

	int num_joy = m_joystick_list.GetCount();
	for (int i = 0; i < num_joy; i++) {
		if (m_joystick_list.GetItemData(i) == enum_id) {
			m_joystick_list.SetCurSel(i);
			break;
		}
	}
}

void CTabSound::OnCalibrate() 
{
	int index = m_joystick_list.GetCurSel();

	if (index < 0)
		return;

    int enum_id = m_joystick_list.GetItemData(index);

	if (enum_id == 99999) {
		MessageBox("No Joystick is setup", "Error");
		return;
	}

	// launch the joystick control panel applet
	WinExec("rundll32.exe shell32.dll,Control_RunDLL joy.cpl", SW_SHOWNORMAL);
}

void CTabSound::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CTabSound::OnSelChangeSoundDevice()
{
	if ( !Settings::is_openal_build() || Settings::is_new_sound_build() )
		return;

	char device_str[256];
	int index = m_sound_api_list.GetCurSel();

	m_sound_api_list.GetLBText(index, device_str);

	// since anything other than the generic software device
	// may not have enough sound sources to also use music/voices
	// be sure to give a warning text to that effect
	if ( stricmp(device_str, "no sound") && stricmp(device_str, "Generic Software") )
		GetDlgItem(IDC_OAL_WARN_STATIC)->ShowWindow(true);
	else
		GetDlgItem(IDC_OAL_WARN_STATIC)->ShowWindow(false);
}
