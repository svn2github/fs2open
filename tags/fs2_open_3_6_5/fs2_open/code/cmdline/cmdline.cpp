/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/Cmdline/cmdline.cpp $
 * $Revision: 2.88 $
 * $Date: 2004-12-24 19:36:12 $
 * $Author: Goober5000 $
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.87  2004/12/11 09:37:17  wmcoolmon
 * Apparently I never commited -mpnoreturn...
 *
 * Revision 2.86  2004/12/02 11:20:33  taylor
 * text of -no_set_gamma cmdline option shouldn't specify D3D since it works in OGL too
 *
 * Revision 2.85  2004/11/29 18:04:53  taylor
 * little reorg to flags so Experimental won't show up twice
 * add/update the wiki links since none of them worked before
 * remove all options that require an argument since they won't work
 *
 * Revision 2.84  2004/11/23 19:29:13  taylor
 * fix 2d warp in D3D, add cmdline option for 3d warp
 *
 * Revision 2.83  2004/09/10 13:51:45  et1
 * Command line option for TBP for warp stuff, "-tbpwarpeffects"
 *
 * Revision 2.82  2004/08/08 02:56:54  phreak
 * fixed the -orbradar launcher string
 *
 * Revision 2.81  2004/08/02 22:40:59  phreak
 * added -orbradar to enable orb rendering for the radar
 *
 * Revision 2.80  2004/08/01 02:31:18  phreak
 * -phreak command line changed to -dualscanlines
 *
 * Revision 2.79  2004/07/29 03:49:44  Kazan
 * fs2_open.exe was suffering the same problem as fred2 -- it was just a little less obvious
 *
 * Revision 2.78  2004/07/26 20:47:25  Kazan
 * remove MCD complete
 *
 * Revision 2.77  2004/07/25 18:46:28  Kazan
 * -fred_no_warn has become -no_warn and applies to both fred and fs2
 * added new ai directive (last commit) and disabled afterburners while performing AIM_WAYPOINTS or AIM_FLY_TO_SHIP
 * fixed player ship speed bug w/ player-use-ai, now stays in formation correctly and manages speed
 * made -radar_reduce ignore itself if no parameter is given (ignoring launcher bug)
 *
 * Revision 2.76  2004/07/12 16:32:42  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 2.75  2004/07/05 05:09:15  bobboau
 * FVF code, only the data that is needed is sent off to the card,,
 * OGL can take advantage of this if they want but it won't break
 * anything if they don't. also state block code has been implemented,
 * that's totaly internal to D3D no high level code is involved.
 *
 * Revision 2.74  2004/06/29 06:00:45  wmcoolmon
 * Added "-load_only_used", which makes FS2 load only used weapon data
 *
 * Revision 2.73  2004/06/26 00:28:05  wmcoolmon
 * Added "-targetinfo" to toggle info next to targeted object.
 *
 * Revision 2.72  2004/06/19 22:15:48  wmcoolmon
 * Added -nomotion debris command line option; also re-ordered categories.
 *
 * Revision 2.71  2004/06/06 12:25:19  randomtiger
 * Added new compression option -pcx32dds, build posted in RSB forum.
 * Changed flag because of launcher bug, have fixed launcher bug, will distribute later.
 * Also removed experimental flag from launcher flag list, stupid people were reporting bugs on unfinished code.
 *
 * Revision 2.70  2004/05/01 22:47:23  randomtiger
 * FS2_open will now take flag settings from cmdline_fso.cfg not cmdline.cfg.
 * This means retail FS2 will not crash because of fs2_open flags.
 * The new version of the launcher (v5) must used to setup flags now.
 * Anyone distributing pre 3.6 builds should make this clear and 3.6 should come with Launcher v5.
 *
 * Revision 2.69  2004/05/01 17:10:18  Kazan
 * Multiple -mod - "-mod ModA,ModB,ModC" in order of priority
 * Giving you:
 * Root: ModA
 * Root: ModB
 * Root: ModC
 * Root: Fs2Default
 * Root: CDRom
 *
 * Revision 2.68  2004/04/26 18:23:47  Kazan
 * -no_fps_capping
 *
 * Revision 2.67  2004/04/26 00:23:25  taylor
 * novbo and snd_preload cmdline options, fix moddir getting appended space
 *
 * Revision 2.66  2004/04/18 19:39:12  randomtiger
 * Added -2d_poof command which allows access to 2D poof rendering
 * Added -radar_reduce to launcher flag description structure
 *
 * Revision 2.65  2004/04/06 01:11:41  Kazan
 * make custom build work again
 *
 * Revision 2.64  2004/04/03 18:11:20  Kazan
 * FRED fixes
 *
 * Revision 2.63  2004/04/02 18:25:16  randomtiger
 * Changed D3D secondry texture system to be off by default to save mem.
 * Added some friendly names for parameters.
 *
 * Revision 2.62  2004/03/20 14:47:12  randomtiger
 * Added base for a general dynamic batching solution.
 * Fixed NO_DSHOW_CODE code path bug.
 *
 * Revision 2.61  2004/03/19 14:51:54  randomtiger
 * New command line parameter: -d3d_lesstmem causes D3D to bypass V's secondry texture system.
 *
 * Revision 2.60  2004/03/19 11:44:04  randomtiger
 * Removed -d3d_notmanaged param.
 * Tided D3D texture code. Merged remaining section code into the rest of the system.
 * Prepared for removal of code causing waste of memory for secondry store of textures.
 *
 * Revision 2.59  2004/03/16 18:37:01  randomtiger
 * Added new launcher flag construction code.
 *
 * Revision 2.58  2004/03/05 09:01:57  Goober5000
 * Uber pass at reducing #includes
 * --Goober5000
 *
 * Revision 2.57  2004/02/27 04:09:55  bobboau
 * fixed a Z buffer error in HTL submodel rendering,
 * and glow points,
 * and other stuff
 *
 * Revision 2.56  2004/02/20 21:58:07  randomtiger
 * Added * to - conversion for start mission code to allow launcher missions with '-' in them.
 * Currently the parsing code counts that as a flag and messes it up.
 *
 * Revision 2.55  2004/02/20 21:45:40  randomtiger
 * Removed some uneeded code between NO_DIRECT3D and added gr_zbias call, ogl is set to a stub func.
 * Changed -htl param to -nohtl. Fixed some badly named functions to match convention.
 * Fixed setup of center_alpha in OGL which was causing crash.
 *
 * Revision 2.54  2004/02/16 11:47:31  randomtiger
 * Removed a lot of files that we dont need anymore.
 * Changed htl to be on by default, command now -nohtl
 * Changed D3D to use a 2D vertex for 2D operations which should cut down on redundant data having to go though the system.
 * Added small change to all -start_mission flag to take you to any mission by filename, very useful for testing.
 * Removed old dshow code and took away timerbar compile flag condition since it uses a runtime flag now.
 *
 * Revision 2.53  2004/02/04 10:14:25  Goober5000
 * changed spec and glow to be off by default; command lines are now -spec and -glow
 * --Goober5000
 *
 * Revision 2.52  2004/01/29 01:34:00  randomtiger
 * Added malloc montoring system, use -show_mem_usage, debug exes only to get an ingame list of heap usage.
 * Also added -d3d_notmanaged flag to activate non managed D3D path, in experimental stage.
 *
 * Revision 2.51  2004/01/24 14:31:26  randomtiger
 * Added the D3D particle code, its not bugfree but works perfectly on my card and helps with the framerate.
 * Its optional and off by default, use -d3d_particle to activiate.
 * Also bumped up D3D ambient light setting, it was way too dark.
 * Its now set to something similar to the original game.
 *
 * Revision 2.50  2003/12/08 22:30:02  randomtiger
 * Put render state and other direct D3D calls repetition check back in, provides speed boost.
 * Fixed bug that caused fullscreen only crash with DXT textures
 * Put dithering back in for tgas and jpgs
 *
 * Revision 2.49  2003/12/04 20:39:09  randomtiger
 * Added DDS image support for D3D
 * Added new command flag '-ship_choice_3d' to activate 3D models instead of ani's in ship choice, feature now off by default
 * Hopefully have fixed D3D text batching bug that caused old values to appear
 * Added Hud_target_object_factor variable to control 3D object sizes of zoom in HUD target
 * Fixed jump nodes not showing
 *
 * Revision 2.48  2003/12/03 19:27:00  randomtiger
 * Changed -t32 flag to -jpgtga
 * Added -query_flag to identify builds with speech not compiled and other problems
 * Now loads up launcher if videocard reg entry not found
 * Now offers to go online to download launcher if its not present
 * Changed target view not to use lower res texture, hi res one is already chached so might as well use it
 *
 * Revision 2.47  2003/11/29 10:52:09  randomtiger
 * Turned off D3D file mapping, its using too much memory which may be hurting older systems and doesnt seem to be providing much of a speed benifit.
 * Added stats command for ingame stats on memory usage.
 * Trys to play intro.mve and intro.avi, just to be safe since its not set by table.
 * Added fix for fonts wrapping round in non standard hi res modes.
 * Changed D3D mipmapping to a good value to suit htl mode.
 * Added new fog colour method which makes use of the bitmap, making this htl feature backcompatible again.
 *
 * Revision 2.46  2003/11/19 20:37:23  randomtiger
 * Almost fully working 32 bit pcx, use -pcx32 flag to activate.
 * Made some commandline variables fit the naming standard.
 * Changed timerbar system not to run pushes and pops if its not in use.
 * Put in a note about not uncommenting asserts.
 * Fixed up a lot of missing POP's on early returns?
 * Perhaps the motivation for Assert functionality getting commented out?
 * Fixed up some bad asserts.
 * Changed nebula poofs to render in 2D in htl, it makes it look how it used to in non htl. (neb.cpp,1248)
 * Before the poofs were creating a nasty stripe effect where they intersected with ships hulls.
 * Put in a special check for the signs of that D3D init bug I need to lock down.
 *
 * Revision 2.44  2003/11/15 18:09:33  randomtiger
 * Put TGA and JPG stuff on -t32 flag
 * Put 32 bit PCX stuff on -pcx32 (still has bugs)
 * Added multisample checking on device initialisation
 * Changed unrecognised parameter message (as requested) to be more user friendly
 * Speech now chooses voice based on reg value set by launcher v3.1
 *
 * Revision 2.43  2003/11/11 03:56:10  bobboau
 * shit load of bug fixing, much of it in nebula and bitmap drawing
 *
 * Revision 2.42  2003/11/09 06:31:38  Kazan
 * a couple of htl functions being called in nonhtl (ie NULL functions) problems fixed
 * conflicts in cmdline and timerbar.h log entries
 * cvs stopped acting like it was on crack obviously
 *
 * Revision 2.41  2003/11/09 04:09:18  Goober5000
 * edited for language
 * --Goober5000
 *
 * Revision 2.40  2003/11/08 22:25:47  Kazan
 * Timerbar was enabled in both release and debug - so i made it a command line option "-timerbar"
 * DONT MESS WITH OTHER PEOPLES INCLUDE PATHS
 * DONT MESS WITH EXEC NAMES (leave it fs2_open_r and fs2_open_d) or paths!
 *
 * Revision 2.39  2003/11/03 18:07:26  randomtiger
 * Added -d3d_no_vsync command to make turning off vsync optional.
 * Removed 32bit command, it doesnt do anything.
 * Made aa multisample reg check safe.
 *
 * Revision 2.38  2003/10/27 23:04:20  randomtiger
 * Added -no_set_gamma flags
 * Fixed up some more non standard res stuff
 * Improved selection of device type, this includes using a pure device when allowed which means dev should not use Get* functions in D3D
 * Made fade in credits work
 * Stopped a call to gr_reser_lighting() in non htl mode when the pointer was NULL, was causing a crash loading a fogged level
 * Deleted directx8 directory content, has never been needed.
 *
 * Revision 2.37  2003/10/26 00:31:58  randomtiger
 * Fixed hulls not drawing (with Phreaks advise).
 * Put my 32bit PCX loading under PCX_32 compile flag until its working.
 * Fixed a bug with res 640x480 I introduced with my non standard mode code.
 * Changed JPG and TGA loading command line param to "-t32"
 *
 * Revision 2.36  2003/10/24 17:35:04  randomtiger
 * Implemented support for 32bit TGA and JPG for D3D
 * Also 32 bit PCX, but it still has some bugs to be worked out
 * Moved convert_24_to_16 out of the bitmap pfunction structures and into packunpack.cpp because thats the only place that uses it.
 *
 * Revision 2.35  2003/10/15 22:03:23  Kazan
 * Da Species Update :D
 *
 * Revision 2.34  2003/10/14 17:39:12  randomtiger
 * Implemented hardware fog for the HT&L code path.
 * It doesnt use the backgrounds anymore but its still an improvement.
 * Currently it fogs to a brighter colour than it should because of Bob specular code.
 * I will fix this after discussing it with Bob.
 *
 * Also tided up some D3D stuff, a cmdline variable name and changed a small bit of
 * the htl code to use the existing D3D engine instead of work around it.
 * And added extra information in version number on bottom left of frontend screen.
 *
 * Revision 2.33  2003/10/12 03:41:36  Kazan
 * #Kazan# FS2NetD client code gone multithreaded, some Fred2 Open -mod stuff [obvious code.lib] including a change in cmdline.cpp, changed Stick's "-nohtl" to "-htl" - HTL is _OFF_ by default here (Bobboau and I decided this was a better idea for now)
 *
 * Revision 2.32  2003/10/10 03:59:40  matt
 * Added -nohtl command line param to disable HT&L, nothing is IFDEFd
 * out now. -Sticks
 *
 * Revision 2.31  2003/09/25 21:12:22  Kazan
 * ##Kazan## FS2NetD Completed!  Just needs some thorough bug checking (i don't think there are any serious bugs)
 * Also D3D8 Screenshots work now.
 *
 * Revision 2.30  2003/09/23 02:42:52  Kazan
 * ##KAZAN## - FS2NetD Support! (FS2 Open PXO) -- Game Server Listing, and mission validation completed - stats storing to come - needs fs2open_pxo.cfg file [VP-able]
 *
 * Revision 2.29  2003/09/14 19:00:02  wmcoolmon
 * Changed "nospec" and "cell" to "Cmdline_nospec" and "Cmdline_cell"
 *
 * Revision 2.28  2003/09/14 18:32:24  wmcoolmon
 * Added "-safeloading" command line parameter, which uses old fs2_retail-style loading code -C
 *
 * Revision 2.27  2003/09/13 06:02:05  Goober5000
 * clean rollback of all of argv's stuff
 * --Goober5000
 *
 * Revision 2.25  2003/09/09 17:10:54  matt
 * Added -nospec cmd line param to disable specular -Sticks
 *
 * Revision 2.24  2003/08/22 07:35:08  bobboau
 * specular code should be bugless now,
 * cell shadeing has been added activated via the comand line '-cell',
 * 3D shockwave models, and a transparency method I'm calling edge and center alpha that could be usefull for other things, ask for details
 *
 * Revision 2.23  2003/08/21 20:54:37  randomtiger
 * Fixed switching - RT
 *
 * Revision 2.22  2003/08/12 03:18:33  bobboau
 * Specular 'shine' mapping;
 * useing a phong lighting model I have made specular highlights
 * that are mapped to the model,
 * rendering them is still slow, but they look real purdy
 *
 * also 4 new (untested) comand lines, the XX is a floating point value
 * -spec_exp XX
 * the n value, you can set this from 0 to 200 (actualy more than that, but this is the recomended range), it will make the highlights bigger or smaller, defalt is 16.0 so start playing around there
 * -spec_point XX
 * -spec_static XX
 * -spec_tube XX
 * these are factors for the three diferent types of lights that FS uses, defalt is 1.0,
 * static is the local stars,
 * point is weapons/explosions/warp in/outs,
 * tube is beam weapons,
 * for thouse of you who think any of these lights are too bright you can configure them you're self for personal satisfaction
 *
 * Revision 2.21  2003/08/09 06:07:23  bobboau
 * slightly better implementation of the new zbuffer thing, it now checks only three diferent formats defalting to the 16 bit if neither the 24 or 32 bit versions are suported
 *
 * Revision 2.20  2003/03/29 09:42:05  Goober5000
 * made beams default shield piercing again
 * also added a beam no pierce command line flag
 * and fixed something else which I forgot :P
 * --Goober5000
 *
 * Revision 2.19  2003/03/20 08:24:45  unknownplayer
 * Modified the command line options so they are all in lower-case characters.
 * Made a slight AI adjustment to how ships choose to attack turrets (they now have a 25% chance of attacking a beam turret which has fired at them from another ship)
 *
 * Revision 2.18  2003/03/18 10:07:00  unknownplayer
 * The big DX/main line merge. This has been uploaded to the main CVS since I can't manage to get it to upload to the DX branch. Apologies to all who may be affected adversely, but I'll work to debug it as fast as I can.
 *
 * Revision 2.17  2003/02/22 04:13:17  wmcoolmon
 * Added "-dshowvid" command-line option, which must be set in order for movies to play.
 *
 * Revision 2.16  2002/12/21 13:39:25  DTP
 * did bit more house keeping. modfied Phreaks fps cmdline a bit, so that we dont have to specific build code.libs for fred, but can use the same code.lib for both fs2_open.exe and fred2_open.exe
 *
 * Revision 2.15  2002/12/17 03:08:18  DTP
 * fix to Show_framerate. seems it will call an unresolved external error during fred builds. modified my ifndefs a bit, dsw modified to include preprocessor tag FRED.
 *
 * Revision 2.14  2002/12/17 02:22:34  DTP
 * wrong name :)
 *
 * Revision 2.13  2002/12/17 02:21:06  DTP
 * cleaned up after phreak :). it will make a mess in debug builds. added a couple of ifndef _debugs.
 *
 * Revision 2.12  2002/12/14 17:09:08  phreak
 * added a command line that displays frames per second when enabled.
 * use "-fps"
 *
 * Revision 2.11  2002/11/10 16:32:42  DTP
 * -DTP reworked mod support,
 *
 * Revision 2.10  2002/11/10 16:30:53  DTP
 * -DTP reworked mod support,
 *
 * Revision 2.9  2002/10/27 23:55:36  DTP
 * DTP; started basic implementation of mod-support
 * plain files only for now. fs2_open.exe -mod X will look for files in fs2/ X /all-legal-subdirectories. no checking/creating dirs yet. directories must be there.
 *
 * Revision 2.8  2002/10/22 23:02:39  randomtiger
 * Made Phreaks alternative scanning style optional under the command line tag "-phreak"
 * Fixed bug that changes HUD colour when targetting debris in a full nebula. - RT
 *
 * Revision 2.7  2002/10/19 03:50:28  randomtiger
 * Added special pause mode for easier action screenshots.
 * Added new command line parameter for accessing all single missions in tech room. - RT
 *
 * Revision 2.6  2002/08/27 13:38:57  penguin
 * Moved DirectX8 stuff to directx8 branch; reverted to previous
 *
 * Revision 2.4.2.4  2002/11/10 11:32:29  randomtiger
 *
 * Made D3D8 mipmapping optional on command line flag -d3dmipmip, off by default.
 * When on is now less blury. - RT
 *
 * Revision 2.4.2.3  2002/11/04 23:53:24  randomtiger
 *
 * Added new command line parameter -d3dlauncher which brings up the launcher.
 * This is needed since FS2 DX8 now stores the last successful details in the registry and
 * uses them to choose the adapter and mode to run in unless its windowed or they are not set.
 * Added some code for Antialiasing but it messes up the font but hopefully that can be fixed later. - RT
 *
 * Revision 2.4.2.2  2002/11/04 21:24:59  randomtiger
 *
 * When running in D3D all ani's are memory mapped for speed, this takes up more memory but stops gametime locking of textures which D3D8 hates.
 * Added new command line tag Cmdline_d3dlowmem for people who dont want to make use of this because they have no memory.
 * Cleaned up some more texture stuff enabled console debug for D3D.
 *
 * Revision 2.4.2.1  2002/08/27 13:19:02  penguin
 * Moved to directx8 branch
 *
 * Revision 2.5  2002/08/18 19:48:28  randomtiger
 * Added new lib files: strmiids and ddraw to get dshow working
 * Added new command line parameter to active direct show movie play: -dshowvid
 * Uncommented movie_play calls and includes
 *
 * Revision 2.4  2002/08/07 00:44:13  DTP
 * Implented -GF4FIX commandline switch
 *
 * Revision 2.3  2002/08/01 01:41:04  penguin
 * The big include file move
 *
 * Revision 2.2  2002/07/29 06:35:15  DTP
 * added -almission commandline arguement, will autoload mission i.e fs2_open.exe -almission kickass will autoload kickass.fs2 which should be a multiplayer mission.
 *
 * Revision 2.1  2002/07/07 19:55:58  penguin
 * Back-port to MSVC
 *
 * Revision 2.0  2002/06/03 04:02:21  penguin
 * Warpcore CVS sync
 *
 * Revision 1.3  2002/05/08 17:29:17  mharris
 * more port tweaks
 *
 * Revision 1.2  2002/05/07 13:22:14  mharris
 * added pstypes.h include
 *
 * Revision 1.1  2002/05/02 18:03:04  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 8     8/26/99 8:51p Dave
 * Gave multiplayer TvT messaging a heavy dose of sanity. Cheat codes.
 * 
 * 7     7/15/99 3:07p Dave
 * 32 bit detection support. Mouse coord commandline.
 * 
 * 6     7/13/99 1:15p Dave
 * 32 bit support. Whee!
 * 
 * 5     6/22/99 9:37p Dave
 * Put in pof spewing.
 * 
 * 4     1/12/99 5:45p Dave
 * Moved weapon pipeline in multiplayer to almost exclusively client side.
 * Very good results. Bandwidth goes down, playability goes up for crappy
 * connections. Fixed object update problem for ship subsystems.
 * 
 * 3     11/17/98 11:12a Dave
 * Removed player identification by address. Now assign explicit id #'s.
 * 
 * 2     10/07/98 10:52a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:48a Dave
 * 
 * 38    10/02/98 3:22p Allender
 * fix up the -connect option and fix the -port option
 * 
 * 37    9/15/98 4:04p Allender
 * added back in the -ip_addr command line switch because it needs to be
 * in the standalone server only executable
 * 
 * 36    9/14/98 11:52a Allender
 * don't use cfile
 * 
 * 35    9/14/98 11:28a Allender
 * support for server bashing of address when received from client.  Added
 * a cmdline.cfg file to process command line arguments from a file
 * 
 * 34    9/08/98 2:20p Allender
 * temporary code to force IP address to a specific value.
 * 
 * 33    8/20/98 5:30p Dave
 * Put in handy multiplayer logfile system. Now need to put in useful
 * applications of it all over the code.
 * 
 * 32    8/07/98 10:39a Allender
 * fixed debug standalone problem where stats would continually get sent
 * to tracker.  more debug code to help find stats problem
 * 
 * 31    7/24/98 11:14a Allender
 * start of new command line options for version 1.04
 * 
 * 30    5/21/98 1:50a Dave
 * Remove obsolete command line functions. Reduce shield explosion packets
 * drastically. Tweak PXO screen even more. Fix file xfer system so that
 * we can guarantee file uniqueness.
 * 
 * 29    5/18/98 9:10p Dave
 * Put in many new PXO features. Fixed skill level bashing in multiplayer.
 * Removed several old command line options. Put in network config files.
 * 
 * 28    5/09/98 7:16p Dave
 * Put in CD checking. Put in standalone host password. Made pilot into
 * popup scrollable.
 * 
 * 27    4/23/98 8:27p Allender
 * basic support for cutscene playback.  Into movie code in place.  Tech
 * room can view cutscenes stored in CDROM_dir variable
 * 
 * 26    4/09/98 5:43p Dave
 * Remove all command line processing from the demo. Began work fixing up
 * the new multi host options screen.
 * 
 * 25    4/02/98 11:40a Lawrance
 * check for #ifdef DEMO instead of #ifdef DEMO_RELEASE
 * 
 * 24    4/01/98 5:56p Dave
 * Fixed a messaging bug which caused msg_all mode in multiplayer not to
 * work. Compile out a host of multiplayer options not available in the
 * demo.
 * 
 * 23    3/14/98 2:48p Dave
 * Cleaned up observer joining code. Put in support for file xfers to
 * ingame joiners (observers or not). Revamped and reinstalled pseudo
 * lag/loss system.
 * 
 * 22    2/22/98 12:19p John
 * Externalized some strings
 * 
 * 21    1/31/98 4:32p Dave
 * Put in new support for VMT player validation, game logging in and game
 * logging out.
 * 
 * 20    12/10/97 4:45p Dave
 * Added in more detailed support for multiplayer packet lag/loss. Fixed
 * some multiplayer stuff. Added some controls to the standalone.
 * 
 * 19    12/09/97 6:14p Lawrance
 * add -nomusic flag
 * 
 * 18    12/01/97 5:10p Dave
 * Fixed a syntax bug.
 * 
 * 17    12/01/97 4:59p Dave
 * Synchronized multiplayer debris objects. Put in pilot popup in main
 * hall. Optimized simulated multiplayer lag module. Fixed a potential
 * file_xfer bug.
 * 
 * 16    11/28/97 7:04p Dave
 * Emergency checkin due to big system crash.
 * 
 * 15    11/28/97 5:06p Dave
 * Put in facilities for simulating multiplayer lag.
 * 
 * 14    11/24/97 5:42p Dave
 * Fixed a file xfer buffer free/malloc problem. Lengthened command line
 * switch string parse length.
 * 
 * 13    11/12/97 4:39p Dave
 * Put in multiplayer campaign support parsing, loading and saving. Made
 * command-line variables better named. Changed some things on the initial
 * pilot select screen.
 * 
 * 12    11/11/97 4:54p Dave
 * Put in support for single vs. multiplayer pilots. Put in initial player
 * selection screen (no command line option yet). Started work on
 * multiplayer campaign file save gaming.
 * 
 * 11    11/11/97 11:55a Allender
 * initialize network at beginning of application.  create new call to set
 * which network protocol to use
 * 
 * 10    9/18/97 10:12p Dave
 * Added -gimmemedals, which gives the current pilot all the medals in the
 * game (debug)
 * 
 * 9     9/18/97 9:20a Dave
 * Minor modifications
 * 
 * 8     9/15/97 11:40p Lawrance
 * remove demo granularity switch
 * 
 * 7     9/09/97 3:39p Sandeep
 * warning level 4 bugs
 * 
 * 6     9/03/97 5:03p Lawrance
 * add support for -nosound command line parm
 * 
 * 5     8/22/97 8:52a Dave
 * Removed a return statement which would have broken the parser out too
 * early.
 * 
 * 4     8/21/97 4:55p Dave
 * Added a switch for multiplayer chat streaming. Added a section for
 * global command line vars.
 * 
 * 3     8/06/97 2:26p Dave
 * Made the command line parse more robust. Made it easier to add and
 * process new command-line switches.
 * 
 * 2     8/04/97 3:13p Dave
 * Added command line functions. See cmdline.cpp for directions on adding
 * new switches
 * 
 * 1     8/04/97 9:58a Dave
 * 
 * $NoKeywords: $
 */

