// TabCommLine.cpp : implementation file
//

#include <io.h>
#include <direct.h>

#include "stdafx.h"
#include "Launcher.h"
#include "TabCommLine.h"

#include "win32func.h"
#include "misc.h"
#include "launcher_settings.h"
#include "mod_settings.h"

#include "iniparser/iniparser.h"
#include "iniparser/dictionary.h"

#include "LauncherDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


int num_eflags = 0;
int num_params = 0;

typedef struct
{
	char name[32];

} EasyFlag;

EasyFlag *easy_flags  = NULL;
Flag	 *exe_params  = NULL;
bool	 *flag_states = NULL;

#define MAX_CMDLINE_SIZE		3000
#define MAX_CUSTOM_PARAM_SIZE	1200

char command_line[MAX_CMDLINE_SIZE]   = "";


/////////////////////////////////////////////////////////////////////////////
// CTabCommLine dialog


CTabCommLine::CTabCommLine(CWnd* pParent /*=NULL*/)
	: CDialog(CTabCommLine::IDD, pParent)
{
	m_flag_gen_in_process = false;

	//{{AFX_DATA_INIT(CTabCommLine)
	//}}AFX_DATA_INIT
}

CTabCommLine::~CTabCommLine()
{
	if(easy_flags)	free(easy_flags);
	if(exe_params)	free(exe_params);
	if(flag_states)	free(flag_states);
}

void CTabCommLine::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTabCommLine)
	DDX_Control(pDX, IDC_FLAG_TYPE, m_flag_type_list);
	DDX_Control(pDX, IDC_FLAG_SETUP, m_easy_flag);
	DDX_Control(pDX, IDC_FLAG_LIST, m_flag_list);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTabCommLine, CDialog)
	//{{AFX_MSG_MAP(CTabCommLine)
	ON_EN_CHANGE(IDC_CUSTOM_PARAM, OnChangeCustomParam)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_FLAG_LIST, OnItemchangedFlagList)
	ON_CBN_SELCHANGE(IDC_FLAG_SETUP, OnSelchangeFlagSetup)
	ON_NOTIFY(NM_DBLCLK, IDC_FLAG_LIST, OnDblclkFlagList)
	ON_CBN_SELCHANGE(IDC_FLAG_TYPE, OnSelchangeFlagType)
	ON_BN_CLICKED(IDC_SETTINGS_NORMAL, OnSettingsNormal)
	ON_BN_CLICKED(IDC_SETTINGS_MOD, OnSettingsMod)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCommLine message handlers

BOOL CTabCommLine::OnInitDialog() 
{
	CDialog::OnInitDialog();
	// Make sure its a checkbox and insert a column 
	m_flag_list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
 	m_flag_list.InsertColumn(0, "Flag", LVCFMT_LEFT, 200);
	return TRUE;  
}

/**
 * @return CString - string holding command line (may be empty)
 */
CString CTabCommLine::GetCommandLine()
{
	return CString(command_line);
}

/**
 * Populate the command line variable, and update the dialog window.
 */
void CTabCommLine::UpdateCommandLine()
{
	char *mod_params = NULL;
	char *standard_params = NULL;
	char *custom_params = NULL;
	bool enable_reset_to_normal = false;	// TODO: what is "normal"?
	bool enable_reset_to_mod = false;
	strcpy(command_line, "");

	// add the EXE to the command line
	if (LauncherSettings::is_exe_path_valid())
	{
		strcat(command_line, LauncherSettings::get_exe_filepath());
	}
	else
	{
		goto done_with_command_line;
	}

	// for non-custom EXEs, that's all we can do
	if (LauncherSettings::get_exe_type() != EXE_TYPE_CUSTOM)
	{
		goto done_with_command_line;
	}

	// add params specified by mod
	mod_params = ModSettings::get_mod_parameters();
	if (strlen(mod_params) > 0)
	{
 		strcat(command_line, mod_params);
		strcat(command_line, " ");
		enable_reset_to_mod = true;
	}

	// add params specified by checkbox
	standard_params = ModSettings::get_standard_parameters();
	if (strlen(standard_params) > 0)
	{
 		strcat(command_line, standard_params);
 		strcat(command_line, " ");
	}

	// add params entered by user
	custom_params = ModSettings::get_custom_parameters();
	if (strlen(custom_params) > 0)
	{
 		strcat(command_line, custom_params);
 		strcat(command_line, " ");
	}

	trim(command_line);

done_with_command_line:
	// draw the dialog window
	GetDlgItem(IDC_COMM_LINE_PATH)->SetWindowText(command_line);
	GetDlgItem(IDC_SETTINGS_NORMAL)->EnableWindow(enable_reset_to_normal);
	GetDlgItem(IDC_SETTINGS_MOD)->EnableWindow(enable_reset_to_mod);
	CLauncherDlg::Redraw();
}

