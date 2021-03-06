/*
 * lab.cpp
 * created by WMCoolmon
 *
 * You may not sell or otherwise commercially exploit the source or things you
 * create based on the source.
 *
 */

/*
 * $Logfile: /Freespace2/code/lab/lab.cpp $
 * $Revision: 1.28.2.9 $
 * $Date: 2007-07-15 02:45:49 $
 * $Author: Goober5000 $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.28.2.8  2007/02/27 01:44:44  Goober5000
 * add two features for WCS: specifyable shield/weapon recharge rates, and removal of linked fire penalty
 *
 * Revision 1.28.2.7  2007/02/12 02:19:45  taylor
 * fix a couple of things that I missed earlier
 *
 * Revision 1.28.2.6  2007/02/11 06:19:07  Goober5000
 * invert the do-collision flag into a don't-do-collision flag, plus fixed a wee lab bug
 *
 * Revision 1.28.2.5  2006/10/27 06:35:29  taylor
 * add "Render full detail" support to the lab as well
 *
 * Revision 1.28.2.4  2006/08/27 18:12:41  taylor
 * make Species_info[] and Asteroid_info[] dynamic
 *
 * Revision 1.28.2.3  2006/08/19 04:26:32  taylor
 * add render option for no glowmaps
 * remove render option for fog (why was this even there??)
 * add tech model view for missiles with special tech models (will hopefully help spur some work towards fixing the currently broken models)
 * handle Z-buf issue that made the lab interface disappear when the transparent render option was ticked
 *
 * Revision 1.28.2.2  2006/07/08 11:27:28  taylor
 * hopefully this should get missile viewing working decently again
 *
 * Revision 1.28.2.1  2006/07/06 21:49:12  taylor
 * add a render option to make the model not spin around, easier to work with in some cases, and better screenshots
 *
 * Revision 1.28  2006/05/13 07:09:25  taylor
 * minor cleanup and a couple extra error checks
 * get rid of some wasteful math from the gr_set_proj_matrix() calls
 *
 * Revision 1.27  2006/04/20 06:32:07  Goober5000
 * proper capitalization according to Volition
 *
 * Revision 1.26  2006/01/18 16:17:33  taylor
 * use HTL wireframe view if in HTL mode
 *
 * Revision 1.25  2005/12/29 08:08:36  wmcoolmon
 * Codebase commit, most notably including objecttypes.tbl
 *
 * Revision 1.24  2005/12/04 19:12:53  wmcoolmon
 * Post-final commit
 *
 * Revision 1.22  2005/10/30 06:44:57  wmcoolmon
 * Codebase commit - nebula.tbl, scripting, new dinky explosion/shockwave stuff, moving muzzle flashes
 *
 * Revision 1.21  2005/10/17 01:54:15  wmcoolmon
 * Make weapons clip less
 *
 * Revision 1.20  2005/10/17 01:51:01  wmcoolmon
 * Weapon models now shown in lab
 *
 * Revision 1.19  2005/10/09 09:18:10  wmcoolmon
 * Added warpin/warpout speed values to the lab
 *
 * Revision 1.18  2005/10/09 00:43:08  wmcoolmon
 * Extendable modular tables (XMTs); added weapon dialogs to the Lab
 *
 * Revision 1.17  2005/09/25 07:27:33  Goober5000
 * and again
 * --Goober5000
 *
 * Revision 1.16  2005/09/25 07:26:05  Goober5000
 * try to fix the headers
 * --Goober5000
 *
 */

#include "wmcgui.h"
#include "gamesequence/gamesequence.h"
#include "io/key.h"
#include "freespace2/freespace.h"

#include "cmdline/cmdline.h"
#include "ship/ship.h"
#include "weapon/weapon.h"
#include "render/3d.h"
#include "lighting/lighting.h"
#include "model/model.h"
#include "missionui/missionscreencommon.h"
#include "weapon/beam.h"
#include "mission/missionparse.h"
#include "species_defs/species_defs.h"

//All sorts of globals

// lab flags
#define LAB_NORMAL			(0)		// default
#define LAB_NO_ROTATION		(1<<0)	// don't rotate models

static GUIScreen *Lab_screen = NULL;

static int Selected_weapon_index = -1;

static int LabViewerShipIndex = -1;
static int LabViewerModelNum = -1;
static int LabViewerModelLOD = 0;
static int ModelFlags = MR_LOCK_DETAIL | MR_AUTOCENTER | MR_NO_FOGGING;
static int LabViewerFlags = LAB_NORMAL;
void change_ship_lod(Tree* caller);

//*****************************Ship Options Window*******************************
static Window* ShipOptWin = NULL;

#define NUM_SHIP_FLAGS	50
Checkbox *soc[NUM_SHIP_FLAGS];

