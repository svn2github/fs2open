/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/Mission/MissionCampaign.cpp $
 * $Revision: 2.40.2.14 $
 * $Date: 2007-11-19 19:50:12 $
 * $Author: Goober5000 $
 *
 * source for dealing with campaigns
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.40.2.13  2007/10/28 16:39:17  taylor
 * fix a loadout bug where the current mission name would get substituted for the previous one by mistake
 *
 * Revision 2.40.2.12  2007/09/02 02:07:43  Goober5000
 * added fixes for #1415 and #1483, made sure every read_file_text had a corresponding setjmp, and sync'd the parse error messages between HEAD and stable
 *
 * Revision 2.40.2.11  2007/03/21 20:54:24  karajorma
 * Bump the number of debriefing stages.
 * Fix an annoying (and erroneous) warning in the campaign editor.
 *
 * Revision 2.40.2.10  2007/02/11 09:09:55  taylor
 * fix simroom crash bug from Goober's rencent change
 * add NULL handling to make sure we don't crap out on sorting
 *
 * Revision 2.40.2.9  2007/02/11 06:38:56  Goober5000
 * when alphabetizing campaigns, ignore "the" at the beginning of a title
 *
 * Revision 2.40.2.8  2007/02/10 06:55:49  Goober5000
 * sort campaigns by title, not by file name
 *
 * Revision 2.40.2.7  2006/10/01 19:27:28  taylor
 * fix for the "branch" bug (still a minor usability issue however, see Mantis bug for details)
 * add a popup to the loopbrief screen when you press ESC, so that we can either accept or decline the loop offer
 *
 * Revision 2.40.2.6  2006/09/14 18:47:39  taylor
 * prevent FRED from hitting an Assert() that it shouldn't have had to hit in the first place
 *
 * Revision 2.40.2.5  2006/09/11 01:04:20  taylor
 * fixes for stuff_string() bounds checking
 * compiler warning fixes
 *
 * Revision 2.40.2.4  2006/08/28 17:14:52  taylor
 * stupid, stupid, stupid...
 *  - fix AVI/MPG movie playback
 *  - fix missing campaign craziness
 *
 * Revision 2.40.2.3  2006/08/27 18:11:37  taylor
 * quite a few fixes to handle missing campaigns better
 * change load order for campaign loading to a full check: Player-specified -> BUILTIN_CAMPAIGN -> First Avaiable.
 * clean up the getting of a list of available campaigns
 * fix simroom issue where single missions, with the [V] icon, would display wrong (this was a retail bug, but it doesn't show normally)
 * fix bug where, if a campaign failed to load, it would still appear available for savefile useage
 * fix bug where, when resetting the campaign info, the num_missions var wasn't 0'd and it could cause a sexp Assert() during reset
 *
 * Revision 2.40.2.2  2006/06/22 14:59:45  taylor
 * fix various things that Valgrind has been complaining about
 *
 * Revision 2.40.2.1  2006/06/10 18:35:05  Goober5000
 * fix parsing/handling of debriefing persona indexes
 * --Goober5000
 *
 * Revision 2.40  2006/04/20 06:32:07  Goober5000
 * proper capitalization according to Volition
 *
 * Revision 2.39  2006/02/16 05:15:48  taylor
 * fix strangeness with persona index thingy (ubyte can't be -1, this needs to be left like this or make an int)
 * address some pilot file corruption using non-linear campaigns, rather simple fix for something that took quite a while to track down
 *
 * Revision 2.38  2006/02/12 10:42:25  Goober5000
 * allow specification of debriefing personas
 * --Goober5000
 *
 * Revision 2.37  2006/01/18 17:45:01  taylor
 * this is probably going to end up doing something bad again when switching mods, hopefully it's not still necessary though
 *
 * Revision 2.36  2005/12/29 08:08:36  wmcoolmon
 * Codebase commit, most notably including objecttypes.tbl
 *
 * Revision 2.35  2005/10/30 20:03:39  taylor
 * add a bunch of Assert()'s and NULL checks to either help debug or avoid errors
 * fix Mantis bug #381
 * fix a small issue with the starfield bitmap removal sexp since it would read one past the array size
 *
 * Revision 2.34  2005/10/30 19:21:45  wmcoolmon
 * Return an error if mission_campaign_next_mission() is called when the campaign has no missions
 *
 * Revision 2.33  2005/10/30 10:45:19  taylor
 * fix a couple of campaign savefile bugs for WMC
 *
 * Revision 2.32  2005/10/29 12:25:49  taylor
 * bullet-proof the loadout read a little bit (did this before and had it locally, must have missed CVS)
 *
 * Revision 2.31  2005/09/16 00:03:51  taylor
 * didn't mean to leave that in there
 *
 * Revision 2.30  2005/08/22 22:24:22  Goober5000
 * some tweaks to parselo, plus ensure that Unicode files don't crash
 * --Goober5000
 *
 * Revision 2.29  2005/08/20 20:38:08  taylor
 * more unix->SCP_UNIX define fixing
 *
 * Revision 2.28  2005/08/18 01:15:41  taylor
 * Address a couple of pilot code issues
 *  - make the same MAX_PILOTS in the barracks that the playerselect screen has since it's a problem here too
 *  - add a -1 check for ship/weapon name lookups to avoid bounds issues.  NOTE: this can empty the techroom of entries if ithits this check, but it won't crash :)
 *
 * Revision 2.27  2005/08/18 00:19:53  Kazan
 * # FEATURE BREAK # This breaks a feature in pilot files temporarily to correct a [reproducable] crash - it is merely commenting out the trouble lines - bug report in mantis and assigned to taylor
 *
 * Revision 2.26  2005/05/12 17:49:13  taylor
 * use vm_malloc(), vm_free(), vm_realloc(), vm_strdup() rather than system named macros
 *   fixes various problems and is past time to make the switch
 *
 * Revision 2.25  2005/05/08 20:26:00  wmcoolmon
 * Dynamically allocated medals
 *
 * Revision 2.24  2005/04/28 01:35:26  wmcoolmon
 * stuff_byte to stuff_ubyte; does the same thing, but with a better name.
 *
 * Revision 2.23  2005/04/25 00:25:15  wmcoolmon
 * MAX_SHIP_CLASSES > Num_ship_classes
 *
 * Revision 2.22  2005/04/12 05:26:36  taylor
 * many, many compiler warning and header fixes (Jens Granseuer)
 * fix free on possible NULL in modelinterp.cpp (Jens Granseuer)
 *
 * Revision 2.21  2005/04/03 08:48:30  Goober5000
 * brought weapon loadout banks into agreement with ship info banks
 * improved error reporting on apply-to-all
 * --Goober5000
 *
 * Revision 2.20  2005/03/27 06:12:38  taylor
 * some pilot file fixing when going between multi and single (partial fix)
 *
 * Revision 2.19  2005/03/13 23:07:35  taylor
 * enable 32-bit to 16-bit TGA conversion with -tga16 cmdline option (experimental)
 * fix crash when upgrading from original campaign stats file to current
 *
 * Revision 2.18  2005/03/10 08:00:08  taylor
 * change min/max to MIN/MAX to fix GCC problems
 * add lab stuff to Makefile
 * build unbreakage for everything that's not MSVC++ 6
 * lots of warning fixes
 * fix OpenGL rendering problem with ship insignias
 * no Warnings() in non-debug mode for Linux (like Windows)
 * some campaign savefile fixage to stop reverting everyones data
 *
 * Revision 2.17  2005/02/23 05:05:38  taylor
 * compiler warning fixes (for MSVC++ 6)
 * have the warp effect only load as many LODs as will get used
 * head off strange bug in release when corrupt soundtrack number gets used
 *    (will still Assert in debug)
 * don't ever try and save a campaign savefile in multi or standalone modes
 * first try at 32bit->16bit color conversion for TGA code (for TGA only ship textures)
 *
 * Revision 2.16  2004/10/31 21:53:24  taylor
 * new pilot code support, no-multiplayer and compiler warning fixes, center mouse cursor for redalert missions
 *
 * Revision 2.15  2004/07/26 20:47:37  Kazan
 * remove MCD complete
 *
 * Revision 2.14  2004/07/17 18:46:08  taylor
 * various OGL and memory leak fixes
 *
 * Revision 2.13  2004/07/12 16:32:53  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 2.12  2004/03/27 22:27:07  randomtiger
 * Committed possible fix to cutscene listing problem in techroom, as described in mantis.
 *
 * Revision 2.11  2004/03/05 09:02:06  Goober5000
 * Uber pass at reducing #includes
 * --Goober5000
 *
 * Revision 2.10  2003/09/05 04:25:29  Goober5000
 * well, let's see here...
 *
 * * persistent variables
 * * rotating gun barrels
 * * positive/negative numbers fixed
 * * sexps to trigger whether the player is controlled by AI
 * * sexp for force a subspace jump
 *
 * I think that's it :)
 * --Goober5000
 *
 * Revision 2.9  2003/03/18 10:07:03  unknownplayer
 * The big DX/main line merge. This has been uploaded to the main CVS since I can't manage to get it to upload to the DX branch. Apologies to all who may be affected adversely, but I'll work to debug it as fast as I can.
 *
 * Revision 2.8  2003/03/03 04:28:36  Goober5000
 * fixed the tech room bug!  yay!
 * --Goober5000
 *
 * Revision 2.7  2003/02/23 03:13:03  wmcoolmon
 * Uncommented the sections of code that actually played a movie for a given mission. (Removed when daveb cleaned out the movie code for the release)
 *
 * Revision 2.6  2003/01/14 04:00:16  Goober5000
 * allowed for up to 256 main halls
 * --Goober5000
 *
 * Revision 2.5  2002/08/27 13:38:58  penguin
 * Moved DirectX8 stuff to directx8 branch; reverted to previous
 *
 * Revision 2.3.2.3  2002/09/24 18:56:43  randomtiger
 * DX8 branch commit
 *
 * This is the scub of UP's previous code with the more up to date RT code.
 * For full details check previous dev e-mails
 *
 * Revision 2.3.2.2  2002/08/28 12:39:36  randomtiger
 * OK, this should be a commit to the DX branch or Im going to be in a lot of trouble.
 * The movie and dx8show files have been cleaned up big time.
 * My debug system is in but has NO EFFECT at all unless a compiler flag is turned on, check h file for details.
 * Aside from that a few changes to help the movie code work properly.
 * Works on most things including GF4 and Voodoo 3. However may not work properly on a voodoo 2.
 * Im going to leave this as a bug for now, serves you right for buying voodoo!
 *
 * Revision 2.3.2.1  2002/08/27 13:22:28  penguin
 * Moved to directx8 branch
 *
 * Revision 2.4  2002/08/18 19:48:29  randomtiger
 * Added new lib files: strmiids and ddraw to get dshow working
 * Added new command line parameter to active direct show movie play: -dshowvid
 * Uncommented movie_play calls and includes
 *
 * Revision 2.3  2002/08/01 01:41:06  penguin
 * The big include file move
 *
 * Revision 2.2  2002/07/29 20:12:31  penguin
 * added #ifdef _WIN32 around windows-specific system headers
 *
 * Revision 2.1  2002/07/07 19:55:59  penguin
 * Back-port to MSVC
 *
 * Revision 2.0  2002/06/03 04:02:24  penguin
 * Warpcore CVS sync
 *
 * Revision 1.4  2002/05/17 06:45:53  mharris
 * More porting tweaks.  It links!  but segfaults...
 *
 * Revision 1.3  2002/05/17 03:06:13  mharris
 * more porting tweaks
 *
 * Revision 1.2  2002/05/03 22:07:09  mharris
 * got some stuff to compile
 *
 * Revision 1.1  2002/05/02 18:03:10  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 23    9/14/99 4:35a Dave
 * Argh. Added all kinds of code to handle potential crashes in debriefing
 * code.
 * 
 * 22    9/09/99 11:40p Dave
 * Handle an Assert() in beam code. Added supernova sounds. Play the right
 * 2 end movies properly, based upon what the player did in the mission.
 * 
 * 21    9/09/99 9:34a Jefff
 * fixed a potential exit-loop bug
 * 
 * 20    9/07/99 6:55p Jefff
 * functionality to break out of a loop.  hacked functionality to jump to
 * a specific mission in a campaign -- doesnt grant ships/weapons from
 * skipped missions tho.
 * 
 * 19    9/07/99 2:19p Jefff
 * clear skip mission player vars on mission skip
 * 
 * 18    9/06/99 9:45p Jefff
 * break out of loop and skip mission support
 * 
 * 17    9/06/99 6:38p Dave
 * Improved CD detection code.
 * 
 * 16    9/03/99 1:32a Dave
 * CD checking by act. Added support to play 2 cutscenes in a row
 * seamlessly. Fixed super low level cfile bug related to files in the
 * root directory of a CD. Added cheat code to set campaign mission # in
 * main hall.
 * 
 * 15    8/27/99 12:04a Dave
 * Campaign loop screen.
 * 
 * 14    8/04/99 5:36p Andsager
 * Show upsell screens at end of demo campaign before returning to main
 * hall.
 * 
 * 13    2/05/99 3:50p Anoop
 * Removed dumb campaign mission stats saving from multiplayer.
 * 
 * 12    12/17/98 2:43p Andsager
 * Modify fred campaign save file to include optional mission loops
 * 
 * 11    12/12/98 3:17p Andsager
 * Clean up mission eval, goal, event and mission scoring.
 * 
 * 10    12/10/98 9:59a Andsager
 * Fix some bugs with mission loops
 * 
 * 9     12/09/98 1:56p Andsager
 * Initial checkin of mission loop
 * 
 * 8     11/05/98 5:55p Dave
 * Big pass at reducing #includes
 * 
 * 7     11/03/98 4:48p Johnson
 * Fixed campaign file versioning bug left over from Silent Threat port.
 * 
 * 6     10/23/98 3:51p Dave
 * Full support for tstrings.tbl and foreign languages. All that remains
 * is to make it active in Fred.
 * 
 * 5     10/13/98 2:47p Andsager
 * Remove reference to Tech_shivan_species_avail
 * 
 * 4     10/13/98 9:28a Dave
 * Started neatening up freespace.h. Many variables renamed and
 * reorganized. Added AlphaColors.[h,cpp]
 * 
 * 3     10/07/98 6:27p Dave
 * Globalized mission and campaign file extensions. Removed Silent Threat
 * special code. Moved \cache \players and \multidata into the \data
 * directory.
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 95    9/10/98 1:17p Dave
 * Put in code to flag missions and campaigns as being MD or not in Fred
 * and FreeSpace. Put in multiplayer support for filtering out MD
 * missions. Put in multiplayer popups for warning of non-valid missions.
 * 
 * 94    9/01/98 4:25p Dave
 * Put in total (I think) backwards compatibility between mission disk
 * freespace and non mission disk freespace, including pilot files and
 * campaign savefiles.
 * 
 * 93    7/06/98 4:10p Hoffoss
 * Fixed some bugs that presented themselves when trying to use a pilot
 * that has a no longer existent campaign active.  Also expanded the
 * campaign load code to actually return a proper error code, instead of
 * always trapping errors internally and crashing, and always returning 0.
 * 
 * 92    6/17/98 9:30a Allender
 * fixed red alert replay stats clearing problem
 * 
 * 91    6/01/98 11:43a John
 * JAS & MK:  Classified all strings for localization.
 * 
 * 90    5/25/98 1:29p Allender
 * end mission sequencing
 * 
 * 89    5/21/98 9:25p Allender
 * endgame movie always viewable at end of campaign
 * 
 * 88    5/13/98 5:14p Allender
 * red alert support to go back to previous mission
 * 
 * 87    5/12/98 4:16p Hoffoss
 * Fixed bug where not all missions in all campaigns were being filtered
 * out of stand alone mission listing in simulator room.
 * 
 * 86    5/05/98 3:29p Hoffoss
 * Changed code so description is BEFORE num players in campaign file.
 * Other code is relying on this ordering.
 * 
 * 85    5/05/98 12:19p Dave
 * campaign description goes *after* num players
 * 
 * 84    5/04/98 5:52p Comet
 * Fixed bug with Galatea/Bastion selection when finishing missions.
 * 
 * 83    5/01/98 2:46p Duncan
 * fix a cfile problem with campaigns related to the new cfile stuff
 * 
 * 82    5/01/98 12:34p John
 * Added code to force FreeSpace to run in the same dir as exe and made
 * all the parse error messages a little nicer.
 * 
 * 81    4/30/98 7:01p Duncan
 * AL: don't allow deletion of campaign files in multiplayer
 * 
 * 80    4/30/98 4:53p John
 * Restructured and cleaned up cfile code.  Added capability to read off
 * of CD-ROM drive and out of multiple pack files.
 *
 * $NoKeywords: $
 */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif

#ifdef SCP_UNIX
#include <sys/stat.h>
#include <glob.h>
#endif

#include "mission/missioncampaign.h"
#include "ui/ui.h"
#include "gamesequence/gamesequence.h"
#include "freespace2/freespace.h"
#include "parse/sexp.h"
#include "playerman/player.h"
#include "mission/missiongoals.h"
#include "cutscene/movie.h"
#include "gamesnd/eventmusic.h"
#include "localization/localize.h"
#include "parse/parselo.h"
#include "ship/ship.h"
#include "weapon/weapon.h"
#include "cfile/cfile.h"
#include "starfield/supernova.h"
#include "cutscene/cutscenes.h"
#include "menuui/techmenu.h"
#include "missionui/missionscreencommon.h"
#include "missionui/redalert.h"



// mission disk stuff
#define CAMPAIGN_SAVEFILE_MAX_SHIPS_OLD						75
#define CAMPAIGN_SAVEFILE_MAX_WEAPONS_OLD						44

#define CAMPAIGN_INITIAL_RELEASE_FILE_VERSION				6

// campaign wasn't ended
int Campaign_ended_in_mission = 0;

// stuff for selecting campaigns.  We need to keep both arrays around since we display the
// list of campaigns by name, but must load campaigns by filename
char *Campaign_names[MAX_CAMPAIGNS] = { NULL };
char *Campaign_file_names[MAX_CAMPAIGNS] = { NULL };
char *Campaign_descs[MAX_CAMPAIGNS] = { NULL };
int	Num_campaigns;
int Campaign_file_missing;
int Campaign_names_inited = 0;

// stuff used for campaign list building
static bool MC_desc = false;
static bool MC_multiplayer = false;

char *campaign_types[MAX_CAMPAIGN_TYPES] = 
{
//XSTR:OFF
	"single",
	"multi coop",
	"multi teams"
//XSTR:ON
};

// modules local variables to deal with getting new ships/weapons available to the player
int Num_granted_ships, Num_granted_weapons;		// per mission counts of new ships and weapons
int Granted_ships[MAX_SHIP_CLASSES];
int Granted_weapons[MAX_WEAPON_TYPES];

// variables to control the UI stuff for loading campaigns
//LOCAL int Campaign_ui_active = 0;
LOCAL UI_WINDOW Campaign_window;
LOCAL UI_LISTBOX Campaign_listbox;
LOCAL UI_BUTTON Campaign_okb, Campaign_cancelb;

// the campaign!!!!!
campaign Campaign;

// variables with deal with the campaign save file
// bumped to 13 by Goober5000 for persistent variables
// bumped to 14 by taylor for ship/weapon table handling
// bumped to 15 by taylor to move stuff from pilot file
#define CAMPAIGN_FILE_VERSION							15
//#define CAMPAIGN_FILE_COMPATIBLE_VERSION		CAMPAIGN_INITIAL_RELEASE_FILE_VERSION
#define CAMPAIGN_FILE_COMPATIBLE_VERSION				12  // 12 is the version of the original FS2
#define CAMPAIGN_FILE_ID								0xbeefcafe

// variables with deal with the campaign stats save file
// bumped to 2 by taylor for ship/weapon table handling
#define CAMPAIGN_STATS_FILE_VERSION					2
#define CAMPAIGN_STATS_FILE_COMPATIBLE_VERSION	1
#define CAMPAIGN_STATS_FILE_ID						0xabbadaad

// mission_campaign_get_name returns a string (which is malloced in this routine) of the name
// of the given freespace campaign file.  In the type field, we return if the campaign is a single
// player or multiplayer campaign.  The type field will only be valid if the name returned is non-NULL
int mission_campaign_get_info(char *filename, char *name, int *type, int *max_players, char **desc)
{
	int rval, i, success = 0;
	char campaign_type[NAME_LENGTH], fname[MAX_FILENAME_LEN];

	Assert( name != NULL );
	Assert( type != NULL );

	strncpy(fname, filename, MAX_FILENAME_LEN - 1);
	if ((strlen(fname) < 4) || stricmp(fname + strlen(fname) - 4, FS_CAMPAIGN_FILE_EXT)){
		strcat(fname, FS_CAMPAIGN_FILE_EXT);
	}
	Assert(strlen(fname) < MAX_FILENAME_LEN);

	// open localization
	lcl_ext_open();

	*type = -1;
	do {
		if ((rval = setjmp(parse_abort)) != 0) {
			mprintf(("MISSIONCAMPAIGN: Unable to parse '%s'!  Error code = %i.\n", fname, rval));
			break;
		}

		read_file_text(fname);
		reset_parse();

		required_string("$Name:");
		stuff_string( name, F_NAME, NAME_LENGTH );
		if (name == NULL) {
			nprintf(("Warning", "No name found for campaign file %s\n", filename));
			break;
		}

		required_string("$Type:");
		stuff_string( campaign_type, F_NAME, NAME_LENGTH );

		for (i=0; i<MAX_CAMPAIGN_TYPES; i++) {
			if ( !stricmp(campaign_type, campaign_types[i]) ) {
				*type = i;
			}
		}

		if (desc) {
			if (optional_string("+Description:")) {
				*desc = stuff_and_malloc_string(F_MULTITEXT, NULL, MISSION_DESC_LENGTH);
			} else {
				*desc = NULL;
			}
		}

		// if this is a multiplayer campaign, get the max players
		if ((*type) != CAMPAIGN_TYPE_SINGLE) {
			skip_to_string("+Num Players:");
			stuff_int(max_players);
		}

		// if we found a valid campaign type
		if ((*type) >= 0) {
			success = 1;
		}
	} while (0);

	// close localization
	lcl_ext_close();

	Assert(success);
	return success;
}

// parses campaign and returns a list of missions in it.  Returns number of missions added to
// the 'list', and up to 'max' missions may be added to 'list'.  Returns negative on error.
//
int mission_campaign_get_mission_list(char *filename, char **list, int max)
{
	int rval, i, num = 0;
	char name[MAX_FILENAME_LEN];

	filename = cf_add_ext(filename, FS_CAMPAIGN_FILE_EXT);

	// read the campaign file and get the list of mission filenames
	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("MISSIONCAMPAIGN: Unable to parse '%s'!  Error code = %i.\n", filename, rval));

		// since we can't return count of allocated elements, free them instead
		for (i=0; i<num; i++)
			vm_free(list[i]);

		num = -1;

	} else {
		read_file_text(filename);
		reset_parse();

		while (skip_to_string("$Mission:") > 0) {
			stuff_string(name, F_NAME, MAX_FILENAME_LEN);
			if (num < max)
				list[num++] = vm_strdup(name);
		}
	}

	return num;
}

