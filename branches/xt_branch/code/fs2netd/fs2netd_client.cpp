/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /freespace2/code/fs2netd/fs2netd_client.cpp $
 * $Revision: 1.1.2.3 $
 * $Date: 2007-11-22 05:04:07 $
 * $Author: taylor $
 *
 * FS2NetD client handler
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.2.2  2007/10/15 08:18:33  taylor
 * Oops, forgot that the v.2 daemon is still on the secondary IP :)
 *
 * Revision 1.1.2.1  2007/10/15 06:43:09  taylor
 * FS2NetD v.2  (still a work in progress, but is ~98% complete)
 *
 *
 * $NoKeywords: $
 */

#include "globalincs/pstypes.h"
#include "fs2netd/fs2netd_client.h"
#include "fs2netd/tcp_client.h"
#include "cfile/cfile.h"
#include "network/multi_log.h"
#include "osapi/osregistry.h"
#include "popup/popup.h"
#include "gamesnd/gamesnd.h"
#include "network/multi.h"
#include "playerman/player.h"
#include "io/timer.h"
#include "network/multiutil.h"
#include "network/multiui.h"
#include "network/stand_gui.h"
#include "network/multi_pxo.h"
#include "bmpman/bmpman.h"
#include "graphics/2d.h"
#include "graphics/font.h"
#include "globalincs/alphacolors.h"
#include "network/multi_options.h"
#include "gamesequence/gamesequence.h"

#ifdef WIN32
//#include <windows.h>
//#include <process.h>
#else
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <cerrno>
#endif

#include <string>
#include <vector>


#define FS2NETD_DEFAULT_PORT			"12009"
#define FS2NETD_DEFAULT_SERVER			"fs2netd.game-warden.com"
#define FS2NETD_DEFAULT_CHAT_SERVER		"fs2netd.game-warden.com"
#define FS2NETD_DEFAULT_BANNER_URL		"http://fs2netd.game-warden.com/files/banners"


extern int Om_tracker_flag; // needed to know whether or not to use FS2OpenPXO
extern int Multi_debrief_stats_accept_code;
extern void HUD_printf(char *format, ...);
extern int game_hacked_data();
void multi_update_valid_tables(); // from multiutil
extern int Multi_create_force_heartbeat;			// to force a master heardbeat packet be sent (rather than waiting for timeout)
extern void send_udp_hole_punch(char *ip, short port, short state);

static int PXO_SID = -1; // FS2 Open PXO Session ID
static char PXO_Server[64] = { 0 };
static ushort PXO_port = 0;
static bool Is_connected = false;
static bool In_process = false;
static bool Logged_in = false;
static int do_full_packet = 1;
static fix timeout = -1;
static fix NextHeartBeat = -1;
static ushort GameServerPort = 0;

static int FS2NetD_file_list_count = -1;
static file_record *FS2NetD_file_list = NULL;

static int FS2NetD_ban_list_count = -1;
static fs2open_banmask *FS2NetD_ban_list = NULL;

std::vector<crc_valid_status> Table_valid_status;

// channel to associate when creating a server
char Multi_fs_tracker_channel[MAX_PATH] = "";

// channel to use when polling the tracker for games
char Multi_fs_tracker_filter[MAX_PATH] = "";


void fs2netd_options_config_init()
{
//	if ( !strlen(Multi_options_g.game_tracker_ip) ) {
		ml_printf("FS2NetD MSG:  Address for game tracker not specified, using default instead (%s).", FS2NETD_DEFAULT_SERVER);
		strncpy( Multi_options_g.game_tracker_ip, FS2NETD_DEFAULT_SERVER, MULTI_OPTIONS_STRING_LEN );
//	}

//	if ( !strlen(Multi_options_g.user_tracker_ip) ) {
		ml_printf("FS2NetD MSG:  Address for user tracker not specified, using default instead (%s).", FS2NETD_DEFAULT_SERVER);
		strncpy( Multi_options_g.user_tracker_ip, FS2NETD_DEFAULT_SERVER, MULTI_OPTIONS_STRING_LEN );
//	}

//	if ( !strlen(Multi_options_g.tracker_port) ) {
		ml_printf("FS2NetD MSG:  Port for game/user trackers not specified, using default instead (%u).", FS2NETD_DEFAULT_PORT);
		strncpy( Multi_options_g.tracker_port, FS2NETD_DEFAULT_PORT, STD_NAME_LEN );
//	}

//	if ( !strlen(Multi_options_g.pxo_ip) ) {
		ml_printf("FS2NetD MSG:  Address for chat server not specified, using default instead (%s).", FS2NETD_DEFAULT_CHAT_SERVER);
		strncpy( Multi_options_g.pxo_ip, FS2NETD_DEFAULT_CHAT_SERVER, MULTI_OPTIONS_STRING_LEN );
//	}

	if ( !strlen(Multi_options_g.pxo_banner_url) ) {
		ml_printf("FS2NetD MSG:  URL for banners not specified, using default instead (%s).", FS2NETD_DEFAULT_BANNER_URL);
		strncpy( Multi_options_g.pxo_banner_url, FS2NETD_DEFAULT_BANNER_URL, MULTI_OPTIONS_STRING_LEN );
	}
}

