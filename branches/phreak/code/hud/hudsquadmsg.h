/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/Hud/HUDsquadmsg.h $
 * $Revision: 2.17.2.1 $
 * $Date: 2006-08-27 18:04:24 $
 * $Author: taylor $
 *
 * header file for squadmate messaging
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.17  2006/04/20 06:32:07  Goober5000
 * proper capitalization according to Volition
 *
 * Revision 2.16  2006/03/18 22:00:43  Goober5000
 * fix comm order initialization bug
 * --Goober5000
 *
 * Revision 2.15  2006/03/01 04:01:37  Goober5000
 * fix comm message localization
 *
 * Revision 2.14  2006/02/02 07:00:29  Goober5000
 * consolidated comm order stuff
 * --Goober5000
 *
 * Revision 2.13  2006/02/02 06:22:58  Goober5000
 * replaced Fred_comm_orders with proper Comm_orders, just for WMC ;)
 * --Goober5000
 *
 * Revision 2.12  2006/02/02 06:04:02  Goober5000
 * restore retail compatibility with comm orders: one logic bugfix, one cosmetic bugfix
 * --Goober5000
 *
 * Revision 2.11  2006/01/13 11:09:45  taylor
 * fix hud comm message screwups (missing support ship, no coverme, etc) that was part :V: bug and (bigger) part Ship_types related bug
 *
 * Revision 2.10  2006/01/10 18:37:46  randomtiger
 * Improvements to voice recognition system.
 * Also function put on -voicer launcher option.
 *
 * Revision 2.9  2006/01/03 17:07:10  randomtiger
 * Added voice recognition functionality for Visual C6 project only.
 * Currently still a work in progress.
 *
 * Revision 2.8  2005/12/29 08:08:34  wmcoolmon
 * Codebase commit, most notably including objecttypes.tbl
 *
 * Revision 2.7  2005/07/13 03:15:52  Goober5000
 * remove PreProcDefine #includes in FS2
 * --Goober5000
 *
 * Revision 2.6  2004/08/11 05:06:25  Kazan
 * added preprocdefines.h to prevent what happened with fred -- make sure to make all fred2 headers include this file as the _first_ include -- i have already modified fs2 files to do this
 *
 * Revision 2.5  2004/03/05 09:02:03  Goober5000
 * Uber pass at reducing #includes
 * --Goober5000
 *
 * Revision 2.4  2003/09/13 08:27:29  Goober5000
 * added some minor things, such as code cleanup and the following:
 * --turrets will not fire at cargo
 * --MAX_SHIELD_SECTIONS substituted for the number 4 in many places
 * --supercaps have their own default message bitfields (distinguished from capships)
 * --turrets are allowed on fighters
 * --jump speed capped at 65m/s, to avoid ship travelling too far
 * --non-huge weapons now scale their damage, instead of arbitrarily cutting off
 * ----Goober5000
 *
 * Revision 2.3  2003/09/13 06:02:05  Goober5000
 * clean rollback of all of argv's stuff
 * --Goober5000
 *
 * Revision 2.1  2002/08/01 01:41:05  penguin
 * The big include file move
 *
 * Revision 2.0  2002/06/03 04:02:23  penguin
 * Warpcore CVS sync
 *
 * Revision 1.1  2002/05/02 18:03:08  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 3     3/30/99 5:40p Dave
 * Fixed reinforcements for TvT in multiplayer.
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 26    9/11/98 2:05p Allender
 * make reinforcements work correctly in multiplayer games.  There still
 * may be a team vs team issue that I haven't thought of yet :-(
 * 
 * 25    5/19/98 12:19p Mike
 * Cheat codes!
 * 
 * 24    5/13/98 5:09p Allender
 * Make send_command functions return whether message was sent or not
 * 
 * 23    5/04/98 8:58a Allender
 * new key saving routine
 * 
 * 22    4/22/98 4:59p Allender
 * new multiplayer dead popup.  big changes to the comm menu system for
 * team vs. team.  Start of debriefing stuff for team vs. team  Make form
 * on my wing work with individual ships who have high priority orders
 * 
 * 21    4/13/98 12:51p Allender
 * made countermeasure succeed indicator work in multiplayer.  Make rearm
 * shortcut work more appropriately.
 * 
 * 20    2/26/98 4:21p John
 * Changed comm_orders around so I could externalize the strings
 * 
 * 19    2/20/98 8:34p Lawrance
 * added hud_squadmsg_reinforcements_available(), since this information
 * is required outside of hudsquadmsg
 * 
 * 18    2/11/98 9:44p Allender
 * rearm repair code fixes.  hud support view shows abort status.  New
 * support ship killed message.  More network stats
 * 
 * 17    11/20/97 12:08a Allender
 * added 'all fighters' option at top level menu.  cleaned up messaging to
 * player a little bit
 * 
 * 16    11/07/97 4:35p Dave
 * Fixed multiplayer support ship bug. Put in player kill/assists client
 * side stats update.
 * 
 * 15    11/03/97 10:09a Allender
 * fixed up graying out of comm menu options
 * 
 * 14    10/31/97 4:59p Allender
 * remove protect_ship from list of filtered orders
 * 
 * 13    10/31/97 4:33p Allender
 * fix bogus repair message.  gray out messages based on players target
 * 
 * 12    9/30/97 5:07p Dave
 * Adapted messaging to correctly work with client-server situations in
 * multiplayer.
 * 
 * 11    9/23/97 4:34p Allender
 * made squadmessaging use main keyboard reading loop instead of separate
 * key_inkey()
 * 
 * 10    9/18/97 5:21p Hoffoss
 * Added column breaks for message list in sexp trees, and added code for
 * orders sexp.
 * 
 * 9     7/25/97 9:52a Allender
 * added order dialog to Fred for designer to choose which orders for ship
 * to accept/ignore.  Code in ship_create to set up variables for Fred use
 * instead of FreeSpace use (for destroying ships before mission starts)
 * 
 * 8     6/16/97 2:58p Allender
 * added ignore my target order to player's comm menu.  Fixed a couple of
 * problem when trying to message wings
 * 
 * 7     4/30/97 2:32p Allender
 * add ability to message enemies as debug tool
 * 
 * 6     4/07/97 3:50p Allender
 * ability to assign > 1 ship to a hotkey.  Enabled use of hotkeys in
 * squadmate messaging
 * 
 * 5     4/05/97 3:46p Allender
 * lots 'o messaging stuff.  Make shortcut keys for squadmate messaging
 * work.  Make menus a little more dynamic
 * 
 * 4     2/06/97 1:49p Allender
 * more messaging stuff -- most menus printout out stuff -- nothing
 * functional in terms of action
 * 
 * 3     2/03/97 4:50p Allender
 * saving/restoring of keys used for messaging mode
 * 
 * 2     1/31/97 9:38a Allender
 * checkpoint for sqaudmate messaging
 * 
 * 1     1/24/97 2:18p Allender
 *
 * $NoKeywords: $
 */