/**
 * When the flag states array has been changed, call this to update the GUI.
  */
void CTabCommLine::RefreshFlagList()
{
	int num_flags = m_flag_list.GetItemCount();

	for(int i = 0; i < num_flags; i++)
	{
		int index = m_flag_list.GetItemData(i);
		m_flag_list.SetCheck(i, flag_states[index]);
	}

	CLauncherDlg::Redraw();
}

/**
 *  This is called when a tick box is ticked or unticked on the parameter list
 */
void CTabCommLine::OnItemchangedFlagList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	*pResult = 0;

	if (pNMListView->iItem < 0) return;
	if (m_flag_gen_in_process) return;

 	int index			= m_flag_list.GetItemData(pNMListView->iItem);
 	flag_states[index]	= (m_flag_list.GetCheck(pNMListView->iItem) != 0);

	// build list of params
	char standard_params[MAX_CMDLINE_SIZE];
	strcpy(standard_params, "");
	for (int i = 0; i < num_params; i++)
	{
		if (flag_states[i])
		{
			strcat(standard_params, exe_params[i].name);		
			strcat(standard_params, " ");		
		}
	}
	trim(standard_params);
	ModSettings::set_standard_parameters(standard_params);

	UpdateCommandLine();
}

/**
 * This is called when something is typed in the custom parameter edit box
 */
void CTabCommLine::OnChangeCustomParam() 
{
	// get trimmed list of params
	CString custom_params;
	GetDlgItem(IDC_CUSTOM_PARAM)->GetWindowText(custom_params);
	trim(custom_params);
	ModSettings::set_custom_parameters(custom_params);

	UpdateCommandLine();	
}

/**
 * @return - EXE_TYPE_NONE, EXE_TYPE_CUSTOM or if recognised exe it returns the flag list.
 */
int CTabCommLine::GetEXEFlags()
{
	if(LauncherSettings::get_exe_type() == EXE_TYPE_NONE) 
		return 0;

	return exe_types[LauncherSettings::get_exe_type()].flags;
}

/**
 * @return int - type of exe (one of following)
 *
 * EXE_TYPE_NONE
 * EXE_TYPE_CUSTOM
 * An integer to reference the 'exe_types' array with
 */
void CTabCommLine::SelectRegPathAndExeType()
{
	if ( !LauncherSettings::is_exe_path_valid() ||
		 !file_exists(LauncherSettings::get_exe_filepath()) )
	{
		LauncherSettings::set_reg_path("", EXE_TYPE_NONE);
		return;
	}

	int exe_type = EXE_TYPE_CUSTOM;

	// Use filename and size to determine official builds
	for (int i = 0; i < MAX_EXE_TYPES; i++)
	{
		// Confirm this by name
		if (stricmp(exe_types[i].exe_name, LauncherSettings::get_exe_nameonly()) == 0)
		{
			exe_type = i;
			break;
		}
	}

	char reg_path[MAX_PATH];

	if (exe_type < MAX_EXE_TYPES)
	{
		sprintf(reg_path, "SOFTWARE\\%s\\%s", 
			exe_types[exe_type].company, 
			exe_types[exe_type].regname);
	}
	else
	{
		sprintf(reg_path, "SOFTWARE\\%s\\%s", 
				exe_types[EXE_TYPE_FS2].company,
				exe_types[EXE_TYPE_FS2].regname);
	}
			
	LauncherSettings::set_reg_path(reg_path, exe_type);
}