void set_ship_flags_ship(ship_info *sip)
{
	unsigned int i=0;
	soc[i++]->SetFlag(&sip->flags, SIF_SUPPORT);
	soc[i++]->SetFlag(&sip->flags, SIF_CARGO);
	soc[i++]->SetFlag(&sip->flags, SIF_FIGHTER);
	soc[i++]->SetFlag(&sip->flags, SIF_BOMBER);
	soc[i++]->SetFlag(&sip->flags, SIF_CRUISER);
	soc[i++]->SetFlag(&sip->flags, SIF_CORVETTE);
	soc[i++]->SetFlag(&sip->flags, SIF_FREIGHTER);
	soc[i++]->SetFlag(&sip->flags, SIF_CAPITAL);
	soc[i++]->SetFlag(&sip->flags, SIF_TRANSPORT);
	soc[i++]->SetFlag(&sip->flags, SIF_NAVBUOY);
	soc[i++]->SetFlag(&sip->flags, SIF_SENTRYGUN);
	soc[i++]->SetFlag(&sip->flags, SIF_ESCAPEPOD);
	soc[i++]->SetFlag(&sip->flags, SIF_GAS_MINER);
	soc[i++]->SetFlag(&sip->flags, SIF_AWACS);
	soc[i++]->SetFlag(&sip->flags, SIF_STEALTH);
	soc[i++]->SetFlag(&sip->flags, SIF_SUPERCAP);
	soc[i++]->SetFlag(&sip->flags, SIF_KNOSSOS_DEVICE);
	soc[i++]->SetFlag(&sip->flags, SIF_DRYDOCK);
	soc[i++]->SetFlag(&sip->flags, SIF_SHIP_COPY);
	soc[i++]->SetFlag(&sip->flags, SIF_BIG_DAMAGE);
	soc[i++]->SetFlag(&sip->flags, SIF_HAS_AWACS);
	soc[i++]->SetFlag(&sip->flags, SIF_SHIP_CLASS_DONT_COLLIDE_INVIS);
	soc[i++]->SetFlag(&sip->flags, SIF_NO_COLLIDE);
	soc[i++]->SetFlag(&sip->flags, SIF_PLAYER_SHIP);
	soc[i++]->SetFlag(&sip->flags, SIF_DEFAULT_PLAYER_SHIP);
	soc[i++]->SetFlag(&sip->flags, SIF_BALLISTIC_PRIMARIES);
	soc[i++]->SetFlag(&sip->flags2, SIF2_FLASH);
	soc[i++]->SetFlag(&sip->flags2, SIF2_SURFACE_SHIELDS);
	soc[i++]->SetFlag(&sip->flags2, SIF2_SHOW_SHIP_MODEL);
	soc[i++]->SetFlag(&sip->flags, SIF_IN_TECH_DATABASE);
	soc[i++]->SetFlag(&sip->flags, SIF_IN_TECH_DATABASE_M);

	//ShipOptWin->SetCaption(sip->name);
}

void zero_ship_flags_win(GUIObject *caller)
{
	ShipOptWin = NULL;
}