#ifndef _HUD_SQUADMSG
#define _HUD_SQUADMSG

#include "network/multi.h"
#include <vector>

#define SM_MODE_TYPE_SELECT			1		//am I going to message a ship or a wing
#define SM_MODE_SHIP_SELECT			2		//choosing actual ship
#define SM_MODE_WING_SELECT			3		//choosing actual wing
#define SM_MODE_SHIP_COMMAND			4		//which command to send to a ship
#define SM_MODE_WING_COMMAND			5		//which command to send to a wing
#define SM_MODE_REINFORCEMENTS		6		//call for reinforcements
#define SM_MODE_REPAIR_REARM			7		//repair/rearm player ship
#define SM_MODE_REPAIR_REARM_ABORT	8		//abort repair/rearm of player ship
#define SM_MODE_ALL_FIGHTERS			9		//message all fighters/bombers

// define for trapping messages send to "all fighters"
#define MESSAGE_ALL_FIGHTERS		-999

struct object;

// defines for messages that can be sent from the player.  Defined at bitfields so that we can enable
// and disable messages on a message by message basis

#define NUM_COMM_ORDER_ITEMS		16

#define ATTACK_TARGET_ITEM			(1<<0)
#define DISABLE_TARGET_ITEM			(1<<1)
#define DISARM_TARGET_ITEM			(1<<2)
#define PROTECT_TARGET_ITEM			(1<<3)
#define IGNORE_TARGET_ITEM			(1<<4)
#define FORMATION_ITEM				(1<<5)
#define COVER_ME_ITEM				(1<<6)
#define ENGAGE_ENEMY_ITEM			(1<<7)
#define CAPTURE_TARGET_ITEM			(1<<8)

// the next are for the support ship only
#define REARM_REPAIR_ME_ITEM		(1<<9)
#define ABORT_REARM_REPAIR_ITEM		(1<<10)
#define STAY_NEAR_ME_ITEM			(1<<11)
#define STAY_NEAR_TARGET_ITEM		(1<<12)
#define KEEP_SAFE_DIST_ITEM			(1<<13)

// next item for all ships again -- to try to preserve relative order within the message menu
#define DEPART_ITEM					(1<<14)

// out of order, but it was this way in the original source
#define DISABLE_SUBSYSTEM_ITEM		(1<<15)


// data structure to hold character string of commands for comm menu
typedef struct comm_order {
	char name[NAME_LENGTH];
	int item;
} comm_order;

extern comm_order Comm_orders[];

// following defines are the set of possible commands that can be given to a ship.  A mission designer
// might not allow some messages

