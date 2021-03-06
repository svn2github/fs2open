/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Stats/Medals.cpp $
 * $Revision: 2.16.2.2 $
 * $Date: 2007-09-02 02:07:47 $
 * $Author: Goober5000 $
 * 
 * $Log: not supported by cvs2svn $
 * Revision 2.16.2.1  2006/09/11 01:17:07  taylor
 * fixes for stuff_string() bounds checking
 *
 * Revision 2.16  2005/09/14 20:03:40  taylor
 * fix ace badges not getting displayed in debriefing
 *
 * Revision 2.15  2005/07/24 00:32:45  wmcoolmon
 * Synced 3D shockwaves' glowmaps with the model, tossed in some medals.tbl
 * support for the demo/FS1
 *
 * Revision 2.14  2005/07/02 19:45:02  taylor
 * ton of non-standard resolution fixes
 *
 * Revision 2.13  2005/05/18 14:03:27  taylor
 * a medals crash fix that isn't influenced by the evil Java empire (Jens Granseuer)
 *
 * Revision 2.12  2005/05/12 17:49:17  taylor
 * use vm_malloc(), vm_free(), vm_realloc(), vm_strdup() rather than system named macros
 *   fixes various problems and is past time to make the switch
 *
 * Revision 2.11  2005/05/08 20:20:46  wmcoolmon
 * Dynamically allocated medals
 *
 * Revision 2.10  2005/02/23 04:57:29  taylor
 * even more bm_unload() -> bm_release() changes
 *
 * Revision 2.9  2005/01/31 10:34:40  taylor
 * merge with Linux/OSX tree - p0131
 *
 * Revision 2.8  2004/07/26 20:47:53  Kazan
 * remove MCD complete
 *
 * Revision 2.7  2004/07/12 16:33:07  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 2.6  2004/03/05 09:02:05  Goober5000
 * Uber pass at reducing #includes
 * --Goober5000
 *
 * Revision 2.5  2004/02/20 04:29:56  bobboau
 * pluged memory leaks,
 * 3D HTL lasers (they work perfictly)
 * and posably fixed Turnsky's shinemap bug
 *
 * Revision 2.4  2003/03/18 10:07:06  unknownplayer
 * The big DX/main line merge. This has been uploaded to the main CVS since I can't manage to get it to upload to the DX branch. Apologies to all who may be affected adversely, but I'll work to debug it as fast as I can.
 *
 * Revision 2.3  2003/01/15 21:29:04  anonymous
 * fixed the demo compilation. Define FS2_DEMO globally to compile as a demo. Make sure warp.pof is in your data/models directory.
 *
 * Revision 2.2.2.1  2002/09/24 18:56:45  randomtiger
 * DX8 branch commit
 *
 * This is the scub of UP's previous code with the more up to date RT code.
 * For full details check previous dev e-mails
 *
 * Revision 2.2  2002/08/01 01:41:10  penguin
 * The big include file move
 *
 * Revision 2.1  2002/07/07 19:56:00  penguin
 * Back-port to MSVC
 *
 * Revision 2.0  2002/06/03 04:02:29  penguin
 * Warpcore CVS sync
 *
 * Revision 1.2  2002/05/15 15:06:11  mharris
 * TEMP PORT FIX: for non-Win32, ifdef out code to assign multiple
 * levels of a medal to remove _splitpath() reference.
 *
 * Revision 1.1  2002/05/02 18:03:13  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 12    10/29/99 10:40p Jefff
 * hack to make german medal names display without actually changing them
 * 
 * 11    9/02/99 3:41p Jefff
 * changed badge voice handling to be similar to promotion voice handling
 * 
 * 10    8/26/99 8:49p Jefff
 * Updated medals screen and about everything that ever touches medals in
 * one way or another.  Sheesh.
 * 
 * 9     7/16/99 1:50p Dave
 * 8 bit aabitmaps. yay.
 * 
 * 8     3/19/99 9:51a Dave
 * Checkin to repair massive source safe crash. Also added support for
 * pof-style nebulae, and some new weapons code.
 * 
 * 8     3/15/99 10:29a Neilk
 * 
 * 7     1/30/99 5:08p Dave
 * More new hi-res stuff.Support for nice D3D textures.
 * 
 * 6     12/18/98 1:13a Dave
 * Rough 1024x768 support for Direct3D. Proper detection and usage through
 * the launcher.
 * 
 * 5     11/30/98 1:07p Dave
 * 16 bit conversion, first run.
 * 
 * 4     10/23/98 3:51p Dave
 * Full support for tstrings.tbl and foreign languages. All that remains
 * is to make it active in Fred.
 * 
 * 3     10/13/98 9:29a Dave
 * Started neatening up freespace.h. Many variables renamed and
 * reorganized. Added AlphaColors.[h,cpp]
 * 
 * 2     10/07/98 10:54a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:51a Dave
 * 
 * 26    6/13/98 3:18p Hoffoss
 * NOX()ed out a bunch of strings that shouldn't be translated.
 * 
 * 25    6/01/98 11:43a John
 * JAS & MK:  Classified all strings for localization.
 * 
 * 24    5/01/98 12:34p John
 * Added code to force FreeSpace to run in the same dir as exe and made
 * all the parse error messages a little nicer.
 * 
 * 23    4/27/98 3:13p Allender
 * make ctrl-enter work on medals screen
 * 
 * 22    4/23/98 8:27p Allender
 * basic support for cutscene playback.  Into movie code in place.  Tech
 * room can view cutscenes stored in CDROM_dir variable
 * 
 * 21    4/12/98 8:30p Allender
 * minor medal changes to show number of medals if > 1
 * 
 * 20    4/10/98 4:51p Hoffoss
 * Made several changes related to tooltips.
 * 
 * 19    3/18/98 12:03p John
 * Marked all the new strings as externalized or not.
 * 
 * 18    3/11/98 5:02p Dave
 * Put in code to support new medals artwork. Assorted multiplayer bug
 * fixes.
 * 
 * 17    3/07/98 5:44p Dave
 * Finished player info popup. Ironed out a few todo bugs.
 * 
 * 16    3/05/98 5:11p Allender
 * fix up medals screen (again).  Don't do state transition when viewing
 * medals from debriefing screen.
 * 
 * 15    3/05/98 2:32p Hoffoss
 * Renamed Rank_bitmap to Rank_bm so it isn't named the same as the
 * variable in the debrief screen which confuses the debugger.
 * 
 * 14    2/22/98 2:48p John
 * More String Externalization Classification
 * 
 * 13    2/09/98 5:21p Hoffoss
 * Made exit from medals screen to previous screen (instead of assuming
 * it's the barracks) work.
 * 
 * 12    1/27/98 4:23p Allender
 * enhanced internal scoring mechanisms.
 * 
 * 11    11/12/97 4:40p Dave
 * Put in multiplayer campaign support parsing, loading and saving. Made
 * command-line variables better named. Changed some things on the initial
 * pilot select screen.
 * 
 * 10    11/12/97 9:30a Dave
 * Fixed rank insignia tooltip not showing up.
 * 
 * 9     11/06/97 4:39p Allender
 * a ton of medal work.  Removed an uneeded elemen in the scoring
 * structure.  Fix up medals screen to apprioriate display medals (after
 * mask was changed).  Fix Fred to only display medals which may actually
 * be granted.  Added image_filename to player struct for Jason Hoffoss
 * 
 * 8     11/05/97 4:43p Allender
 * reworked medal/rank system to read all data from tables.  Made Fred
 * read medals.tbl.  Changed ai-warp to ai-warp-out which doesn't require
 * waypoint for activation
 *
 * $NoKeywords: $
 */


#include "stats/medals.h"
#include "menuui/snazzyui.h"
#include "gamesequence/gamesequence.h"
#include "playerman/player.h"
#include "palman/palman.h"
#include "ui/ui.h"
#include "io/key.h"
#include "gamesnd/gamesnd.h"
#include "globalincs/alphacolors.h"
#include "localization/localize.h"
#include "parse/parselo.h"

#ifndef NDEBUG
#include "cmdline/cmdline.h"
#endif

int Num_medals = 0;

//#define MAX_MEDAL_TYPES 63 // the # of medals which exist so far

/*
#define CALLSIGN_X 198
#define CALLSIGN_Y 80
#define CALLSIGN_W (439-CALLSIGN_X)
#define CALLSIGN_H (116-CALLSIGN_Y)
*/

// define for the medal information
std::vector<medal_stuff> Medals;
//badge_stuff Badge_info[MAX_BADGES];

// holds indices into Medals array of the badges for # kills
int Badge_index[MAX_BADGES];

// the rank section of the screen
#define RANK_MEDAL_REGION		12			// region number of the rank medal

// coords for indiv medal bitmaps
int Medal_coords[GR_NUM_RESOLUTIONS][MAX_MEDALS][2] = {
	{				// GR_640
		{ 89, 47 },					// eps. peg. lib
		{ 486, 47 },				// imp. order o' vasuda
		{ 129, 130 },				// dist flying cross
		{ 208, 132 },				// soc service
		{ 361, 131 },				// dist intel cross
		{ 439, 130 },				// order of galatea
		{ 64, 234 },				// meritorious unit comm.
		{ 153, 234 },				// medal of valor
		{ 239, 241 },				// gtva leg of honor
		{ 326, 240 },				// allied defense citation
		{ 411, 234 },				// neb campaign victory
		{ 494, 234 },				// ntf campaign victory
		{ 189, 80 },				// rank
		{ 283, 91 },				// wings
		{ 372, 76 },				// bronze kills badge
		{ 403, 76 },				// silver kills badge
#ifndef FS2_DEMO
		{ 435, 76 },				// gold kills badge
		{ 300, 152 },				// SOC unit crest
#endif
	},
	{				// GR_1024
		{ 143, 75 },				// eps. peg. lib
		{ 777, 75 },				// imp. order o' vasuda
		{ 206, 208 },				// dist flying cross
		{ 333, 212 },				// soc service
		{ 578, 210 },				// dist intel cross
		{ 703, 208 },				// order of galatea
		{ 103, 374 },				// meritorious unit comm.
		{ 245, 374 },				// medal of valor
		{ 383, 386 },				// gtva leg of honor
		{ 522, 384 },				// allied defense citation
		{ 658, 374 },				// neb campaign victory
		{ 790, 374 },				// ntf campaign victory
		{ 302, 128 },				// rank
		{ 453, 146 },				// wings
		{ 595, 121 },				// bronze kills badge
		{ 646, 121 },				// silver kills badge
#ifndef FS2_DEMO
		{ 696, 121 },				// gold kills badge
		{ 480, 244 },				// SOC unit crest
#endif
	}
};

// coords for the medal title
static int Medals_label_coords[GR_NUM_RESOLUTIONS][3] = {
	{ 241, 458, 300 },			// GR_640 x, y, w
	{ 386, 734, 480 }				// GR_1024 x, y, w
};

#define MEDALS_NUM_BUTTONS			1
#define MEDALS_EXIT					0	
ui_button_info Medals_buttons[GR_NUM_RESOLUTIONS][MEDALS_NUM_BUTTONS] = {
	{ // GR_640
		ui_button_info("MEB_18",	574,	432,	-1,	-1,	18),
	},
	{ // GR_1024
		ui_button_info("2_MEB_18",	919,	691,	-1,	-1,	18),
	}
};

#define MEDALS_NUM_TEXT				1
UI_XSTR Medals_text[GR_NUM_RESOLUTIONS][MEDALS_NUM_TEXT] = {
	{	// GR_640
		{"Exit",		1466,		587,	416,	UI_XSTR_COLOR_PINK, -1,	&Medals_buttons[GR_640][MEDALS_EXIT].button },
	},
	{	// GR_1024
		{"Exit",		1466,		943,	673,	UI_XSTR_COLOR_PINK, -1,	&Medals_buttons[GR_1024][MEDALS_EXIT].button },
	},
};

static char* Medals_background_filename[GR_NUM_RESOLUTIONS] = {
	"MedalsDisplayEmpty",
	"2_MedalsDisplayEmpty"
};

static char* Medals_mask_filename[GR_NUM_RESOLUTIONS] = {
	"Medals-m",
	"2_Medals-m"
};

static int Medals_callsign_y[GR_NUM_RESOLUTIONS] = {
	54, 89
};

scoring_struct *Player_score=NULL;

int Medals_mode;
player *Medals_player;

// -----------------------------------------------------------------------------
// Main medals screen state
//
#define NUM_MEDAL_REGIONS			MAX_MEDALS + 1				// the extra one is for the rank medal

static bitmap *Medals_mask;
int Medals_mask_w, Medals_mask_h;
//static int Medal_palette;              // Medal palette bitmap
static int Medals_bitmap_mask;         // the mask for the medal case
static int Medals_bitmap;              // the medal case itself
static std::vector<int> Medal_bitmaps;  // bitmaps for the individual medals
static int Rank_bm;							// bitmap for the rank medal

static MENU_REGION Medal_regions[NUM_MEDAL_REGIONS]; // a semi-hack for now because we only have 4 medals, but we also include the close button

static UI_WINDOW Medals_window;

//#define MAX_MEDALS_BUTTONS						1
//#define MEDAL_BUTTON_EXIT						0
//static UI_BUTTON Medal_buttons[MAX_MEDALS_BUTTONS];

/*static char *Medal_button_names[MAX_MEDALS_BUTTONS] = {
//XSTR:OFF
	"MX_17"
//XSTR:ON
};
*/
/*
static int Medal_button_coords[MAX_MEDALS_BUTTONS][2] = {
	{561,411}
};
static int Medal_button_masks[MAX_MEDALS_BUTTONS] = {
	17
};
*/


#define MEDAL_BITMAP_INIT (1<<0)
#define MASK_BITMAP_INIT  (1<<1)
int Init_flags;

void medal_stuff::clone(const medal_stuff &m)
{
	memcpy(name, m.name, NAME_LENGTH);
	memcpy(bitmap, m.bitmap, NAME_LENGTH);
	num_versions = m.num_versions;
	kills_needed = m.kills_needed;
	badge_num = m.badge_num;
	memcpy(voice_base, m.voice_base, MAX_FILENAME_LEN);

	if (m.promotion_text)
		promotion_text = vm_strdup(m.promotion_text);
	else
		promotion_text = NULL;
}

// assignment operator
const medal_stuff &medal_stuff::operator=(const medal_stuff &m)
{
	if (this != &m) {
		if (promotion_text) {
			vm_free(promotion_text);
			promotion_text = NULL;
		}
		clone(m);
	}

	return *this;
}


void parse_medal_tbl()
{
	int rval, i;
	int num_badges = 0;

	// open localization
	lcl_ext_open();

	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("TABLES: Unable to parse '%s'!  Error code = %i.\n", "medals.tbl", rval));
		lcl_ext_close();
		return;
	}

	read_file_text("medals.tbl", CF_TYPE_TABLES);
	reset_parse();

	// parse in all the rank names
	Num_medals = 0;
	required_string("#Medals");
	while ( required_string_either("#End", "$Name:") )
	{
		medal_stuff temp_medal;

		required_string("$Name:");
		stuff_string( temp_medal.name, F_NAME, NAME_LENGTH );

		required_string("$Bitmap:");
		stuff_string( temp_medal.bitmap, F_NAME, MAX_FILENAME_LEN );

		required_string("$Num mods:");
		stuff_int( &temp_medal.num_versions);

		// some medals are based on kill counts.  When string +Num Kills: is present, we know that
		// this medal is a badge and should be treated specially
		if ( optional_string("+Num Kills:") ) {
			char buf[MULTITEXT_LENGTH];

			stuff_int( &temp_medal.kills_needed );

			if(optional_string("$Wavefile 1:"))
				stuff_string(temp_medal.voice_base, F_NAME, MAX_FILENAME_LEN);
			if(optional_string("$Wavefile 2:"))
				stuff_string(temp_medal.voice_base, F_NAME, MAX_FILENAME_LEN);
			//stuff_string(Badge_info[bi].wave2, F_NAME, NULL, MAX_FILENAME_LEN);

			if(optional_string("$Wavefile Base:"))
				stuff_string(temp_medal.voice_base, F_NAME, MAX_FILENAME_LEN);

			//required_string("$Wavefile 2:");
			//stuff_string(Badge_info[bi].wave2, F_NAME, NULL, MAX_FILENAME_LEN);

			required_string("$Promotion Text:");
			stuff_string(buf, F_MULTITEXT, sizeof(buf));
			temp_medal.promotion_text = vm_strdup(buf);

			// since badges (ace ranks) are handled a little differently we need to know
			// which we are in the list of badges.
			temp_medal.badge_num = num_badges++;
		}

		Medals.push_back(temp_medal);
	}

	required_string("#End");

	Num_medals = Medals.size();

	// be sure that the badges kill numbers show up in order
	//WMC - I don't think this is needed anymore due to my changes to post-mission functions
	//but I'm keeping it here to be sure.
	int prev_badge_kills = 0;
	for (i = 0; i < Num_medals; i++ )
	{
		if ( Medals[i].kills_needed < prev_badge_kills && Medals[i].kills_needed != 0)
			Error(LOCATION, "Badges must appear sorted by lowest kill # first in medals.tbl\nFind Allender for most information.");

		if(Medals[i].kills_needed > 0)
			prev_badge_kills = Medals[i].kills_needed;
	}

	// close localization
	lcl_ext_close();
}