void mission_campaign_free_list()
{
	int i;

	if ( !Campaign_names_inited )
		return;

	for (i = 0; i < Num_campaigns; i++) {
		if (Campaign_names[i] != NULL) {
			vm_free(Campaign_names[i]);
			Campaign_names[i] = NULL;
		}

		if (Campaign_file_names[i] != NULL) {
			vm_free(Campaign_file_names[i]);
			Campaign_file_names[i] = NULL;
		}

		if (Campaign_descs[i] != NULL) {
			vm_free(Campaign_descs[i]);
			Campaign_descs[i] = NULL;
		}
	}

	Num_campaigns = 0;
	Campaign_names_inited = 0;
}

void mission_campaign_maybe_add( char *filename, int multiplayer )
{
	Int3();

/*	char name[NAME_LENGTH];
	int type, max_players;

	if ( mission_campaign_get_info( filename, name, &type, &max_players) ) {
		if ( !multiplayer && ( type == CAMPAIGN_TYPE_SINGLE) ) {
			Campaign_names[Num_campaigns] = vm_strdup(name);
			Campaign_file_names[Num_campaigns] = vm_strdup(filename);
			Num_campaigns++;
		}
	}*/
}

int mission_campaign_maybe_add(char *filename)
{
	char name[NAME_LENGTH];
	char *desc = NULL;
	int type, max_players;

	if ( mission_campaign_get_info( filename, name, &type, &max_players, &desc) ) {
		if ( !MC_multiplayer && (type == CAMPAIGN_TYPE_SINGLE) ) {
			Campaign_names[Num_campaigns] = vm_strdup(name);

			if (MC_desc)
				Campaign_descs[Num_campaigns] = desc;

			Num_campaigns++;

			return 1;
		}
	}

	if (desc != NULL)
		vm_free(desc);
 
	return 0;
}

// mission_campaign_build_list() builds up the list of campaigns that the user might
// be able to pick from.  It uses the multiplayer flag to tell if we should display a list
// of single or multiplayer campaigns.  This routine sets the Num_campaigns and Campaign_names
// global variables
void mission_campaign_build_list(bool desc, bool sort, bool multiplayer)
{
	char wild_card[10];
	int i, j, incr = 0;
	char *t = NULL;
	int rc = 0;

	if (Campaign_names_inited)
		return;

	MC_desc = desc;
	MC_multiplayer = multiplayer;

	memset(wild_card, 0, sizeof(wild_card));
	strcpy(wild_card, NOX("*"));
	strcat(wild_card, FS_CAMPAIGN_FILE_EXT);

	// if we have already been loaded then free everything and reload
	if (Num_campaigns != 0)
		mission_campaign_free_list();

	// set filter for cf_get_file_list() if there isn't one set already (the simroom has a special one)
	if (Get_file_list_filter == NULL)
		Get_file_list_filter = mission_campaign_maybe_add;


	// now get the list of all mission names
	// NOTE: we don't do sorting here, but we assume CF_SORT_NAME, and do it manually below
	rc = cf_get_file_list(MAX_CAMPAIGNS, Campaign_file_names, CF_TYPE_MISSIONS, wild_card, CF_SORT_NONE);
	Assert( rc == Num_campaigns );


	// now sort everything, if we are supposed to
	if (sort) {
		incr = Num_campaigns / 2;

		while (incr > 0) {
			for (i = incr; i < Num_campaigns; i++) {
				j = i - incr;
	
				while (j >= 0) {
					char *name1 = Campaign_names[j];
					char *name2 = Campaign_names[j + incr];

					// if we hit this then a coder probably did something dumb (like not needing to sort)
					if ( (name1 == NULL) || (name2 == NULL) ) {
						Int3();
						break;
					}

					if ( !strnicmp(name1, "the ", 4) )
						name1 += 4;

					if ( !strnicmp(name2, "the ", 4) )
						name2 += 4;

					if (stricmp(name1, name2) > 0) {
						// first, do filenames
						t = Campaign_file_names[j];
						Campaign_file_names[j] = Campaign_file_names[j + incr];
						Campaign_file_names[j + incr] = t;

						// next, actual names
						t = Campaign_names[j];
						Campaign_names[j] = Campaign_names[j + incr];
						Campaign_names[j + incr] = t;

						// finally, do descriptions
						if (desc) {
							t = Campaign_descs[j];
							Campaign_descs[j] = Campaign_descs[j + incr];
							Campaign_descs[j + incr] = t;
						}

						j -= incr;
					} else {
						break;
					}
				}
			}

			incr /= 2;
		}
	}

	// Done!
	Campaign_names_inited = 1;
}


