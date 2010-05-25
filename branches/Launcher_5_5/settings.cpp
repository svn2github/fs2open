#include "stdafx.h"
#include "settings.h"

#include "iniparser/iniparser.h"
#include "iniparser/dictionary.h"

char Settings::exe_filepath[MAX_PATH] = "";
char Settings::exe_pathonly[MAX_PATH] = "";
char Settings::exe_nameonly[MAX_PATH] = "";
	
bool Settings::exe_path_valid = false;

char Settings::mod_name[MAX_PATH] = "";
char Settings::reg_path[MAX_PATH] = "";
int  Settings::exe_type = -1;

char *Settings::ini_main        = "launcher6.ini";
char *Settings::ini_mod_default = "mod.ini";
char *Settings::ini_mod_custom  = "settings.ini";
char *Settings::command_cfg     = "cmdline_fso.cfg";

char flags_custom[10240]      = "";
char flags_default_mod[10240] = "";
char flags_custom_mod[10240]  = "";

int Settings::build_flags = 0;

#define BUILD_CAP_OPENAL	(1<<0)
#define BUILD_CAP_NO_D3D	(1<<1)
#define BUILD_CAP_NEW_SND	(1<<2)


void Settings::set_exe_filepath(char *path)
{
	if(path == NULL || strlen(path) == 0)
	{
		return;
	}

	strcpy(exe_filepath, path);
	strcpy(exe_pathonly, path);

	char *final_slash = strrchr(exe_pathonly, '\\');

	if(final_slash)
	{
		strcpy(exe_nameonly, final_slash+1);
		*final_slash = '\0';
		exe_path_valid = true;
	}
	else
	{
		*exe_pathonly = '\0';
		strcpy(exe_nameonly, path);
		exe_path_valid = false;
	}						 
}

void Settings::set_reg_path(char *path, int type)
{
	strcpy(reg_path, path);
	Settings::exe_type = type;
}

bool Settings::get_mod_from_ini()
{
	dictionary *ini = iniparser_load(Settings::ini_main);
	if(ini == NULL)
	{
		return false;
	}

	iniparser_dump(ini, stderr);

	char *mod = iniparser_getstr(ini, "launcher:active_mod");
	if(mod == NULL)
	{
		return false;
	}
		
	strcpy(mod_name, mod);
	iniparser_freedict(ini);
	return true;
}

bool Settings::get_exe_path_from_ini()
{
	dictionary *ini = iniparser_load(Settings::ini_main);
	if(ini == NULL)
	{
		return false;
	}

	iniparser_dump(ini, stderr);

	char *exe_path = iniparser_getstr(ini, "launcher:exe_filepath");
	if(exe_path == NULL)
	{
		return false;
	}
		
	set_exe_filepath(exe_path);
	iniparser_freedict(ini);
	return true;
}

void Settings::set_build_flags(int flags)
{
	build_flags = flags;
}

bool Settings::is_openal_build()
{
	return ((build_flags & BUILD_CAP_OPENAL) != 0);
}

bool Settings::is_no_d3d_build()
{
	return ((build_flags & BUILD_CAP_NO_D3D) != 0);
}

bool Settings::is_new_sound_build()
{
	return ((build_flags & BUILD_CAP_NEW_SND) != 0);
}


/*
void Settings::inis_open()
{
	// Open main ini
	// Open MOD default
	// Open MOD custom
}

void Settings::inis_write_ini()
{
	if(mod selected)
	{
		// write to custom MOD
	}
	else
	{
		// write to main ini
	}
}
*/
