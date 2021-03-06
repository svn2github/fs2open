/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Demo/Demo.h $
 * $Revision: 2.2 $
 * $Date: 2005-07-13 02:50:51 $
 * $Author: Goober5000 $
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.1  2004/08/11 05:06:21  Kazan
 * added preprocdefines.h to prevent what happened with fred -- make sure to make all fred2 headers include this file as the _first_ include -- i have already modified fs2 files to do this
 *
 * Revision 2.0  2002/06/03 04:02:21  penguin
 * Warpcore CVS sync
 *
 * Revision 1.1  2002/05/02 18:03:04  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 3     3/29/99 6:17p Dave
 * More work on demo system. Got just about everything in except for
 * blowing ships up, secondary weapons and player death/warpout.
 * 
 * 2     3/28/99 5:58p Dave
 * Added early demo code. Make objects move. Nice and framerate
 * independant, but not much else. Don't use yet unless you're me :)
 *  
 *  
 * $NoKeywords: $
 */


#ifndef _FS2_DEMO_SYSTEM_HEADER_FILE
#define _FS2_DEMO_SYSTEM_HEADER_FILE

// -----------------------------------------------------------------------------------------------------------------------------
// DEMO DEFINES/VARS
//

// compile in the demo system
#define DEMO_SYSTEM

struct object;
struct ship;

// an error reading or writing the demo file
#define DEMO_ERROR_NONE					0					// no error
#define DEMO_ERROR_GENERAL				1					// general problem
#define DEMO_ERROR_FRAMESIZE			2					// frame size was too big (> 32k)
#define DEMO_ERROR_DISK_SPACE			3					// out of disk space
#define DEMO_ERROR_DISK_ACCESS		4					// problem accessing disk
#define DEMO_ERROR_VERSION				5					// bad version #
#define DEMO_ERROR_MISSION				6					// different mission file checksums
extern int Demo_error;

extern int Demo_make;

// -----------------------------------------------------------------------------------------------------------------------------
// DEMO FUNCTIONS
//

// do frame for the demo - start for the frame
int demo_do_frame_start();

// do frame for the demo - end for the frame
int demo_do_frame_end();

// initialize a demo for recording
// NOTE : call this after loading the mission and going through the briefing, but _before_ physically moving into the mission
int demo_start_record(char *file);

// initialize a demo for playback - calling this will load up the demo file and move the player into the playback state
int demo_start_playback(char *file);

// finish the demo, whether recording or playing back
void demo_close();

// if we should run the simulation for this object, or let the demo system handle it
int demo_should_sim(object *objp);


// functions for posting critical events to the demo system ------------------------------------------------------------

// post an object dump event (only used internally)
void demo_POST_object_dump();

// post a primary fired event
void demo_POST_primary_fired(object *objp, int banks, int linked);

// post a unique message
void demo_POST_unique_message(char *id, char *who_from, int m_source, int priority);

// post a builtin message
void demo_POST_builtin_message(int type, ship *shipp, int priority, int timing);

// post an object create message
void demo_POST_obj_create(char *pobj_name, int signature);

// post a warpin event
void demo_POST_warpin(int signature, int ship_flags);

// post a warpout event
void demo_POST_warpout(int signature, int ship_flags);

// post a departed event
void demo_POST_departed(int signature, int ship_flags);

// post a ship kill event
void demo_POST_ship_kill(object *objp);

#endif