// gets optional ship/weapon information
void mission_campaign_get_sw_info()
{
	int i, count, ship_list[MAX_SHIP_CLASSES], weapon_list[MAX_WEAPON_TYPES];

	// set allowable ships to the SIF_PLAYER_SHIPs
	memset( Campaign.ships_allowed, 0, sizeof(Campaign.ships_allowed) );
	for (i = 0; i < Num_ship_classes; i++ ) {
		if ( Ship_info[i].flags & SIF_PLAYER_SHIP )
			Campaign.ships_allowed[i] = 1;
	}

	for (i = 0; i < MAX_WEAPON_TYPES; i++ )
		Campaign.weapons_allowed[i] = 1;

	if ( optional_string("+Starting Ships:") ) {
		for (i = 0; i < Num_ship_classes; i++ )
			Campaign.ships_allowed[i] = 0;

		count = stuff_int_list(ship_list, MAX_SHIP_CLASSES, SHIP_INFO_TYPE);

		// now set the array elements stating which ships we are allowed
		for (i = 0; i < count; i++ ) {
			if ( Ship_info[ship_list[i]].flags & SIF_PLAYER_SHIP )
				Campaign.ships_allowed[ship_list[i]] = 1;
		}
	}

	if ( optional_string("+Starting Weapons:") ) {
		for (i = 0; i < MAX_WEAPON_TYPES; i++ )
			Campaign.weapons_allowed[i] = 0;

		count = stuff_int_list(weapon_list, MAX_WEAPON_TYPES, WEAPON_POOL_TYPE);

		// now set the array elements stating which ships we are allowed
		for (i = 0; i < count; i++ )
			Campaign.weapons_allowed[weapon_list[i]] = 1;
	}
}

// mission_campaign_load starts a new campaign.  It reads in the mission information in the campaign file
// It also sets up all variables needed inside of the game to deal with starting mission numbers, etc
//
// Note: Due to difficulties in generalizing this function, parts of it are duplicated throughout
// this file.  If you change the format of the campaign file, you should be sure these related
// functions work properly and update them if it breaks them.
int mission_campaign_load( char *filename, player *pl, int load_savefile )
{
	int len, rval, i;
	char name[NAME_LENGTH], type[NAME_LENGTH];

	filename = cf_add_ext(filename, FS_CAMPAIGN_FILE_EXT);

	// open localization
	lcl_ext_open();	

	if ( pl == NULL )
		pl = Player;

	if ( !Fred_running && load_savefile && (pl == NULL) ) {
		Int3();
		load_savefile = 0;
	}

	// read the mission file and get the list of mission filenames
	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("Error parsing '%s'\r\nError code = %i.\r\n", filename, rval));

		// close localization
		lcl_ext_close();

		// Ok, we have a problem.  We have a campaign set in the pilot file but can't find
		// the actual campaign.  In this case we bypass the norm and try to access the campaign
		// savefile anyway.  We HAVE to do this or we get data loss at some point. - taylor
		if ( !Fred_running && !(pl->flags & PLAYER_FLAGS_IS_MULTI) ) {
			if ( mission_campaign_savefile_load(filename, pl) ) {
				Campaign_file_missing = 1;
				return CAMPAIGN_ERROR_MISSING;
			}
		}

		Campaign.filename[0] = 0;
		Campaign.num_missions = 0;
	
		return CAMPAIGN_ERROR_CORRUPT;

	} else {
		// be sure to remove all old malloced strings of Mission_names
		// we must also free any goal stuff that was from a previous campaign
		// this also frees sexpressions so the next call to init_sexp will be able to reclaim
		// nodes previously used by another campaign.
		mission_campaign_close();

		strcpy( Campaign.filename, filename );

		// only initialize the sexpression stuff when Fred isn't running.  It'll screw things up major
		// if it does
		if ( !Fred_running ){
			init_sexp();		// must initialize the sexpression stuff
		}

		read_file_text( filename );
		reset_parse();
		memset( &Campaign, 0, sizeof(Campaign) );

		// copy filename to campaign structure minus the extension
		len = strlen(filename) - 4;
		Assert(len < MAX_FILENAME_LEN);
		strncpy(Campaign.filename, filename, len);
		Campaign.filename[len] = 0;

		required_string("$Name:");
		stuff_string( name, F_NAME, NAME_LENGTH );
		
		//Store campaign name in the global struct
		strcpy( Campaign.name, name );

		required_string( "$Type:" );
		stuff_string( type, F_NAME, NAME_LENGTH );

		for (i = 0; i < MAX_CAMPAIGN_TYPES; i++ ) {
			if ( !stricmp(type, campaign_types[i]) ) {
				Campaign.type = i;
				break;
			}
		}

		if ( i == MAX_CAMPAIGN_TYPES )
			Error(LOCATION, "Unknown campaign type %s!", type);

		Campaign.desc = NULL;
		if (optional_string("+Description:"))
			Campaign.desc = stuff_and_malloc_string(F_MULTITEXT, NULL, MISSION_DESC_LENGTH);

		// if the type is multiplayer -- get the number of players
		Campaign.num_players = 0;
		if ( Campaign.type != CAMPAIGN_TYPE_SINGLE) {
			required_string("+Num players:");
			stuff_int( &(Campaign.num_players) );
		}		

		// parse flags - Goober5000
		Campaign.flags = CF_DEFAULT_VALUE;
		if (optional_string("$Flags:"))
		{
			stuff_int( &(Campaign.flags) );
		}

		// parse the optional ship/weapon information
		mission_campaign_get_sw_info();

		// parse the mission file and actually read in the mission stuff
		Campaign.num_missions = 0;
		while ( required_string_either("#End", "$Mission:") ) {
			cmission *cm;

			required_string("$Mission:");
			stuff_string(name, F_NAME, NAME_LENGTH);
			cm = &Campaign.missions[Campaign.num_missions];
			cm->name = vm_strdup(name);

			cm->briefing_cutscene[0] = 0;
			if ( optional_string("+Briefing Cutscene:") )
				stuff_string( cm->briefing_cutscene, F_NAME, NAME_LENGTH );

			cm->flags = 0;
			if (optional_string("+Flags:"))
				stuff_int(&cm->flags);

			// Goober5000 - new main hall stuff!
			cm->main_hall = 0;
			if (optional_string("+Main Hall:"))
				stuff_ubyte(&cm->main_hall);

			// deal with previous campaign versions
			if (cm->flags & CMISSION_FLAG_BASTION)
				cm->main_hall = 1;

			// Goober5000 - new debriefing persona stuff!
			cm->debrief_persona_index = 0;
			if (optional_string("+Debriefing Persona Index:"))
				stuff_ubyte(&cm->debrief_persona_index);

			cm->formula = -1;
			if ( optional_string("+Formula:") ) {
				cm->formula = get_sexp_main();
				if ( !Fred_running ) {
					Assert ( cm->formula != -1 );
					sexp_mark_persistent( cm->formula );

				} else {
					if ( cm->formula == -1 ){
						// close localization
						lcl_ext_close();

						return CAMPAIGN_ERROR_SEXP_EXHAUSTED;
					}
				}
			}

			// Do mission looping stuff
			cm->has_mission_loop = 0;
			if ( optional_string("+Mission Loop:") ) {
				cm->has_mission_loop = 1;
			}

			cm->mission_loop_desc = NULL;
			if ( optional_string("+Mission Loop Text:")) {
				cm->mission_loop_desc = stuff_and_malloc_string(F_MULTITEXT, NULL, MISSION_DESC_LENGTH);
			}

			cm->mission_loop_brief_anim = NULL;
			if ( optional_string("+Mission Loop Brief Anim:")) {
				cm->mission_loop_brief_anim = stuff_and_malloc_string(F_MULTITEXT, NULL, MAX_FILENAME_LEN);
			}

			cm->mission_loop_brief_sound = NULL;
			if ( optional_string("+Mission Loop Brief Sound:")) {
				cm->mission_loop_brief_sound = stuff_and_malloc_string(F_MULTITEXT, NULL, MAX_FILENAME_LEN);
			}

			cm->mission_loop_formula = -1;
			if ( optional_string("+Formula:") ) {
				cm->mission_loop_formula = get_sexp_main();
				if ( !Fred_running ) {
					Assert ( cm->mission_loop_formula != -1 );
					sexp_mark_persistent( cm->mission_loop_formula );

				} else {
					if ( cm->mission_loop_formula == -1 ){
						// close localization
						lcl_ext_close();

						return CAMPAIGN_ERROR_SEXP_EXHAUSTED;
					}
				}
			}

			if (optional_string("+Level:")) {
				stuff_int( &cm->level );
				if ( cm->level == 0 )  // check if the top (root) of the whole tree
					Campaign.next_mission = Campaign.num_missions;

			} else
				Campaign.realign_required = 1;

			if (optional_string("+Position:"))
				stuff_int( &cm->pos );
			else
				Campaign.realign_required = 1;

			if (Fred_running) {
				cm->num_goals = -1;
				cm->num_events = -1;
				cm->num_saved_variables = -1;
				cm->notes = NULL;

			} else {
				cm->num_goals = 0;
				cm->num_events = 0;
				cm->num_saved_variables = -1;
			}

			cm->goals = NULL;
			cm->events = NULL;
			cm->saved_variables = NULL;
			Campaign.num_missions++;
		}
	}

	// set up the other variables for the campaign stuff.  After initializing, we must try and load
	// the campaign save file for this player.  Since all campaign loads go through this routine, I
	// think this place should be the only necessary place to load the campaign save stuff.  The campaign
	// save file will get written when a mission has ended by player choice.
	Campaign.next_mission = 0;
	Campaign.prev_mission = -1;
	Campaign.current_mission = -1;
	Campaign.loop_mission = CAMPAIGN_LOOP_MISSION_UNINITIALIZED;

	// loading the campaign will get us to the current and next mission that the player must fly
	// plus load all of the old goals that future missions might rely on.
	if (!Fred_running && load_savefile && (Campaign.type == CAMPAIGN_TYPE_SINGLE)) {
		if (pl == NULL) {
			Assert((Player_num >= 0) && (Player_num < MAX_PLAYERS));
			pl = &Players[Player_num];
		}

		Assert( pl != NULL );

		mission_campaign_savefile_load(Campaign.filename, pl);
	}

	// close localization
	lcl_ext_close();

	// all is good here, move along
	Campaign_file_missing = 0;

	return 0;
}

// mission_campaign_load_by_name() loads up a freespace campaign given the filename.  This routine
// is used to load up campaigns when a pilot file is loaded.  Generally, the
// filename will probably be the freespace campaign file, but not necessarily.
int mission_campaign_load_by_name( char *filename )
{
	char name[NAME_LENGTH],test[5];
	int type,max_players;

	// make sure to tack on .fsc on the end if its not there already
	if(strlen(filename) > 0){
		if(strlen(filename) > 4){
			strcpy(test,filename+(strlen(filename)-4));
			if(strcmp(test, FS_CAMPAIGN_FILE_EXT)!=0){
				strcat(filename, FS_CAMPAIGN_FILE_EXT);
			}
		} else {
			strcat(filename, FS_CAMPAIGN_FILE_EXT);
		}
	} else {
		Error(LOCATION,"Tried to load campaign file with illegal length/extension!");
	}

	if (!mission_campaign_get_info(filename, name, &type, &max_players)){
		return -1;	
	}

	Num_campaigns = 0;
	Campaign_file_names[Num_campaigns] = filename;
	Campaign_names[Num_campaigns] = name;
	Num_campaigns++;
	mission_campaign_load(filename);		
	return 0;
}

int mission_campaign_load_by_name_csfe( char *filename, char *callsign )
{
	Game_mode |= GM_NORMAL;
	strcpy(Player->callsign, callsign);
	return mission_campaign_load_by_name( filename);
}


// mission_campaign_init initializes some variables then loads the default FreeSpace single player campaign.
void mission_campaign_init()
{
	memset(&Campaign, 0, sizeof(Campaign) );

	Campaign_file_missing = 0;
}

// Fill in the root of the campaign save filename
void mission_campaign_savefile_generate_root(char *filename, player *pl)
{
	char base[_MAX_FNAME];

	Assert ( strlen(Campaign.filename) != 0 );

	if (pl == NULL) {
		Assert((Player_num >= 0) && (Player_num < MAX_PLAYERS));
		pl = &Players[Player_num];
	}

	Assert( pl != NULL );

	// build up the filename for the save file.  There could be a problem with filename length,
	// but this problem can get fixed in several ways -- ignore the problem for now though.
	_splitpath( Campaign.filename, NULL, NULL, base, NULL );

	Assert ( (strlen(base) + strlen(pl->callsign) + 1) < _MAX_FNAME );

	sprintf( filename, NOX("%s.%s."), pl->callsign, base );
}