void CTabCommLine::ConstructFlagList()
{
	int k;

	// If this is a retail FS2 exe skip all this
	if (LauncherSettings::get_exe_type() != EXE_TYPE_CUSTOM)
	{
		m_easy_flag.ResetContent();
		m_easy_flag.EnableWindow(FALSE);

		ConstructFlagListRetail();
		ConstructFlagListInternal();

		return;
	}

	m_flag_type_list.EnableWindow(TRUE);
	m_flag_list.EnableWindow(TRUE);
	m_easy_flag.EnableWindow(TRUE);

	// Delete and generate new flag file
	char flag_file[MAX_PATH];
	sprintf(flag_file, "%s\\flags.lch",LauncherSettings::get_exe_pathonly());

	DeleteFile(flag_file);

	if ( !run_file((LPTSTR) LauncherSettings::get_exe_nameonly(), (LPTSTR) LauncherSettings::get_exe_pathonly()," -get_flags", true) )
	{
		MessageBox("Unable to query FreeSpace Open for launcher flag information!  Is the EXE present?");
		return;
	}

	FILE *fp = fopen(flag_file, "r");

	int focount = 2000;
	while (fp == NULL && focount)
	{
		fp = fopen(flag_file, "r");
		focount--;
	}

	if (fp == NULL)
	{
		MessageBox("Failed to read launcher flag file", "Fatal Error", MB_OK);
		return;
	}

	int eflags_struct_size;
	int params_struct_size;

	fread(&eflags_struct_size, sizeof(int), 1, fp);
	fread(&params_struct_size, sizeof(int), 1, fp);

	if (eflags_struct_size != sizeof(EasyFlag) ||
	   params_struct_size != sizeof(Flag))
	{
		MessageBox("Launcher and fs2_open versions do not work with each other", "Fatal Error", MB_OK);
		fclose(fp);
		DeleteFile(flag_file);
		return;
	}

	if (easy_flags)
		free(easy_flags);
	if (exe_params)
		free(exe_params);
	if (flag_states)
		free(flag_states);

	fread(&num_eflags, sizeof(int), 1, fp);
	easy_flags = (EasyFlag *) malloc(sizeof(EasyFlag) * num_eflags); 

	if (easy_flags == NULL)
	{
		MessageBox("Failed to allocate enough memory for easy_flags", "Fatal Error", MB_OK);
		fclose(fp);
		DeleteFile(flag_file);
		return;
	}  

	fread(easy_flags, sizeof(EasyFlag) * num_eflags, 1, fp);
	
	fread(&num_params, sizeof(int), 1, fp);
	exe_params  = (Flag *) malloc(sizeof(Flag) * num_params); 
	flag_states = (bool *) malloc(sizeof(bool) * num_params);

	memset(flag_states, 0, sizeof(bool) * num_params);

	if (exe_params == NULL || flag_states == NULL)
	{
		MessageBox("Failed to allocate enough memory for exe_params", "Fatal Error", MB_OK);
		fclose(fp);
		DeleteFile(flag_file);
		return;
	}

	fread(exe_params, sizeof(Flag) * num_params, 1, fp);

	// hack for OpenAL check
	if ( (filelength(fileno(fp)) - ftell(fp)) == 1 ) {
		LauncherSettings::set_openal_build(true);
	}

	fclose(fp);

	// go ahead and delete the file now, it's just taking up space at this point
	DeleteFile(flag_file);

	// Setup Easy Flags
	m_easy_flag.ResetContent();
	for (k = 0; k < num_eflags; k++)
	{
		m_easy_flag.InsertString(k,easy_flags[k].name);
	}

	// Setup Flag Types
	m_flag_type_list.ResetContent();

	int count = 0;
	char last_word[FLAG_TYPE_LEN] = "";
	for (k = 0; k < num_params; k++)
	{
		if (strcmp(exe_params[k].type, last_word) != 0)
		{
			m_flag_type_list.InsertString(count, exe_params[k].type);
			strcpy(last_word, exe_params[k].type);
			count++;

			if (count > 19)
			{
				MessageBox("Flag type count is 20 or more, please report to coder","Error");
				count = 19;
				break;
			}
		}
	}

	m_flag_type_list.SetCurSel(0);
	ConstructFlagListInternal();
}

void CTabCommLine::ConstructFlagListRetail()
{
	int i, count = 0;
	char last_word[FLAG_TYPE_LEN] = "";

	if ( exe_types[LauncherSettings::get_exe_type()].flags & FLAG_FS1 )
		num_params = Num_retail_params_FS1;
	else
		num_params = Num_retail_params_FS2;

	exe_params  = (Flag *) malloc(sizeof(Flag) * num_params);
	flag_states = (bool *) malloc(sizeof(bool) * num_params);

	if ( (exe_params == NULL) || (flag_states == NULL) )
	{
		MessageBox("Memory allocation failure!");
		return;
	}

	if ( exe_types[LauncherSettings::get_exe_type()].flags & FLAG_FS1 )
		memcpy( exe_params, retail_params_FS1, sizeof(Flag) * num_params );
	else
		memcpy( exe_params, retail_params_FS2, sizeof(Flag) * num_params );

	memset( flag_states, 0, sizeof(bool) * num_params );

	// setup flag types
	m_flag_type_list.ResetContent();
	m_flag_type_list.EnableWindow(TRUE);

	for (i = 0; i < num_params; i++) {
		if ( strcmp(exe_params[i].type, last_word) ) {
			m_flag_type_list.InsertString(count, exe_params[i].type);
			strcpy(last_word, exe_params[i].type);
			count++;
		}
	}

	m_flag_type_list.SetCurSel(0);
}