static int fs2netd_connect_do()
{
	int retval = FS2NetD_ConnectToServer(PXO_Server, PXO_port);

	Sleep(5);

	switch (retval) {
		// connection failed
		case -1:
			Is_connected = false;
			return 2;

		// still trying to connect
		case 0:
			return 0;

		// connected!
		case 1:
			Is_connected = true;
			return 1;
	}

	return 0;
}

void fs2netd_connect()
{
	int rc = 0;

	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return;
	}

	if (Is_connected) {
		return;
	}


	if ( !PXO_port ) {
		Assert( strlen(Multi_options_g.game_tracker_ip) );
		Assert( strlen(Multi_options_g.tracker_port) );
	
		if ( strlen(Multi_options_g.game_tracker_ip) ) {
			strncpy( PXO_Server, Multi_options_g.game_tracker_ip, sizeof(PXO_Server) - 1 );
		} else {
			ml_printf("FS2NetD ERROR:  No server specified in multi.cfg!  Using default instead (%s)!", FS2NETD_DEFAULT_SERVER);
			strncpy( PXO_Server, FS2NETD_DEFAULT_SERVER, sizeof(PXO_Server) - 1 );
		}

		if ( strlen(Multi_options_g.tracker_port) ) {
			long tmp = strtol(Multi_options_g.tracker_port, (char**)NULL, 10);

			if ( (tmp < 1024) || (tmp > USHRT_MAX) ) {
				ml_printf("FS2NetD ERROR:  The port specified in multi.cfg, '%i', is outside of the required range, %i through %i!", tmp, 1024, USHRT_MAX);
				ml_printf("Fs2NetD ERROR:  Setting port to default value (%s) ...", FS2NETD_DEFAULT_PORT);
				PXO_port = (ushort) strtol(FS2NETD_DEFAULT_PORT, (char**)NULL, 10);
			} else {
				PXO_port = (ushort)tmp;
			}
		} else {
			PXO_port = (ushort) strtol(FS2NETD_DEFAULT_PORT, (char**)NULL, 10);
		}
	}

	In_process = true;

	if (Is_standalone) {
		do { rc = fs2netd_connect_do(); } while (!rc);
	} else {
		popup_till_condition(fs2netd_connect_do, XSTR("&Cancel", 779), XSTR("Connecting into FS2NetD", -1));
	}

	In_process = false;
}

int fs2netd_login_do()
{
	if (PXO_SID < 0) {
		if ( Is_standalone && std_gen_is_active() ) {
			std_gen_set_text("Verifying username and password", 1);
		} else {
			popup_change_text( XSTR("Verifying username and password", -1) );
		}

		if (timeout == -1) {
			timeout = timer_get_fixed_seconds() + (15 * F1_0);
		}

		// if timeout passes then bail on SID failure
		if ( timer_get_fixed_seconds() > timeout ) {
			timeout = -1;
			return 2;
		}

		PXO_SID = FS2NetD_Login(Multi_tracker_login, Multi_tracker_passwd, do_full_packet);

		// if we have already been through once then only deal with the recieve packet next time
		do_full_packet = 0;

		// invalid login
		if (PXO_SID == -2) {
			timeout = -1;
			return 1;
		}

		if (PXO_SID >= 0) {
			ml_printf("FS2NetD MSG: Login '%s' is valid, session ID is %i!", Multi_tracker_login, PXO_SID);
			do_full_packet = 1;
			timeout = -1;
		}
	} else {
		if ( Is_standalone && std_gen_is_active() ) {
			std_gen_set_text("Getting pilot stats", 1);
		} else {
			popup_change_text( XSTR("Getting pilot stats", -1) );
		}

		if (timeout == -1) {
			timeout = timer_get_fixed_seconds() + (30 * F1_0);
		}

		// if timeout passes then bail on stats failure
		if ( timer_get_fixed_seconds() > timeout ) {
			timeout = -1;
			return 2;
		}

		int rescode = FS2NetD_GetPlayerData(PXO_SID, Players[Player_num].callsign, &Players[Player_num], true, do_full_packet);

		do_full_packet = 0;

		if ( rescode != -1 ) {
			timeout = -1;
			return (rescode + 3);
		}
	}

	return 0;
}