// mission_campaign_savefile_save saves the state of the campaign.  This function will probably always be called
// then the player is done flying a mission in the campaign path.  It will save the missions played, the
// state of the goals, etc.
int mission_campaign_savefile_save()
{
	char filename[_MAX_FNAME];
	CFILE *fp;
	int i,j;

	// never try to save the campaign file for the standalone server or in multiplayer
	if ((Game_mode & GM_MULTIPLAYER) && (Game_mode & GM_STANDALONE_SERVER)) {
		return 0;
	}

	Assert((Player_num >= 0) && (Player_num < MAX_PLAYERS));
	player *pl = &Players[Player_num];

	Assert( pl != NULL );

	// if this is a multi pilot then we shouldn't even be here!!
	if (pl->flags & PLAYER_FLAGS_IS_MULTI)
		return 0;

	// catch a case where the campaign hasn't been switched yet after being unavailable
	if ( strlen(Campaign.filename) == 0 )
		return 0;

	// make sure that we don't try to save if the campaign is missing since it's
	// a pretty good bet that the data isn't sane.
	if ( Campaign_file_missing )
		return 0;

	memset(filename, 0, _MAX_FNAME);
	mission_campaign_savefile_generate_root(filename, pl);

	// name the file differently depending on whether we're in single player or multiplayer mode
	// single player : *.csg
	strcat( filename, NOX("cs2"));	// use new filename with new format - taylor

	fp = cfopen(filename,"wb", CFILE_NORMAL, CF_TYPE_SINGLE_PLAYERS);

	if (!fp)
		return errno;

	// Write out campaign file info
	cfwrite_int( CAMPAIGN_FILE_ID,fp );
	cfwrite_int( CAMPAIGN_FILE_VERSION,fp );

	// put in the file signature (single or multiplayer campaign) - see MissionCampaign.h for the #defines
	cfwrite_int( CAMPAIGN_SINGLE_PLAYER_SIG, fp );

	// do we need to write out the filename of the campaign?
	cfwrite_string_len( Campaign.filename, fp );
	cfwrite_int( Campaign.prev_mission, fp );
	cfwrite_int( Campaign.next_mission, fp );
	cfwrite_int( Campaign.loop_reentry, fp );
	cfwrite_int( Campaign.loop_enabled, fp );

	// write out the information for ships/weapons which this player is allowed to use
	cfwrite_int(Num_ship_classes, fp);
	cfwrite_int(Num_weapon_types, fp);

	for ( i = 0; i < Num_ship_classes; i++ ){
		cfwrite_char( Campaign.ships_allowed[i], fp );
		cfwrite_string_len( Ship_info[i].name, fp );
	}

	for ( i = 0; i < Num_weapon_types; i++ ){
		cfwrite_char( Campaign.weapons_allowed[i], fp );
		cfwrite_string_len( Weapon_info[i].name, fp );
	}

	// write out the completed mission matrix.  Used to tell which missions the player
	// can replay in the simulator.  Also, each completed mission contains a list of the goals
	// that were in the mission along with the goal completion status.
	cfwrite_int( Campaign.num_missions_completed, fp );
	for (i = 0; i < MAX_CAMPAIGN_MISSIONS; i++ ) {
		if ( Campaign.missions[i].completed ) {
			cfwrite_int( i, fp );
			cfwrite_int( Campaign.missions[i].num_goals, fp );
			for ( j = 0; j < Campaign.missions[i].num_goals; j++ ) {
				cfwrite_string_len( Campaign.missions[i].goals[j].name, fp );
				cfwrite_char( Campaign.missions[i].goals[j].status, fp );
			}
			cfwrite_int( Campaign.missions[i].num_events, fp );
			for ( j = 0; j < Campaign.missions[i].num_events; j++ ) {
				cfwrite_string_len( Campaign.missions[i].events[j].name, fp );
				cfwrite_char( Campaign.missions[i].events[j].status, fp );
			}

			// write out campaign-persistent variables - Goober5000
			cfwrite_int( Campaign.missions[i].num_saved_variables, fp );
			for ( j = 0; j < Campaign.missions[i].num_saved_variables; j++ ) {
				cfwrite_int( Campaign.missions[i].saved_variables[j].type, fp );
				cfwrite_string_len( Campaign.missions[i].saved_variables[j].text, fp );
				cfwrite_string_len( Campaign.missions[i].saved_variables[j].variable_name, fp );
			}

			// write out the stats information to disk.	
			scoring_struct stats_tmp;
			memset( &stats_tmp, 0, sizeof(scoring_struct) );
			memcpy( &stats_tmp, &Campaign.missions[i].stats, sizeof(scoring_struct) );

			// swap values if needed
			for ( j = 0; j < Num_ship_classes; j++ )
				stats_tmp.kills[j] = INTEL_INT(Campaign.missions[i].stats.kills[j]);

			stats_tmp.score = INTEL_INT(Campaign.missions[i].stats.score);
			stats_tmp.rank = INTEL_INT(Campaign.missions[i].stats.rank);
			stats_tmp.assists = INTEL_INT(Campaign.missions[i].stats.assists);
			stats_tmp.kill_count = INTEL_INT(Campaign.missions[i].stats.kill_count);
			stats_tmp.kill_count_ok = INTEL_INT(Campaign.missions[i].stats.kill_count_ok);
			stats_tmp.p_shots_fired = INTEL_INT(Campaign.missions[i].stats.p_shots_fired);
			stats_tmp.s_shots_fired = INTEL_INT(Campaign.missions[i].stats.s_shots_fired);
			stats_tmp.p_shots_hit = INTEL_INT(Campaign.missions[i].stats.p_shots_hit);
			stats_tmp.s_shots_hit = INTEL_INT(Campaign.missions[i].stats.s_shots_hit);
			stats_tmp.p_bonehead_hits = INTEL_INT(Campaign.missions[i].stats.p_bonehead_hits);
			stats_tmp.s_bonehead_hits = INTEL_INT(Campaign.missions[i].stats.s_bonehead_hits);
			stats_tmp.bonehead_kills = INTEL_INT(Campaign.missions[i].stats.bonehead_kills);

			for ( j = 0; j < MAX_MEDALS; j++ )
				stats_tmp.medals[j] = INTEL_INT(Campaign.missions[i].stats.medals[j]);

			// save to file
			cfwrite( &stats_tmp, sizeof(scoring_struct), 1, fp );

			// write flags
			cfwrite_int(Campaign.missions[i].flags, fp);
		}
	}

	// our current mainhall
	cfwrite_ubyte((ubyte) pl->main_hall, fp);

	// red-alert status
	red_alert_write_wingman_status_campaign(fp);

	// start techroom data -----------------------------------------------------
	ubyte out;

	// write the ship and weapon count
	cfwrite_int(Intel_info_size, fp);

	// write all ship flags out
	for (i=0; i<Num_ship_classes; i++) {
		out = (Ship_info[i].flags & SIF_IN_TECH_DATABASE) ? (ubyte)1 : (ubyte)0;		
		cfwrite_ubyte(out, fp);				
	}

	// write all weapon types out
	for (i=0; i<Num_weapon_types; i++) {
		out = (Weapon_info[i].wi_flags & WIF_IN_TECH_DATABASE) ? (ubyte)1 : (ubyte)0;
		cfwrite_ubyte(out, fp);
	}	

	// write all intel entry flags out
	for (i=0; i<Intel_info_size; i++) {
		out = (Intel_info[i].flags & IIF_IN_TECH_DATABASE) ? (ubyte)1 : (ubyte)0;
		cfwrite_ubyte(out, fp);
	}
	// end techroom data -------------------------------------------------------

	// begin player loadout ----------------------------------------------------
	wss_unit *slot;	

	cfwrite_string_len(Player_loadout.filename, fp);
	cfwrite_string_len(Player_loadout.last_modified, fp);

	// write ship pool
	for ( i = 0; i < Num_ship_classes; i++ ) {
		cfwrite_int(Player_loadout.ship_pool[i], fp);
	}

	// write weapons pool
	for ( i = 0; i < Num_weapon_types; i++ ) {
		cfwrite_int(Player_loadout.weapon_pool[i], fp);
	}

	// write ship loadouts
	for ( i = 0; i < MAX_WSS_SLOTS; i++ ) {
		slot = &Player_loadout.unit_data[i];
		cfwrite_int(slot->ship_class, fp);

		for ( j = 0; j < MAX_SHIP_WEAPONS; j++ ) {
			cfwrite_int(slot->wep[j], fp);
			cfwrite_int(slot->wep_count[j], fp);
		}
	}
	// end player loadout ------------------------------------------------------

	// begin player stats ------------------------------------------------------
	cfwrite_int(pl->stats.score, fp);
	cfwrite_int(pl->stats.rank, fp);
	cfwrite_int(pl->stats.assists, fp);

	Assert(Num_medals == MAX_MEDALS);

	for (i=0; i<Num_medals; i++){
		cfwrite_int(pl->stats.medals[i], fp);
	}

	for(;i<MAX_MEDALS;i++){
		cfwrite_int(0, fp);
	}

	int total = MAX_SHIP_CLASSES;
	while (total && !pl->stats.kills[total - 1]){  // find last used element
		total--;
	}

	cfwrite_int(total, fp);
	for (i=0; i<total; i++){
		cfwrite_ushort((ushort)pl->stats.kills[i], fp);
	//	cfwrite_string_len(Ship_info[i].name, fp);
	}

	cfwrite_int(pl->stats.kill_count, fp);
	cfwrite_int(pl->stats.kill_count_ok, fp);

	cfwrite_uint(pl->stats.p_shots_fired, fp);
	cfwrite_uint(pl->stats.s_shots_fired, fp);
	cfwrite_uint(pl->stats.p_shots_hit, fp);
	cfwrite_uint(pl->stats.s_shots_hit, fp);
	cfwrite_uint(pl->stats.p_bonehead_hits, fp);
	cfwrite_uint(pl->stats.s_bonehead_hits, fp);
	cfwrite_uint(pl->stats.bonehead_kills, fp);
	// end stats ---------------------------------------------------------------

	// write the cutscenes which have been viewed
	cfwrite_int(Cutscenes_viewable, fp);

	cfclose( fp );

	return 0;
}


// The following function always only ever ever ever called by CSFE!!!!!
int campaign_savefile_save(char *pname)
{
	if (Campaign.type == CAMPAIGN_TYPE_SINGLE)
		Game_mode &= ~GM_MULTIPLAYER;
	else
		Game_mode |= GM_MULTIPLAYER;

	strcpy(Player->callsign, pname);
	//memcpy(&Campaign, camp, sizeof(campaign));
	return mission_campaign_savefile_save();
}


// the below two functions is internal to this module.  It is here so that I can group the save/load
// functions together.
//

// mission_campaign_savefile_delete deletes any save file in the players directory for the given
// campaign filename
void mission_campaign_savefile_delete( char *cfilename, int is_multi )
{
	char filename[_MAX_FNAME], base[_MAX_FNAME];

	_splitpath( cfilename, NULL, NULL, base, NULL );

	if ( Player->flags & PLAYER_FLAGS_IS_MULTI ) {
		return;	// no such thing as a multiplayer campaign savefile
	}

	sprintf( filename, NOX("%s.%s.cs2"), Player->callsign, base ); // only support the new filename here - taylor

	cf_delete( filename, CF_TYPE_SINGLE_PLAYERS );
}

// same as mission_campaign_savefile_delete() except that it deletes old format .csg and .css
void mission_campaign_savefile_delete_old( char *cfilename)
{
	char filename[_MAX_FNAME], base[_MAX_FNAME];
	char filename2[_MAX_FNAME];

	_splitpath( cfilename, NULL, NULL, base, NULL );

	if ( Player->flags & PLAYER_FLAGS_IS_MULTI ) {
		return;	// no such thing as a multiplayer campaign savefile
	}

	sprintf( filename, NOX("%s.%s.csg"), Player->callsign, base );
	sprintf( filename2, NOX("%s.%s.css"), Player->callsign, base );

	cf_delete( filename, CF_TYPE_SINGLE_PLAYERS );
	cf_delete( filename2, CF_TYPE_SINGLE_PLAYERS );
}

void campaign_delete_save( char *cfn, char *pname)
{
	strcpy(Player->callsign, pname);
	mission_campaign_savefile_delete(cfn);
}

// next function deletes all the save files for this particular pilot.  Just call cfile function
// which will delete multiple files
// Player_select_mode tells us whether we are deleting single or multiplayer files
void mission_campaign_delete_all_savefiles( char *pilot_name, int is_multi )
{
	int dir_type, num_files, i;
	char *names[MAX_CAMPAIGNS], file_spec[MAX_FILENAME_LEN + 2], *ext;
	char filename[1024];
	int (*filter_save)(char *filename);

	if ( is_multi ) {
		return;				// can't have multiplayer campaign save files
	}

	ext = NOX(".cs2");
	dir_type = CF_TYPE_SINGLE_PLAYERS;

	sprintf(file_spec, NOX("%s.*%s"), pilot_name, ext);

	// HACK HACK HACK HACK!!!!  cf_get_file_list is not reentrant.  Pretty dumb because it should
	// be.  I have to save any file filters
	filter_save = Get_file_list_filter;
	Get_file_list_filter = NULL;
	num_files = cf_get_file_list(MAX_CAMPAIGNS, names, dir_type, file_spec);
	Get_file_list_filter = filter_save;

	for (i=0; i<num_files; i++) {
		strcpy(filename, names[i]);
		strcat(filename, ext);
		cf_delete(filename, dir_type);
		vm_free(names[i]);
	}
}