#include <direct.h>
#include <string.h>
#include <stdlib.h>
#include "globalincs/pstypes.h"
#include "cmdline/cmdline.h"
#include "globalincs/linklist.h"
#include "globalincs/systemvars.h"
#include "network/multi.h"
#include "species_defs/species_defs.h"
#include "hud/hudconfig.h"


// variables
class cmdline_parm {
public:
	cmdline_parm *next, *prev;
	char *name;						// name of parameter, must start with '-' char
	char *help;						// help text for this parameter
	char *args;						// string value for parameter arguements (NULL if no arguements)
	int name_found;				// true if parameter on command line, otherwise false

	cmdline_parm(char *name, char *help);
	~cmdline_parm();
	int found();
	int get_int();
	float get_float();
	char *str();
};

float Cmdline_max_subdivide = 0.0f;

int Cmdline_env = 0;
int Cmdline_decals = 0;
int Cmdline_alpha_env = 0;

enum
{
	// DO NOT CHANGE ANYTHING ABOUT THESE FIRST TWO OR WILL MESS UP THE LAUNCHER
	EASY_DEFAULT  =  1 << 1,
	EASY_ALL_ON   =  1 << 2,

	EASY_MEM_ON   =  1 << 3,
	EASY_MEM_OFF  =  1 << 4,

