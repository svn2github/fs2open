/*
 * Created by Hassan "Karajorma" Kazmi for the FreeSpace2 Source Code Project.
 * You may not sell or otherwise commercially exploit the source or things you
 * create based on the source.
 */

#include "globalincs/pstypes.h"
#include "globalincs/def_files.h"
#include "mission/missioncampaign.h"
#include "mission/missionmessage.h"
#include "mod_table/mod_table.h"
#include "localization/localize.h"
#include "parse/parselo.h"

int Directive_wait_time;
bool True_loop_argument_sexps;
bool Fixed_turret_collisions;
bool Damage_impacted_subsystem_first;
bool Cutscene_camera_disables_hud;
bool Alternate_chaining_behavior;
int Default_ship_select_effect;
int Default_weapon_select_effect;
bool Enable_external_shaders = false;


void parse_mod_table(char *filename)
{
	int rval;
	// SCP_vector<SCP_string> lines;

	// open localization
	lcl_ext_open();

	if ((rval = setjmp(parse_abort)) != 0)
	{
		mprintf(("TABLES: Unable to parse '%s'!  Error code = %i.\n", (filename) ? filename : "<default game_settings.tbl>", rval));
		lcl_ext_close();
		return;
	}

	if (filename == NULL)
		read_file_text_from_array(defaults_get_file("game_settings.tbl"));
	else
		read_file_text(filename, CF_TYPE_TABLES);

	reset_parse();	

	// start parsing
	optional_string("#CAMPAIGN SETTINGS");

	if (optional_string("$Default Campaign File Name:")) {
		char temp[MAX_FILENAME_LEN];
		stuff_string(temp, F_NAME, MAX_FILENAME_LEN);

		// remove extension?
		char *p = strrchr(temp, '.');
		if (p != NULL) {
			mprintf(("Game Settings Table: Removing extension on default campaign file name %s\n", temp));
			*p = 0;
		}

		// check length
		int maxlen = (MAX_FILENAME_LEN - 4);
		int len = strlen(temp);
		if (len > maxlen) {
			Warning(LOCATION, "Token too long: [%s].  Length = %i.  Max is %i.\n", temp, len, maxlen);
			temp[maxlen] = 0;
		}

		strcpy_s(Default_campaign_file_name, temp);
	}

	optional_string("#HUD SETTINGS");

	// how long should the game wait before displaying a directive?
	if (optional_string("$Directive Wait Time:")) {
		stuff_int(&Directive_wait_time);
	}

	if (optional_string("$Cutscene camera disables HUD:")) {
		stuff_boolean(&Cutscene_camera_disables_hud);
	}

	optional_string("#SEXP SETTINGS"); 

	if (optional_string("$Loop SEXPs Then Arguments:")) { 
		stuff_boolean(&True_loop_argument_sexps);
		if (True_loop_argument_sexps) {
			mprintf(("Game Settings Table: Using Reversed Loops For SEXP Arguments\n"));
		} else {
			mprintf(("Game Settings Table: Using Standard Loops For SEXP Arguments\n"));
		}
	}

	if (optional_string("$Use Alternate Chaining Behavior:")) {
		stuff_boolean(&Alternate_chaining_behavior);
		if (Alternate_chaining_behavior) {
			mprintf(("Game Settings Table: Using alternate event chaining behavior\n"));
		} else {
			mprintf(("Game Settings Table: Using standard event chaining behavior\n"));
		}
	}

	optional_string("#GRAPHICS SETTINGS");

	if (optional_string("$Enable External Shaders:")) {
		stuff_boolean(&Enable_external_shaders);
		if (Enable_external_shaders)
			mprintf(("Game Settings Table: External shaders are enabled\n"));
		else
			mprintf(("Game Settings Table: External shaders are DISABLED\n"));
	}
	
	optional_string("#OTHER SETTINGS"); 

	if (optional_string("$Fixed Turret Collisions:")) { 
		stuff_boolean(&Fixed_turret_collisions);
	}

	if (optional_string("$Damage Impacted Subsystem First:")) { 
		stuff_boolean(&Damage_impacted_subsystem_first);
	}

	if (optional_string("$Default ship select effect:")) {
		char effect[NAME_LENGTH];
		stuff_string(effect, F_NAME, NAME_LENGTH);
		if (!stricmp(effect, "FS2"))
			Default_ship_select_effect = 2;
		else if (!stricmp(effect, "FS1"))
			Default_ship_select_effect = 1;
		else if (!stricmp(effect, "off"))
			Default_ship_select_effect = 0;
	}

	if (optional_string("$Default weapon select effect:")) {
		char effect[NAME_LENGTH];
		stuff_string(effect, F_NAME, NAME_LENGTH);
		if (!stricmp(effect, "FS2"))
			Default_weapon_select_effect = 2;
		else if (!stricmp(effect, "FS1"))
			Default_weapon_select_effect = 1;
		else if (!stricmp(effect, "off"))
			Default_weapon_select_effect = 0;
	}

	required_string("#END");

	// close localization
	lcl_ext_close();
}

void mod_table_init()
{	
	// first parse the default table
	parse_mod_table(NULL);

	// if a mod.tbl exists read it
	if (cf_exists_full("game_settings.tbl", CF_TYPE_TABLES)) {
		parse_mod_table("game_settings.tbl");
	}

	// parse any modular tables
	parse_modular_table("*-mod.tbm", parse_mod_table);
}