// mission_campaign_savefile_load takes a filename of a campaign file as a parameter and loads all
// of the information stored in the campaign file.
int mission_campaign_savefile_load( char *cfilename, player *pl )
{
	char filename[_MAX_FNAME], base[_MAX_FNAME];
	int id, version, i, num, j, num_stats_blocks, idx;
	int type_sig;
	ubyte sw;
	char s_name[MAX_SHIP_CLASSES][NAME_LENGTH];
	char w_name[MAX_WEAPON_TYPES][NAME_LENGTH];
	int n_ships;
	CFILE *fp;
	int force_update = 0;
	int kill_count[MAX_SHIP_CLASSES];
	int k_count;
	int sid = -1, wid = -1;
	int set_defaults = 1; // should we zero out tech values or not (yes by default)

	Assert ( strlen(cfilename) != 0 );

	if ( !strlen(cfilename) )
		return 0;

	if (pl == NULL) {
		Assert((Player_num >= 0) && (Player_num < MAX_PLAYERS));
		pl = &Players[Player_num];
	}

	Assert( pl != NULL );

	// if this pilot is multi then we shouldn't even be here!!
	if (pl->flags & PLAYER_FLAGS_IS_MULTI)
		return 0;

	// little quick hackery to avoid overwriting current techroom/campaing data with what's in the campaign file
	// if and only if this campaign is what's currently active (shouldn't do this from the pilotselect screen though)
	extern int Player_select_screen_active;
	if (!Player_select_screen_active && !strcmp(cfilename, pl->current_campaign)) {
		set_defaults = 0;
	}

	// probably only called from single player games anymore!!! should be anyway
//	Assert( Game_mode & GM_NORMAL );		// get allender or DaveB.  trying to save campaign in multiplayer

	// build up the filename for the save file.  There could be a problem with filename length,
	// but this problem can get fixed in several ways -- ignore the problem for now though.
	_splitpath( cfilename, NULL, NULL, base, NULL );

	Assert ( (strlen(base) + strlen(pl->callsign) + 1) < _MAX_FNAME );

	// support new filenames by default - taylor
	if(Game_mode & GM_MULTIPLAYER)
		sprintf( filename, NOX("%s.%s.ms2"), pl->callsign, base );
	else
		sprintf( filename, NOX("%s.%s.cs2"), pl->callsign, base );

	fp = cfopen(filename, "rb", CFILE_NORMAL, CF_TYPE_SINGLE_PLAYERS );

	// if we didn't open the new formats try the old ones - taylor
	if ( !fp ) {
		if(Game_mode & GM_MULTIPLAYER)
			sprintf( filename, NOX("%s.%s.msg"), pl->callsign, base );
		else
			sprintf( filename, NOX("%s.%s.csg"), pl->callsign, base );

		fp = cfopen(filename, "rb", CFILE_NORMAL, CF_TYPE_SINGLE_PLAYERS );

		if ( !fp )
			return 0;
	}

	id = cfread_int( fp );

	if ( id != (int)CAMPAIGN_FILE_ID ) {
		Warning(LOCATION, "Campaign save file has invalid signature");
		cfclose( fp );
		return 0;
	}

	version = cfread_int( fp );
	if ( version < CAMPAIGN_FILE_COMPATIBLE_VERSION ) {
		Warning(LOCATION, "Campaign save file too old -- not compatible.  Deleting file.\nYou can continue from here without trouble\n\n");
		cfclose( fp );
		cf_delete( filename, CF_TYPE_SINGLE_PLAYERS );
		return 0;
	}

	// verify that we are loading the correct type of campaign file for the mode that we are in.
	if(version >= 3)
		type_sig = cfread_int( fp );
	else
		type_sig = (int)CAMPAIGN_SINGLE_PLAYER_SIG;

	// the actual check
	Assert( ((Game_mode & GM_MULTIPLAYER) && (type_sig == (int)CAMPAIGN_MULTI_PLAYER_SIG)) || (!(Game_mode & GM_MULTIPLAYER) && (type_sig == (int)CAMPAIGN_SINGLE_PLAYER_SIG)) );

	Campaign.type = type_sig == (int)CAMPAIGN_SINGLE_PLAYER_SIG ? CAMPAIGN_TYPE_SINGLE : CAMPAIGN_TYPE_MULTI_COOP;

	// read in the filename of the campaign and compare the filenames to be sure that
	// we are reading data that really belongs to this campaign.  I think that this check
	// is redundant.
	cfread_string_len( filename, _MAX_FNAME, fp );
	/*if ( stricmp( filename, cfilename) ) {	//	Used to be !stricmp.  How did this ever work? --MK, 11/9/97
		Warning(LOCATION, "Campaign save file appears corrupt because of mismatching filenames.");
		cfclose(fp);
		return;
	}*/

	Campaign.prev_mission = cfread_int( fp );
	Campaign.next_mission = cfread_int( fp );
	Campaign.loop_reentry = cfread_int( fp );
	Campaign.loop_enabled = cfread_int( fp );

	//  load information about ships/weapons allowed	
	int ship_count, weapon_count;

	// if earlier than mission disk version, use old MAX_SHIP_CLASSES, otherwise read from the file		
	if(version <= CAMPAIGN_INITIAL_RELEASE_FILE_VERSION){
		ship_count = CAMPAIGN_SAVEFILE_MAX_SHIPS_OLD;
		weapon_count = CAMPAIGN_SAVEFILE_MAX_WEAPONS_OLD;
	} else {
		ship_count = cfread_int(fp);
		weapon_count = cfread_int(fp);
	}

	Assert(ship_count <= MAX_SHIP_CLASSES);
	Assert(weapon_count <= MAX_WEAPON_TYPES);

	memset( s_name, 0, sizeof(char) * MAX_SHIP_CLASSES * NAME_LENGTH );
	memset( w_name, 0, sizeof(char) * MAX_WEAPON_TYPES * NAME_LENGTH );

	for ( i = 0; i < ship_count; i++ ){
		if (version >= 14) {
			sw = cfread_ubyte( fp );
			cfread_string_len(s_name[i], NAME_LENGTH, fp);
			sid = ship_info_lookup(s_name[i]);
			if (sid == -1)
				continue;

			Campaign.ships_allowed[sid] = sw;
		} else {
			Campaign.ships_allowed[i] = cfread_ubyte( fp );
		}
	}

	for ( i = 0; i < weapon_count; i++ ){
		if (version >= 14) {
			sw = cfread_ubyte( fp );
			cfread_string_len(w_name[i], NAME_LENGTH, fp);
			wid = weapon_info_lookup(w_name[i]);
			if (wid == -1)
				continue;

			Campaign.weapons_allowed[wid] = sw;
		} else {
			Campaign.weapons_allowed[i] = cfread_ubyte( fp );
		}
	}	

	// read in the completed mission matrix.  Used to tell which missions the player
	// can replay in the simulator.  Also, each completed mission contains a list of the goals
	// that were in the mission along with the goal completion status.
	Campaign.num_missions_completed = cfread_int( fp );
	for (i = 0; i < Campaign.num_missions_completed; i++ ) {
		num = cfread_int( fp );
		Campaign.missions[num].completed = 1;
		Campaign.missions[num].num_goals = cfread_int( fp );
		
		// be sure to malloc out space for the goals stuff, then zero the memory!!!  Don't do malloc
		// if there are no goals
		if ( Campaign.missions[num].num_goals > 0 ) {
			Campaign.missions[num].goals = (mgoal *)vm_malloc( Campaign.missions[num].num_goals * sizeof(mgoal) );

			Assert( Campaign.missions[num].goals != NULL );

			if (Campaign.missions[num].goals != NULL)
				memset( Campaign.missions[num].goals, 0, sizeof(mgoal) * Campaign.missions[num].num_goals );

			// now read in the goal information for this mission
			for ( j = 0; j < Campaign.missions[num].num_goals; j++ ) {
				cfread_string_len( Campaign.missions[num].goals[j].name, NAME_LENGTH, fp );
				Campaign.missions[num].goals[j].status = cfread_char( fp );
			}
		}

		// get the events from the savefile
		Campaign.missions[num].num_events = cfread_int( fp );
		
		// be sure to malloc out space for the events stuff, then zero the memory!!!  Don't do malloc
		// if there are no events
		if ( Campaign.missions[num].num_events > 0 ) {
			Campaign.missions[num].events = (mevent *)vm_malloc( Campaign.missions[num].num_events * sizeof(mevent) );

			Assert( Campaign.missions[num].events != NULL );

			if (Campaign.missions[num].events != NULL)
				memset( Campaign.missions[num].events, 0, sizeof(mevent) * Campaign.missions[num].num_events );
	
			// now read in the event information for this mission
			for ( j = 0; j < Campaign.missions[num].num_events; j++ ) {
				cfread_string_len( Campaign.missions[num].events[j].name, NAME_LENGTH, fp );
				Campaign.missions[num].events[j].status = cfread_char( fp );
			}
		}

		// Goober5000 - get the variables from the savefile -------------------------
		if (version >= 13) {
			Campaign.missions[num].num_saved_variables = cfread_int( fp );
		
			// be sure to malloc out space for the variables stuff, then zero the memory!!!  Don't do malloc
			// if there are no variables
			if ( Campaign.missions[num].num_saved_variables > 0 ) {
				Campaign.missions[num].saved_variables = (sexp_variable *)vm_malloc( Campaign.missions[num].num_saved_variables * sizeof(sexp_variable) );

				Assert( Campaign.missions[num].saved_variables != NULL );

				if (Campaign.missions[num].saved_variables != NULL)
					memset( Campaign.missions[num].saved_variables, 0, sizeof(sexp_variable) * Campaign.missions[num].num_saved_variables );

				// now read in the variable information for this mission
				for ( j = 0; j < Campaign.missions[num].num_saved_variables; j++ ) {
					Campaign.missions[num].saved_variables[j].type = cfread_int( fp );
					cfread_string_len( Campaign.missions[num].saved_variables[j].text, TOKEN_LENGTH, fp );
					cfread_string_len( Campaign.missions[num].saved_variables[j].variable_name, TOKEN_LENGTH, fp );
				}
			}
		}

		// mission specific stats - taylor
		if (version >= 15) {
			cfread( &Campaign.missions[num].stats, sizeof(scoring_struct), 1, fp );

			// copy current values to temp and swap if needed
			for ( j = 0; j < ship_count; j++ )
				kill_count[j] = INTEL_INT(Campaign.missions[num].stats.kills[j]);

			// translate to new tables
			for ( j = 0; j < ship_count; j++ ) {
				int kn = ship_info_lookup(s_name[j]);
				Campaign.missions[num].stats.kills[kn] = kill_count[j];
			}

			// swap values
			Campaign.missions[num].stats.score = INTEL_INT(Campaign.missions[num].stats.score);
			Campaign.missions[num].stats.rank = INTEL_INT(Campaign.missions[num].stats.rank);
			Campaign.missions[num].stats.assists = INTEL_INT(Campaign.missions[num].stats.assists);
			Campaign.missions[num].stats.kill_count = INTEL_INT(Campaign.missions[num].stats.kill_count);
			Campaign.missions[num].stats.kill_count_ok = INTEL_INT(Campaign.missions[num].stats.kill_count_ok);
			Campaign.missions[num].stats.p_shots_fired = INTEL_INT(Campaign.missions[num].stats.p_shots_fired);
			Campaign.missions[num].stats.s_shots_fired = INTEL_INT(Campaign.missions[num].stats.s_shots_fired);
			Campaign.missions[num].stats.p_shots_hit = INTEL_INT(Campaign.missions[num].stats.p_shots_hit);
			Campaign.missions[num].stats.s_shots_hit = INTEL_INT(Campaign.missions[num].stats.s_shots_hit);
			Campaign.missions[num].stats.p_bonehead_hits = INTEL_INT(Campaign.missions[num].stats.p_bonehead_hits);
			Campaign.missions[num].stats.s_bonehead_hits = INTEL_INT(Campaign.missions[num].stats.s_bonehead_hits);
			Campaign.missions[num].stats.bonehead_kills = INTEL_INT(Campaign.missions[num].stats.bonehead_kills);

			for (j=0; j < MAX_MEDALS; j++) {
				Campaign.missions[num].stats.medals[j] = INTEL_INT(Campaign.missions[num].stats.medals[j]);
			}
		}

		// done with variables ------------------------------------------------------

		// now read flags
		Campaign.missions[num].flags = cfread_int(fp);
	}
	
	// total stuff, moved here from pilot file ---------------------------------
	if (version >= 15) {
		// which mainhall are we on?
		pl->main_hall = cfread_ubyte(fp);
	
		// red-alert wingman status
		red_alert_read_wingman_status_campaign(fp, s_name, w_name);

		// begin techroom data -------------------------------------------------
		int intel_count;
		ubyte in;

		intel_count = cfread_int(fp);
		Assert(intel_count <= MAX_INTEL_ENTRIES);

		// zero out all data so that we can start anew
		if (set_defaults) {
			for (idx=0; idx<MAX_SHIP_CLASSES; idx++) {
				Ship_info[idx].flags &= ~SIF_IN_TECH_DATABASE;
			}
		
			for (idx=0; idx<MAX_WEAPON_TYPES; idx++) {
				Weapon_info[idx].wi_flags &= ~WIF_IN_TECH_DATABASE;
			}

			for (idx=0; idx<MAX_INTEL_ENTRIES; idx++) {
				Intel_info[idx].flags &= ~IIF_IN_TECH_DATABASE;
			}
		}

		// read all ships in
		for (idx=0; idx<ship_count; idx++) {
			in = cfread_ubyte(fp);

			sid = ship_info_lookup(s_name[idx]);
			if (sid == -1)
				continue;

			if (in) {
				Ship_info[sid].flags |= SIF_IN_TECH_DATABASE | SIF_IN_TECH_DATABASE_M;
			} else if (set_defaults) {
				Ship_info[sid].flags &= ~SIF_IN_TECH_DATABASE;
			}
		}

		// read all weapons in
		for (idx=0; idx<weapon_count; idx++) {
			in = cfread_ubyte(fp);

			wid = weapon_info_lookup(w_name[idx]);
			if (wid == -1)
				continue;

			if (in) {
				Weapon_info[wid].wi_flags |= WIF_IN_TECH_DATABASE;
			} else if (set_defaults) {
				Weapon_info[wid].wi_flags &= ~WIF_IN_TECH_DATABASE;
			}	
		}
	
		// read all intel entries in
		for (idx=0; idx<intel_count; idx++) {
			in = cfread_ubyte(fp);

			if (idx >= Intel_info_size)
				continue;

			if (in) {
				Intel_info[idx].flags |= IIF_IN_TECH_DATABASE;
			} else if (set_defaults) {
				Intel_info[idx].flags &= ~IIF_IN_TECH_DATABASE;
			}
		}

		// end techroom data ---------------------------------------------------

		// begin player loadout ------------------------------------------------
		char trash[MAX_FILENAME_LEN+DATE_TIME_LENGTH];
		int pool_count;
		wss_unit *slot;

		if (set_defaults) {
			cfread_string_len(Player_loadout.filename, MAX_FILENAME_LEN, fp);
			cfread_string_len(Player_loadout.last_modified, DATE_TIME_LENGTH, fp);
		} else {
			cfread_string_len(trash, MAX_FILENAME_LEN, fp);
			cfread_string_len(trash + MAX_FILENAME_LEN, DATE_TIME_LENGTH, fp);
		}

		// read in ship pool
		for ( i = 0; i < ship_count; i++ ) {
			pool_count = cfread_int(fp);
			// don't set this again and again (for WMC)
			if (set_defaults) {
				sid = ship_info_lookup(s_name[i]);

				if (sid < 0)
					continue;

				Player_loadout.ship_pool[sid] = pool_count;
			}
		}

		// read in weapons pool
		for ( i = 0; i < weapon_count; i++ ) {
			pool_count = cfread_int(fp);
			// don't set this again and again (for WMC)
			if (set_defaults) {
				wid = weapon_info_lookup(w_name[i]);

				if (wid < 0)
					continue;

				Player_loadout.weapon_pool[wid] = pool_count;
			}
		}

		int wep_tmp = -1;
		int shp_tmp = -1;

		// read in loadout info
		for ( i = 0; i < MAX_WSS_SLOTS; i++ ) {
			slot = &Player_loadout.unit_data[i];
			shp_tmp = cfread_int(fp);
			// don't set this again and again (for WMC)
			if (set_defaults) {
				slot->ship_class = ship_info_lookup(s_name[shp_tmp]); // -1 should be ok here
			}

			for ( j = 0; j < MAX_SHIP_WEAPONS; j++ ) {
				wep_tmp = cfread_int(fp);
				// don't set this again and again (for WMC)
				if (set_defaults) {
					slot->wep_count[j] = cfread_int(fp);
					slot->wep[j] = weapon_info_lookup(w_name[wep_tmp]); // -1 should be ok here
				} else {
					// we need to read some dummy info for the wep_counts
					wep_tmp = cfread_int(fp);
				}
			}
		}
		// end player loadout --------------------------------------------------

		// begin total pilot stats ---------------------------------------------
		// init a blank stats block before loading new
		init_scoring_element( &pl->stats );

		// TODO: need to work out the best way to not overwrite current values with this
		//       stuff but still handle garbage collection, future stats changes properly
		//       (don't forget to make NUM_MEDALS non-breaking on next update either!!)
		pl->stats.score = cfread_int(fp);
		pl->stats.rank = cfread_int(fp);
		pl->stats.assists = cfread_int(fp);

		for (i=0; i < Num_medals; i++) {
			pl->stats.medals[i] = cfread_int(fp);
		}

		int k_total = cfread_int(fp);

		for (i=0; i<k_total; i++) {
			k_count = cfread_ushort(fp);

			sid = ship_info_lookup(s_name[i]);
			if (sid == -1)
				continue;

			pl->stats.kills[sid] = k_count;
		}

		pl->stats.kill_count = cfread_int(fp);
		pl->stats.kill_count_ok = cfread_int(fp);
	
		pl->stats.p_shots_fired = cfread_uint(fp);
		pl->stats.s_shots_fired = cfread_uint(fp);
		pl->stats.p_shots_hit = cfread_uint(fp);
		pl->stats.s_shots_hit = cfread_uint(fp);
	
		pl->stats.p_bonehead_hits = cfread_uint(fp);
		pl->stats.s_bonehead_hits = cfread_uint(fp);
		pl->stats.bonehead_kills = cfread_uint(fp);
		// end total pilot stats -----------------------------------------------

		// cutscenes
		if (set_defaults) {
			Cutscenes_viewable = cfread_int(fp);
		}
	}
	// done with total stuff ---------------------------------------------------

	// do we need to update to new format?
	if ( version < CAMPAIGN_FILE_VERSION )
		force_update = 1;

	cfclose( fp );

	if (version < 15) {
		// 4/17/98
		// now, try and read in the campaign stats saved information.  This code was added for the 1.03 patch
		// since the stats data was never written out to disk.  We try and open the file, and if we cannot find
		// it, then simply return
		sprintf( filename, NOX("%s.%s.css"), pl->callsign, base );

		fp = cfopen(filename, "rb", CFILE_NORMAL, CF_TYPE_SINGLE_PLAYERS );
		if ( !fp )
			goto Done;

		id = cfread_int( fp );
		if ( id != (int)CAMPAIGN_STATS_FILE_ID ) {
			Warning(LOCATION, "Campaign stats save file has invalid signature");
			cfclose( fp );
			goto Done;
		}

		version = cfread_int( fp );
		if ( version < CAMPAIGN_STATS_FILE_COMPATIBLE_VERSION ) {
			Warning(LOCATION, "Campaign save file too old -- not compatible.  Deleting file.\nYou can continue from here without trouble\n\n");
			cfclose( fp );
			cf_delete( filename, CF_TYPE_SINGLE_PLAYERS );
			goto Done;
		}

		
		if (version >= 2) {
			// read in number of ships that were written to the file
			n_ships = cfread_int( fp );

			Assert( n_ships <= MAX_SHIP_CLASSES );

			// read in ship names
			for ( i = 0; i < n_ships; i++ ) {
				cfread_string_len( s_name[i], NAME_LENGTH, fp );
			}
		}

		const int ORIG_MAX_SHIP_CLASSES = 130;
		int read_size = 0;

		// figure out the size of data we need to read each pass
		if (version >= 2) {
			// basic size, start with current and subtract what we don't want
			read_size = sizeof(scoring_struct);

			// if MAX_SHIP_CLASSES has changed from what we expect then come up with a modified size:
			// new_max_ship_types - old_max_ship_types * sizeof(int) * 3-times-MAX_SHIP_CLASSES-is-used
			if (MAX_SHIP_CLASSES > ORIG_MAX_SHIP_CLASSES)
				read_size -= (MAX_SHIP_CLASSES - ORIG_MAX_SHIP_CLASSES) * sizeof(int) * 3;
		} else {
			// basic size, start with current and subtract what we don't want
			read_size = sizeof(scoring_struct);
			// we moved to int from ushort for kills[MAX_SHIP_CLASSES], m_kills[MAX_SHIP_CLASSES] & m_okKills[MAX_SHIP_CLASSES]
			read_size -= (sizeof(int) - sizeof(ushort)) * 3 * ORIG_MAX_SHIP_CLASSES;
			// we moved to int from ushort for m_dogfight_kills[MAX_PLAYERS]
			read_size -= (sizeof(int) - sizeof(ushort)) * MAX_PLAYERS;

			// if MAX_SHIP_CLASSES has changed from what we expect then come up with a modified size:
			// new_max_ship_types - old_max_ship_types * sizeof(ushort) * 3-times-MAX_SHIP_CLASSES-is-used
			// we moved from ushort to int for these 3 so take that into account
			if (MAX_SHIP_CLASSES > ORIG_MAX_SHIP_CLASSES)
				read_size -= (MAX_SHIP_CLASSES - ORIG_MAX_SHIP_CLASSES) * (sizeof(int) - sizeof(ushort)) * 3;
		}

		Assert( read_size );

		num_stats_blocks = cfread_int( fp );
		for (i = 0; i < num_stats_blocks; i++ ) {
			num = cfread_int( fp );
			cfread( &Campaign.missions[num].stats, read_size, 1, fp );

			if (version >= 2) {
				// copy current values to temp and swap if needed
				for ( j = 0; j < MAX_SHIP_CLASSES; j++ )
					kill_count[j] = INTEL_INT(Campaign.missions[num].stats.kills[j]);

				// translate to new tables
				for ( j = 0; j < MAX_SHIP_CLASSES; j++ ) {
					idx = ship_info_lookup(s_name[j]);
					Campaign.missions[num].stats.kills[idx] = kill_count[j];
				}
			}

			// swap values
			Campaign.missions[num].stats.score = INTEL_INT(Campaign.missions[num].stats.score);
			Campaign.missions[num].stats.rank = INTEL_INT(Campaign.missions[num].stats.rank);
			Campaign.missions[num].stats.assists = INTEL_INT(Campaign.missions[num].stats.assists);
			Campaign.missions[num].stats.kill_count = INTEL_INT(Campaign.missions[num].stats.kill_count);
			Campaign.missions[num].stats.kill_count_ok = INTEL_INT(Campaign.missions[num].stats.kill_count_ok);
			Campaign.missions[num].stats.p_shots_fired = INTEL_INT(Campaign.missions[num].stats.p_shots_fired);
			Campaign.missions[num].stats.s_shots_fired = INTEL_INT(Campaign.missions[num].stats.s_shots_fired);
			Campaign.missions[num].stats.p_shots_hit = INTEL_INT(Campaign.missions[num].stats.p_shots_hit);
			Campaign.missions[num].stats.s_shots_hit = INTEL_INT(Campaign.missions[num].stats.s_shots_hit);
			Campaign.missions[num].stats.p_bonehead_hits = INTEL_INT(Campaign.missions[num].stats.p_bonehead_hits);
			Campaign.missions[num].stats.s_bonehead_hits = INTEL_INT(Campaign.missions[num].stats.s_bonehead_hits);
			Campaign.missions[num].stats.bonehead_kills = INTEL_INT(Campaign.missions[num].stats.bonehead_kills);

			for (j=0; j < MAX_MEDALS; j++) {
				Campaign.missions[num].stats.medals[j] = INTEL_INT(Campaign.missions[num].stats.medals[j]);
			}
		}

		// do we need to update to new format?
		if ( version < CAMPAIGN_STATS_FILE_VERSION )
			force_update = 1;

		cfclose(fp);
	} // version < 15

Done:
	// -- taylor --
	// force a save to update a campaign file to newest version since this
	// normally isn't done until a campaign mission is successfully completed
	if ( force_update ) {
		mission_campaign_savefile_save();
		// now delete old files
		mission_campaign_savefile_delete_old(cfilename);
	}

	return 1;
}