	// Add new flags here

	// Combos
	EASY_MEM_ALL_ON  = EASY_ALL_ON  | EASY_MEM_ON,
	EASY_DEFAULT_MEM = EASY_DEFAULT | EASY_MEM_OFF,
};

typedef struct
{
	// DO NOT CHANGE THE SIZE OF THIS STRING!
	char name[32];

} EasyFlag;

EasyFlag easy_flags[] =
{
	"Custom",	 
	"Default FS2 (All features off)",
	"All features on",
	"High memory usage features on",
	"High memory usage features off",
};

// DO NOT CHANGE **ANYTHING** ABOUT THIS STRUCTURE AND ITS CONTENT
typedef struct
{
	char  name[20];		// The actual flag
	char  desc[40];		// The text that will appear in the launcher (unless its blank, other name is shown)
	bool  fso_only;		// true if this is a fs2_open only feature
	int   on_flags;		// Easy flag which will turn this feature on
	int   off_flags;	// Easy flag which will turn this feature off
	char  type[16];		// Launcher uses this to put flags under different headings
	char  web_url[256];	// Link to documentation of feature (please use wiki or somewhere constant)

} Flag;

// Please group them by type, ie Graphics, gameplay etc, maximum 20 different types
Flag exe_params[] = 
{
	"-spec",		  "Enable specular",				true,	EASY_ALL_ON,		EASY_DEFAULT,		"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.spec", 
	"-glow",		  "Enable glowmaps",				true,	EASY_MEM_ALL_ON,	EASY_DEFAULT_MEM,	"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.glow", 
	"-pcx32",		  "Enable 32bit textures",			true,	EASY_MEM_ALL_ON,	EASY_DEFAULT_MEM,	"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.pcx32", 
	"-jpgtga",		  "Enable jpg,tga textures",		true,	EASY_MEM_ALL_ON,	EASY_DEFAULT_MEM,	"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.jpgtga", 
	"-d3dmipmap",	  "Enable mipmapping",				true,	EASY_MEM_ALL_ON,	EASY_DEFAULT_MEM,	"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.d3dmipmap", 
	"-nomotiondebris","Disable motion debris",			true,	EASY_ALL_ON,		EASY_DEFAULT,		"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.nomotiondebris",
	"-2d_poof",		  "Stops fog intersect hull",		true,	EASY_ALL_ON,		EASY_DEFAULT,		"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.2d_poof", 
	"-cell",		  "Enable cell shading",			true,	0,					EASY_DEFAULT,		"Graphics",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.cell", 

	"-pcx2dds",	      "Compress pcx",			        true,   0,					EASY_DEFAULT,		"Game Speed",	"", 
	"-d3d_no_vsync",  "Disable vertical sync",			true,	0,					EASY_DEFAULT,		"Game Speed",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.d3d_no_vsync", 

	"-nobeampierce",  "",								true,	0,					EASY_DEFAULT,		"Gameplay",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.nobeampierce", 
	"-dualscanlines", "Another pair of scanning lines", true,	0,					EASY_DEFAULT,		"Gameplay",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.dualscanlines", 
	"-ship_choice_3d","Use models for ship selection",	true,	0,					EASY_DEFAULT,		"Gameplay",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.ship_choice_3d", 
	"-targetinfo",	  "Enable info next to target",		true,	0,					EASY_DEFAULT,		"Gameplay",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.targetinfo",
	"-orbradar",	  "Enables 3d radar",				true,   0,					EASY_DEFAULT,		"Gameplay",		"",
	"-3dwarp",        "Enable 3d warp",                 true,   0,					EASY_DEFAULT,      "Gameplay",     "",
    "-tbpwarpeffects","Enable TBP Warp Effects",        true,   0,					EASY_DEFAULT,      "Gameplay",     "", // TBP warp effects -Et1
	"-ballistic_gauge",	"Enable ballistic ammo gauge",	true,	0,					EASY_DEFAULT,		"Gameplay",		"",

	"-snd_preload",	  "Preload mission game sounds",	true,	EASY_MEM_ALL_ON,	EASY_DEFAULT_MEM,	"Audio",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.snd_preload",
	"-noaudio",		  "Disable sound and music",		false,	0,					EASY_DEFAULT,		"Audio",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.nosound",
	"-nomusic",		  "Disable music",					false,	0,					EASY_DEFAULT,		"Audio",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.nomusic",

	"-standalone",	  "",								false,	0,					EASY_DEFAULT,		"Multi",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.standalone", 
	"-startgame",	  "",								false,	0,					EASY_DEFAULT,		"Multi",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.startgame", 
	"-closed",		  "",								false,	0,					EASY_DEFAULT,		"Multi",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.closed", 
	"-restricted",	  "",								false,	0,					EASY_DEFAULT,		"Multi",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.restricted", 
	"-multilog",	  "",								false,	0,					EASY_DEFAULT,		"Multi",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.multilog", 
	"-clientdamage",  "",								false,	0,					EASY_DEFAULT,		"Multi",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.clientdamage",	
	"-mpnoreturn",	  "Disables flight deck option",	true,	0,					EASY_DEFAULT,		"Multi",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php/Command-Line%20Reference#-mpnoreturn",

	"-oldfire",		  "",								true,	0,					EASY_DEFAULT,		"Troubleshoot",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.oldfire", 
	"-nohtl",		  "Software mode (very slow)",		true,	0,					EASY_DEFAULT,		"Troubleshoot",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.nohtl", 
	"-no_set_gamma",  "Disable setting of gamma",		true,	0,					EASY_DEFAULT,		"Troubleshoot",	"", 
	"-dnoshowvid", 	  "Disable video playback",			true,	0,					EASY_DEFAULT,		"Troubleshoot",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.dnoshowvid", 
	"-safeloading",	  "",								true,	0,					EASY_DEFAULT,		"Troubleshoot",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.safeloading", 
	"-query_speech",  "Does this build have speech?",   true,	0,					EASY_DEFAULT,		"Troubleshoot",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.query_speech",
	"-d3d_bad_tsys",  "Enable inefficient textures",	false,	0,					EASY_DEFAULT,		"Troubleshoot",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.d3d_bad_tsys",	
	"-novbo",		  "Disable OpenGL VBO",				true,	0,					EASY_DEFAULT,		"Troubleshoot", "",	

	"-env",			  "environment mapping maping",		true,	0,					EASY_DEFAULT,		"Experimental",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.env", 
	"-alpha_env",	  "uses uses alpha for env maping",	true,	0,					EASY_DEFAULT,		"Experimental",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.alpha_env", 
	"-decals",		  "impact decals",					true,	0,					EASY_DEFAULT,		"Experimental",	"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.decals",
	"-loadonlyused",  "Loads only used weapons",		true,	0,					EASY_DEFAULT,		"Experimental",	"",

	"-fps",			  "Show frames per seconds",		false,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.fps", 
	"-window",		  "Run in window",					true,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.window", 
	"-timerbar",	  "",								true,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.timerbar", 
	"-stats",		  "Show statistics",				true,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.stats", 
	"-coords",		  "Show coordinates",				false,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.coords", 
	"-show_mem_usage","Show memory usage",				true,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.show_mem_usage", 
	"-pofspew",		  "",								false,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.pofspew", 
	"-tablecrcs",	  "",								true,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.tablecrcs", 
	"-missioncrcs",   "",								true,	0,					EASY_DEFAULT,		"Dev Tool",		"http://dynamic4.gamespy.com/~freespace/fsdoc/index.php?pagename=Command-Line%20Reference#x2d.missioncrcs", 
};