//WMC - not needed anymore
//badge_stuff has a deconstructor
/*
void medal_close(){
	for(int i = 0; i<MAX_BADGES; i++)if(Badge_info[i].promotion_text)vm_free(Badge_info[i].promotion_text);
}*/

// replacement for -gimmemedals
DCF(medals, "Grant or revoke medals")
{
	int i;

	if (Dc_command)
	{
		dc_get_arg(ARG_STRING | ARG_INT | ARG_NONE);

		if (Dc_arg_type & ARG_INT)
		{
			int idx = Dc_arg_int;

			if (idx < 0 || idx >= MAX_MEDALS)
			{
				dc_printf("Medal index %d is out of range\n", idx);
				return;
			}

			dc_printf("Granted %s\n", Medals[idx].name);
			Player->stats.medals[idx]++;
		}
		else if (Dc_arg_type & ARG_STRING)
		{
			if (!strcmp(Dc_arg, "all"))
			{
				for (i = 0; i < MAX_MEDALS; i++)
					Player->stats.medals[i]++;

				dc_printf("Granted all medals\n");
			}
			else if (!strcmp(Dc_arg, "clear"))
			{
				for (i = 0; i < MAX_MEDALS; i++)
					Player->stats.medals[i] = 0;

				dc_printf("Cleared all medals\n");
			}
			else if (!strcmp(Dc_arg, "demote"))
			{
				if (Player->stats.rank > 0)
					Player->stats.rank--;

				dc_printf("Demoted to %s\n", Ranks[Player->stats.rank].name);
			}
			else if (!strcmp(Dc_arg, "promote"))
			{
				if (Player->stats.rank < MAX_FREESPACE2_RANK)
					Player->stats.rank++;

				dc_printf("Promoted to %s\n", Ranks[Player->stats.rank].name);
			}
			else
			{
				Dc_help = 1;
			}
		}
		else
		{
			dc_printf("The following medals are available:\n");
			for (i = 0; i < MAX_MEDALS; i++)
				dc_printf("%d: %s\n", i, Medals[i].name);
		}

		Dc_status = 0;
	}

	if (Dc_help)
	{
		dc_printf ("Usage: gimmemedals all | clear | promote | demote | [index]\n");
		dc_printf ("       [index] --  index of medal to grant\n");
		dc_printf ("       with no parameters, displays the available medals\n");
		Dc_status = 0;
	}

	if (Dc_status)
	{
		dc_printf("You have the following medals:\n");

		for (i = 0; i < MAX_MEDALS; i++)
		{
			if (Player->stats.medals[i] > 0)
				dc_printf("%d %s\n", Player->stats.medals[i], Medals[i].name);
		}
		dc_printf("%s\n", Ranks[Player->stats.rank].name);
	}
}