// the following code only ever called by CSFE!!!!
void campaign_savefile_load(char *fname, char *pname)
{
	if (Campaign.type==CAMPAIGN_TYPE_SINGLE) {
		Game_mode &= ~GM_MULTIPLAYER;
		Game_mode &= GM_NORMAL;
	}
	else
		Game_mode |= GM_MULTIPLAYER;
	strcpy(Player->callsign, pname);
	mission_campaign_savefile_load(fname);
}

// mission_campaign_next_mission sets up the internal veriables of the campaign
// structure so the player can play the next mission.  If there are no more missions
// available in the campaign, this function returns -1, else 0 if the mission was
// set successfully
int mission_campaign_next_mission()
{
	if ( (Campaign.next_mission == -1) || (strlen(Campaign.name) == 0)) // will be set to -1 when there is no next mission
		return -1;

	if(Campaign.num_missions < 1)
		return -2;

	Campaign.current_mission = Campaign.next_mission;	
	strncpy( Game_current_mission_filename, Campaign.missions[Campaign.current_mission].name, MAX_FILENAME_LEN );

	// check for end of loop.
	if (Campaign.current_mission == Campaign.loop_reentry) {
		Campaign.loop_enabled = 0;
	}

	// reset the number of persistent ships and weapons for the next campaign mission
	Num_granted_ships = 0;
	Num_granted_weapons = 0;
	return 0;
}

// mission_campaign_previous_mission() gets called to go to the previous mission in
// the campaign.  Used only for Red Alert missions
int mission_campaign_previous_mission()
{
	if ( !(Game_mode & GM_CAMPAIGN_MODE) )
		return 0;

	if ( Campaign.prev_mission == -1 )
		return 0;

	Campaign.current_mission = Campaign.prev_mission;
	Campaign.next_mission = Campaign.current_mission;
	Campaign.num_missions_completed--;
	Campaign.missions[Campaign.next_mission].completed = 0;
	mission_campaign_savefile_save();

	// reset the player stats to be the stats from this level
	memcpy( &Player->stats, &Campaign.missions[Campaign.current_mission].stats, sizeof(Player->stats) );

	strncpy( Game_current_mission_filename, Campaign.missions[Campaign.current_mission].name, MAX_FILENAME_LEN );
	Num_granted_ships = 0;
	Num_granted_weapons = 0;

	return 1;
}

/*
// determine what the next mission is after the current one.  Because this evaluates an sexp,
// and that could check just about anything, the results are only going to be valid in
// certain places.
// DA 12/09/98 -- To allow for mission loops, need to maintain call with store stats
int mission_campaign_eval_next_mission( int store_stats )
{
	char *name;
	int cur, i, j;
	cmission *mission;

	Campaign.next_mission = -1;
	cur = Campaign.current_mission;
	name = Campaign.missions[cur].name;

	mission = &Campaign.missions[cur];

	// first we must save the status of the current missions goals in the campaign mission structure.
	// After that, we can determine which mission is tagged as the next mission.  Finally, we
	// can save the campaign save file
	// we might have goal and event status if the player replayed a mission
	if ( mission->num_goals > 0 ) {
		vm_free( mission->goals );
	}

	mission->num_goals = Num_goals;
	if ( mission->num_goals > 0 ) {
		mission->goals = (mgoal *)vm_malloc( sizeof(mgoal) * Num_goals );
		Assert( mission->goals != NULL );
	}

	// copy the needed info from the Mission_goal struct to our internal structure
	for (i = 0; i < Num_goals; i++ ) {
		if ( strlen(Mission_goals[i].name) == 0 ) {
			char name[NAME_LENGTH];

			sprintf(name, NOX("Goal #%d"), i);
			//Warning(LOCATION, "Mission goal in mission %s must have a +Name field! using %s for campaign save file\n", mission->name, name);
			strcpy( mission->goals[i].name, name);
		} else
			strcpy( mission->goals[i].name, Mission_goals[i].name );
		Assert ( Mission_goals[i].satisfied != GOAL_INCOMPLETE );		// should be true or false at this point!!!
		mission->goals[i].status = (char)Mission_goals[i].satisfied;
	}

	// do the same thing for events as we did for goals
	// we might have goal and event status if the player replayed a mission
	if ( mission->num_events > 0 ) {
		vm_free( mission->events );
	}

	mission->num_events = Num_mission_events;
	if ( mission->num_events > 0 ) {
		mission->events = (mevent *)vm_malloc( sizeof(mevent) * Num_mission_events );
		Assert( mission->events != NULL );
	}

	// copy the needed info from the Mission_goal struct to our internal structure
	for (i = 0; i < Num_mission_events; i++ ) {
		if ( strlen(Mission_events[i].name) == 0 ) {
			char name[NAME_LENGTH];

			sprintf(name, NOX("Event #%d"), i);
			nprintf(("Warning", "Mission goal in mission %s must have a +Name field! using %s for campaign save file\n", mission->name, name));
			strcpy( mission->events[i].name, name);
		} else
			strcpy( mission->events[i].name, Mission_events[i].name );

		// getting status for the events is a little different.  If the formula value for the event entry
		// is -1, then we know the value of the result field will never change.  If the formula is
		// not -1 (i.e. still being evaluated at mission end time), we will write "incomplete" for the
		// event evaluation
		if ( Mission_events[i].formula == -1 ) {
			if ( Mission_events[i].result )
				mission->events[i].status = EVENT_SATISFIED;
			else
				mission->events[i].status = EVENT_FAILED;
		} else
			Int3();
	}

	// Goober5000 - handle campaign-persistent variables -------------------------------------
	if (mission->num_saved_variables > 0) {
		vm_free( mission->saved_variables );
	}

	mission->num_saved_variables = sexp_campaign_persistent_variable_count();
	if ( mission->num_saved_variables > 0) {
		mission->saved_variables = (sexp_variable *)vm_malloc( sizeof(sexp_variable) * mission->num_saved_variables);
		Assert( mission->saved_variables != NULL );
	}

	// copy the needed variable info
	j=0;
	for (i = 0; i < sexp_variable_count(); i++) {
		if (Sexp_variables[i].type & SEXP_VARIABLE_CAMPAIGN_PERSISTENT)
		{
			mission->saved_variables[j].type = Sexp_variables[i].type;
			strcpy(mission->saved_variables[j].text, Sexp_variables[i].text);
			strcpy(mission->saved_variables[j].variable_name, Sexp_variables[i].variable_name);
			j++;
		}
	}
	// --------------------------------------------------------------------------

	// maybe store the alltime stats which would be current at the end of this mission
	if ( store_stats ) {
		memcpy( &mission->stats, &Player->stats, sizeof(Player->stats) );
		scoring_backout_accept( &mission->stats );
	}

	if ( store_stats ) {	// second (last) time through, so use choose loop_mission if chosen
		if ( Campaign.loop_enabled ) {
			Campaign.next_mission = Campaign.loop_mission;
		} else {
			// evaluate next mission (straight path)
			if (Campaign.missions[cur].formula != -1) {
				flush_sexp_tree(Campaign.missions[cur].formula);  // force formula to be re-evaluated
				eval_sexp(Campaign.missions[cur].formula);  // this should reset Campaign.next_mission to proper value
			}
		}
	} else {

		// evaluate next mission (straight path)
		if (Campaign.missions[cur].formula != -1) {
			flush_sexp_tree(Campaign.missions[cur].formula);  // force formula to be re-evaluated
			eval_sexp(Campaign.missions[cur].formula);  // this should reset Campaign.next_mission to proper value
		}

		// evaluate mission loop mission (if any) so it can be used if chosen
		if ( Campaign.missions[cur].has_mission_loop ) {
			int copy_next_mission = Campaign.next_mission;
			// Set temporarily to -1 so we know if loop formula fails to assign
			Campaign.next_mission = -1;  // Cannot exit campaign from loop
			if (Campaign.missions[cur].mission_loop_formula != -1) {
				flush_sexp_tree(Campaign.missions[cur].mission_loop_formula);  // force formula to be re-evaluated
				eval_sexp(Campaign.missions[cur].mission_loop_formula);  // this should reset Campaign.next_mission to proper value
			}

			Campaign.loop_mission = Campaign.next_mission;
			Campaign.next_mission = copy_next_mission;
		}
	}

	if (Campaign.next_mission == -1)
		nprintf(("allender", "No next mission to proceed to.\n"));
	else
		nprintf(("allender", "Next mission is number %d [%s]\n", Campaign.next_mission, Campaign.missions[Campaign.next_mission].name));

	return Campaign.next_mission;
} */