bool fs2netd_login()
{
	bool retval = true;
	int rc;

	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return false;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return false;
	}

	if ( Logged_in && (PXO_SID >= 0) ) {
		return true;
	}

	Logged_in = false;

	fs2netd_connect();

	if ( !Is_connected ) {
		if ( !Is_standalone ) {
			popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("Failed to connect to FS2NetD server!", -1));
		}

		return false;
	}

	char error_str[256];

	do_full_packet = 1;

	In_process = true;

	if (Is_standalone) {
		do { rc = fs2netd_login_do(); } while (!rc);
	} else {
		rc = popup_till_condition(fs2netd_login_do, XSTR("&Cancel", 779), XSTR("Logging into FS2NetD", -1));
	}

	In_process = false;

	memset( error_str, 0, sizeof(error_str) );

	switch (rc) {
		// the action was cancelled
		case 0:
			retval = false;
			break;

		// didn't get a session id
		case 1:
			ml_printf("FS2NetD ERROR:  Login %s/%s is invalid!", Multi_tracker_login, Multi_tracker_passwd);
			strcpy(error_str, "Login failed!");
			retval = false;
			break;

		// unknown failure fetching pilot data
		case 2:
			ml_printf("FS2NetD ERROR:  UNKNOWN ERROR when fetching pilot data");
			strcpy(error_str, "An Unknown Error (probably a timeout) occured when trying to retrieve your pilot data.");
			retval = false;
			break;

		// success!!
		case 3:
			ml_printf("FS2NetD MSG:  Got Pilot data");
			retval = true;
			break;

		// success!!  pilot was created
		case 4:
			ml_printf("FS2NetD MSG:  Created New Pilot");
			strcpy(error_str, "New Pilot has been created.");
			retval = true;
			break;

		// invalid pilot name
		case 5:
			ml_printf("FS2NetD ERROR:  Invalid Pilot!");
			strcpy(error_str, "Invalid Pilot name - A serious error has occured, Contact the FS2NetD Administrator!");
			retval = false;
			break;

		// the session id was invalid
		case 6:
			ml_printf("FS2NetD ERROR:  Invalid SID!");
			strcpy(error_str, "Invalid SID - A serious error has occured, Contact the FS2NetD Administrator!");
			retval = false;
			break;

		default:
			ml_printf("FS2NetD ERROR:  Unknown return case for GetPlayerData()");
			strcpy(error_str, "Unkown return case from GetPlayerData(). Contact the FS2NetD Administrator!");
			retval = false;
			break;
	}

	if ( !Is_standalone && strlen(error_str) ) {
		popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, error_str);
	}

	if (retval) {
		Logged_in = true;
	}

	return retval;
}

void fs2netd_do_frame()
{
	int rc, buffer_size, buffer_offset;
	char buffer[300], str[256];
	ubyte pid = 0;
	int itemp;
	static fix NextPing = -1;
	static fix NextHolePunch = -1;
	static fix GotPong = -1;
	bool reset = false;

	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return;
	}

	// not connected to server
	if ( !Is_connected ) {
		return;
	}

	// in a previous processing loop, so don't do a frame until that has completed
	if ( In_process ) {
		return;
	}


	// if we didn't get a PONG within 4 minutes the server connection must have dropped
	if ( (GotPong != -1) && ((NextPing - GotPong) > (240 * F1_0)) ) {
		ml_printf("FS2NetD WARNING:  Lost connection to server!");
		FS2NetD_Disconnect();

		Is_connected = false;
		Logged_in = false;
		PXO_SID = -1;

		NextHeartBeat = -1;
		NextHolePunch = -1;
		NextPing = -1;
		GotPong = -1;

		// try to reinit the server connection
		fs2netd_login();

		// make sure that we are good to go
		if ( !Is_connected ) {
			if (!Is_standalone) {
				gamesnd_play_iface(SND_GENERAL_FAIL);
				popup(PF_USE_AFFIRMATIVE_ICON | PF_TITLE_BIG | PF_TITLE_RED, 1, POPUP_OK, "ERROR:\nLost connection to the FS2NetD server!");
			}

			return;
		} else {
			ml_printf("FS2NetD NOTICE:  Connection to server has been reestablished!");
		}
	}

	// send out ping every 60 seconds
	if ( (NextPing == -1) || (timer_get_fixed_seconds() >= NextPing) ) {
		// if we have seen a long period of time between pings then reset the pong time too
		if ( (timer_get_fixed_seconds() - NextPing) > (120 * F1_0) ) {
			reset = true;
		}

		NextPing = timer_get_fixed_seconds() + (60 * F1_0);

		// we go ahead and set the initial GotPong here, even though we haven't gotten a pong yet
		if ( (GotPong == -1) || reset ) {
			GotPong = NextPing;
			reset = false;
		}

		FS2NetD_Ping();

		// also send out a SID check to keep our login verified
		if ( FS2NetD_CheckValidSID(PXO_SID) < 0 ) {
			ml_printf("FS2NetD WARNING:  Unable to validate login!");
			FS2NetD_Disconnect();

			Logged_in = false;
			PXO_SID = -1;

			NextHeartBeat = -1;
			NextHolePunch = -1;
			NextPing = -1;
			GotPong = -1;

			// try to log in again
			fs2netd_login();

			// make sure that we are good to go
			if ( !Is_connected ) {
				if (!Is_standalone) {
					gamesnd_play_iface(SND_GENERAL_FAIL);
					popup(PF_USE_AFFIRMATIVE_ICON | PF_TITLE_BIG | PF_TITLE_RED, 1, POPUP_OK, "ERROR:\nLost connection to the FS2NetD server!");
				}
		
				return;
			} else {
				ml_printf("FS2NetD NOTICE:  Connection to server has been reestablished!");
			}
		}

		ml_printf("FS2NetD sent PING/IDENT");
	}

	// handle server heart beats
	fs2netd_server_send_heartbeat();