// here are the command line parameters that we will be using for FreeSpace
cmdline_parm standalone_arg("-standalone", NULL);
cmdline_parm mpnoreturn_arg("-mpnoreturn", NULL);	//Removes 'Return to Flight Deck' in respawn dialog -C
cmdline_parm nosound_arg("-nosound", NULL);
cmdline_parm nomusic_arg("-nomusic", NULL);
cmdline_parm startgame_arg("-startgame", NULL);
cmdline_parm gamename_arg("-gamename", NULL);
cmdline_parm gamepassword_arg("-password", NULL);
cmdline_parm gameclosed_arg("-closed", NULL);
cmdline_parm gamerestricted_arg("-restricted", NULL);
cmdline_parm allowabove_arg("-allowabove", NULL);
cmdline_parm allowbelow_arg("-allowbelow", NULL);
cmdline_parm port_arg("-port", NULL);
cmdline_parm connect_arg("-connect", NULL);
cmdline_parm multilog_arg("-multilog", NULL);
cmdline_parm server_firing_arg("-oldfire", NULL);
cmdline_parm client_dodamage("-clientdamage", NULL);
cmdline_parm pof_spew("-pofspew", NULL);
cmdline_parm mouse_coords("-coords", NULL);
cmdline_parm timeout("-timeout", NULL);
cmdline_parm window("-window", NULL);
cmdline_parm almission_arg("-almission", NULL); //DTP for autoload Multi mission
cmdline_parm allslev_arg("-allslev", NULL); //Give access to all single player missions
cmdline_parm dualscanlines_arg("-dualscanlines", NULL); // Change to phreaks options including new targetting code
cmdline_parm targetinfo_arg("-targetinfo", NULL);	//Adds ship name/class to right of target box -C
cmdline_parm dnoshowvid_arg("-dnoshowvid", NULL); // Allows video streaming
cmdline_parm mod_arg("-mod", NULL); //DTP modsupport
cmdline_parm fps_arg("-fps", NULL);
cmdline_parm cache_ani_arg("-cache_ani", NULL);  
cmdline_parm d3dmipmap_arg("-d3dmipmap", NULL);
cmdline_parm beams_no_pierce_shields_arg("-nobeampierce", NULL);	// beams do not pierce shields - Goober5000
cmdline_parm fov_arg("-fov", NULL);	// comand line FOV -Bobboau
cmdline_parm spec_exp_arg("-spec_exp", NULL);	// comand line FOV -Bobboau
cmdline_parm spec_point_arg("-spec_point", NULL);	// comand line FOV -Bobboau
cmdline_parm spec_static_arg("-spec_static", NULL);	// comand line FOV -Bobboau
cmdline_parm spec_tube_arg("-spec_tube", NULL);	// comand line FOV -Bobboau
cmdline_parm safeloading_arg("-safeloading", NULL); //Uses old loading method -C
cmdline_parm nomotiondebris_arg("-nomotiondebris", NULL); //Removes those ugly floating rocks -C
cmdline_parm spec_arg("-spec", NULL); // use specular highlighting -Sticks
cmdline_parm glow_arg("-glow", NULL); // use Bobs glow code
cmdline_parm MissionCRCs("-missioncrcs", NULL);
cmdline_parm TableCRCs("-tablecrcs", NULL);
cmdline_parm htl_arg("-nohtl", NULL); //Use HT&L	  
cmdline_parm cell_arg("-cell", NULL);
cmdline_parm jpgtga_arg("-jpgtga",NULL);
cmdline_parm no_set_gamma_arg("-no_set_gamma",NULL);
cmdline_parm d3d_no_vsync_arg("-d3d_no_vsync", NULL);
cmdline_parm pcx32_arg("-pcx32",NULL);
cmdline_parm pcx32dds_arg("-pcx2dds",NULL);
cmdline_parm timerbar_arg("-timerbar", NULL);
cmdline_parm stats_arg("-stats", NULL);
cmdline_parm query_speech_arg("-query_speech", NULL);
cmdline_parm ship_choice_3d_arg("-ship_choice_3d", NULL);
cmdline_parm dxt_arg("-dxt",NULL);
cmdline_parm d3d_particle_arg("-d3d_particle",NULL);
cmdline_parm show_mem_usage_arg("-show_mem_usage",NULL);
cmdline_parm rt_arg("-rt",NULL);
cmdline_parm start_mission_arg("-start_mission",NULL);
cmdline_parm ambient_factor_arg("-ambient_factor",NULL);
cmdline_parm get_flags_arg("-get_flags",NULL);
cmdline_parm d3d_lesstmem_arg("-d3d_bad_tsys",NULL);
cmdline_parm batch_3dunlit_arg("-batch_3dunlit",NULL);
cmdline_parm fred2_htl_arg("-fredhtl",NULL);
cmdline_parm nowarn_arg("-no_warn", NULL);
cmdline_parm max_subdivide_arg("-max_subdivide", NULL);	// comand line maximum level of tesleation for n-patches -Bobboau
cmdline_parm env("-env", NULL);	
cmdline_parm alpha_env("-alpha_env", NULL);	
cmdline_parm decals("-decals", NULL);	
cmdline_parm orb_radar("-orbradar",NULL);
cmdline_parm TBPWarpEffects( "-tbpwarpeffects", NULL ); // TBP warp effects -Et1
cmdline_parm use_3dwarp("-3dwarp", NULL);
cmdline_parm ballistic_gauge("-ballistic_gauge", NULL );