void CTabCommLine::ConstructFlagListInternal()
{
	int type_index = m_flag_type_list.GetCurSel();

	char type_text[FLAG_TYPE_LEN];
	m_flag_type_list.GetWindowText(type_text, FLAG_TYPE_LEN);

	// Setup flags
	m_flag_list.DeleteAllItems();
	m_flag_list.EnableWindow(TRUE);

	int exe_flags = 0;

	// If we are not sure what this exe is give access to all parameters
	if(LauncherSettings::get_exe_type() != EXE_TYPE_NONE)
	{
		exe_flags = exe_types[LauncherSettings::get_exe_type()].flags;
	}

	m_flag_gen_in_process = true;

	// For each parameter
	for(int i = 0, j = 0; i < num_params; i++)
	{
		if(strcmp(exe_params[i].type, type_text) != 0)
			continue;

		if(exe_params[i].fso_only && !(exe_types[LauncherSettings::get_exe_type()].flags & FLAG_FS2OPEN))
		  	continue;

		// Insert this item but keep a record of the index number
		// The insert item index will not stay as is and will mess up if there are gaps (unshown params)
		if(strlen(exe_params[i].desc) > 0)
			m_flag_list.InsertItem(j, exe_params[i].desc, 0);
		else
			m_flag_list.InsertItem(j, exe_params[i].name, 0);

	  	m_flag_list.SetItemData(j, i);
		j++;
	}

	m_flag_gen_in_process = false;
}

void CTabCommLine::LoadSettings()
{
	ModSettings::load_user();
	PopulateFlagStates();

	RefreshFlagList();
	GetDlgItem(IDC_CUSTOM_PARAM)->SetWindowText(ModSettings::get_custom_parameters());

	UpdateCommandLine();
}

void CTabCommLine::PopulateFlagStates()
{
	char *new_standard_params = strdup(ModSettings::get_standard_parameters());
	char *new_custom_params = strdup(ModSettings::get_custom_parameters());

	// tokenize the standard parameters and copy them to 
	
	for (int i = 0; i < num_params; i++)
	{
		exe_params[i].name, 

}
	// Seperate custom flags from standard one
	for(int i = 0; i < num_params; i++) {
		// while going through the cmdline make sure to grab only the option that we
		// are looking for, but if one similar then keep searching for the exact match
		do {
			found_str = strstr(&custom_param[0] + get_new_offset, exe_params[i].name);

			if (found_str && (*(found_str + strlen(exe_params[i].name))) && (*(found_str + strlen(exe_params[i].name)) != ' ') ) {
				// the new offset should be our current location + the length of the current option
				get_new_offset = (strlen(custom_param) - strlen(found_str) + strlen(exe_params[i].name));
			} else {
				get_new_offset = 0;
			}
		} while ( get_new_offset );

		if (found_str == NULL)
			continue;

		flag_states[i] = true;

		if (end_string_here == NULL)
			end_string_here = found_str;
	}

	// Cut the standard options out of the custom string
	if (end_string_here != NULL) {
	   	if (end_string_here > custom_param && end_string_here[-1] == ' ') {
			end_string_here[-1] = '\0';
		} else {
			end_string_here[0] = '\0';
		}
	}



/**
 * Save command line to settings.ini file.
 */
void CTabCommLine::SaveSettings()
{
	ModSettings::save_user();
}

// User has selected new easy select type
void CTabCommLine::OnSelchangeFlagSetup() 
{				   
  	int easy_flag = m_easy_flag.GetCurSel();

	// Custom setting, leave it alone
	if(easy_flag == 0) return;

	for(int i = 0; i < num_params; i++)
	{
		if(exe_params[i].on_flags & (1 << easy_flag))
			flag_states[i] = true;

		if(exe_params[i].off_flags & (1 << easy_flag))
			flag_states[i] = false;
	}

   	RefreshFlagList();
   	UpdateCommandLine();
}

// User has selected a new flag group type
void CTabCommLine::OnSelchangeFlagType() 
{
  	ConstructFlagListInternal();
	RefreshFlagList();
}

// User has double clicked flag
void CTabCommLine::OnDblclkFlagList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

	if(exe_params == NULL) return;
	if(pNMListView->iItem < 0) return;

	int index = m_flag_list.GetItemData(pNMListView->iItem);

	if(index < 0 || index > num_params)
	{
		MessageBox("Bad m_flag_list index value","Error");
	}

	if(strlen(exe_params[index].web_url) > 0)
	{
		if(exe_params[index].web_url[0] == 'h' && 
		   exe_params[index].web_url[1] == 't' && 
		   exe_params[index].web_url[2] == 't' && 
		   exe_params[index].web_url[3] == 'p')
		{
			open_web_page(exe_params[index].web_url);
		}
		else
		{
			MessageBox(exe_params[index].web_url,"Info");
		}
	}
	else
		MessageBox("No online help for this feature","Sorry");
}

void CTabCommLine::OnSettingsNormal() 
{
	UpdateCommandLine();	
}

void CTabCommLine::OnSettingsMod() 
{
	ModSettings::set_standard_parameters("");
	ModSettings::set_custom_parameters("");
	UpdateCommandLine();
}