void medal_main_init(player *pl, int mode)
{
	int idx;

	Assert(pl != NULL);
	Medals_player = pl;

   Player_score = &Medals_player->stats;

#ifndef NDEBUG
	if(Cmdline_gimme_all_medals){
		//int idx;
		for(idx=0; idx < MAX_MEDALS; idx++){
			Medals_player->stats.medals[idx] = 1;		
		}
	}
#endif

	Medals_mode = mode;

	snazzy_menu_init();
	Medals_window.create( 0, 0, gr_screen.max_w_unscaled, gr_screen.max_h_unscaled, 0 );	

	// create the interface buttons
	for (idx=0; idx<MEDALS_NUM_BUTTONS; idx++) {
		// create the object
		Medals_buttons[gr_screen.res][idx].button.create(&Medals_window, "", Medals_buttons[gr_screen.res][idx].x, Medals_buttons[gr_screen.res][idx].y, 1, 1, 0, 1);

		// set the sound to play when highlighted
		Medals_buttons[gr_screen.res][idx].button.set_highlight_action(common_play_highlight_sound);

		// set the ani for the button
		Medals_buttons[gr_screen.res][idx].button.set_bmaps(Medals_buttons[gr_screen.res][idx].filename);

		// set the hotspot
		Medals_buttons[gr_screen.res][idx].button.link_hotspot(Medals_buttons[gr_screen.res][idx].hotspot);
	}	

	// add all xstrs
	for (idx=0; idx<MEDALS_NUM_TEXT; idx++) {
		Medals_window.add_XSTR(&Medals_text[gr_screen.res][idx]);
	}


	Init_flags = 0;	

	//init_medal_palette();
	
	Medals_bitmap = bm_load(Medals_background_filename[gr_screen.res]);
	if (Medals_bitmap < 0) {
	   Error(LOCATION, "Error loading medal background bitmap %s", Medals_background_filename[gr_screen.res]);
	} else {
		Init_flags |= MEDAL_BITMAP_INIT;
	}

	Medals_mask_w = -1;
	Medals_mask_h = -1;
      
	Medals_bitmap_mask = bm_load(Medals_mask_filename[gr_screen.res]);
	if(Medals_bitmap_mask < 0){
		Error(LOCATION, "Error loading medal mask file %s", Medals_mask_filename[gr_screen.res]);
	} else {
		Init_flags |= MASK_BITMAP_INIT;
		Medals_mask = bm_lock(Medals_bitmap_mask, 8, BMP_AABITMAP);
		bm_get_info(Medals_bitmap_mask, &Medals_mask_w, &Medals_mask_h);
	}
	init_medal_bitmaps();
	init_snazzy_regions();

	gr_set_color_fast(&Color_normal);

	Medals_window.set_mask_bmap(Medals_mask_filename[gr_screen.res]);
}