//WMC - Formerly FIGHTER_MESSAGES
#define DEFAULT_MESSAGES	(ATTACK_TARGET_ITEM | DISABLE_TARGET_ITEM | DISARM_TARGET_ITEM | PROTECT_TARGET_ITEM | IGNORE_TARGET_ITEM | FORMATION_ITEM | COVER_ME_ITEM | ENGAGE_ENEMY_ITEM | DEPART_ITEM | DISABLE_SUBSYSTEM_ITEM)
/*
#define BOMBER_MESSAGES		FIGHTER_MESSAGES			// bombers can do the same things as fighters

#define TRANSPORT_MESSAGES	(ATTACK_TARGET_ITEM | CAPTURE_TARGET_ITEM | DEPART_ITEM )
#define FREIGHTER_MESSAGES	TRANSPORT_MESSAGES		// freighters can do the same things as transports

#define CRUISER_MESSAGES	(ATTACK_TARGET_ITEM | DEPART_ITEM)

#define CAPITAL_MESSAGES	(DEPART_ITEM)				// can't order capitals to do much!!!!

#define SUPERCAP_MESSAGES	(0)							// supercaps ignore you :p

#define SUPPORT_MESSAGES	(REARM_REPAIR_ME_ITEM | ABORT_REARM_REPAIR_ITEM | STAY_NEAR_ME_ITEM | STAY_NEAR_TARGET_ITEM | KEEP_SAFE_DIST_ITEM | DEPART_ITEM )
*/
// these messages require an active target.  They are also the set of messages
// which cannot be given to a ship when the target is on the same team, or the target
// is not a ship.
#define ENEMY_TARGET_MESSAGES		(ATTACK_TARGET_ITEM | DISABLE_TARGET_ITEM | DISARM_TARGET_ITEM | IGNORE_TARGET_ITEM | STAY_NEAR_TARGET_ITEM | CAPTURE_TARGET_ITEM | DISABLE_SUBSYSTEM_ITEM )
#define FRIENDLY_TARGET_MESSAGES	(PROTECT_TARGET_ITEM)

#define TARGET_MESSAGES	(ENEMY_TARGET_MESSAGES | FRIENDLY_TARGET_MESSAGES)



typedef struct squadmsg_history {
	int order_to;			// ship/wing that received the order
	int order;				// order that the ship/wing received (see defines above)
	int target;				// target of the order
	int order_from;			// ship that sent the order
	int special_index;		// any extra data the order might need (subsystem names for instance)
	fix order_time;			// when this order was sent (or received by the server in multiplayer)
	squadmsg_history(): order_to(-1), order(-1), target(-1), order_from(-1), special_index(-1), order_time(0) {};
} squadmsg_history;

extern std::vector<squadmsg_history> Squadmsg_history; 

/*
#define SQUADMSG_HISTORY_MAX 160

typedef struct squadmsg_history {
	int ship;  // ship that received the order
	int order;  // order that the ship received (see defines above)
	int target;  // ship that is the target of the order

	squadmsg_history(): ship(-1), order(-1), target(-1) {};
} squadmsg_history;


extern int squadmsg_history_index;
extern squadmsg_history Squadmsg_history[SQUADMSG_HISTORY_MAX];
*/

extern int Multi_squad_msg_local;
extern int Multi_squad_msg_targ; 

extern void hud_init_squadmsg();
extern void hud_init_comm_orders();
extern void hud_squadmsg_toggle();						// toggles the state of messaging mode
extern void hud_squadmsg_shortcut( int command );	// use of a shortcut key
extern int hud_squadmsg_hotkey_select( int k );	// a hotkey was hit -- maybe send a message to those ship(s)
extern void hud_squadmsg_save_keys( int do_scroll = 0 );					// saves into local area keys which need to be saved/restored when in messaging mode
extern int hud_squadmsg_do_frame();
extern int hud_query_order_issued(char *to, char *order_name, char *target = NULL, int timestamp = 0, char *from = NULL, char *special_index = NULL);
extern int hud_squadmsg_read_key( int k );			// called from high level keyboard code

extern void hud_squadmsg_repair_rearm( int toggle_state, object *obj = NULL );
extern void hud_squadmsg_repair_rearm_abort( int toggle_state, object *obj = NULL );
extern void hud_squadmsg_rearm_shortcut();


#define SQUADMSG_HISTORY_NO_UPDATE		0
#define SQUADMSG_HISTORY_UPDATE			1
#define SQUADMSG_HISTORY_ADD_ENTRY		2

extern int hud_squadmsg_send_ship_command( int shipnum, int command, int send_message, int update_history = SQUADMSG_HISTORY_ADD_ENTRY, int player_num = -1 );
extern int hud_squadmsg_send_wing_command( int wingnum, int command, int send_message, int update_history = SQUADMSG_HISTORY_ADD_ENTRY, int player_num = -1 );
extern void hud_squadmsg_send_to_all_fighters( int command, int player_num = -1 );
extern void hud_squadmsg_call_reinforcement(int reinforcement_num, int player_num = -1);

extern int hud_squadmsg_reinforcements_available(int team);

//#ifndef NDEBUG
void hud_enemymsg_toggle();						// debug function to allow messaging of enemies
//#endif

// Added for voicer implementation
void hud_squadmsg_do_mode( int mode );

#endif
