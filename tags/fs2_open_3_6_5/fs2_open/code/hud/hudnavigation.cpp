// HUDNavigation.cpp
// Derek Meek
// 4-30-2004

/*
 * $Logfile: /Freespace2/code/Hud/HUDNavigation.cpp $
 * $Revision: 1.9 $
 * $Date: 2004-11-27 10:51:01 $
 * $Author: taylor $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.8  2004/08/20 05:13:07  Kazan
 * wakka wakka - fix minor booboo
 *
 * Revision 1.7  2004/07/27 18:52:10  Kazan
 * squished another
 *
 * Revision 1.6  2004/07/26 20:47:32  Kazan
 * remove MCD complete
 *
 * Revision 1.5  2004/07/26 19:39:49  Kazan
 * add conditional compilation directive
 *
 * Revision 1.4  2004/07/26 17:54:04  Kazan
 * Autopilot system completed -- i am dropping plans for GUI nav map
 * All builds should have ENABLE_AUTO_PILOT defined from now on (.dsp's i am committing reflect this) the system will only be noticed if the mission designer brings it online by defining a nav point
 * Fixed FPS counter during time compression
 *
 * Revision 1.3  2004/07/12 16:32:49  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 1.2  2004/07/01 16:38:19  Kazan
 * working on autonav
 *
 * Revision 1.1  2004/05/24 07:23:09  taylor
 * filename case change
 *
 * Revision 1.1  2004/05/07 23:50:14  Kazan
 * Sorry Guys!
 *
 *
 *
 *
 */

#include "PreProcDefines.h"

#if defined(ENABLE_AUTO_PILOT)
#include "hud/hudnavigation.h"
#include "autopilot/autopilot.h"
#include "hud/hudtarget.h"
#include "ship/ship.h"
#include "object/object.h"
#include "render/3d.h"
#include "hud/hud.h"
#include "hud/hudbrackets.h"
#include "hud/hudtargetbox.h"



extern int sexp_distance2(int obj1, char *subj);
extern void hud_target_show_dist_on_bracket(int x, int y, float distance);
extern void draw_brackets_square(int x1, int y1, int x2, int y2);
extern void draw_brackets_square_quick(int x1, int y1, int x2, int y2, int thick);
extern float Cmdline_fov;
// Draws the Navigation stuff on the HUD
void HUD_Draw_Navigation()
{
	if (CurrentNav != -1 && Navs[CurrentNav].flags & NP_VALIDTYPE && !(Navs[CurrentNav].flags & NP_NOSELECT))
	{
		
		// ----------------- debug stuff -----------------
		/*char *name = Navs[CurrentNav].GetInteralName();
		int dista = sexp_distance2(Player_ship->objnum, name);
		delete[] name;


		char diststr[128];
		sprintf(diststr, "Range to Nav: %d", dista);
		gr_string( 20, 130, diststr);
		gr_string( 20, 120, Navs[CurrentNav].NavName);*/
		// ------------------------------------------------

		int in_cockpit;
		if (!(Viewer_mode & (VM_EXTERNAL | VM_SLEWED |/* VM_CHASE |*/ VM_DEAD_VIEW | VM_WARP_CHASE | VM_PADLOCK_ANY))) 
			in_cockpit = 1;
		else  
			in_cockpit = 0;

		vertex target_point;					// temp vertex used to find screen position for 3-D object;
		vector target_pos;

		//Players[Player_num].lead_indicator_active = 0;


		target_pos = Navs[CurrentNav].GetPosition();//get it's position
		float dist = vm_vec_dist_quick(&Objects[Player_ship->objnum].pos,&target_pos);

		// find the current target vertex 
		//
		// The 2D screen pos depends on the current viewer position and orientation.  

		
		color NavColor;

		unsigned int alpha = HUD_COLOR_ALPHA_MAX * 16;

		//if (CanAutopilot())
		//	alpha = HUD_COLOR_ALPHA_MAX * 16;

		if (Navs[CurrentNav].flags & NP_VISITED)
			gr_init_alphacolor( &NavColor,	0xFF, 0xFF, 0x00, alpha);
		else
			gr_init_alphacolor( &NavColor,	0x80, 0x80, 0xff, alpha);

		gr_set_color_fast(&NavColor);
		


		g3_rotate_vertex(&target_point,&target_pos);
		g3_project_vertex(&target_point);


		int box_scale = 15;
		if (dist < 1000)
			box_scale = int(float(dist)/1000.0f * 15);
		if (box_scale < 4)
			box_scale=4;

		//SAFEPOINT(s)
		if (!(target_point.flags & PF_OVERFLOW) && target_point.codes == 0)
		{  // make sure point projected and target center is not on screen
			//hud_show_brackets(targetp, &target_point);

			int x = int(target_point.sx);
			int y = int(target_point.sy);
			draw_brackets_square(x-box_scale, y-box_scale, 
							     x+box_scale, y+box_scale);

			gr_set_color_fast(&NavColor);
			// draw the nav name
			gr_string(x-(box_scale+10), y-(box_scale+20), Navs[CurrentNav].NavName);
			// draw distance to target in lower right corner of box
			hud_target_show_dist_on_bracket(x+(box_scale+10),y+(box_scale+10),dist);

		}


		if ( in_cockpit && target_point.codes != 0) {
			gr_set_color_fast(&NavColor);
			hud_draw_offscreen_indicator(&target_point, &target_pos, dist);	
		}
	}
	/*
	else
	{
		gr_string( 20, 120, "No Nav Point Selected");
	}*/
}
#endif //ENABLE_AUTO_PILOT