// this is just a hack to display translated names without actually changing the names, 
// which would break stuff
void medals_translate_name(char *name, int max_len)
{
	if (!strcmp(name, "Epsilon Pegasi Liberation")) {
		strncpy(name, "Epsilon Pegasi Befreiungsmedaille", max_len);

	} else if (!strcmp(name, "Imperial Order of Vasuda")) {
		strncpy(name, "Imperialer Orden von Vasuda ", max_len);

	} else if (!strcmp(name, "Distinguished Flying Cross")) {
		strncpy(name, "Fliegerkreuz Erster Klasse", max_len);

	} else if (!strcmp(name, "SOC Service Medallion")) {
		strncpy(name, "SEK-Dienstmedaille ", max_len);

	} else if (!strcmp(name, "Intelligence Cross")) {
		strncpy(name, "Geheimdienstkreuz am Bande", max_len);

	} else if (!strcmp(name, "Order of Galatea")) {
		strncpy(name, "Orden von Galatea ", max_len);

	} else if (!strcmp(name, "Meritorious Unit Commendation")) {
		strncpy(name, "Ehrenspange der Allianz", max_len);

	} else if (!strcmp(name, "Medal of Valor")) {
		strncpy(name, "Tapferkeitsmedaille ", max_len);

	} else if (!strcmp(name, "GTVA Legion of Honor")) {
		strncpy(name, "Orden der GTVA-Ehrenlegion", max_len);

	} else if (!strcmp(name, "Allied Defense Citation")) {
		strncpy(name, "Alliierte Abwehrspange ", max_len);

	} else if (!strcmp(name, "Nebula Campaign Victory Star")) {
		strncpy(name, "Nebel-Siegesstern", max_len);

	} else if (!strcmp(name, "NTF Campaign Victory Star")) {
		strncpy(name, "NTF-Siegesstern ", max_len);

	} else if (!strcmp(name, "Rank")) {
		strncpy(name, "Dienstgrad", max_len);

	} else if (!strcmp(name, "Wings")) {
		strncpy(name, "Fliegerspange", max_len);

	} else if (!strcmp(name, "Ace")) {
		strncpy(name, "Flieger-As", max_len);	

	} else if (!strcmp(name, "Double Ace")) {
		strncpy(name, "Doppel-As ", max_len);

	} else if (!strcmp(name, "Triple Ace")) {
		strncpy(name, "Dreifach-As ", max_len);
		
	} else if (!strcmp(name, "SOC Unit Crest")) {
		strncpy(name, "SEK-Abzeichen ", max_len);	
	}
}