/*
	// send out UDP hole punch updates
	// we only do this sparingly however, with the updates being sent:
	//   - every 15 seconds when at the lobby or game start screens
	//   - and a simple refresh every 5 minutes otherwise
	if ( (NextHolePunch == -1) || (timer_get_fixed_seconds() >= NextHolePunch) ) {
		int hp_timeout = 300;

		switch ( gameseq_get_state() ) {
			case GS_STATE_MULTI_JOIN_GAME:
			case GS_STATE_MULTI_START_GAME:
			case GS_STATE_PXO:
			case GS_STATE_PXO_HELP:
				hp_timeout = 15;
				break;
		}

		NextHolePunch = timer_get_fixed_seconds() + (hp_timeout * F1_0);


		UDP_hole_punch_data *cur = Net_player->hole_punch_data;

		while (cur != NULL) {
			if (cur->state == UDP_HOLE_PUNCH_STATE_DONE) {
				UDP_hole_punch_data *prev = Net_player->hole_punch_data;

				while (prev->next != cur) {
					prev = prev->next;
				}

				cur = prev->next = cur->next;

				continue;
			}

			send_udp_hole_punch(cur->ip, cur->port, cur->state);

			ml_printf("FS2NetD Info:  Sent UDP hole punch to '%s'", cur->ip);
	
			cur = cur->next;
		}
	}
*/

	// Check for GWall messages - ping replies, etc
	if ( (rc = FS2NetD_GetData(buffer, sizeof(buffer))) != -1 ) {
		int rc_total = rc;
		buffer_offset = 0;

		while (rc_total > buffer_offset) {
			// make sure we have enough data to try and process
			if (rc_total < BASE_PACKET_SIZE) {
				break;
			}

			PXO_GET_DATA( pid );
			PXO_GET_INT( buffer_size );

			while ( (rc_total < buffer_size) && ((sizeof(buffer) - rc_total) > 0) ) {
				if ( (rc = FS2NetD_GetData(buffer+rc_total, sizeof(buffer) - rc_total)) != -1 ) {
					rc_total += rc;
				} else {
					break;
				}
			}

			if (buffer_size <= 0) {
				break;
			}

			// we don't have the full packet, so bail
			if ( rc_total < (buffer_offset+buffer_size-BASE_PACKET_SIZE) ) {
				break;
			}

			// processing time!
			switch (pid) {
				case PCKT_PING: {
					PXO_GET_INT( itemp );

					ml_printf("FS2NetD received PING");

					FS2NetD_Pong(itemp);
					break;
				}

				case PCKT_PONG: {
					PXO_GET_INT( itemp );

					ml_printf("FS2NetD received PONG: %d ms", timer_get_milliseconds() - itemp);

					GotPong = timer_get_fixed_seconds();
					break;
				}

				case PCKT_NETOWRK_WALL: {
					PXO_GET_STRING( str );
					ml_printf("FS2NetD WALL received MSG: %s", str);
	
					switch (Netgame.game_state) {
						case NETGAME_STATE_FORMING:
						case NETGAME_STATE_BRIEFING:
						case NETGAME_STATE_MISSION_SYNC:
						case NETGAME_STATE_DEBRIEF:
							multi_display_chat_msg(str, 0, 0);
							break;

						/* -- Won't Happen - multi_do_frame() is not called during paused state 
							  so the game will not even receive the data during it
						case NETGAME_STATE_PAUSED: // EASY!
							send_game_chat_packet(Net_player, str, MULTI_MSG_ALL, NULL);
							break;
						*/

						case NETGAME_STATE_IN_MISSION: // gotta make it paused
							//multi_pause_request(1); 
							//send_game_chat_packet(Net_player, str, MULTI_MSG_ALL, NULL);
							HUD_printf(str);
							break;

						default:
							// do-nothing
							break;
					}

					break;
				}

				case PCKT_CHAT_CHAN_COUNT_REPLY: {
					PXO_GET_STRING( str );
					PXO_GET_INT( itemp );

					if ( (itemp < 0) || (itemp > USHRT_MAX) ) {
						itemp = 0;
					}

					multi_pxo_channel_count_update(str, itemp);

					break;
				}

				case PCKT_VALID_SID_REPLY: {
					ubyte login_status = 0;

					PXO_GET_DATA( login_status );

					ml_printf("FS2NetD IDENT:  Got %s login check", (login_status == 1) ? NOX("valid") : NOX("invalid"));

					if (login_status != 1) {
						Logged_in = false;
						PXO_SID = -1;
					}
				}

				default: {
					ml_printf("Unexpected FS2NetD Packet - PID = %x", pid);
					break;
				}
			}

			buffer_offset += buffer_size;
		}
	}
}