void ship_flags_window(Button *caller)
{
	if(ShipOptWin != NULL)
		return;

	GUIObject* cwp = Lab_screen->Add(new Window("Ship options", gr_screen.max_w - 200, 200));
	ShipOptWin = (Window*)cwp;
	unsigned int i=0;
	int y = 0;

	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("SUPPORT", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("CARGO", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("FIGHTER", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("BOMBER", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("CRUISER", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("CORVETTE", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("FREIGHTER", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("CAPITAL", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("TRANSPORT", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("NAVBUOY", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("SENTRYGUN", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("ESCAPEPOD", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("GAS MINER", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("AWACS", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("STEALTH", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("SUPERCAP", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("KNOSSOS DEVICE", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("DRYDOCK", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("SHIP COPY", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("BIG DAMAGE", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("HAS AWACS", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("NO COLLIDE INVISIBLE", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("NO COLLIDE", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("PLAYER SHIP", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("DEFAULT PLAYER SHIP", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("BALLISTIC PRIMARIES", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("FLASH", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("SURFACE SHIELDS", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("SHOW SHIP MODEL", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("IN TECH DATABASE", 0, y));
	y += soc[i++]->GetHeight() + 10;
	soc[i] = (Checkbox*) cwp->AddChild(new Checkbox("IN TECH DATABASE MULTI", 0, y));

	if(LabViewerShipIndex != -1)
		set_ship_flags_ship(&Ship_info[LabViewerShipIndex]);

	cwp->SetCloseFunction(zero_ship_flags_win);
}
//*****************************Ship Variables Window*******************************
static Window* ShipVarWin = NULL;

#define NUM_SHIP_VARIABLES		50
Text *svt[NUM_SHIP_VARIABLES];

#define SVW_SET_SI_VAR(var)	svt[i]->SetText(sip->var);	\
	svt[i++]->SetSaveLoc(&sip->var, T_ST_ONENTER)

#define SVW_SET_VAR(var)	svt[i]->SetText(var);	\
	svt[i++]->SetSaveLoc(&var, T_ST_ONENTER)

extern int Hud_shield_filename_count;

void set_ship_variables_ship(ship_info *sip)
{
	unsigned int i=0;
	svt[i++]->SetText(sip->name);
	svt[i]->SetText(sip->species);
	svt[i++]->SetSaveLoc(&sip->species, T_ST_ONENTER, Species_info.size()-1, 0);
	svt[i]->SetText(sip->class_type);
	svt[i++]->SetSaveLoc(&sip->class_type, T_ST_ONENTER, Ship_types.size()-1, 0);

	SVW_SET_SI_VAR(density);
	SVW_SET_SI_VAR(damp);
	SVW_SET_SI_VAR(rotdamp);
	SVW_SET_SI_VAR(max_vel.xyz.x);
	SVW_SET_SI_VAR(max_vel.xyz.y);
	SVW_SET_SI_VAR(max_vel.xyz.z);
	SVW_SET_SI_VAR(warpin_speed);
	SVW_SET_SI_VAR(warpout_speed);

	SVW_SET_SI_VAR(max_shield_strength);
	SVW_SET_SI_VAR(max_hull_strength);
	SVW_SET_SI_VAR(subsys_repair_rate);
	SVW_SET_SI_VAR(hull_repair_rate);
	SVW_SET_SI_VAR(cmeasure_max);
	svt[i]->SetText(sip->shield_icon_index);
	svt[i++]->SetSaveLoc(&sip->shield_icon_index, T_ST_ONENTER, Hud_shield_filename_count-1, 0);

	SVW_SET_SI_VAR(power_output);
	SVW_SET_SI_VAR(max_overclocked_speed);
	SVW_SET_SI_VAR(max_weapon_reserve);
	SVW_SET_SI_VAR(max_shield_regen_per_second);
	SVW_SET_SI_VAR(max_weapon_regen_per_second);

	SVW_SET_SI_VAR(afterburner_fuel_capacity);
	SVW_SET_SI_VAR(afterburner_burn_rate);
	SVW_SET_SI_VAR(afterburner_recover_rate);

	SVW_SET_SI_VAR(shockwave.inner_rad);
	SVW_SET_SI_VAR(shockwave.outer_rad);
	SVW_SET_SI_VAR(shockwave.damage);
	SVW_SET_SI_VAR(shockwave.blast);
	SVW_SET_SI_VAR(explosion_propagates);
	SVW_SET_SI_VAR(shockwave.speed);
	SVW_SET_SI_VAR(shockwave_count);

	SVW_SET_SI_VAR(closeup_zoom);
	SVW_SET_SI_VAR(closeup_pos.xyz.x);
	SVW_SET_SI_VAR(closeup_pos.xyz.y);
	SVW_SET_SI_VAR(closeup_pos.xyz.z);

	//Test
	/*
	SVW_SET_VAR(Objects[Ships[0].objnum].pos.xyz.x);
	SVW_SET_VAR(Objects[Ships[0].objnum].pos.xyz.y);
	SVW_SET_VAR(Objects[Ships[0].objnum].pos.xyz.z);

	SVW_SET_VAR(Objects[Ships[0].objnum].phys_info.vel.xyz.x);
	SVW_SET_VAR(Objects[Ships[0].objnum].phys_info.vel.xyz.y);
	SVW_SET_VAR(Objects[Ships[0].objnum].phys_info.vel.xyz.z);*/
}

#define SVW_LEFTWIDTH		150
#define SVW_RIGHTWIDTH		100
#define SVW_RIGHTX			160

void zero_ship_var_win(GUIObject *caller)
{
	ShipVarWin = NULL;
}
#define SVW_ADD_TEXT_HEADER(name) y += (cwp->AddChild(new Text(name, name, SVW_RIGHTX/2, y + 10, SVW_RIGHTWIDTH))->GetHeight() + 15)

#define SVW_ADD_TEXT(name) cwp->AddChild(new Text(name, name, 0, y, SVW_LEFTWIDTH));			\
	svt[i] = (Text*) cwp->AddChild(new Text(std::string (name) + std::string("Editbox"), "", SVW_RIGHTX, y, SVW_RIGHTWIDTH, 12, T_EDITTABLE));	\
	y += svt[i++]->GetHeight() + 5														\

void ship_variables_window(Button *caller)
{
	if(ShipVarWin != NULL)
		return;

	GUIObject* cwp = Lab_screen->Add(new Window("Ship variables", gr_screen.max_w - (SVW_RIGHTX + SVW_RIGHTWIDTH + 25), 200));
	ShipVarWin = (Window*)cwp;
	unsigned int i = 0;
	int y = 0;

	cwp->AddChild(new Text("Name", "", y, SVW_LEFTWIDTH));
	svt[i] = (Text*) cwp->AddChild(new Text("Ship name", "<None>", SVW_RIGHTX, y, SVW_RIGHTWIDTH, 12));
	y += svt[i++]->GetHeight() + 5;
	SVW_ADD_TEXT("Species");
	SVW_ADD_TEXT("Type");

	//Physics
	SVW_ADD_TEXT_HEADER("Physics");
	SVW_ADD_TEXT("Density");
	SVW_ADD_TEXT("Damp");
	SVW_ADD_TEXT("Rotdamp");
	SVW_ADD_TEXT("Max vel (x)");
	SVW_ADD_TEXT("Max vel (y)");
	SVW_ADD_TEXT("Max vel (z)");
	SVW_ADD_TEXT("Warp in speed");
	SVW_ADD_TEXT("Warp out speed");

	//Other
	SVW_ADD_TEXT_HEADER("Stats");
	SVW_ADD_TEXT("Shields");
	SVW_ADD_TEXT("Hull");
	SVW_ADD_TEXT("Subsys repair rate");
	SVW_ADD_TEXT("Hull repair rate");
	SVW_ADD_TEXT("Countermeasures");
	SVW_ADD_TEXT("HUD Icon");

	SVW_ADD_TEXT_HEADER("Power");
	SVW_ADD_TEXT("Power output");
	SVW_ADD_TEXT("Max oclk speed");
	SVW_ADD_TEXT("Max weapon reserve");
	SVW_ADD_TEXT("Shield regeneration rate");
	SVW_ADD_TEXT("Weapon regeneration rate");

	SVW_ADD_TEXT_HEADER("Afterburner");
	SVW_ADD_TEXT("Fuel");
	SVW_ADD_TEXT("Burn rate");
	SVW_ADD_TEXT("Recharge rate");

	SVW_ADD_TEXT_HEADER("Explosion");
	SVW_ADD_TEXT("Inner radius");
	SVW_ADD_TEXT("Outer radius");
	SVW_ADD_TEXT("Damage");
	SVW_ADD_TEXT("Blast");
	SVW_ADD_TEXT("Propagates");
	SVW_ADD_TEXT("Shockwave speed");
	SVW_ADD_TEXT("Shockwave count");

	//Techroom
	SVW_ADD_TEXT_HEADER("Techroom");
	SVW_ADD_TEXT("Closeup zoom");
	SVW_ADD_TEXT("Closeup pos (x)");
	SVW_ADD_TEXT("Closeup pos (y)");
	SVW_ADD_TEXT("Closeup pos (z)");

	//Test
	/*
	SVW_ADD_TEXT_HEADER(Ships[0].ship_name);
	SVW_ADD_TEXT("Mission pos (x)");
	SVW_ADD_TEXT("Mission pos (y)");
	SVW_ADD_TEXT("Mission pos (z)");
	SVW_ADD_TEXT("Mission vel (x)");
	SVW_ADD_TEXT("Mission vel (y)");
	SVW_ADD_TEXT("Mission vel (z)");
	*/

	if(LabViewerShipIndex != -1)
		set_ship_variables_ship(&Ship_info[LabViewerShipIndex]);

	ShipVarWin->SetCloseFunction(zero_ship_var_win);
}


//*****************************Options Window*******************************
static Window* RenderOptWin = NULL;
void zero_render_opt_win(GUIObject *caller)
{
	RenderOptWin = NULL;
}

void make_options_window(Button *caller)
{
	if(RenderOptWin != NULL)
		return;

	GUIObject* ccp;
	RenderOptWin = (Window*)Lab_screen->Add(new Window("Options", gr_screen.max_w - 300, 200));
	int y = 0;

	ccp = RenderOptWin->AddChild(new Checkbox("No Glowmap", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_NO_GLOWMAPS);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("No rotation", 0, y));
	((Checkbox*)ccp)->SetFlag(&LabViewerFlags, LAB_NO_ROTATION);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("No lighting", 0,  y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_NO_LIGHTING);

	/*y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("No texturing", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_NO_TEXTURING);*/

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("No smoothing", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_NO_SMOOTHING);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("No Z-buffer", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_NO_ZBUFFER);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("No culling", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_NO_CULL);

	/*y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("No Fogging", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_NO_FOGGING);*/

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Wireframe", 0, y));
	if (!Cmdline_nohtl)
		((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_OUTLINE_HTL | MR_NO_POLYS);
	else
		((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_OUTLINE | MR_NO_POLYS);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Transparent", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_ALL_XPARENT);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Away norms transparent", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_EDGE_ALPHA);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Toward norms transparent", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_CENTER_ALPHA);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show pivots", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_PIVOTS);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show paths", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_PATHS);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show bay paths", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_BAY_PATHS);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show radius", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_RADIUS);

	/*y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show damage", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_DAMAGE);*/

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show shields", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_SHIELDS);

	/*y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show thrusters", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_THRUSTERS);*/

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Show invisible faces", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_SHOW_INVISIBLE_FACES);

	y += ccp->GetHeight() + 10;
	ccp = RenderOptWin->AddChild(new Checkbox("Render full detail", 0, y));
	((Checkbox*)ccp)->SetFlag(&ModelFlags, MR_FULL_DETAIL);

	RenderOptWin->SetCloseFunction(zero_render_opt_win);
}

//*****************************Shiplist Window*******************************
static Window* ShipClassWin = NULL;
void zero_ship_class_win(GUIObject *caller)
{
	ShipClassWin = NULL;
}
void ships_make_window(Button* caller)
{

	if(ShipClassWin != NULL)
		return;

	//GUIObject* cgp;
	ShipClassWin = (Window*)Lab_screen->Add(new Window("Ship classes", 50, 50));

	Tree* cmp = (Tree*)ShipClassWin->AddChild(new Tree("Ship tree", 0, 0));
	TreeItem *ctip, *stip;
	TreeItem **species_nodes = new TreeItem*[Species_info.size()+1];
	int i,j;

	//Add species nodes
	for(i = 0; i < (int)Species_info.size(); i++)
	{
		species_nodes[i] = cmp->AddItem(NULL, Species_info[i].species_name, 0, false);
	}
	//Just in case. I don't actually think this is possible though.
	species_nodes[Species_info.size()] = cmp->AddItem(NULL, "Other", 0, false);

	//Now add the ships
	std::string lod_name;
	char buf[8];
	for(i = 0; i < Num_ship_classes; i++)
	{
		if(Ship_info[i].species >= 0 && Ship_info[i].species < (int)Species_info.size())
			stip = species_nodes[Ship_info[i].species];
		else
			stip = species_nodes[Species_info.size()];

		ctip = cmp->AddItem(stip, Ship_info[i].name, i, false);
		for(j = 0; j < Ship_info[i].num_detail_levels; j++)
		{
			sprintf(buf, "%d", j);
			lod_name = "LOD ";
			lod_name += buf;

			cmp->AddItem(ctip, lod_name, j, false, change_ship_lod);
		}
	}

	//Get rid of any empty nodes
	//No the <= is not a mistake :)
	for(i = 0; i <= (int)Species_info.size(); i++)
	{
		if(!species_nodes[i]->HasChildren())
		{
			delete species_nodes[i];
		}
	}

	ShipClassWin->SetCloseFunction(zero_ship_class_win);
}

//*****************************Description Window*******************************
static Window* DescWin = NULL;
static Text* DescText = NULL;

void zero_descwin(GUIObject* caller)
{
	DescWin = NULL;
	DescText = NULL;
}

void make_another_window(Button* caller)
{
	if(DescWin != NULL)
		return;

	DescWin = (Window*)Lab_screen->Add(new Window("Description", gr_screen.max_w - gr_screen.max_w/3 - 50, gr_screen.max_h - gr_screen.max_h/6 - 50, gr_screen.max_w/3, gr_screen.max_h/6));
	DescText = (Text*)DescWin->AddChild(new Text("Description Text", "No ship selected.", 0, 0));
	if(LabViewerShipIndex != -1)
	{
		DescText->SetText(Ship_info[LabViewerShipIndex].tech_desc);
		DescWin->SetCaption(Ship_info[LabViewerShipIndex].name);
		//DescText->SetSaveStringPtr(&Ship_info[LabViewerShipIndex].tech_desc, T_ST_ONENTER, T_ST_MALLOC);
	}
		
	DescText->SetCloseFunction(zero_descwin);
}
//*****************************Subsystem Window*******************************
//WMC - disabled because I don't feel like making it work right now
/*static Text *sst[NUM_SHIP_VARIABLES];
static int ssn = 0;

#define SSW_LEFTWIDTH		150
#define SSW_RIGHTWIDTH		100
#define SSW_RIGHTX			160

#define SSW_SET_MS_VAR(var)	sst[i]->SetText(msp->var);	\
	sst[i++]->SetSaveLoc(&msp->var, T_ST_ONENTER)

#define SSW_ADD_TEXT(name) cwp->AddChild(new Text(name, name, 0, y, SSW_LEFTWIDTH));			\
	sst[ssn] = (Text*) cwp->AddChild(new Text(std::string (name) + std::string("Editbox"), "", SSW_RIGHTX, y, SSW_RIGHTWIDTH, 12, T_EDITTABLE));	\
	y += sst[ssn++]->GetHeight() + 5

static Window*	ShipSubsysWin = NULL;
static Tree*	ShipSubsysTree = NULL;

void ship_zero_subsys(GUIObject* caller)
{
	ShipSubsysWin = NULL;
}

void ship_change_subsys(Tree *caller)
{
	ship_info *sip = &Ship_info[LabViewerShipIndex];
	model_subsystem *msp = &sip->subsystems[(int)(caller->GetSelectedItem()->GetData())];
	
	int i = 0;
	sst[i++]->SetText(std::string(msp->name));
	SSW_SET_MS_VAR(radius);
	SSW_SET_MS_VAR(turn_rate);
}

void ship_update_subsys_window(ship_info *sip)
{
	ShipSubsysTree->ClearItems();
	int i;

	//Add species nodes
	ShipSubsysTree->ClearItems();
	for(i = 0; i < sip->n_subsystems; i++)
	{
		ShipSubsysTree->AddItem(NULL, sip->subsystems[i].subobj_name, i, false, ship_change_subsys);
	}
}

void ship_subsys_window(Button* caller)
{
	if(ShipSubsysWin != NULL)
		return;

	Window * cwp = ShipSubsysWin = (Window*)Lab_screen->Add(new Window("Ship subsystems", 200, 200));
	ShipSubsysTree = (Tree*)ShipSubsysWin->AddChild(new Tree("Ship tree", 0, 0));
	
	int y = 200;
	cwp->AddChild(new Text("Name", "", y, SSW_LEFTWIDTH));
	sst[ssn] = (Text*) cwp->AddChild(new Text("Subsys name", "<None>", SSW_RIGHTX, y, SSW_RIGHTWIDTH, 12));
	y += sst[ssn++]->GetHeight() + 5;
	SSW_ADD_TEXT("Radius");
	SSW_ADD_TEXT("Turn rate");
	
	if(LabViewerShipIndex != -1)
		ship_update_subsys_window(&Ship_info[LabViewerShipIndex]);
		
	ShipSubsysWin->SetCloseFunction(ship_zero_subsys);
}
*/
//*****************************Ship display stuff*******************************

void labviewer_change_model(char* model_fname, int lod=0, int ship_index=-1)
{
	if(LabViewerModelNum != -1)
	{
		model_page_out_textures(LabViewerModelNum);
		model_unload(LabViewerModelNum);
	}
	LabViewerModelNum = model_load(model_fname, 0, NULL, 0);
	LabViewerModelLOD = lod;
	LabViewerShipIndex = ship_index;
}

void change_ship_lod(Tree* caller)
{
	int ship_index = (int)(caller->GetSelectedItem()->GetParentItem()->GetData());

	labviewer_change_model(Ship_info[ship_index].pof_file
		, caller->GetSelectedItem()->GetData()
		, ship_index);

	if(DescText != NULL && DescWin != NULL)
	{
		DescWin->SetCaption(Ship_info[LabViewerShipIndex].name);
		DescText->SetText(Ship_info[LabViewerShipIndex].tech_desc);
		//DescText->SetSaveStringPtr(&Ship_info[LabViewerShipIndex].tech_desc, T_ST_ONENTER, T_ST_MALLOC);
	}

	if(ShipOptWin != NULL)
	{
		set_ship_flags_ship(&Ship_info[LabViewerShipIndex]);
	}

	if(ShipVarWin != NULL)
	{
		set_ship_variables_ship(&Ship_info[LabViewerShipIndex]);
	}
	
	//WMC - disabled because I don't feel like making it work right now
	/*
	if(ShipSubsysWin != NULL)
	{
		ship_update_subsys_window(&Ship_info[LabViewerShipIndex]);
	}
	*/
}

//*****************************Weapon list*******************************
void change_weapon(Tree* caller);
static Window* WeapClassWin = NULL;
void zero_weap_class_win(GUIObject *caller)
{
	WeapClassWin = NULL;
}

void weap_show_tech_model(Tree* caller)
{
	int weap_index = (int)(caller->GetSelectedItem()->GetParentItem()->GetData());

	labviewer_change_model(Weapon_info[weap_index].tech_model, caller->GetSelectedItem()->GetData());
}

void weaps_make_window(Button* caller)
{

	if(WeapClassWin != NULL)
		return;

	//GUIObject* cgp;
	WeapClassWin = (Window*)Lab_screen->Add(new Window("Weapon classes", 50, 50));

	Tree* cmp = (Tree*)WeapClassWin->AddChild(new Tree("Weapon tree", 0, 0));
	
	TreeItem *cwip, *stip;
	//Unfortunately these are hardcoded
	TreeItem **type_nodes = new TreeItem*[Num_weapon_subtypes];
	int i;

	//Add type nodes
	for(i = 0; i < Num_weapon_subtypes; i++)
	{
		type_nodes[i] = cmp->AddItem(NULL, Weapon_subtype_names[i], 0, false);
	}

	//Now add the weapons
	for(i = 0; i < Num_weapon_types; i++)
	{
		if(Weapon_info[i].subtype == WP_UNUSED) {
			continue;
		} else if(Weapon_info[i].subtype >= Num_weapon_subtypes) {
			Warning(LOCATION, "Invalid weapon subtype found on weapon %s", Weapon_info[i].name);
			continue;
		}
		
		if(Weapon_info[i].wi_flags & WIF_BEAM)
			stip = type_nodes[WP_BEAM];
		else
			stip = type_nodes[Weapon_info[i].subtype];

		cwip = cmp->AddItem(stip, Weapon_info[i].name, i, false, change_weapon);

		if ( strlen(Weapon_info[i].tech_model) > 0 ) {
			cmp->AddItem(cwip, "Tech Model", 0, false, weap_show_tech_model);
		}
	}

	//Get rid of any empty nodes
	//No the <= is not a mistake :)
	for(i = 0; i < Num_weapon_subtypes; i++)
	{
		if(!type_nodes[i]->HasChildren())
		{
			delete type_nodes[i];
		}
	}

	WeapClassWin->SetCloseFunction(zero_weap_class_win);
}

//*****************************Weapon Variables Window*******************************
static Window* WeapVarWin = NULL;

#define NUM_WEAP_VARIABLES		50
Text *wvt[NUM_WEAP_VARIABLES];

#define WVW_SET_WI_VAR(var)	wvt[i]->SetText(wip->var);	\
	wvt[i++]->SetSaveLoc(&wip->var, T_ST_ONENTER)

#define WVW_SET_VAR(var)	wvt[i]->SetText(var);	\
	wvt[i++]->SetSaveLoc(&var, T_ST_ONENTER)

void weap_set_variables(weapon_info *wip)
{
	unsigned int i=0;
	wvt[i++]->SetText(std::string(wip->name));
	wvt[i]->SetText(wip->subtype);
	wvt[i++]->SetSaveLoc(&wip->subtype, T_ST_ONENTER, Num_weapon_subtypes-1, 0);
	
	WVW_SET_WI_VAR(mass);
	WVW_SET_WI_VAR(max_speed);
	WVW_SET_WI_VAR(lifetime);
	WVW_SET_WI_VAR(weapon_range);
	WVW_SET_WI_VAR(WeaponMinRange);
	
	WVW_SET_WI_VAR(fire_wait);
	WVW_SET_WI_VAR(damage);
	WVW_SET_WI_VAR(armor_factor);
	WVW_SET_WI_VAR(shield_factor);
	WVW_SET_WI_VAR(subsystem_factor);
	
	WVW_SET_WI_VAR(damage_type_idx);
	
	WVW_SET_WI_VAR(shockwave.speed);
	
	WVW_SET_WI_VAR(turn_time);
	WVW_SET_WI_VAR(fov);
	WVW_SET_WI_VAR(min_lock_time);
	WVW_SET_WI_VAR(lock_pixels_per_sec);
	WVW_SET_WI_VAR(catchup_pixels_per_sec);
	WVW_SET_WI_VAR(catchup_pixel_penalty);
	WVW_SET_WI_VAR(swarm_count);
	WVW_SET_WI_VAR(SwarmWait);
}

#define WVW_LEFTWIDTH		150
#define WVW_RIGHTWIDTH		100
#define WVW_RIGHTX			160

void zero_weap_var_win(GUIObject *caller)
{
	WeapVarWin = NULL;
}
#define WVW_ADD_TEXT_HEADER(name) y += (cwp->AddChild(new Text(name, name, WVW_RIGHTX/2, y + 10, WVW_RIGHTWIDTH))->GetHeight() + 15)

#define WVW_ADD_TEXT(name) cwp->AddChild(new Text(name, name, 0, y, SVW_LEFTWIDTH));			\
	wvt[i] = (Text*) cwp->AddChild(new Text(std::string (name) + std::string("Editbox"), "", WVW_RIGHTX, y, WVW_RIGHTWIDTH, 12, T_EDITTABLE));	\
	y += wvt[i++]->GetHeight() + 5														\

void weap_variables_window(Button *caller)
{
	if(WeapVarWin != NULL)
		return;

	GUIObject* cwp = Lab_screen->Add(new Window("Weapon variables", gr_screen.max_w - (WVW_RIGHTX + WVW_RIGHTWIDTH + 25), 200));
	WeapVarWin = (Window*)cwp;
	unsigned int i = 0;
	int y = 0;

	cwp->AddChild(new Text("Name", "", y, WVW_LEFTWIDTH));
	wvt[i] = (Text*) cwp->AddChild(new Text("Weapon name", "<None>", WVW_RIGHTX, y, WVW_RIGHTWIDTH, 12));
	y += wvt[i++]->GetHeight() + 5;
	WVW_ADD_TEXT("Subtype");

	//Physics
	WVW_ADD_TEXT_HEADER("Physics");
	WVW_ADD_TEXT("Mass");
	WVW_ADD_TEXT("Max speed");
	WVW_ADD_TEXT("Lifetime");
	WVW_ADD_TEXT("Range");
	WVW_ADD_TEXT("Min Range");
	
	WVW_ADD_TEXT_HEADER("Damage");
	WVW_ADD_TEXT("Fire wait");
	WVW_ADD_TEXT("Damage");
	WVW_ADD_TEXT("Armor factor");
	WVW_ADD_TEXT("Shield factor");
	WVW_ADD_TEXT("Subsys factor");
	
	WVW_ADD_TEXT_HEADER("Armor");
	WVW_ADD_TEXT("Damage type");
	
	WVW_ADD_TEXT_HEADER("Shockwave");
	WVW_ADD_TEXT("Speed");
	
	WVW_ADD_TEXT_HEADER("Missiles");
	WVW_ADD_TEXT("Turn time");
	WVW_ADD_TEXT("FOV");
	WVW_ADD_TEXT("Min locktime");
	WVW_ADD_TEXT("Pixels/sec");
	WVW_ADD_TEXT("Catchup pixels/sec");
	WVW_ADD_TEXT("Catchup pixel pen.");
	WVW_ADD_TEXT("Swarm count");
	WVW_ADD_TEXT("Swarm wait");
	

	if(Selected_weapon_index != -1)
		weap_set_variables(&Weapon_info[Selected_weapon_index]);

	WeapVarWin->SetCloseFunction(zero_weap_var_win);
}

//*****************************Weapon thingy*******************************
void change_weapon(Tree* caller)
{
	Selected_weapon_index = (int)(caller->GetSelectedItem()->GetData());

	if(Weapon_info[Selected_weapon_index].render_type == WRT_POF)
	{
		labviewer_change_model(Weapon_info[Selected_weapon_index].pofbitmap_name);
	}

	if(WeapVarWin != NULL)
	{
		weap_set_variables(&Weapon_info[Selected_weapon_index]);
	}
}

//************************Ship renderer****************************
static int Trackball_mode = 1;
static int Trackball_active = 0;
void show_ship(float frametime)
{
	static float LabViewerShipRot = 0.0f;
	static matrix LabViewerOrient = IDENTITY_MATRIX;

	if(LabViewerModelNum == -1) {
		int w, h;
		gr_get_string_size(&w, &h, "Viewer off");
		gr_string(gr_screen.clip_right - w, gr_screen.clip_bottom - h, "Viewer off", false);
		return;
	}

	/*if(sip == NULL)
	{
		draw_model_icon(LabViewerModelNum, ModelFlags, .5f / 1.25f, 0, 0, gr_screen.clip_right, gr_screen.clip_bottom);
		return;
	}*/

	float rev_rate;
	angles rot_angles, view_angles;
	int z;
	ship_info *sip = NULL;
	if(LabViewerShipIndex > -1) {
		sip = &Ship_info[LabViewerShipIndex];
	}

	// get correct revolution rate

	rev_rate = REVOLUTION_RATE;
	if(sip != NULL)
	{
		z = sip->flags;
		if (z & SIF_BIG_SHIP) {
			rev_rate *= 1.7f;
		}
		if (z & SIF_HUGE_SHIP) {
			rev_rate *= 3.0f;
		}
	}

	// rotate the ship as much as required for this frame
	if (Trackball_active) {
		int dx, dy;
		matrix mat1, mat2;

			mouse_get_delta(&dx, &dy);
			if (dx || dy) {
				vm_trackball(-dx, -dy, &mat1);
				vm_matrix_x_matrix(&mat2, &mat1, &LabViewerOrient);
				LabViewerOrient = mat2;
			}
	}
	else if ( !(LabViewerFlags & LAB_NO_ROTATION) )
	{
		LabViewerShipRot += PI2 * frametime / rev_rate;
		while (LabViewerShipRot > PI2){
			LabViewerShipRot -= PI2;	
		}

		// setup stuff needed to render the ship
		view_angles.p = -0.6f;
		view_angles.b = 0.0f;
		view_angles.h = 0.0f;
		vm_angles_2_matrix(&LabViewerOrient, &view_angles);

		rot_angles.p = 0.0f;
		rot_angles.b = 0.0f;
		rot_angles.h = LabViewerShipRot;
		vm_rotate_matrix_by_angles(&LabViewerOrient, &rot_angles);
	}

//	gr_set_clip(Tech_ship_display_coords[gr_screen.res][0], Tech_ship_display_coords[gr_screen.res][1], Tech_ship_display_coords[gr_screen.res][2], Tech_ship_display_coords[gr_screen.res][3]);	
	//gr_set_clip(Ship_anim_coords[gr_screen.res][0], Ship_anim_coords[gr_screen.res][1], Tech_ship_display_coords[gr_screen.res][2], Tech_ship_display_coords[gr_screen.res][3]);		

	// render the ship
	g3_start_frame(1);
	if(sip != NULL) {
		g3_set_view_matrix(&sip->closeup_pos, &vmd_identity_matrix, sip->closeup_zoom * 1.3f);
	}
	else
	{
		//FIND THE LARGEST RADIUS
		polymodel *pm = model_get(LabViewerModelNum);
		bsp_info *bs = NULL;	//tehe
		float largest_radius = 0.0f;
		Assert( pm != NULL );
		for(int i = 0; i < pm->n_models; i++)
		{
			if(!pm->submodel[i].is_thruster)
			{
				bs = &pm->submodel[i];
				if(bs->rad > largest_radius)
					largest_radius = bs->rad;
			}
		}

		//MAKE A VECTOR FROM IT
		vec3d closeup_pos;
		closeup_pos.xyz.x = 0.0f;
		closeup_pos.xyz.y = 0.0f;
		closeup_pos.xyz.z = -(largest_radius * 2.0f);

		g3_set_view_matrix(&closeup_pos, &vmd_identity_matrix, 1.2f);
	}

	if (!Cmdline_nohtl) {
		gr_set_proj_matrix(Proj_fov, gr_screen.clip_aspect, 1.0f, Max_draw_distance);
		gr_set_view_matrix(&Eye_position, &Eye_matrix);
	}

	// lighting for techroom
	light_reset();
	vec3d light_dir = vmd_zero_vector;
	light_dir.xyz.y = 1.0f;	
	light_add_directional(&light_dir, 0.65f, 1.0f, 1.0f, 1.0f);
	// light_filter_reset();
	light_rotate_all();
	// lighting for techroom

	model_set_outline_color(255, 255, 255);
	model_clear_instance(LabViewerModelNum);
	model_set_detail_level(LabViewerModelLOD);
	model_render(LabViewerModelNum, &LabViewerOrient, &vmd_zero_vector, ModelFlags);

	if (!Cmdline_nohtl) 
	{
		gr_end_view_matrix();
		gr_end_proj_matrix();
	}

	g3_end_frame();
	//gr_reset_clip();
}

void get_out_of_lab(Button* caller)
{
	gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
}

//***************************************
static bool Lab_in_mission;

void lab_init()
{
	if(gameseq_get_pushed_state() == GS_STATE_GAME_PLAY)
		Lab_in_mission = true;
	else
		Lab_in_mission = false;

	beam_pause_sounds();

	//If we were viewing a ship, load 'er up
	if(LabViewerShipIndex != -1)
	{
		LabViewerModelNum = model_load(Ship_info[LabViewerShipIndex].pof_file, 0, NULL);
	}

	//If you want things to stay as you left them, uncomment this and "delete Lab_screen"
	//of course, you still need to delete it somewhere else (ie when FreeSpace closes)
	/*
	if(Lab_screen != NULL)
	{
		GUI_system->PushScreen(Lab_screen);
		return;
	}
	*/

	//We start by creating the screen/toolbar
	Lab_screen = GUI_system.PushScreen(new GUIScreen("Lab"));
	GUIObject *cwp;
	GUIObject *cbp;
	cwp = Lab_screen->Add(new Window("Toolbar", 0, 0, -1, -1, WS_NOTITLEBAR | WS_NONMOVEABLE));
	int x = 0;
	cbp = cwp->AddChild(new Button("Ship classes", 0, 0, ships_make_window));
	x += cbp->GetWidth() + 10;
	cbp = cwp->AddChild(new Button("Class Description", x, 0, make_another_window));
	x += cbp->GetWidth() + 10;
	if(!Lab_in_mission)
	{
		cbp = cwp->AddChild(new Button("Render options", x, 0, make_options_window));
		x += cbp->GetWidth() + 10;
	}
	cbp = cwp->AddChild(new Button("Class options", x, 0, ship_flags_window));
	x += cbp->GetWidth() + 10;
	cbp = cwp->AddChild(new Button("Class variables", x, 0, ship_variables_window));
	//WMC - disabled because I don't feel like making it work right now
	/*
	x += cbp->GetWidth() + 10;
	cbp = cwp->AddChild(new Button("Class subsystems", x, 0, ship_subsys_window));
	*/
	x += cbp->GetWidth() + 10;
	cbp = cwp->AddChild(new Button("Exit", x, 0, get_out_of_lab));
	
	cwp = Lab_screen->Add(new Window("Weapons toolbar", 0, cwp->GetHeight(), -1, -1, WS_NOTITLEBAR | WS_NONMOVEABLE));
	x = 0;
	cbp = cwp->AddChild(new Button("Weapon classes", 0, 0, weaps_make_window));
	x += cbp->GetWidth() + 10;
	cbp = cwp->AddChild(new Button("Weapon variables", x, 0, weap_variables_window));
}

extern void game_render_frame_setup(vec3d *eye_pos, matrix *eye_orient);
extern void game_render_frame(vec3d *eye_pos, matrix *eye_orient);
void lab_do_frame(float frametime)
{
	gr_clear();
	if(Lab_in_mission)
	{
		vec3d eye_pos;
		matrix eye_orient;
		game_render_frame_setup(&eye_pos, &eye_orient);
		game_render_frame( &eye_pos, &eye_orient );
	}
	else
	{
		show_ship(frametime);
	}
	if(GUI_system.OnFrame(frametime, Trackball_active==0 ? true : false, false) == GSOF_NOTHINGPRESSED && mouse_down(MOUSE_LEFT_BUTTON))
	{
		Trackball_active = 1;
		Trackball_mode = 1;
	}
	else
	{
		Trackball_active = 0;
	}
	gr_flip();
}

void lab_close()
{
	delete Lab_screen;

	if(LabViewerModelNum != -1)
	{
		model_page_out_textures(LabViewerModelNum);
		//model_unload(LabViewerModelNum);
	}
	LabViewerModelNum = -1;
	beam_unpause_sounds();
	//audiostream_unpause_all();
	game_flush();
}