//Experimental
cmdline_parm load_only_used("-loadonlyused", NULL);

cmdline_parm poof_2d_arg("-2d_poof", NULL);
cmdline_parm Radar_Range_Clamp("-radar_reduce", NULL);

cmdline_parm no_vbo_arg("-novbo", NULL);
cmdline_parm snd_preload_arg("-snd_preload", NULL);

cmdline_parm no_fpscap("-no_fps_capping", NULL);

int Cmdline_mpnoreturn = 0;
int Cmdline_show_stats = 0;
int Cmdline_timerbar = 0;
int Cmdline_multi_stream_chat_to_file = 0;
int Cmdline_freespace_no_sound = 0;
int Cmdline_freespace_no_music = 0;
int Cmdline_gimme_all_medals = 0;
int Cmdline_use_last_pilot = 0;
int Cmdline_multi_protocol = -1;
int Cmdline_cd_check = 1;
int Cmdline_start_netgame = 0;
int Cmdline_closed_game = 0;
int Cmdline_restricted_game = 0;
int Cmdline_network_port = -1;
char *Cmdline_game_name = NULL;
char *Cmdline_game_password = NULL;
char *Cmdline_rank_above= NULL;
char *Cmdline_rank_below = NULL;
char *Cmdline_connect_addr = NULL;
char *Cmdline_almission = NULL;	//DTP for autoload multi mission.
char *Cmdline_mod = NULL; //DTP for mod arguement
int Cmdline_multi_log = 0;
int Cmdline_server_firing = 0;
int Cmdline_client_dodamage = 0;
int Cmdline_spew_pof_info = 0;
int Cmdline_mouse_coords = 0;
int Cmdline_timeout = -1;
int Cmdline_SpewMission_CRCs = 0; // Kazan for making valid mission lists
int Cmdline_SpewTable_CRCs = 0;
int Cmdline_ship_choice_3d = 0;
int Cmdline_d3d_particle = 0;
int Cmdline_orb_radar = 0;
int Cmdline_TBPWarpEffects = 0; // TBP warp effects -Et1
int Cmdline_3dwarp = 0;