// Evaluate next campaign mission - set as Campaign.next_mission.  Also set Campaign.loop_mission
void mission_campaign_eval_next_mission()
{
	Campaign.next_mission = -1;
	int cur = Campaign.current_mission;

	// evaluate next mission (straight path)
	if (Campaign.missions[cur].formula != -1) {
		flush_sexp_tree(Campaign.missions[cur].formula);  // force formula to be re-evaluated
		eval_sexp(Campaign.missions[cur].formula);  // this should reset Campaign.next_mission to proper value
	}

	// evaluate mission loop mission (if any) so it can be used if chosen
	if ( Campaign.missions[cur].has_mission_loop ) {
		int copy_next_mission = Campaign.next_mission;
		// Set temporarily to -1 so we know if loop formula fails to assign
		Campaign.next_mission = -1;
		if (Campaign.missions[cur].mission_loop_formula != -1) {
			flush_sexp_tree(Campaign.missions[cur].mission_loop_formula);  // force formula to be re-evaluated
			eval_sexp(Campaign.missions[cur].mission_loop_formula);  // this should reset Campaign.next_mission to proper value
		}

		Campaign.loop_mission = Campaign.next_mission;
		Campaign.next_mission = copy_next_mission;
	}

	if (Campaign.next_mission == -1) {
		nprintf(("allender", "No next mission to proceed to.\n"));
	} else {
		nprintf(("allender", "Next mission is number %d [%s]\n", Campaign.next_mission, Campaign.missions[Campaign.next_mission].name));
	}

}

// Store mission's goals and events in Campaign struct
// Goober5000 - added variables
void mission_campaign_store_goals_and_events_and_variables()
{
	char *name;
	int cur, i, j;
	cmission *mission;

	cur = Campaign.current_mission;
	name = Campaign.missions[cur].name;

	mission = &Campaign.missions[cur];

	// first we must save the status of the current missions goals in the campaign mission structure.
	// After that, we can determine which mission is tagged as the next mission.  Finally, we
	// can save the campaign save file
	// we might have goal and event status if the player replayed a mission
	if ( mission->goals != NULL ) {
		vm_free( mission->goals );
		mission->goals = NULL;
	}

	mission->num_goals = Num_goals;
	if ( mission->num_goals > 0 ) {
		mission->goals = (mgoal *)vm_malloc( sizeof(mgoal) * Num_goals );
		Assert( mission->goals != NULL );
	}

	// copy the needed info from the Mission_goal struct to our internal structure
	for (i = 0; i < Num_goals; i++ ) {
		if ( strlen(Mission_goals[i].name) == 0 ) {
			char goal_name[NAME_LENGTH];

			sprintf(goal_name, NOX("Goal #%d"), i);
			//Warning(LOCATION, "Mission goal in mission %s must have a +Name field! using %s for campaign save file\n", mission->name, name);
			strcpy( mission->goals[i].name, goal_name);
		} else
			strcpy( mission->goals[i].name, Mission_goals[i].name );
		Assert ( Mission_goals[i].satisfied != GOAL_INCOMPLETE );		// should be true or false at this point!!!
		mission->goals[i].status = (char)Mission_goals[i].satisfied;
	}

	// do the same thing for events as we did for goals
	// we might have goal and event status if the player replayed a mission
	if ( mission->events != NULL ) {
		vm_free( mission->events );
		mission->events = NULL;
	}

	mission->num_events = Num_mission_events;
	if ( mission->num_events > 0 ) {
		mission->events = (mevent *)vm_malloc( sizeof(mevent) * Num_mission_events );
		Assert( mission->events != NULL );
	}

	// copy the needed info from the Mission_goal struct to our internal structure
	for (i = 0; i < Num_mission_events; i++ ) {
 		if ( strlen(Mission_events[i].name) == 0 ) {
			char event_name[NAME_LENGTH];

			sprintf(event_name, NOX("Event #%d"), i);
			nprintf(("Warning", "Mission goal in mission %s must have a +Name field! using %s for campaign save file\n", mission->name, name));
			strcpy( mission->events[i].name, event_name);
		} else
			strcpy( mission->events[i].name, Mission_events[i].name );

		// getting status for the events is a little different.  If the formula value for the event entry
		// is -1, then we know the value of the result field will never change.  If the formula is
		// not -1 (i.e. still being evaluated at mission end time), we will write "incomplete" for the
		// event evaluation
		if ( Mission_events[i].formula == -1 ) {
			if ( Mission_events[i].result )
				mission->events[i].status = EVENT_SATISFIED;
			else
				mission->events[i].status = EVENT_FAILED;
		} else
			Int3();
	}

	// Goober5000 - handle campaign-persistent variables -------------------------------------
	if (mission->saved_variables != NULL) {
		vm_free( mission->saved_variables );
		mission->saved_variables = NULL;
	}

	mission->num_saved_variables = sexp_campaign_persistent_variable_count();
	if ( mission->num_saved_variables > 0) {
		mission->saved_variables = (sexp_variable *)vm_malloc( sizeof(sexp_variable) * mission->num_saved_variables);
		Assert( mission->saved_variables != NULL );
	}

	// copy the needed variable info
	j=0;
	for (i = 0; i < sexp_variable_count(); i++) {
		if (Sexp_variables[i].type & SEXP_VARIABLE_CAMPAIGN_PERSISTENT)
		{
			mission->saved_variables[j].type = Sexp_variables[i].type;
			strcpy(mission->saved_variables[j].text, Sexp_variables[i].text);
			strcpy(mission->saved_variables[j].variable_name, Sexp_variables[i].variable_name);
			j++;
		}
	}
	// --------------------------------------------------------------------------
}

// this function is called when the player's mission is over.  It updates the internal store of goals
// and their status then saves the state of the campaign in the campaign file.  This gets called
// after player accepts mission results in debriefing.
void mission_campaign_mission_over(bool do_next_mission)
{
	int mission_num, i;
	cmission *mission;

	// I don't think that we should have a record for these -- maybe we might??????  If we do,
	// then we should free them
	if ( !(Game_mode & GM_CAMPAIGN_MODE) ){
		return;
	}

	mission_num = Campaign.current_mission;
	Assert( mission_num != -1 );
	mission = &Campaign.missions[mission_num];

	// determine if any ships/weapons were granted this mission
	for ( i=0; i<Num_granted_ships; i++ ){
		Campaign.ships_allowed[Granted_ships[i]] = 1;
	}

	for ( i=0; i<Num_granted_weapons; i++ ){
		Campaign.weapons_allowed[Granted_weapons[i]] = 1;	
	}

	// Goober5000 - player-persistent variables are handled when the mission is
	// over, not necessarily when the mission is accepted

	// DKA 12/11/98 - Unneeded already evaluated and stored
	// determine what new mission we are moving to.
	//	mission_campaign_eval_next_mission(1);

	// update campaign.mission stats (used to allow backout inRedAlert)
	// .. but we don't do this if we are inside of the prev/current loop hack
	if ( Campaign.prev_mission != Campaign.current_mission ) {
		memcpy( &mission->stats, &Player->stats, sizeof(Player->stats) );
		if(!(Game_mode & GM_MULTIPLAYER)){
			scoring_backout_accept( &mission->stats );
		}
	}

	// if we are moving to a new mission, then change our data.  If we are staying on the same mission,
	// then we don't want to do anything.  Remove information about goals/events
	if ( Campaign.next_mission != mission_num ) {
		Campaign.prev_mission = mission_num;
		Campaign.current_mission = -1;

		// very minor support for non-linear campaigns - taylor
		if (Campaign.missions[mission_num].completed != 1) {
			Campaign.num_missions_completed++;
			Campaign.missions[mission_num].completed = 1;
		}

		// save the scoring values from the previous mission at the start of this mission -- for red alert

		// save the state of the campaign in the campaign save file and move to the end_game state
		if (Campaign.type == CAMPAIGN_TYPE_SINGLE) {
			mission_campaign_savefile_save();
		}

	} else {
		// free up the goals and events which were just malloced.  It's kind of like erasing any fact
		// that the player played this mission in the campaign.
		if (mission->goals != NULL) {
			vm_free( mission->goals );
			mission->goals = NULL;
		}
		mission->num_goals = 0;

		if (mission->events != NULL) {
			vm_free( mission->events );
			mission->events = NULL;
		}
		mission->num_events = 0;

		// Goober5000
		if (mission->saved_variables != NULL) {
			vm_free( mission->saved_variables );
			mission->saved_variables = NULL;
		}
		mission->num_saved_variables = 0;

		Sexp_nodes[mission->formula].value = SEXP_UNKNOWN;
	}

	// new main hall behavior - Goober5000
	Assert(Player);
	Player->main_hall = Campaign.missions[Campaign.next_mission].main_hall;

	if (do_next_mission)
		mission_campaign_next_mission();			// sets up whatever needs to be set to actually play next mission
}

// called when the game closes -- to get rid of memory errors for Bounds checker
void mission_campaign_close()
{
	int i;

	if (Campaign.desc != NULL) {
		vm_free(Campaign.desc);
		Campaign.desc = NULL;
	}

	// be sure to remove all old malloced strings of Mission_names
	// we must also free any goal stuff that was from a previous campaign
	for ( i=0; i<Campaign.num_missions; i++ ) {
		if ( Campaign.missions[i].name != NULL ) {
			vm_free(Campaign.missions[i].name);
			Campaign.missions[i].name = NULL;
		}

		if (Campaign.missions[i].notes != NULL) {
			vm_free(Campaign.missions[i].notes);
			Campaign.missions[i].notes = NULL;
		}

		if ( Campaign.missions[i].goals != NULL ) {
			vm_free ( Campaign.missions[i].goals );
			Campaign.missions[i].goals = NULL;
		}

		if ( Campaign.missions[i].events != NULL ) {
			vm_free ( Campaign.missions[i].events );
			Campaign.missions[i].events = NULL;
		}

		// Goober5000
		if ( Campaign.missions[i].saved_variables != NULL ) {
			vm_free ( Campaign.missions[i].saved_variables );
			Campaign.missions[i].saved_variables = NULL;
		}

		// the next three are strdup'd return values from parselo.cpp - taylor
		if (Campaign.missions[i].mission_loop_desc != NULL) {
			vm_free(Campaign.missions[i].mission_loop_desc);
			Campaign.missions[i].mission_loop_desc = NULL;
		}

		if (Campaign.missions[i].mission_loop_brief_anim != NULL) {
			vm_free(Campaign.missions[i].mission_loop_brief_anim);
			Campaign.missions[i].mission_loop_brief_anim = NULL;
		}

		if (Campaign.missions[i].mission_loop_brief_sound != NULL) {
			vm_free(Campaign.missions[i].mission_loop_brief_sound);
			Campaign.missions[i].mission_loop_brief_sound = NULL;
 		}

		if ( !Fred_running ){
			sexp_unmark_persistent(Campaign.missions[i].formula);		// free any sexpression nodes used by campaign.
		}

		Campaign.missions[i].num_goals = 0;
		Campaign.missions[i].num_events = 0;
		Campaign.missions[i].num_saved_variables = 0;	// Goober5000
	}

	Campaign.num_missions = 0;
}

