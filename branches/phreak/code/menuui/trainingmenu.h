/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/MenuUI/TrainingMenu.h $
 * $Revision: 2.2 $
 * $Date: 2005-07-13 03:26:00 $
 * $Author: Goober5000 $
 *
 * Header file for code that controls the Training menu
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.1  2004/08/11 05:06:27  Kazan
 * added preprocdefines.h to prevent what happened with fred -- make sure to make all fred2 headers include this file as the _first_ include -- i have already modified fs2 files to do this
 *
 * Revision 2.0  2002/06/03 04:02:24  penguin
 * Warpcore CVS sync
 *
 * Revision 1.1  2002/05/02 18:03:10  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 7     8/31/97 6:38p Lawrance
 * pass in frametime to do_frame loop
 * 
 * 6     11/21/96 7:14p Lawrance
 * converted menu code to use a file (menu.tbl) to get the data for the
 * menu
 * 
 * 5     11/13/96 4:02p Lawrance
 * complete over-haul of the menu system and the states associated with
 * them
 * 
 * 4     11/13/96 8:32a Lawrance
 * streamlined menu code
 * 
 * 3     11/06/96 8:54a Lawrance
 * added revision templates, made more efficient
 *
 * $NoKeywords: $
 *
*/

#ifndef _TRAININGMENU_H
#define _TRAININGMENU_H

#define TRAINING_MENU_MAX_CHOICES	3	// keep up to date if any more choices added!

// these are the colour values of the pixels that form the different training menu regions
#define TRAINING_MENU_TRAINING_MISSIONS_MASK			1
#define TRAINING_MENU_REPLAY_MISSIONS_MASK			2
#define TRAINING_MENU_RETURN_MASK						3

// function prototypes
//
void training_menu_init();
void training_menu_close();
void training_menu_do_frame(float frametime);

#endif