void fs2netd_server_send_heartbeat(bool force)
{
	if ( !Om_tracker_flag ) {
		return;
	}

	// if we aren't hosting this game then bail
	if ( !(Net_player->flags & NETINFO_FLAG_GAME_HOST) ) {
		return;
	}

	// is it actually time for a new hb?
	if ( !force && (timer_get_fixed_seconds() < NextHeartBeat) ) {
		return;
	}

	// don't bother if there is nothing to actually send yet
	if ( !strlen(Netgame.mission_name) ) {
		return;
	}

	FS2NetD_SendHeartBeat();

	GameServerPort = Netgame.server_addr.port;

	// we only need to send the chat channel update once per game server that is created
	if (NextHeartBeat == -1) {
		// sleep a little bit first to make sure that the server had time to register
		Sleep(50);

		fs2netd_update_chat_channel();
	}

	// set timeout for every 2 minutes
	NextHeartBeat = timer_get_fixed_seconds() + (120 * F1_0);
	Multi_create_force_heartbeat = 0;

	ml_printf("FS2NetD sent HeartBeat");
}

void fs2netd_server_disconnect()
{
	if ( !Om_tracker_flag ) {
		return;
	}

	// if we aren't hosting this game then bail
	if ( !(Net_player->flags & NETINFO_FLAG_GAME_HOST) ) {
		return;
	}

	if (GameServerPort == 0) {
		return;
	}

	FS2NetD_SendServerDisconnect(GameServerPort);

	GameServerPort = 0;
	NextHeartBeat = -1;

	ml_printf("FS2NetD sent game_server disconnect");
}

int fs2netd_load_servers_do()
{
	if (timeout == -1) {
		timeout = timer_get_fixed_seconds() + (30 * F1_0);
	}

	// if timeout passes then bail on stats failure
	if ( timer_get_fixed_seconds() > timeout ) {
		timeout = -1;
		return 3;
	}

	int rescode = FS2NetD_GetServerList(do_full_packet);

	do_full_packet = 0;

	if (rescode) {
		timeout = -1;
		return rescode;
	}

	return 0;
}

int fs2netd_load_servers()
{
	int rc = 0;

	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return 0;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return 0;
	}

	if ( !Is_connected ) {
		return 0;
	}

	
	// free up any existing server list
	multi_free_server_list();

	do_full_packet = 1;

	In_process = true;

	if (Is_standalone) {
		do { rc = fs2netd_load_servers_do(); } while (!rc);
	} else {
		rc = popup_till_condition(fs2netd_load_servers_do, XSTR("&Cancel", 779), XSTR("Requesting list of servers", -1));
	}

	In_process = false;

	switch (rc) {
		// operation canceled
		case 0:
			return 0;

		// successful
		case 1:
			return 1;

		// failed to send request packet
		case 2:
			if ( !Is_standalone ) {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("Server request failed!", -1));
			}

			return -1;
		
		// it timed out
		case 3:
			if ( !Is_standalone ) {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("Server request timed out!", -1));
			}

			return -1;
	}

	return 0;
}

void fs2netd_debrief_init()
{
	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return;
	}

	if ( !Om_tracker_flag ) {
		return;
	}

	if ( !Is_connected ) {
		return;
	}


	uint CurrentMissionChsum;
	bool mValidStatus = false;

	cf_chksum_long(Netgame.mission_name, &CurrentMissionChsum);

	mValidStatus = FS2NetD_CheckSingleMission(Netgame.mission_name, CurrentMissionChsum);

	if ( ((multi_num_players() > 1) || (Multi_num_players_at_start > 1)) && !game_hacked_data() && mValidStatus ) {
		// verify that we are logged in before doing anything else
		fs2netd_login();

		int spd_ret = FS2NetD_SendPlayerData(PXO_SID, Players[Player_num].callsign, Multi_tracker_login, &Players[Player_num]);

		switch (spd_ret) { // 0 = pilot updated, 1  = invalid pilot, 2 = invalid (expired?) sid
			case -1:
				multi_display_chat_msg( XSTR("<Did not receive response from server within timeout period>", -1), 0, 0 );
				multi_display_chat_msg( XSTR("<Your stats may not have been stored>", -1), 0, 0 );
				multi_display_chat_msg( XSTR("<This is not a critical error>", -1), 0, 0 );
				Multi_debrief_stats_accept_code = 1;
				break;

			case 0:
				multi_display_chat_msg( XSTR("<stats have been accepted>", 850), 0, 0 );
				Multi_debrief_stats_accept_code = 1;
				break;

			case 1:
				multi_display_chat_msg( XSTR("<stats have been tossed>", 850), 0, 0 );
				multi_display_chat_msg( XSTR("WARNING: Your pilot was invalid, this is a serious error, possible data corruption", -1), 0, 0 );
				Multi_debrief_stats_accept_code = 0;
				break;

			case 2:
				// we really shouldn't be here with the new code, but handle it just in case
				Int3();
			
				fs2netd_login();

				if (PXO_SID >= 0) {
					if ( !FS2NetD_SendPlayerData(PXO_SID, Players[Player_num].callsign, Multi_tracker_login, &Players[Player_num]) ) {
						multi_display_chat_msg( XSTR("<stats have been accepted>", 850), 0, 0 );
						Multi_debrief_stats_accept_code = 1;
						break;
					 }
				}

				multi_display_chat_msg( XSTR("<stats have been tossed>", 851), 0, 0 );
				Multi_debrief_stats_accept_code = 0;
				break;

			default:
				multi_display_chat_msg( XSTR("Unknown Stats Store Request Reply", -1), 0, 0 );
				break;
		}
	} else {
		multi_display_chat_msg( XSTR("<stats have been tossed>", 851), 0, 0 );
		Multi_debrief_stats_accept_code = 0;
	}
}