void blit_label(char *label, int *coords, int num)
{
	int x, y, sw;
	char text[256];

	gr_set_color_fast(&Color_bright);

	// translate medal names before displaying
	// can't translate in table cuz the names are used in comparisons
	if (Lcl_gr) {
		char translated_label[256];
		strncpy(translated_label, label, 256);
		medals_translate_name(translated_label, 256);

		// set correct string
		if ( num > 1 ) {
			sprintf( text, NOX("%s (%d)"), translated_label, num );
		} else {
			sprintf( text, "%s", translated_label );
		}		
	} else {
		// set correct string
		if ( num > 1 ) {
			sprintf( text, NOX("%s (%d)"), label, num );
		} else {
			sprintf( text, "%s", label );
		}
	}

	// find correct coords
	gr_get_string_size(&sw, NULL, text);
	x = Medals_label_coords[gr_screen.res][0] + (Medals_label_coords[gr_screen.res][2] - sw) / 2;
	y = Medals_label_coords[gr_screen.res][1];

	// do it
	gr_string(x, y, text);
}

void blit_callsign()
{
	gr_set_color_fast(&Color_normal);

	// nothing special, just do it.
	gr_string(0x8000, Medals_callsign_y[gr_screen.res], Medals_player->callsign);
}

int medal_main_do()
{
   int region,selected, k;

	k = Medals_window.process();	

	// process an exit command
	if ((k == KEY_ESC) && (Medals_mode == MM_NORMAL)) {
		gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
	}

	// draw the background medal display case
	gr_reset_clip();
	GR_MAYBE_CLEAR_RES(Medals_bitmap);
	if(Medals_bitmap != -1){
		gr_set_bitmap(Medals_bitmap);
		gr_bitmap(0,0);
	}

	// check to see if a button was pressed
	if( (k == (KEY_CTRLED|KEY_ENTER)) || (Medals_buttons[gr_screen.res][MEDALS_EXIT].button.pressed()) ) {	
		gamesnd_play_iface(SND_COMMIT_PRESSED);
		if(Medals_mode == MM_NORMAL){
			gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
		} else {
			// any calling popup function will know to close the screen down
			return 0;
		}		
	}

	// blit medals also takes care of blitting the rank insignia
	blit_medals(); 
	blit_callsign();	
	
	region = snazzy_menu_do((ubyte*)Medals_mask->data, Medals_mask_w, Medals_mask_h, NUM_MEDAL_REGIONS, Medal_regions, &selected);
	switch (region) {
		case ESC_PRESSED:
			if (Medals_mode == MM_NORMAL) {
				gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
			} else {
				// any calling popup function will know to close the screen down
				return 0;
			}
			break;

		case RANK_MEDAL_REGION :
			blit_label(Ranks[Player_score->rank].name, &Medal_coords[gr_screen.res][region][0], 1);
			break;

		case -1:
			break;

		default :
      	if (Player_score->medals[region] > 0){
				blit_label(Medals[region].name, &Medal_coords[gr_screen.res][region][0], Player_score->medals[region] );
			}
			break;
	} // end switch

	Medals_window.draw();

	gr_flip();

	return 1;
}