// extract the mission filenames for a campaign.  
//
// filename	=>	name of campaign file
//	dest		=> storage for the mission filename, must be already allocated
// num		=> output parameter for the number of mission filenames in the campaign
//
// note that dest should allocate at least dest[MAX_CAMPAIGN_MISSIONS][NAME_LENGTH]
int mission_campaign_get_filenames(char *filename, char dest[][NAME_LENGTH], int *num)
{
	int	rval;

	// read the mission file and get the list of mission filenames
	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("MISSIONCAMPAIGN: Unable to parse '%s'!  Error code = %i.\n", filename, rval));
		return rval;

	} else {
		Assert( strlen(filename) < MAX_FILENAME_LEN );  // make sure no overflow
		read_file_text(filename);
		reset_parse();

		required_string("$Name:");
		advance_to_eoln(NULL);

		required_string( "$Type:" );
		advance_to_eoln(NULL);

		// parse the mission file and actually read in the mission stuff
		*num = 0;
		while ( skip_to_string("$Mission:") == 1 ) {
			stuff_string(dest[*num], F_NAME, NAME_LENGTH);
			(*num)++;
		}
	}

	return 0;
}

// function to read the goals and events from a mission in a campaign file and store that information
// in the campaign structure for use in the campaign editor, error checking, etc
void read_mission_goal_list(int num)
{
	char *filename, notes[NOTES_LENGTH], goals[MAX_GOALS][NAME_LENGTH];
	char events[MAX_MISSION_EVENTS][NAME_LENGTH];
	int i, z, rval, event_count, count = 0;

	filename = Campaign.missions[num].name;

	// open localization
	lcl_ext_open();	
	
	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("MISSIONCAMPAIGN: Unable to parse '%s'!  Error code = %i.\n", filename, rval));
		lcl_ext_close();
		return;
	}

	read_file_text(filename);
	reset_parse();
	init_sexp();

	// first, read the mission notes for this mission.  Used in campaign editor
	if (skip_to_string("#Mission Info")) {
		if (skip_to_string("$Notes:")) {
			stuff_string(notes, F_NOTES, NOTES_LENGTH);
			if (Campaign.missions[num].notes){
				vm_free(Campaign.missions[num].notes);
			}

			Campaign.missions[num].notes = (char *) vm_malloc(strlen(notes) + 1);
			strcpy(Campaign.missions[num].notes, notes);
		}
	}

	event_count = 0;
	// skip to events section in the mission file.  Events come before goals, so we process them first
	if ( skip_to_string("#Events") ) {
		while (1) {
			if (skip_to_string("$Formula:", "#Goals") != 1){
				break;
			}

			z = skip_to_string("+Name:", "$Formula:");
			if (!z){
				break;
			}

			if (z == 1){
				stuff_string(events[event_count], F_NAME, NAME_LENGTH);
			} else {
				sprintf(events[event_count], NOX("Event #%d"), event_count + 1);
			}

			event_count++;
			Assert(event_count < MAX_MISSION_EVENTS);
		}
	}

	count = 0;
	if (skip_to_string("#Goals")) {
		while (1) {
			if (skip_to_string("$Type:", "#End") != 1){
				break;
			}

			z = skip_to_string("+Name:", "$Type:");
			if (!z){
				break;
			}

			if (z == 1){
				stuff_string(goals[count], F_NAME, NAME_LENGTH);
			} else {
				sprintf(goals[count], NOX("Goal #%d"), count + 1);
			}

			count++;
			Assert(count < MAX_GOALS);
		}
	}

	Campaign.missions[num].num_goals = count;
	if (count) {
		Campaign.missions[num].goals = (mgoal *) vm_malloc(count * sizeof(mgoal));
		Assert(Campaign.missions[num].goals);  // make sure we got the memory
		memset(Campaign.missions[num].goals, 0, count * sizeof(mgoal));

		for (i=0; i<count; i++){
			strcpy(Campaign.missions[num].goals[i].name, goals[i]);
		}
	}
		// copy the events
	Campaign.missions[num].num_events = event_count;
	if (event_count) {
		Campaign.missions[num].events = (mevent *)vm_malloc(event_count * sizeof(mevent));
		Assert ( Campaign.missions[num].events );
		memset(Campaign.missions[num].events, 0, event_count * sizeof(mevent));

		for (i = 0; i < event_count; i++ ){
			strcpy(Campaign.missions[num].events[i].name, events[i]);
		}
	}

	// Goober5000 - variables do not need to be read here

	// close localization
	lcl_ext_close();
}

// function to return index into Campaign's list of missions of the mission with the given
// filename.  This function tried to be a little smart about filename looking for the .fsm
// extension since filenames are stored with the extension in the campaign file.  Returns
// index of mission in campaign structure.  -1 if mission name not found.
int mission_campaign_find_mission( char *name )
{
	int i;
	char realname[_MAX_PATH];

	if (name == NULL)
		return -1;

	// look for an extension on the file.  If no extension, add default ".fsm" onto the
	// end of the filename
	strcpy(realname, name );
	if ( strchr(name, '.') == NULL ){
		sprintf(realname, NOX("%s%s"), name, FS_MISSION_FILE_EXT );
	}

	for (i = 0; i < Campaign.num_missions; i++ ) {
		if ( !stricmp(realname, Campaign.missions[i].name) ){
			return i;
		}
	}

	return -1;
}

void mission_campaign_maybe_play_movie(int type)
{
	int mission;
	char *filename;

	// only support pre mission movies for now.
	Assert ( type == CAMPAIGN_MOVIE_PRE_MISSION );

	if ( !(Game_mode & GM_CAMPAIGN_MODE) )
		return;

	mission = Campaign.current_mission;
	Assert( mission != -1 );

	// get a possible filename for a movie to play.
	filename = NULL;
	switch( type ) {
	case CAMPAIGN_MOVIE_PRE_MISSION:
		if ( strlen(Campaign.missions[mission].briefing_cutscene) )
			filename = Campaign.missions[mission].briefing_cutscene;
		break;

	default:
		Int3();
		break;
	}

	// no filename, no movie!
	if ( !filename )
		return;

	movie_play( filename );	//Play the movie!
	cutscene_mark_viewable( filename );
}

// return the type of campaign of the passed filename
int mission_campaign_parse_is_multi(char *filename, char *name)
{	
	int i, rval;
	char temp[NAME_LENGTH];
	
	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("MISSIONCAMPAIGN: Unable to parse '%s'!  Error code = %i.\n", filename, rval));
		return -1;
	}

	read_file_text( filename );
	reset_parse();
	
	required_string("$Name:");
	stuff_string( temp, F_NAME, NAME_LENGTH );	
	if ( name )
		strcpy( name, temp );

	required_string( "$Type:" );
	stuff_string( temp, F_NAME, NAME_LENGTH );

	for (i = 0; i < MAX_CAMPAIGN_TYPES; i++ ) {
		if ( !stricmp(temp, campaign_types[i]) ) {
			return i;
		}
	}

	Error(LOCATION, "Unknown campaign type %s", temp );
	return -1;
}

// functions to save persistent information during a mission -- which then might or might not get
// saved out when the mission is over depending on whether player replays mission or commits.
void mission_campaign_save_persistent( int type, int sindex )
{
	// based on the type of information, save it off for possible saving into the campsign
	// savefile when the mission is over
	if ( type == CAMPAIGN_PERSISTENT_SHIP ) {
		Assert( Num_granted_ships < MAX_SHIP_CLASSES );
		Granted_ships[Num_granted_ships] = sindex;
		Num_granted_ships++;
	} else if ( type == CAMPAIGN_PERSISTENT_WEAPON ) {
		Assert( Num_granted_weapons < MAX_WEAPON_TYPES );
		Granted_weapons[Num_granted_weapons] = sindex;
		Num_granted_weapons++;
	} else
		Int3();
}

// returns 0: loaded, !0: error
int mission_load_up_campaign( player *pl )
{
	if ( pl == NULL )
		pl = Player;

	if ( strlen(pl->current_campaign) ) {
		return mission_campaign_load(pl->current_campaign, pl);
	} else {
		int rc = mission_campaign_load(BUILTIN_CAMPAIGN, pl);

		// if the builtin campaign is missing/corrupt then try and fall back on whatever is available
		if (rc) {
			// no descriptions, and no sorting since we want the actual first entry found
			mission_campaign_build_list(false, false);

			if ( (Campaign_file_names[0] != NULL) && strlen(Campaign_file_names[0]) )
				rc = mission_campaign_load(Campaign_file_names[0], pl);

			mission_campaign_free_list();
		}

		return rc;
	}
}

// for end of campaign in the single player game.  Called when the end of campaign state is
// entered, which is triggered when the end-campaign sexpression is hit

void mission_campaign_end_init()
{
	// no need to do any initialization.
}

void mission_campaign_end_do()
{
	// play the movies
	event_music_level_close();
	mission_goal_fail_incomplete();
	scoring_level_close();
	mission_campaign_mission_over();

	// eventually we'll want to play one of two options (good ending or bad ending)
	// did the supernova blow?
	if(Supernova_status == SUPERNOVA_HIT)
	{
		// no soup for you!
		movie_play_two("endpart1.mve", "endprt2b.mve");			// good ending
	} else {
		// no soup for you!
		movie_play_two("endpart1.mve", "endprt2a.mve");			// good ending
	}	

#ifdef FS2_DEMO
	gameseq_post_event( GS_EVENT_END_DEMO );
#else	
	gameseq_post_event( GS_EVENT_MAIN_MENU );
#endif
}

void mission_campaign_end_close()
{
	// nothing to do here.
}


// skip to the next mission in the campaign
// this also posts the state change by default.  pass 0 to override that
void mission_campaign_skip_to_next(int start_game)
{
	// mark all goals/events complete
	// these do not really matter, since is-previous-event-* and is-previous-goal-* sexps check
	// to see if the mission was skipped, and use defaults accordingly.
	mission_goal_mark_objectives_complete();
	mission_goal_mark_events_complete();

	// mark mission as skipped
	Campaign.missions[Campaign.current_mission].flags |= CMISSION_FLAG_SKIPPED;

	// store
	mission_campaign_store_goals_and_events_and_variables();

	// now set the next mission
	mission_campaign_eval_next_mission();

	// clear out relevant player vars
	Player->failures_this_session = 0;
	Player->show_skip_popup = 1;

	if (start_game) {
		// proceed to next mission or main hall
		if ((Campaign.missions[Campaign.current_mission].has_mission_loop) && (Campaign.loop_mission != -1)) {
			// go to loop solicitation
			gameseq_post_event(GS_EVENT_LOOP_BRIEF);
		} else {
			// closes out mission stuff, sets up next one
			mission_campaign_mission_over();

			if ( Campaign.next_mission == -1 ) {
				// go to main hall, tha campaign is over!
				gameseq_post_event(GS_EVENT_MAIN_MENU);
			} else {
				// go to next mission
				gameseq_post_event(GS_EVENT_START_GAME);
			}
		}
	}
}


// breaks your ass out of the loop
// this also posts the state change
void mission_campaign_exit_loop()
{
	// set campaign to loop reentry point
	Campaign.next_mission = Campaign.loop_reentry;
	Campaign.current_mission = -1;
	Campaign.loop_enabled = 0;

	// set things up for next mission
	mission_campaign_next_mission();
	gameseq_post_event(GS_EVENT_START_GAME);
}


// used for jumping to a particular campaign mission
// all pvs missions marked skipped
// this relies on correct mission ordering in the campaign file
void mission_campaign_jump_to_mission(char *name)
{
	int i = 0, mission_num = -1;
	char dest_name[64], *p;

	// load in the campaign junk
	mission_load_up_campaign();

	// tack the .fs2 onto the input name
	strcpy(dest_name, name);
	p = strchr(dest_name, '.');
	if (p != NULL)
		*p = '\0';
	strcat(dest_name, ".fs2");

	// search for our mission
	for (i = 0; i < Campaign.num_missions; i++) {
		if ((Campaign.missions[i].name != NULL) && !stricmp(Campaign.missions[i].name, dest_name)) {
			mission_num = i;
			break;
		} else {
			Campaign.missions[i].flags |= CMISSION_FLAG_SKIPPED;
			Campaign.num_missions_completed = i;
		}
	}

	if (mission_num < 0) {
		// if we got here, no match was found
		// restart the campaign
		mission_campaign_savefile_delete(Campaign.filename);
		mission_campaign_load(Campaign.filename);
	} else {
		for (i = 0; i < MAX_SHIP_CLASSES; i++) {
			Campaign.ships_allowed[i] = 1;
		}
		for (i = 0; i < MAX_WEAPON_TYPES; i++) {
			Campaign.weapons_allowed[i] = 1;
		}

		Campaign.next_mission = mission_num;
		Campaign.prev_mission = mission_num - 1;
		mission_campaign_next_mission();
		Game_mode |= GM_CAMPAIGN_MODE;

		gameseq_post_event(GS_EVENT_START_GAME);
	}
}

// Goober5000
void mission_campaign_save_player_persistent_variables()
{
	int i, j, index;

	// make sure we are actually playing a campaign
	if (!(Game_mode & GM_CAMPAIGN_MODE))
		return;

	// make sure this is a single-player campaign
	if (!(Campaign.type == CAMPAIGN_TYPE_SINGLE))
		return;

	// now save variables
	for (i=0; i<sexp_variable_count(); i++)
	{
		// if we get to save it
		if (Sexp_variables[i].type & SEXP_VARIABLE_PLAYER_PERSISTENT)
		{
			index = -1;

			// search for the previous instance of this variable
			for (j=0; j<Player->num_variables; j++)
			{
				// found
				if (!(stricmp(Sexp_variables[i].variable_name, Player->player_variables[j].variable_name)))
				{
					index = j;
					break;
				}
			}

			// if not found, allocate new variable
			if (index < 0)
			{
				// check first for overflow
				if (Player->num_variables == MAX_SEXP_VARIABLES)
				{
					Error(LOCATION, "Too many player-persistent variables (last valid variable: %s, max number of variables: %d).  The remaining ones will not be saved.  Press OK to continue.", Player->player_variables[Player->num_variables-1].variable_name, Player->num_variables);
					return;
				}

				// set index and increment
				index = Player->num_variables;
				Player->num_variables++;
			}

			// finally, save this variable
			Player->player_variables[index].type = Sexp_variables[i].type;
			strcpy(Player->player_variables[index].text, Sexp_variables[i].text);
			strcpy(Player->player_variables[index].variable_name, Sexp_variables[i].variable_name);
		}
	}
}