int fs2netd_update_ban_list_do()
{
	if (timeout == -1) {
		timeout = timer_get_fixed_seconds() + (30 * F1_0);
	}

	// if timeout passes then bail on stats failure
	if ( timer_get_fixed_seconds() > timeout ) {
		timeout = -1;
		return 2;
	}

	FS2NetD_ban_list = FS2NetD_GetBanList(&FS2NetD_ban_list_count, do_full_packet);

	do_full_packet = 0;

	if ( (FS2NetD_ban_list != NULL) || (FS2NetD_ban_list_count >= 0) ) {
		timeout = -1;
		return 1;
	}

	return 0;
}

void fs2netd_update_ban_list()
{
	int rc = 0;

	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return;
	}

	if (!Is_connected) {
		return;
	}


	// destroy the file prior to updating
	cf_delete( "banlist.cfg", CF_TYPE_DATA );

	do_full_packet = 1;

	In_process = true;

	if (Is_standalone) {
		do { rc = fs2netd_update_ban_list_do(); } while (!rc);
	} else {
		rc = popup_till_condition(fs2netd_update_ban_list_do, XSTR("&Cancel", 779), XSTR("Requesting IP ban list", -1));
	}

	In_process = false;


	if (FS2NetD_ban_list) {
		CFILE *banlist_cfg = cfopen("banlist.cfg", "wt", CFILE_NORMAL, CF_TYPE_DATA);

		if (banlist_cfg != NULL) {
			for (int i = 0; i < FS2NetD_ban_list_count; i++) {
				cfputs( FS2NetD_ban_list[i].ip_mask, banlist_cfg );
			}

			cfclose(banlist_cfg);
		}

		delete[] FS2NetD_ban_list;
	}

	FS2NetD_ban_list = NULL;
	FS2NetD_ban_list_count = -1;
}

bool fs2netd_player_banned(net_addr *addr)
{
	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return false;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return false;
	}

	if ( !Is_connected ) {
		return false;
	}

	char line[32]; // no line should be larger than 16, but let's be safe
	char ip_str[32];
	memset(ip_str, 0, 32);
	memset(line, 0, 32);

	bool retval = false;
	CFILE *banlist_cfg = cfopen("banlist.cfg", "rt", CFILE_NORMAL, CF_TYPE_DATA);

	if (banlist_cfg == NULL) {
		return false;
	}

	psnet_addr_to_string( ip_str, addr );

	while ( !cfeof(banlist_cfg) && !retval ) {
		cfgets(line, 32, banlist_cfg);

		if ( !strnicmp(ip_str, line, strlen(line)) ) {
			retval = true; // BANNINATED!!!
		}
	}

	cfclose(banlist_cfg);

	return retval;
}

int fs2netd_get_valid_missions_do()
{
	if (timeout == -1) {
		timeout = timer_get_fixed_seconds() + (30 * F1_0);
	}

	// get the available CRCs from the server if we need to
	if ( !FS2NetD_file_list && (FS2NetD_file_list_count < 0) ) {
		FS2NetD_file_list = FS2NetD_GetMissionsList(&FS2NetD_file_list_count, do_full_packet);

		do_full_packet = 0;

		// if timeout passes then bail on crc failure
		if ( timer_get_fixed_seconds() > timeout ) {
			timeout = -1;
			return 1;
		}
	}
	// we should have the CRCs, or there were no missions, so process them
	else {
		static char **file_names = NULL;
		static int idx = 0, count = 0;

		bool found = false;
		int file_index = 0;
		char valid_status = MVALID_STATUS_UNKNOWN;
		char full_name[MAX_FILENAME_LEN], wild_card[10];
		char val_text[MAX_FILENAME_LEN+15];
		int i;
		uint checksum = 0;

		// oops, something went wrong here...
		if (FS2NetD_file_list_count <= 0) {
			return 2;
		}

		if (file_names == NULL) {
			// allocate filename space	
			file_names = (char**) vm_malloc_q( sizeof(char*) * 1024 ); // 1024 files should be safe!

			if (file_names == NULL) {
				return 3;
			}

			memset( wild_card, 0, sizeof(wild_card) );
			strcpy( wild_card, NOX("*") );
			strcat( wild_card, FS_MISSION_FILE_EXT );

			idx = count = cf_get_file_list(1024, file_names, CF_TYPE_MISSIONS, wild_card);
		}

		// drop idx first thing
		idx--;

		// we should be done validating, or not just have nothing locally to validate
		if (idx < 0) {
			for (idx = 0; idx < count; idx++) {
				if (file_names[idx] != NULL) {
					vm_free(file_names[idx]);
					file_names[idx] = NULL;
				}
			}

			vm_free(file_names);
			file_names = NULL;

			idx = count = 0;

			return 4;
		}


		// verify all filenames that we know about with their CRCs
		// NOTE: that this is done for one file per frame, since this is inside of a popup
		memset( full_name, 0, MAX_FILENAME_LEN );
		strncpy( full_name, cf_add_ext(file_names[idx], FS_MISSION_FILE_EXT), sizeof(full_name) - 1 );

		memset( val_text, 0, sizeof(val_text) );
		snprintf( val_text, sizeof(val_text) - 1, "Validating:  %s", full_name );

		if (Is_standalone ) {
			if ( std_gen_is_active() ) {
				std_gen_set_text(val_text, 1);
			}
		} else {
			popup_change_text(val_text);
		}

		cf_chksum_long(full_name, &checksum);

		// try and find the file
		file_index = multi_create_lookup_mission(full_name);

		found = false;

		if (file_index >= 0) {
			for (i = 0; (i < FS2NetD_file_list_count) && (!found); i++) {
				if ( !stricmp(full_name, FS2NetD_file_list[i].name) ) {
					if (FS2NetD_file_list[i].crc32 == checksum) {
						found = true;
						valid_status = MVALID_STATUS_VALID;
					} else {
						valid_status = MVALID_STATUS_INVALID;
					}


					Multi_create_mission_list[file_index].valid_status = valid_status;
				}
			}

			if (found) {
				ml_printf("FS2NetD Mission Validation: %s  =>  Valid!", full_name);
			} else {
				ml_printf("FS2NetD Mission Validation: %s  =>  INVALID! -- 0x%08x", full_name, checksum);
			}
		}
	}

	return 0;
}