int Cmdline_window = 0;
int Cmdline_allslev = 0;
int Cmdline_dualscanlines	= 0;
int Cmdline_targetinfo = 0;
int Cmdline_dnoshowvid = 0;
int Cmdline_show_fps = 0;
int Cmdline_safeloading = 0;
int Cmdline_nospec = 0;
int Cmdline_noglow = 0;
int Cmdline_dxt = 0;

int Cmdline_cache_ani = 0;
int Cmdline_d3dmipmap = 0;
int Cmdline_rt = 0;
char *Cmdline_start_mission = NULL;
int Cmdline_ambient_factor  = 128;
int Cmdline_2d_poof			= 0;

// Lets keep a convention here
int Cmdline_nohtl = 0;
int Cmdline_jpgtga = 0;
int Cmdline_no_set_gamma = 0;
int Cmdline_d3d_no_vsync = 0;
int Cmdline_pcx32 = 0;
int Cmdline_pcx32dds = 0;
int Cmdline_nomotiondebris = 0;
int Cmdline_query_speech = 0;

int Cmdline_show_mem_usage = 0;
int Cmdline_d3d_lesstmem = 0;

int Cmdline_beams_no_pierce_shields = 0;	// Goober5000
int Cmdline_FRED2_htl = 0; // turn HTL on in fred - Kazan
int CmdLine_NoWarn = 0; // turn warnings off in FRED

int Cmdline_ballistic_gauge = 0;	// WMCoolmon's gauge thingy

//Experimental
int Cmdline_load_only_used;

int Cmdline_novbo = 0; // turn off OGL VBO support, troubleshooting
int Cmdline_snd_preload = 0; // preload game sounds during mission load

int Cmdline_NoFPSCap; // Disable FPS capping - kazan

//char FreeSpace_Directory[256]; // allievating a cfilesystem problem caused by fred -- Kazan

static cmdline_parm Parm_list(NULL, NULL);
static int Parm_list_inited = 0;

float Cmdline_fov = 0.75f;
extern float VIEWER_ZOOM_DEFAULT;
extern float Viewer_zoom;

int Cmdline_cell = 0;
int Cmdline_batch_3dunlit = 0;

//	Return true if this character is an extra char (white space and quotes)
int is_extra_space(char ch)
{
	return ((ch == ' ') || (ch == '\t') || (ch == 0x0a) || (ch == '\'') || (ch == '\"'));
}


// eliminates all leading and trailing extra chars from a string.  Returns pointer passed in.
char *drop_extra_chars(char *str)
{
	int s, e;

	s = 0;
	while (str[s] && is_extra_space(str[s]))
		s++;

	e = strlen(str);
	while (e > s) {
		if (!is_extra_space(str[e])){
			break;
		}

		e--;
	}

	if (e > s){
		memmove(str, str + s, e - s + 1);
	}

	str[e - s + 1] = 0;
	return str;
}


// internal function - copy the value for a parameter agruement into the cmdline_parm arg field
void parm_stuff_args(cmdline_parm *parm, char *cmdline)
{
	char buffer[1024];
	memset(buffer, 0, 1024);
	char *dest = buffer;

	cmdline += strlen(parm->name);

	while ((*cmdline != 0) && (*cmdline != '-')) {
		*dest++ = *cmdline++;
	}

	drop_extra_chars(buffer);

	// mwa 9/14/98 -- made it so that newer command line arguments found will overwrite
	// the old arguments
//	Assert(parm->args == NULL);
	if ( parm->args != NULL ) {
		delete( parm->args );
		parm->args = NULL;
	}

	int size = strlen(buffer) + 1;
	if (size > 0) {
		parm->args = new char[size];
		memset(parm->args, 0, size);
		strcpy(parm->args, buffer);
	}
}


// internal function - parse the command line, extracting parameter arguements if they exist
// cmdline - command line string passed to the application
void os_parse_parms(char *cmdline)
{
	// locate command line parameters
	cmdline_parm *parmp;
	char *cmdline_offset;

	for (parmp = GET_FIRST(&Parm_list); parmp !=END_OF_LIST(&Parm_list); parmp = GET_NEXT(parmp) ) {
		cmdline_offset = strstr(cmdline, parmp->name);
		if (cmdline_offset) {
			parmp->name_found = 1;
			parm_stuff_args(parmp, cmdline_offset);
		}
	}
}


