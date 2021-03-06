/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/Io/Joy_ff.h $
 * $Revision: 2.5 $
 * $Date: 2005-07-13 03:15:52 $
 * $Author: Goober5000 $
 *
 * Code for joystick Force Feedback.
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.4  2005/04/17 05:38:28  taylor
 * updated Linux joystick code that's a bit less insane speed wise
 * remove ability to build without joystick support, no reason to keep it around
 * fix unusable warning flag with libjpeg building
 *
 * Revision 2.3  2005/04/05 05:53:18  taylor
 * s/vector/vec3d/g, better support for different compilers (Jens Granseuer)
 *
 * Revision 2.2  2004/08/11 05:06:25  Kazan
 * added preprocdefines.h to prevent what happened with fred -- make sure to make all fred2 headers include this file as the _first_ include -- i have already modified fs2 files to do this
 *
 * Revision 2.1  2004/03/05 09:02:04  Goober5000
 * Uber pass at reducing #includes
 * --Goober5000
 *
 * Revision 2.0  2002/06/03 04:02:24  penguin
 * Warpcore CVS sync
 *
 * Revision 1.2  2002/05/10 06:05:08  mharris
 * Porting... added ifndef NO_JOYSTICK
 *
 * Revision 1.1  2002/05/02 18:03:08  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 4     5/08/98 5:31p Hoffoss
 * Isolated the joystick force feedback code more from dependence on other
 * libraries.
 * 
 * 3     5/07/98 12:24a Hoffoss
 * Finished up sidewinder force feedback support.
 * 
 * 2     5/04/98 11:08p Hoffoss
 * Expanded on Force Feedback code, and moved it all into Joy_ff.cpp.
 * Updated references everywhere to it.
 * 
 * $NoKeywords: $
 */

#ifndef __JOY_FF_H__
#define __JOY_FF_H__

#include "globalincs/pstypes.h"

int joy_ff_init();
void joy_ff_shutdown();
void joy_ff_stop_effects();
void joy_ff_mission_init(vec3d v);
void joy_reacquire_ff();
void joy_unacquire_ff();
void joy_ff_play_vector_effect(vec3d *v, float scaler);
void joy_ff_play_dir_effect(float x, float y);
void joy_ff_play_primary_shoot(int gain);
void joy_ff_play_secondary_shoot(int gain);
void joy_ff_adjust_handling(int speed);
void joy_ff_docked();
void joy_ff_play_reload_effect();
void joy_ff_afterburn_on();
void joy_ff_afterburn_off();
void joy_ff_explode();
void joy_ff_fly_by(int mag);
void joy_ff_deathroll();

#endif