bool fs2netd_get_valid_missions()
{
	int rc = 0;

	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return false;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return false;
	}

	// maybe try to init first
	fs2netd_login();

	// if we didn't connect to FS2NetD then bail out now
	if ( !Is_connected ) {
		return false;
	}

	
	do_full_packet = 1;

	In_process = true;

	if (Is_standalone) {
		do { rc = fs2netd_get_valid_missions_do(); } while (!rc);
	} else {
		rc = popup_till_condition(fs2netd_get_valid_missions_do, XSTR("&Cancel", 779), XSTR("Starting mission validation", -1));
	}

	In_process = false;

	if (FS2NetD_file_list != NULL) {
		delete[] FS2NetD_file_list;
		FS2NetD_file_list = NULL;
	}

	FS2NetD_file_list_count = -1;

	switch (rc) {
		// canceled by popup
		case 0:
			return false;

		// timed out
		case 1:
			if ( !Is_standalone ) {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("Mission validation timed out!", -1));
			}

			return false;

		// no missions
		case 2:
			if ( !Is_standalone ) {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("No missions are available from the server for validation!", -1));
			}

			return false;

		// out of memory
		case 3:
			if ( !Is_standalone ) {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("Memory error during mission validation!", -1));
			}

			return false;
	}

	return true;
}

int fs2netd_update_valid_tables_do()
{
	if (timeout == -1) {
		timeout = timer_get_fixed_seconds() + (30 * F1_0);
	}

	int rc = FS2NetD_ValidateTableList(do_full_packet);

	// if timeout passes then bail on crc failure
	if ( timer_get_fixed_seconds() > timeout ) {
		timeout = -1;
		return 1;
	}

	do_full_packet = 0;

	if ( rc == 0 ) {
		return 0;
	}

	switch (rc) {
		// some error occured, assume that there are no valid table crcs
		case -1:
			timeout = -1;
			return 2;

		// timeout
		case 1:
			timeout = -1;
			return 1;

		// done!
		case 2:
			timeout = -1;
			return 3;
	}

	return 0;
}

int fs2netd_update_valid_tables()
{
	int rc;
	int hacked = 0;

	// if there are no tables to check with then bail
	if ( Table_valid_status.empty() ) {
		return -1;
	}

	// if we're not on FS2NetD then don't bother with this function
	if ( !Om_tracker_flag && (Game_mode & GM_MULTIPLAYER) ) {
		return -1;
	}

	// maybe try to init first
	fs2netd_login();

	// if we didn't connect to FS2NetD then bail out now
	if ( !Is_connected ) {
		return -1;
	}

	// if we're a standalone, show a dialog saying "validating tables"
	if (Game_mode & GM_STANDALONE_SERVER) {
		std_create_gen_dialog("Validating tables");
		std_gen_set_text("Querying FS2NetD:", 1);
	}

	do_full_packet = 1;

	In_process = true;

	if (Is_standalone) {
		do { rc = fs2netd_update_valid_tables_do(); } while (!rc);
	} else {
		rc = popup_till_condition(fs2netd_update_valid_tables_do, XSTR("&Cancel", 779), XSTR("Starting table validation", -1));
	}

	In_process = false;

	switch (rc) {
		// canceled by popup
		case 0:
			return -1;

		// timed out
		case 1: {
			if ( !Is_standalone ) {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("Table validation timed out!", -1));
			}

			return -1;
		}

		// no tables
		case 2: {
			if ( !Is_standalone ) {
				popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, XSTR("No tables are available from the server for validation!", -1));
			}

			return -1;
		}
	}

	// output the status of table validity to multi.log
	for (uint i = 0; i < Table_valid_status.size(); i++) {
		if (Table_valid_status[i].valid) {
			ml_printf("FS2NetD Table Check: '%s' -- Valid!", Table_valid_status[i].name);
		} else {
			ml_printf("FS2NetD Table Check: '%s' -- INVALID (0x%x)!", Table_valid_status[i].name, Table_valid_status[i].crc32);
			hacked = 1;
		}
	}

	// if we're a standalone, kill the validate dialog
	if (Game_mode & GM_STANDALONE_SERVER) {
		std_destroy_gen_dialog();
	}

	return hacked;
}