// validate the command line parameters.  Display an error if an unrecognized parameter is located.
void os_validate_parms(char *cmdline)
{
	cmdline_parm *parmp;
	char seps[] = " ,\t\n";
	char *token;
	int parm_found;

   token = strtok(cmdline, seps);
   while(token != NULL) {
	
		if (token[0] == '-') {
			parm_found = 0;
			for (parmp = GET_FIRST(&Parm_list); parmp !=END_OF_LIST(&Parm_list); parmp = GET_NEXT(parmp) ) {
				if (!stricmp(parmp->name, token)) {
					parm_found = 1;
					break;
				}
			}

			if (parm_found == 0) {
				// Changed this to MessageBox, this is a user error not a developer
				char buffer[128];
				sprintf(buffer,"Unrecogzined command line parameter %s, continue?",token);
				if( MessageBox(NULL, buffer, "Warning", MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
					exit(0);
			}
		}

		token = strtok(NULL, seps);
	}
}


// Call once to initialize the command line system
//
// cmdline - command line string passed to the application
void os_init_cmdline(char *cmdline)
{
	FILE *fp;

	// read the cmdline.cfg file from the data folder, and pass the command line arguments to
	// the the parse_parms and validate_parms line.  Read these first so anything actually on
	// the command line will take precedence
	fp = fopen("data\\cmdline_fso.cfg", "rt");

	// if the file exists, get a single line, and deal with it
	if ( fp ) {
		char buf[1024], *p;

		fgets(buf, 1024, fp);

		// replace the newline character with a NULL
		if ( (p = strrchr(buf, '\n')) != NULL ) {
			*p = '\0';
		}

		os_parse_parms(buf);
		os_validate_parms(buf);
		fclose(fp);
	}

	os_parse_parms(cmdline);
	os_validate_parms(cmdline);
}


// arg constructor
// name_ - name of the parameter, must start with '-' character
// help_ - help text for this parameter
cmdline_parm::cmdline_parm(char *name_, char *help_)
{
	name = name_;
	help = help_;
	args = NULL;
	name_found = 0;

	if (Parm_list_inited == 0) {
		list_init(&Parm_list);
		Parm_list_inited = 1;
	}

	if (name != NULL) {
		list_append(&Parm_list, this);
	}
}


// destructor - frees any allocated memory
cmdline_parm::~cmdline_parm()
{
#ifndef FRED
	if (args) {
		delete [] args;
		args = NULL;
	}
#endif
}


// returns - true if the parameter exists on the command line, otherwise false
int cmdline_parm::found()
{
	return name_found;
}


// returns - the interger representation for the parameter arguement
int cmdline_parm::get_int()
{
	Assert(args);
	return atoi(args);
}


// returns - the float representation for the parameter arguement
float cmdline_parm::get_float()
{
	Assert(args);
	return (float)atof(args);
}


// returns - the string value for the parameter arguement
char *cmdline_parm::str()
{
	Assert(args);
	return args;
}

// external entry point into this modules

bool SetCmdlineParams()
// Sets externed variables used for communication cmdline information
{
	//getcwd(FreeSpace_Directory, 256); // set the directory to our fs2 root
	if (no_fpscap.found())
	{
		Cmdline_NoFPSCap = 1;
	}

	if(load_only_used.found())
	{
		Cmdline_load_only_used = 1;
	}

	if(poof_2d_arg.found())
	{
		Cmdline_2d_poof = 1;
	}

	if (Radar_Range_Clamp.found())
	{
		if (Radar_Range_Clamp.get_float() > 0.0f)
			Radar_ranges[RR_MAX_RANGES-1] = Radar_Range_Clamp.get_float();
	}

	if (nowarn_arg.found())
	{
		CmdLine_NoWarn = 1;
	}

	if (fred2_htl_arg.found())
	{
		Cmdline_FRED2_htl = 1;
	}

	if (timerbar_arg.found()) {
		Cmdline_timerbar = 1;
	}

	if (MissionCRCs.found()) {
		Cmdline_SpewMission_CRCs = 1;
	}

	if (TableCRCs.found()) {
		Cmdline_SpewTable_CRCs = 1;
	}


	// is this a standalone server??
	if (standalone_arg.found()) {
		Is_standalone = 1;
	}

	if(mpnoreturn_arg.found()) {
		Cmdline_mpnoreturn = 1;
	}

	// run with no sound
	if ( nosound_arg.found() ) {
		Cmdline_freespace_no_sound = 1;
	}

	// run with no music
	if ( nomusic_arg.found() ) {
		Cmdline_freespace_no_music = 1;
	}

	// should we start a network game
	if ( startgame_arg.found() ) {
		Cmdline_use_last_pilot = 1;
		Cmdline_start_netgame = 1;
	}

	// closed network game
	if ( gameclosed_arg.found() ) {
		Cmdline_closed_game = 1;
	}

	// restircted network game
	if ( gamerestricted_arg.found() ) {
		Cmdline_restricted_game = 1;
	}

	// get the name of the network game
	if ( gamename_arg.found() ) {
		Cmdline_game_name = gamename_arg.str();

		// be sure that this string fits in our limits
		if ( strlen(Cmdline_game_name) > MAX_GAMENAME_LEN ) {
			Cmdline_game_name[MAX_GAMENAME_LEN-1] = '\0';
		}
	}

	// get the password for a pssword game
	if ( gamepassword_arg.found() ) {
		Cmdline_game_password = gamepassword_arg.str();

		// be sure that this string fits in our limits
		if ( strlen(Cmdline_game_name) > MAX_PASSWD_LEN ) {
			Cmdline_game_name[MAX_PASSWD_LEN-1] = '\0';
		}
	}

	// set the rank above/below arguments
	if ( allowabove_arg.found() ) {
		Cmdline_rank_above = allowabove_arg.str();
	}
	if ( allowbelow_arg.found() ) {
		Cmdline_rank_below = allowbelow_arg.str();
	}

	// get the port number for games
	if ( port_arg.found() ) {
		Cmdline_network_port = port_arg.get_int();
	}

	// the connect argument specifies to join a game at this particular address
	if ( connect_arg.found() ) {
		Cmdline_use_last_pilot = 1;
		Cmdline_connect_addr = connect_arg.str();
	}

	// see if the multilog flag was set
	if ( multilog_arg.found() ){
		Cmdline_multi_log = 1;
	}	

	// maybe use old-school server-side firing
	if (server_firing_arg.found() ){
		Cmdline_server_firing = 1;
	}

	// maybe use old-school client damage
	if(client_dodamage.found()){
		Cmdline_client_dodamage = 1;
	}	

	// spew pof info
	if(pof_spew.found()){
		Cmdline_spew_pof_info = 1;
	}

	// mouse coords
	if(mouse_coords.found()){
		Cmdline_mouse_coords = 1;
	}

	// net timeout
	if(timeout.found()){
		Cmdline_timeout = timeout.get_int();
	}

	// d3d windowed
	if(window.found()){
		Cmdline_window = 1;
	}
	if(almission_arg.found()){//DTP for autoload mission // developer oritentated
		Cmdline_almission = almission_arg.str();
		Cmdline_use_last_pilot = 1;
		Cmdline_start_netgame = 1;
	}

	if (allslev_arg.found() ) {
		Cmdline_allslev = 1;
	}

	if(dualscanlines_arg.found() ) {
		Cmdline_dualscanlines = 1;
	}

	if(targetinfo_arg.found())
	{
		Cmdline_targetinfo = 1;
	}

	if(dnoshowvid_arg.found() ) {
		Cmdline_dnoshowvid = 1;
	}


	if(mod_arg.found() ) {
		Cmdline_mod = mod_arg.str();

		// be sure that this string fits in our limits
		/* This has to be disabled because the max size is going to be mods*MAX_FILENAME_LEN
		if ( strlen(Cmdline_mod) > MAX_FILENAME_LEN ) {
			Cmdline_mod[MAX_FILENAME_LEN-1] = '\0';
		}*/

		// strip off blank space it it's there
		if ( Cmdline_mod[strlen(Cmdline_mod)-1] == ' ' ) {
			Cmdline_mod[strlen(Cmdline_mod)-1] = '\0';
		}

		// Ok - mod stacking support
		int len = strlen(Cmdline_mod);
		char *modlist = new char[len+2];
		memset(modlist, 0, len+2);
		strcpy(modlist, Cmdline_mod);

		//modlist[len]= '\0'; // double null termination at the end

		// null terminate each individual
		for (int i = 0; i < len; i++)
		{
			if (modlist[i] == ',')
				modlist[i] = '\0';
		}
		
		//copy over - we don't have to delete[] Cmdline_mod because it's a pointer to an automatic global char*
		Cmdline_mod = modlist;
	}


	if (fps_arg.found())
	{
		Cmdline_show_fps = 1;
	}

	if ( safeloading_arg.found() ) {
		Cmdline_safeloading = 1;
	}

	if ( nomotiondebris_arg.found() ) {
		Cmdline_nomotiondebris = 1;
	}

	if( cache_ani_arg.found() ) {
		Cmdline_cache_ani = 1;
	}

	if( d3dmipmap_arg.found() ) {
		Cmdline_d3dmipmap = 1;
	}

	if( stats_arg.found() ) {
		Cmdline_show_stats = 1;
	}

	if ( beams_no_pierce_shields_arg.found() ) {
		Cmdline_beams_no_pierce_shields = 1;
	}

	if ( fov_arg.found() ) {
		Viewer_zoom = VIEWER_ZOOM_DEFAULT = Cmdline_fov = fov_arg.get_float();
	}

	if (orb_radar.found())
	{
		Cmdline_orb_radar = 1;
	}

    // TBP warp effects -Et1
    if( TBPWarpEffects.found() )
    {

        Cmdline_TBPWarpEffects = 1;

    }

	if ( use_3dwarp.found() ) {
		Cmdline_3dwarp = 1;
	}

//specular comand lines
	if ( spec_exp_arg.found() ) {
		specular_exponent_value = spec_exp_arg.get_float();
	}

	if ( spec_point_arg.found() ) {
		static_point_factor = spec_point_arg.get_float();
	}

	if ( spec_static_arg.found() ) {
		static_light_factor = spec_static_arg.get_float();
	}

	if ( spec_tube_arg.found() ) {
		static_tube_factor = spec_tube_arg.get_float();
	}

	if ( cell_arg.found() ) {
		Cmdline_cell = 1;
	}

	if ( spec_arg.found() )
	{
		Cmdline_nospec = 0;
	}
	else
	{
		Cmdline_nospec = 1;
	}

	if ( htl_arg.found() ) 
	{
		Cmdline_nohtl = 1;
	}

	if( jpgtga_arg.found() )
	{	  
		Cmdline_jpgtga = 1;
	}

	if( no_set_gamma_arg.found() )
	{
		Cmdline_no_set_gamma = 1;
	}

	if(d3d_no_vsync_arg.found() )
	{
		Cmdline_d3d_no_vsync = 1;
	}

	if(pcx32_arg.found() )
	{
		Cmdline_pcx32 = 1;
	}

	if(pcx32dds_arg.found() )
	{
		Cmdline_pcx32dds = 1;
	}

	if(glow_arg.found() )
	{
		Cmdline_noglow = 0;
	}
	else
	{
		Cmdline_noglow = 1;
	}

	if(query_speech_arg.found() )
	{
		Cmdline_query_speech = 1;
	}

	if(ship_choice_3d_arg.found() )
	{
		Cmdline_ship_choice_3d = 1;
	}

	if(dxt_arg.found()) {
		Cmdline_dxt = dxt_arg.get_int();

		if(Cmdline_dxt < 0 || Cmdline_dxt > 5)
		{
			Cmdline_dxt = 0;
		}
	}

	if(d3d_particle_arg.found()) {
		Cmdline_d3d_particle = 1;
	}

	if(show_mem_usage_arg.found()) {
		Cmdline_show_mem_usage = 1;
	}

	if(rt_arg.found())
	{
		Cmdline_rt = 1;
	}

	if(start_mission_arg.found())
	{
		Cmdline_start_mission = start_mission_arg.str();

		if(Cmdline_start_mission != NULL && strlen(Cmdline_start_mission) > 0)
		{
			char *temp = Cmdline_start_mission;

			while(*temp)
			{
				if(*temp == '*')
					*temp = '-';

				temp++;
			}
		}
	}

	if(ambient_factor_arg.found())
	{
		Cmdline_ambient_factor = ambient_factor_arg.get_int();
	}

	if(get_flags_arg.found())
	{
		FILE *fp = fopen("flags.lch","w");

		if(fp == NULL)
		{
			MessageBox(NULL,"Error creating flag list for launcher", "Error", MB_OK);
			return false; 
		}

		int easy_flag_size	= sizeof(EasyFlag);
		int flag_size		= sizeof(Flag);

		int num_easy_flags	= sizeof(easy_flags) / easy_flag_size;
		int num_flags		= sizeof(exe_params) / flag_size;

		// Launcher will check its using structures of the same size
		fwrite(&easy_flag_size, sizeof(int), 1, fp);
		fwrite(&flag_size, sizeof(int), 1, fp);

		fwrite(&num_easy_flags, sizeof(int), 1, fp);
		fwrite(&easy_flags, sizeof(easy_flags), 1, fp);

		fwrite(&num_flags, sizeof(int), 1, fp);
		fwrite(&exe_params, sizeof(exe_params), 1, fp);

		fclose(fp);
		return false; 
	}

	Cmdline_d3d_lesstmem = !d3d_lesstmem_arg.found();

	if(batch_3dunlit_arg.found())
	{
		Cmdline_batch_3dunlit = 1;
	}

	if ( no_vbo_arg.found() )
	{
		Cmdline_novbo = 1;
	}

	if ( snd_preload_arg.found() )
	{
		Cmdline_snd_preload = 1;
	}

	if ( max_subdivide_arg.found() ) {
		Cmdline_max_subdivide = max_subdivide_arg.get_float();
	}else{
		Cmdline_max_subdivide = 0;
	}

	if ( alpha_env.found() ) {
		Cmdline_alpha_env = 1;
	}else{
		Cmdline_alpha_env = 0;
	}

	if ( env.found() ) {
		Cmdline_env = 1;
	}else{
		Cmdline_env = 0;
	}

	if ( decals.found() ) {
		Cmdline_decals = 1;
	}else{
		Cmdline_decals = 0;
	}

	if ( ballistic_gauge.found() ) {
		Cmdline_ballistic_gauge = 1;
	} else {
		Cmdline_ballistic_gauge = 0;
	}

	return true; 
}


int fred2_parse_cmdline(int argc, char *argv[])
{
	if (argc > 1) {
		// kind of silly -- combine arg list into single string for parsing,
		// but it fits with the win32-centric existing code.
		char *cmdline = NULL;
		unsigned int arglen = 0;
		int i;
		for (i = 1;  i < argc;  i++)
			arglen += strlen(argv[i]);
		if (argc > 2)
			arglen += argc - 2; // leave room for the separators
		cmdline = new char [arglen+1];
		i = 1;
		memset(cmdline, 0, arglen+1); // clear it out

		strcpy(cmdline, argv[i]);
		for (i=2; i < argc;  i++) {
			strcat(cmdline, " ");
			strcat(cmdline, argv[i]);
		}
		os_init_cmdline(cmdline);
		delete [] cmdline;
	} else {
		// no cmdline args
		os_init_cmdline("");
	}

	return SetCmdlineParams();
}

#ifdef _WIN32
int parse_cmdline(char *cmdline)
#else
int parse_cmdline(int argc, char *argv[])
#endif
{
   #ifdef _WIN32
	os_init_cmdline(cmdline);
   #else
	if (argc > 1) {
		// kind of silly -- combine arg list into single string for parsing,
		// but it fits with the win32-centric existing code.
		char *cmdline = NULL;
		unsigned int arglen = 0;
		int i;
		for (i = 1;  i < argc;  i++)
			arglen += strlen(argv[i]);
		if (argc > 2)
			arglen += argc - 2; // leave room for the separators
		cmdline = new char [arglen+1];
		i = 1;
		strcpy(cmdline, argv[i]);
		for ( ; i < argc;  i++) {
			strcat(cmdline, " ");
			strcat(cmdline, argv[i]);
		}
		os_init_cmdline(cmdline);
		delete [] cmdline;
	} else {
		// no cmdline args
		os_init_cmdline("");
	}
   #endif



	// --------------- Kazan -------------
	// If you're looking for the list of if (someparam.found()) { cmdline_someparam = something; } look above at this function
	// I did this because of fred2_parse_cmdline()
	return SetCmdlineParams();
}

//float global_scaleing_factor = 3.0f;