void medal_main_close()
{
	int idx;
	if (Init_flags & MEDAL_BITMAP_INIT)
		bm_release(Medals_bitmap);

	if (Init_flags & MASK_BITMAP_INIT) {
		bm_unlock(Medals_bitmap_mask);
		bm_release(Medals_bitmap_mask);
	}

   for (idx=Medal_bitmaps.size()-1; idx >= 0; idx--) {
		if (Medal_bitmaps[idx] > -1){
			bm_release(Medal_bitmaps[idx]);
		}
		Medal_bitmaps.pop_back();
	}

   Player_score = NULL;
	Medals_window.destroy();
	snazzy_menu_close();
	palette_restore_palette();
}

/*
void init_medal_palette()
{
	Medal_palette = bm_load("MedalsPalette.pcx");
	if(Medal_palette > -1){
#ifndef HARDWARE_ONLY
		palette_use_bm_palette(Medal_palette);
#endif
	}
}
*/

// function to load in the medals for this player.  It loads medals that the player has (known
// by whether or not a non-zero number is present in the player's medal array), then loads the
// rank bitmap

void init_medal_bitmaps()
{
	int idx;
	Assert(Player_score);

	for (idx=0; idx<Num_medals; idx++) {
		Medal_bitmaps.push_back(-1);
		if (Player_score->medals[idx] > 0) {
			int num_medals;
			char filename[NAME_LENGTH], base[NAME_LENGTH];
			
			// possibly load a different filename that is specified by the bitmap filename
			// for this medal.  if the player has > 1 of these types of medals, then determien
			// which of the possible version to use based on the player's count of this medal
			strcpy( filename, Medals[idx].bitmap );

			_splitpath( filename, NULL, NULL, base, NULL );

			num_medals = Player_score->medals[idx];

			// can't display more than the maximum number of version for this medal
			if ( num_medals > Medals[idx].num_versions )
				num_medals = Medals[idx].num_versions;

			if ( num_medals > 1 ) {
				// append the proper character onto the end of the medal filename.  Base version
				// has no character. next version is a, then b, etc.
				sprintf( base, "%s%c", base, (num_medals-2)+'a');
			}

			// hi-res support
			if (gr_screen.res == GR_1024) {
				sprintf( filename, "2_%s", base );
			}

			// base now contains the actual medal bitmap filename needed to load
			// we don't need to pass extension to bm_load anymore, so just use the basename
			// as is.
         Medal_bitmaps[idx] = bm_load( filename );
			Assert( Medal_bitmaps[idx] != -1 );
		}
	}

	// load up rank insignia
	if (gr_screen.res == GR_1024) {
		char filename[NAME_LENGTH];
		sprintf(filename, "2_%s", Ranks[Player_score->rank].bitmap);
		Rank_bm = bm_load(filename);
	} else {
		Rank_bm = bm_load(Ranks[Player_score->rank].bitmap);
	}
}

void init_snazzy_regions()
{
	int idx;

	// snazzy regions for the medals/ranks, etc.
	for (idx=0; idx<MAX_MEDALS; idx++) {
		if (idx == RANK_MEDAL_REGION) 
			continue;

		snazzy_menu_add_region(&Medal_regions[idx], "", idx, 0);
	}

	// add the rank medal region
	snazzy_menu_add_region(&Medal_regions[RANK_MEDAL_REGION], "", RANK_MEDAL_REGION,0);
}


// blit the medals -- this includes the rank insignia
void blit_medals()
{
	int idx;

	for (idx=0; idx<Num_medals; idx++) {
		if (Player_score->medals[idx] > 0) {
			gr_set_bitmap(Medal_bitmaps[idx]);
			gr_bitmap(Medal_coords[gr_screen.res][idx][0], Medal_coords[gr_screen.res][idx][1]);
		}
	}

	// now blit rank, since that "medal" doesn't get loaded (or drawn) the normal way
	gr_set_bitmap(Rank_bm);
	gr_bitmap(Medal_coords[gr_screen.res][RANK_MEDAL_REGION][0], Medal_coords[gr_screen.res][RANK_MEDAL_REGION][1]);
}