void fs2netd_add_table_validation(char *tblname)
{
	uint chksum = 0;

	// if the tbl name isn't valid then just assume that the tbl is too
	if ( (tblname == NULL) || !strlen(tblname) ) {
		return;
	}

	CFILE *tbl = cfopen(tblname, "rt", CFILE_NORMAL, CF_TYPE_TABLES);

	if (tbl == NULL) {
		return;
	}

	cf_chksum_long(tbl, &chksum);

	cfclose(tbl);

	crc_valid_status tbl_crc;

	strncpy(tbl_crc.name, tblname, NAME_LENGTH);
	tbl_crc.crc32 = chksum;
	tbl_crc.valid = 0;

	Table_valid_status.push_back( tbl_crc );
}

int fs2netd_get_pilot_info(const char *callsign, player *out_plr, bool first_call)
{
	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return -2;
	}

	if ( !(Game_mode & GM_MULTIPLAYER) ) {
		return -2;
	}

	if ( (out_plr == NULL) || (callsign == NULL) || !(strlen(callsign)) ) {
		return -2;
	}

	player new_plr;

	if (first_call) {
		memset( &new_plr, 0, sizeof(player) );
		strncpy( new_plr.callsign, callsign, CALLSIGN_LEN );

		memset( out_plr, 0, sizeof(player) );
	}

	int rc = FS2NetD_GetPlayerData(-2, callsign, &new_plr, false, (int)first_call );

	// some sort of failure
	if (rc > 0) {
		return -2;
	}

	if (rc == 0) {
		memcpy( out_plr, &new_plr, sizeof(player) );
	}

	// we should only be returning -1 (processing) or 0 (got data successfully)
	return rc;
}

void fs2netd_close()
{
	Table_valid_status.clear();

	if (FS2NetD_file_list != NULL) {
		delete[] FS2NetD_file_list;
		FS2NetD_file_list = NULL;
	}

	if (FS2NetD_ban_list != NULL) {
		delete[] FS2NetD_ban_list;
		FS2NetD_ban_list = NULL;
	}
}

void fs2netd_update_chat_channel()
{
	if ( !Om_tracker_flag ) {
		return;
	}

	if ( !Is_connected ) {
		return;
	}

	if ( !strlen(Multi_fs_tracker_channel) ) {
		return;
	}

	FS2NetD_ChatChannelUpdate(Multi_fs_tracker_channel);
}

void fs2netd_update_game_count(char *chan_name)
{
	if ( !Om_tracker_flag ) {
		return;
	}

	if ( !Is_connected ) {
		return;
	}

	if ( (chan_name == NULL) || !strlen(chan_name) ) {
		return;
	}

	FS2NetD_GameCountUpdate(chan_name);
}


void fs2netd_udp_hole_punch()
{
/*	// don't bother with this if we aren't on FS2NetD
	if ( !Om_tracker_flag ) {
		return;
	}

	if ( !Is_connected ) {
		return;
	}

	server_item *cur = Game_server_head;
	char ip[16];
	int i = 0;

	// give them a second to respond normally.
	fix end_time = timer_get_fixed_seconds() + F1_0;

	while ( timer_get_fixed_seconds() <= end_time ) {
		multi_do_frame();
	}


	do {
		i++;
		cur = cur->next;
	} while (cur != Game_server_head);


	// if everyone is counted then just bail
	if (i == Active_game_count) {
		ml_printf("FS2NetD Info: (UDP hole-punch) All servers responding.");
		return;
	}

	// check every server.
	do {
		bool has_responded = false;

		// this is only for TCP connections (which should always be the case for us anyway)
		if (cur->server_addr.type != NET_TCP) {
			continue;
		}

		// see if this server has responded normally.
		if ( Active_game_count > 0 ) {
			active_game *agcur = Active_game_head;

			do {
				// Check if they're equal.
				if ( (agcur->server_addr.port == cur->server_addr.port) &&
						memcmp(agcur->server_addr.addr, cur->server_addr.addr, 4) )
				{
					has_responded = true;
					break;
				}

				agcur = agcur->next;
			} while (agcur != Active_game_head);
		}

		if (has_responded) {
			cur = cur->next;
			continue;
		}

		memset(ip, 0, sizeof(ip));
		psnet_addr_to_string(ip, &cur->server_addr);

		ml_printf("FS2NetD Info: (UDP hole-punch) Requesting response from %s", ip);

		FS2NetD_UDPHolePunch(ip);

		cur = cur->next;
	} while (cur != Game_server_head);*/
}

