/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/Graphics/GrD3DInternal.h $
 * $Revision: 2.55 $
 * $Date: 2006-09-11 06:36:38 $
 * $Author: taylor $
 *
 * Prototypes for the variables used internally by the Direct3D renderer
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.54  2006/05/27 17:07:48  taylor
 * remove grd3dparticle.* and grd3dbatch.*, they are obsolete
 * allow us to build without D3D support under Windows (just define NO_DIRECT3D)
 * clean up TMAP flags
 * fix a couple of minor OpenGL state change issues with spec and env map rendering
 * make sure we build again for OS X (OGL extension functions work a little different there)
 * render targets always need to be power-of-2 to avoid incomplete buffer issues in the code
 * when we disable culling in opengl_3dunlit be sure that we re-enable it on exit of function
 * re-fix screenshots
 * add true alpha blending support (with cmdline for now since the artwork has the catch up)
 * draw lines with float positioning, to be more accurate with resizing on non-standard resolutions
 * don't load cubemaps from file for D3D, not sure how to do it anyway
 * update geometry batcher code, memory fixes, dynamic stuff, basic fixage, etc.
 *
 * Revision 2.53  2006/04/15 00:13:22  phreak
 * gr_flash_alpha(), much like gr_flash(), but allows an alpha value to be passed
 *
 * Revision 2.52  2006/01/30 06:40:49  taylor
 * better lighting for OpenGL
 * remove some extra stuff that was from sectional bitmaps since we don't need it anymore
 * some basic lighting code cleanup
 *
 * Revision 2.51  2006/01/20 17:15:16  taylor
 * gr_*_bitmap_ex() stuff, D3D side is 100% untested to even compile
 * several other very minor changes as well
 *
 * Revision 2.50  2005/12/29 00:52:57  phreak
 * changed around aabitmap calls to accept a "mirror" parameter.  defaults to false, and is only true for mirrored briefing icons.
 * If the mirror param is true, then the picture is mirrored about the y-axis so left becomes right and vice versa.
 *
 * Revision 2.49  2005/12/06 02:53:02  taylor
 * clean up some D3D debug messages to better match new OGL messages (for easier debugging)
 * remove D3D_32bit variable since it's basically useless and the same thing can be done another way
 *
 * Revision 2.48  2005/11/13 06:44:18  taylor
 * small bit of EFF cleanup
 * add -img2dds support
 * cleanup some D3D stuff (missing a lot since the old code is so unstable I couldn't get it working like I wanted)
 * some minor OGL cleanup and small performance changes
 * converge the various pcx_read_bitmap* functions into one
 * cleanup/rename/remove some cmdline options
 *
 * Revision 2.47  2005/07/19 04:52:56  taylor
 * fix resize fixes
 *
 * Revision 2.46  2005/07/13 02:50:47  Goober5000
 * remove PreProcDefine #includes in FS2
 * --Goober5000
 *
 * Revision 2.45  2005/04/24 12:56:42  taylor
 * really are too many changes here:
 *  - remove all bitmap section support and fix problems with previous attempt
 *  ( code/bmpman/bmpman.cpp, code/bmpman/bmpman.h, code/globalincs/pstypes.h,
 *    code/graphics/2d.cpp, code/graphics/2d.h code/graphics/grd3dbmpman.cpp,
 *    code/graphics/grd3dinternal.h, code/graphics/grd3drender.cpp, code/graphics/grd3dtexture.cpp,
 *    code/graphics/grinternal.h, code/graphics/gropengl.cpp, code/graphics/gropengl.h,
 *    code/graphics/gropengllight.cpp, code/graphics/gropengltexture.cpp, code/graphics/gropengltexture.h,
 *    code/graphics/tmapper.h, code/network/multi_pinfo.cpp, code/radar/radarorb.cpp
 *    code/render/3ddraw.cpp )
 *  - use CLAMP() define in gropengl.h for gropengllight instead of single clamp() function
 *  - remove some old/outdated code from gropengl.cpp and gropengltexture.cpp
 *
 * Revision 2.44  2005/04/24 02:38:31  wmcoolmon
 * Moved gr_rect and gr_shade to be API-nonspecific as the OGL/D3D functions were virtually identical
 *
 * Revision 2.43  2005/04/05 05:53:17  taylor
 * s/vector/vec3d/g, better support for different compilers (Jens Granseuer)
 *
 * Revision 2.42  2005/03/20 00:09:07  phreak
 * Added gr_draw_htl_line and gr_draw_htl sphere
 * There still needs to be D3D versions implemented, but OGL is done.
 * Follow that or ask phreak about how its implemented/
 *
 * Revision 2.41  2005/03/07 13:10:21  bobboau
 * commit of render target code, d3d should be totaly functional,
 * OGL still needs implementation.
 *
 * Revision 2.40  2005/03/04 03:24:44  bobboau
 * made all the gr_d3d_tmapper variants use dynamic a vertex buffer rather
 * than drawprimitiveup, this also means that these functions can now
 * handle as much poly data as you might want to through at them
 *
 * Revision 2.39  2005/03/01 06:55:40  bobboau
 * oh, hey look I've commited something :D
 * animation system, weapon models detail box alt-tab bug, probly other stuff
 *
 * Revision 2.38  2005/02/27 10:38:06  wmcoolmon
 * Nonstandard res stuff
 *
 * Revision 2.37  2005/02/23 05:11:13  taylor
 * more consolidation of various graphics variables
 * some header cleaning
 * only one tmapper_internal for OGL, don't use more than two tex/pass now
 * seperate out 2d matrix mode to allow -2d_poof in OGL and maybe fix missing
 *    interface when set 3d matrix stuff doesn't have corresponding end
 * add dump_frame stuff for OGL, mostly useless but allows trailer recording
 *
 * Revision 2.36  2005/02/18 09:51:06  wmcoolmon
 * Updates for better nonstandard res support, as well as a fix to the Perseus crash bug I've been experiencing. Bobb, you might want to take a look at my change to grd3d.cpp
 *
 * Revision 2.35  2005/02/10 04:01:42  wmcoolmon
 * Low-level code for better hi-res support; better error reporting for vertex errors on model load.
 *
 * Revision 2.34  2005/02/04 23:29:31  taylor
 * merge with Linux/OSX tree - p0204-3
 *
 * Revision 2.33  2005/01/14 05:28:57  wmcoolmon
 * gr_curve
 *
 * Revision 2.32  2005/01/01 11:24:22  taylor
 * good OpenGL spec mapping
 * fix VBO crash with multitexture using same uv coord data
 * little speedup of opengl_tcache_frame()
 * error message to make sure hardware supports the minimum texture size
 * move OpenGL version check out of the extention printout code
 * disable 2d_poof with OpenGL
 *
 * Revision 2.31  2004/09/26 16:24:51  taylor
 * handle lost devices better, fix movie crash
 *
 * Revision 2.30  2004/08/11 05:06:24  Kazan
 * added preprocdefines.h to prevent what happened with fred -- make sure to make all fred2 headers include this file as the _first_ include -- i have already modified fs2 files to do this
 *
 * Revision 2.29  2004/07/05 05:09:19  bobboau
 * FVF code, only the data that is needed is sent off to the card,,
 * OGL can take advantage of this if they want but it won't break
 * anything if they don't. also state block code has been implemented,
 * that's totaly internal to D3D no high level code is involved.
 *
 * Revision 2.28  2004/06/28 02:13:07  bobboau
 * high level index buffer suport and d3d implementation,
 * OGL people need to get this working on your end as it's broke now
 *
 * Revision 2.27  2004/04/26 12:41:46  taylor
 * gr_preload() pointer for use in bmpman.cpp
 *
 * Revision 2.26  2004/04/11 13:56:33  randomtiger
 * Adding batching functions here and there and into gr_screen for use with OGL when its ready.
 *
 * Revision 2.25  2004/03/20 14:47:13  randomtiger
 * Added base for a general dynamic batching solution.
 * Fixed NO_DSHOW_CODE code path bug.
 *
 * Revision 2.24  2004/03/19 14:51:55  randomtiger
 * New command line parameter: -d3d_lesstmem causes D3D to bypass V's secondry texture system.
 *
 * Revision 2.23  2004/02/20 21:45:41  randomtiger
 * Removed some uneeded code between NO_DIRECT3D and added gr_zbias call, ogl is set to a stub func.
 * Changed -htl param to -nohtl. Fixed some badly named functions to match convention.
 * Fixed setup of center_alpha in OGL which was causing crash.
 *
 * Revision 2.22  2004/02/16 11:47:33  randomtiger
 * Removed a lot of files that we dont need anymore.
 * Changed htl to be on by default, command now -nohtl
 * Changed D3D to use a 2D vertex for 2D operations which should cut down on redundant data having to go though the system.
 * Added small change to all -start_mission flag to take you to any mission by filename, very useful for testing.
 * Removed old dshow code and took away timerbar compile flag condition since it uses a runtime flag now.
 *
 * Revision 2.21  2004/02/14 00:18:31  randomtiger
 * Please note that from now on OGL will only run with a registry set by Launcher v4. See forum for details.
 * OK, these changes effect a lot of file, I suggest everyone updates ASAP:
 * Removal of many files from project.
 * Removal of meanless Gr_bitmap_poly variable.
 * Removal of glide, directdraw, software modules all links to them, and all code specific to those paths.
 * Removal of redundant Fred paths that arent needed for Fred OGL.
 * Have seriously tidied the graphics initialisation code and added generic non standard mode functionality.
 * Fixed many D3D non standard mode bugs and brought OGL up to the same level.
 * Removed texture section support for D3D8, voodoo 2 and 3 cards will no longer run under fs2_open in D3D, same goes for any card with a maximum texture size less than 1024.
 *
 * Revision 2.20  2004/01/26 20:03:51  randomtiger
 * Fix to blurring of interface bitmaps from TGA and JPG.
 * Changes to the pointsprite system, better but not perfect yet.
 *
 * Revision 2.19  2004/01/24 14:31:27  randomtiger
 * Added the D3D particle code, its not bugfree but works perfectly on my card and helps with the framerate.
 * Its optional and off by default, use -d3d_particle to activiate.
 * Also bumped up D3D ambient light setting, it was way too dark.
 * Its now set to something similar to the original game.
 *
 * Revision 2.18  2003/12/08 22:30:02  randomtiger
 * Put render state and other direct D3D calls repetition check back in, provides speed boost.
 * Fixed bug that caused fullscreen only crash with DXT textures
 * Put dithering back in for tgas and jpgs
 *
 * Revision 2.17  2003/12/05 18:17:06  randomtiger
 * D3D now supports loading for DXT1-5 into the texture itself, defaults to on same as OGL.
 * Fixed bug in old ship choice screen that stopped ani repeating.
 * Changed all builds (demo, OEM) to use retail reg path, this means launcher can set all them up successfully.
 *
 * Revision 2.16  2003/11/29 10:52:09  randomtiger
 * Turned off D3D file mapping, its using too much memory which may be hurting older systems and doesnt seem to be providing much of a speed benifit.
 * Added stats command for ingame stats on memory usage.
 * Trys to play intro.mve and intro.avi, just to be safe since its not set by table.
 * Added fix for fonts wrapping round in non standard hi res modes.
 * Changed D3D mipmapping to a good value to suit htl mode.
 * Added new fog colour method which makes use of the bitmap, making this htl feature backcompatible again.
 *
 * Revision 2.15  2003/11/19 20:37:24  randomtiger
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
 * Revision 2.14  2003/11/07 18:31:02  randomtiger
 * Fixed a nohtl call to htl funcs (crash with NULL pointer)
 * Fixed a bug with 32bit PCX code.
 * Fixed a bug in the d3d_string batch system that was messing up screen shaking.
 * Added a couple of checks to try and stop timerbar push and pop overloads, check returns missing pops if you use the system.
 * Put in a higher res icon until we get something better sorted out.
 *
 * Revision 2.13  2003/11/06 21:10:26  randomtiger
 * Added my batching solution for more efficient d3d_string.
 * Its part of the new grd3dbatch module, most of this isnt in use but it might help out later so I've left it in.
 *
 * Revision 2.12  2003/11/01 21:59:21  bobboau
 * new matrix handeling code, and fixed some problems with 3D lit verts,
 * several other small fixes
 *
 * Revision 2.11  2003/10/24 17:35:05  randomtiger
 * Implemented support for 32bit TGA and JPG for D3D
 * Also 32 bit PCX, but it still has some bugs to be worked out
 * Moved convert_24_to_16 out of the bitmap pfunction structures and into packunpack.cpp because thats the only place that uses it.
 *
 * Revision 2.10  2003/10/17 17:18:42  randomtiger
 * Big restructure for D3D and new modules grd3dlight and grd3dsetup
 *
 * Revision 2.9  2003/10/16 00:17:14  randomtiger
 * Added incomplete code to allow selection of non-standard modes in D3D (requires new launcher).
 * As well as initialised in a different mode, bitmaps are stretched and for these modes
 * previously point filtered textures now use linear to keep them smooth.
 * I also had to shuffle some of the GR_1024 a bit.
 * Put my HT&L flags in ready for my work to sort out some of the render order issues.
 * Tided some other stuff up.
 *
 * Revision 2.8  2003/10/14 17:39:13  randomtiger
 * Implemented hardware fog for the HT&L code path.
 * It doesnt use the backgrounds anymore but its still an improvement.
 * Currently it fogs to a brighter colour than it should because of Bob specular code.
 * I will fix this after discussing it with Bob.
 *
 * Also tided up some D3D stuff, a cmdline variable name and changed a small bit of
 * the htl code to use the existing D3D engine instead of work around it.
 * And added extra information in version number on bottom left of frontend screen.
 *
 * Revision 2.7  2003/09/26 14:37:14  bobboau
 * commiting Hardware T&L code, everything is ifdefed out with the compile flag HTL
 * still needs a lot of work, ubt the frame rates were getting with it are incredable
 * the biggest problem it still has is a bad lightmanegment system, and the zbuffer
 * doesn't work well with things still getting rendered useing the sofware pipeline, like thrusters,
 * and weapons, I think these should be modifyed to be sent through hardware,
 * it would be slightly faster and it would likely fix the problem
 *
 * also the thruster glow/particle stuff I did is now in.
 *
 * Revision 2.6  2003/08/22 07:35:08  bobboau
 * specular code should be bugless now,
 * cell shadeing has been added activated via the comand line '-cell',
 * 3D shockwave models, and a transparency method I'm calling edge and center alpha that could be usefull for other things, ask for details
 *
 * Revision 2.5  2003/08/16 03:52:23  bobboau
 * update for the specmapping code includeing
 * suport for seperate specular levels on lights and
 * optional strings for the stars table
 * code has been made more organised,
 * though there seems to be a bug in the state selecting code
 * resulting in the HUD being rendered incorectly
 * and specmapping failing ocasionaly
 *
 * Revision 2.4  2003/07/04 02:27:48  phreak
 * added support for cloaking.
 * i will need to contact someone who knows d3d to get this to work
 *
 * Revision 2.3  2003/03/18 10:07:02  unknownplayer
 * The big DX/main line merge. This has been uploaded to the main CVS since I can't manage to get it to upload to the DX branch. Apologies to all who may be affected adversely, but I'll work to debug it as fast as I can.
 *
 * Revision 2.2  2002/10/05 16:46:09  randomtiger
 * Added us fs2_open people to the credits. Worth looking at just for that.
 * Added timer bar code, by default its not compiled in.
 * Use TIMEBAR_ACTIVE in project and dependancy code settings to activate.
 * Added the new timebar files with the new code.
 *
 * Revision 2.1.2.9  2002/11/11 21:26:04  randomtiger
 *
 * Tided up D3DX8 calls, did some documentation and add new file: grd3dcalls.cpp. - RT
 *
 * Revision 2.1.2.8  2002/11/04 21:24:59  randomtiger
 *
 * When running in D3D all ani's are memory mapped for speed, this takes up more memory but stops gametime locking of textures which D3D8 hates.
 * Added new command line tag Cmdline_d3dlowmem for people who dont want to make use of this because they have no memory.
 * Cleaned up some more texture stuff enabled console debug for D3D.
 *
 * Revision 2.1.2.7  2002/10/30 22:57:21  randomtiger
 *
 * Changed DX8 code to not use set render and texture states if they are already set to that value.
 * Disabled buffer saving and restoring code when windowed to stop DX8 debug runs from crashing. - RT
 *
 * Revision 2.1.2.6  2002/10/21 16:33:41  randomtiger
 * Added D3D only 32 bit TGA functionality. Will load a texture as big as your graphics card allows. Code not finished yet and will forge the beginnings of the new texture system. - RT
 *
 * Revision 2.1.2.5  2002/10/11 18:50:54  randomtiger
 * Checked in fix for 16 bit problem, thanks to Righteous1
 * Removed a fair bit of code that was used by the 16 bit code path which no longer exists.
 * 32 bit and 16 bit should now work in exactly the same way. - RT
 *
 * Revision 2.1.2.4  2002/10/04 00:48:42  randomtiger
 * Fixed video memory leaks
 * Added code to cope with lost device, not tested
 * Got rid of some DX5 stuff we definately dont need
 * Moved some enum's into internal,h because gr_d3d_set_state should be able to be called from any dx file
 * Cleaned up some stuff - RT
 *
 * Revision 2.1.2.3  2002/10/02 11:40:19  randomtiger
 * Bmpmap has been reverted to an old non d3d8 version.
 * All d3d8 code is now in the proper place.
 * PCX code is now working to an extent. Problems with alpha though.
 * Ani's work slowly with alpha problems.
 * Also I have done a bit of tidying - RT
 *
 * Revision 2.1.2.2  2002/09/28 22:13:43  randomtiger
 * Sorted out some bits and pieces. The background nebula blends now which is nice. � RT
 *
 * Revision 2.1.2.1  2002/09/24 18:56:42  randomtiger
 * DX8 branch commit
 *
 * This is the scub of UP's previous code with the more up to date RT code.
 * For full details check previous dev e-mails
 *
 * Revision 2.1  2002/08/01 01:41:05  penguin
 * The big include file move
 *
 * Revision 2.0  2002/06/03 04:02:22  penguin
 * Warpcore CVS sync
 *
 * Revision 1.1  2002/05/02 18:03:07  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 5     7/13/99 1:15p Dave
 * 32 bit support. Whee!
 * 
 * 4     7/09/99 9:51a Dave
 * Added thick polyline code.
 * 
 * 3     6/29/99 10:35a Dave
 * Interface polygon bitmaps! Whee!
 * 
 * 2     10/07/98 10:52a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 21    5/23/98 4:14p John
 * Added code to preload textures to video card for AGP.   Added in code
 * to page in some bitmaps that weren't getting paged in at level start.
 * 
 * 20    5/20/98 9:45p John
 * added code so the places in code that change half the palette don't
 * have to clear the screen.
 * 
 * 19    5/12/98 10:34a John
 * Added d3d_shade functionality.  Added d3d_flush function, since the
 * shader seems to get reorganzed behind the overlay text stuff!
 * 
 * 18    5/12/98 8:18a John
 * Put in code to use a different texture format for alpha textures and
 * normal textures.   Turned off filtering for aabitmaps.  Took out
 * destblend=invsrccolor alpha mode that doesn't work on riva128. 
 * 
 * 17    5/11/98 10:19a John
 * Added caps checking
 * 
 * 16    5/07/98 3:02p John
 * Mpre texture cleanup.   You can now reinit d3d without a crash.
 * 
 * 15    5/07/98 9:54a John
 * Added in palette flash functionallity.
 * 
 * 14    5/06/98 11:21p John
 * Fixed a bitmap bug with Direct3D.  Started adding new caching code into
 * D3D.
 * 
 * 13    5/06/98 8:41p John
 * Fixed some font clipping bugs.   Moved texture handle set code for d3d
 * into the texture module.
 * 
 * 12    5/06/98 5:30p John
 * Removed unused cfilearchiver.  Removed/replaced some unused/little used
 * graphics functions, namely gradient_h and _v and pixel_sp.   Put in new
 * DirectX header files and libs that fixed the Direct3D alpha blending
 * problems.
 * 
 * 11    5/05/98 10:37p John
 * Added code to optionally use execute buffers.
 * 
 * 10    5/04/98 3:36p John
 * Got zbuffering working with Direct3D.
 * 
 * 9     4/14/98 12:15p John
 * Made 16-bpp movies work.
 * 
 * 8     3/12/98 5:36p John
 * Took out any unused shaders.  Made shader code take rgbc instead of
 * matrix and vector since noone used it like a matrix and it would have
 * been impossible to do in hardware.   Made Glide implement a basic
 * shader for online help.  
 * 
 * 7     3/10/98 4:18p John
 * Cleaned up graphics lib.  Took out most unused gr functions.   Made D3D
 * & Glide have popups and print screen.  Took out all >8bpp software
 * support.  Made Fred zbuffer.  Made zbuffer allocate dynamically to
 * support Fred.  Made zbuffering key off of functions rather than one
 * global variable.
 * 
 * 6     3/09/98 6:06p John
 * Restructured font stuff to avoid duplicate code in Direct3D and Glide.
 * Restructured Glide to avoid redundent state setting.
 * 
 * 5     3/08/98 12:33p John
 * Made d3d cleanup free textures.  Made d3d always divide texture size by
 * 2 for now.
 * 
 * 4     3/07/98 8:29p John
 * Put in some Direct3D features.  Transparency on bitmaps.  Made fonts &
 * aabitmaps render nice.
 * 
 * 3     2/17/98 7:28p John
 * Got fonts and texturing working in Direct3D
 * 
 * 2     2/07/98 7:50p John
 * Added code so that we can use the old blending type of alphacolors if
 * we want to.  Made the stars use them.
 * 
 * 1     2/03/98 9:24p John
 *
 * $NoKeywords: $
 */

#ifndef _GRD3DINTERNAL_H
#define _GRD3DINTERNAL_H

#ifndef NO_DIRECT3D

#include <windows.h>

#include <d3d8.h>
#include "graphics/2d.h"
#include "graphics/grinternal.h"


// easy macro for doing a Release() on d3d stuff
// we give release three chances and Assert() if there is still a reference
//   x	==  the texture/surface/etc. that you want to release
//   i  ==  the integer that will get the ref count (must be declared in calling function)
#define D3D_RELEASE(x, i)	{		\
	(i) = 0;						\
	if ( (x) != NULL ) {			\
		(i) = (x)->Release();		\
		if ( (i) > 0 ) {			\
			(i) = (x)->Release();	\
		}							\
		if ( (i) > 0 ) {			\
			(i) = (x)->Release();	\
		}							\
		Assert( (i) < 1 );			\
		(x) = NULL;					\
	}								\
}

/* Structures */

// Keeping the globals in here now to try and keep some order
typedef struct {

	static IDirect3D8 *lpD3D;
	static IDirect3DDevice8 *lpD3DDevice;
	static D3DCAPS8 d3d_caps;
	static D3DPRESENT_PARAMETERS d3dpp; 

	static bool D3D_inited;
	static bool D3D_activate;
	static bool D3D_Antialiasing;
	static bool D3D_window;

	static int D3D_rendition_uvs;
	static int D3D_zbias;

	static int unlit_3D_batch;

	static float texture_adjust_u;
	static float texture_adjust_v;

} GlobalD3DVars;

/*
 * Stolen definion of DDPIXELFORMAT
 */
typedef struct
{
    bool	dw_is_alpha_mode;	// pixel format flags
	DWORD	dwRGBBitCount;		// how many bits per pixel
	DWORD	dwRBitMask;			// mask for red bit
	DWORD	dwGBitMask;			// mask for green bits
	DWORD	dwBBitMask;			// mask for blue bits
	DWORD	dwRGBAlphaBitMask;	// mask for alpha channel

} PIXELFORMAT;

typedef struct tcache_slot_d3d {

	IDirect3DTexture8 *d3d8_thandle;

	float						u_scale, v_scale;
	int							bitmap_id;
	int							size;
	char						used_this_frame;
	int							time_created;
	ushort						w, h;
} tcache_slot_d3d;

// This vertex type tells D3D that it has already been transformed an lit
// D3D will simply display the polygon with no extra processing
typedef struct { 
    float sx, sy, sz; 
	float rhw; 
    DWORD color; 
    float tu, tv; 

} D3DVERTEX2D;

// This vertex type tells D3D that it has already been transformed an lit
// D3D will simply display the polygon with no extra processing
typedef struct { 
    float sx, sy, sz; 
	float rhw; 

    DWORD color; 
    DWORD specular; 
    float tu, tv; 
    float env_u, env_v; 

} D3DTLVERTEX;

// This vertex type should be used for vertices that have already been lit
// make sure lighting is set to off while these polygons are rendered 
//(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)
typedef struct { 
    float sx, sy, sz;
  
    DWORD color; 
    DWORD specular; 
    float tu, tv; 

} D3DLVERTEX;

// Renders a normal polygon that is to be transformed and lit by D3D
typedef struct { 
    float sx, sy, sz;
  	float nx, ny, nz;

    float tu, tv, tu2, tv2; 

} D3DVERTEX;

typedef struct {
	float x, y, z;
	float size;
	// Warning this custom value is not in use by D3D
	DWORD custom;
} D3DPOINTVERTEX;

typedef struct {
	int fvf;
	int size;

} VertexTypeInfo;

/* Enums and typedefs */

enum
{
	D3DVT_VERTEX2D,
	D3DVT_TLVERTEX,
	D3DVT_LVERTEX,
	D3DVT_VERTEX,
	D3DVT_PVERTEX,
	D3DVT_MAX
};

typedef float D3DVALUE;

/* External vars - booo! */

// 16 bit formats for pcx media
extern D3DFORMAT default_non_alpha_tformat;
extern D3DFORMAT default_alpha_tformat;
extern D3DFORMAT default_compressed_format;

extern PIXELFORMAT AlphaTextureFormat;
extern PIXELFORMAT NonAlphaTextureFormat;

/* Functions */
/*
void set_stage_for_defuse();
void set_stage_for_glow_mapped_defuse();
void set_stage_for_defuse_and_non_mapped_spec();
void set_stage_for_glow_mapped_defuse_and_non_mapped_spec();
bool set_stage_for_spec_mapped();
void set_stage_for_cell_shaded();
void set_stage_for_cell_glowmapped_shaded();
void set_stage_for_additive_glowmapped();
*/
void d3d_tcache_init();
void d3d_tcache_cleanup();
void d3d_tcache_flush();
void d3d_tcache_frame();

void d3d_flush();

int d3d_tcache_set(int bitmap_id, int bitmap_type, float *u_ratio, float *v_ratio, int fail_on_full=0, int force = 0, int stage = 0);
int d3d_tcache_set_internal(int bitmap_id, int bitmap_type, float *u_ratio, float *v_ratio, int fail_on_full=0, int force = 0, int stage = 0);

// Functions in GrD3DRender.cpp stuffed into gr_screen structure
void gr_d3d_flash(int r, int g, int b);
void gr_d3d_flash_alpha(int r, int g, int b, int a);
void gr_d3d_zbuffer_clear(int mode);
int gr_d3d_zbuffer_get();
int gr_d3d_zbuffer_set(int mode);
void gr_d3d_tmapper( int nverts, vertex **verts, uint flags );
void gr_d3d_scaler(vertex *va, vertex *vb );
void gr_d3d_aascaler(vertex *va, vertex *vb );
void gr_d3d_pixel(int x, int y, bool resize);
void gr_d3d_clear();
void gr_d3d_set_clip(int x,int y,int w,int h,bool resize);
void gr_d3d_reset_clip();
void gr_d3d_set_bitmap( int bitmap_num, int alphablend_mode, int bitblt_mode, float alpha );
void gr_d3d_bitmap(int x, int y);
void gr_d3d_bitmap_ex(int x, int y, int w, int h, int sx, int sy, bool resize);
void gr_d3d_aabitmap_ex(int x,int y,int w,int h,int sx,int sy,bool resize, bool mirror);
void gr_d3d_aabitmap(int x, int y, bool resize, bool mirror);
//void gr_d3d_rect(int x,int y,int w,int h,bool resize);
//void gr_d3d_shade(int x,int y,int w,int h);
void gr_d3d_create_font_bitmap();
void gr_d3d_char(int x,int y,int letter);
void gr_d3d_string( int sx, int sy, char *s, bool resize = true );
void gr_d3d_circle( int xc, int yc, int d, bool resize = true );
void gr_d3d_curve( int xc, int yc, int r, int direction );
void gr_d3d_line(int x1,int y1,int x2,int y2, bool resize = true);
void gr_d3d_aaline(vertex *v1, vertex *v2);
void gr_d3d_gradient(int x1,int y1,int x2,int y2, bool resize = true);
void gr_d3d_set_palette(ubyte *new_palette, int restrict_alphacolor);
void gr_d3d_diamond(int x, int y, int width, int height);
void gr_d3d_print_screen(char *filename);
void gr_d3d_push_texture_matrix(int unit);
void gr_d3d_pop_texture_matrix(int unit);
void gr_d3d_translate_texture_matrix(int unit, vec3d *shift);
void gr_d3d_zbias(int zbias);
void gr_d3d_set_fill_mode(int mode);

void gr_d3d_draw_htl_line(vec3d *start, vec3d* end);
void gr_d3d_draw_htl_sphere(float rad);

void d3d_render_timer_bar(int colour, float x, float y, float w, float h);

// GrD3DRender functions
void gr_d3d_set_state( gr_texture_source ts, gr_alpha_blend ab, gr_zbuffer_type zt );
void gr_d3d_tmapper_internal_batch_3d_unlit( int nverts, vertex *verts, uint flags);	

// GrD3DCall functions
void d3d_reset_render_states();
HRESULT d3d_SetRenderState( D3DRENDERSTATETYPE render_state_type,  DWORD render_state, bool set = true, bool init = false );
HRESULT d3d_DrawPrimitive(int vertex_type, D3DPRIMITIVETYPE prim_type, LPVOID pvertices, DWORD vertex_count);
void d3d_reset_texture_stage_states();
HRESULT d3d_SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value, bool set = true, bool init = false);
BOOL d3d_lost_device(bool force = false);
HRESULT d3d_SetTexture(int stage, IDirect3DBaseTexture8* texture_ptr);
HRESULT d3d_SetVertexShader(uint vertex_type);
HRESULT d3d_CreateVertexBuffer(int vertex_type, int size, DWORD usage, void **buffer);
int d3d_get_num_prims(int vertex_count, D3DPRIMITIVETYPE prim_type);

// GrD3Dtexture
int gr_d3d_preload(int bitmap_num, int is_aabitmap );

bool d3d_init_light();
int d3d_create_texture(int bitmap_handle, int bitmap_type, tcache_slot_d3d *tslot, int fail_on_full);

extern D3DCOLOR ambient_light;
extern VertexTypeInfo vertex_types[];

struct dynamic_buffer{
	dynamic_buffer():size(0),buffer(NULL){}
	~dynamic_buffer(){if(buffer)buffer->Release();}

	void allocate(int n_verts, int type);
	void allocate(int n_verts, uint _fvf, int _size);

	void lock(ubyte**v, int v_type){
		fvf = vertex_types[v_type].fvf;
		vsize = vertex_types[v_type].size;
		buffer->Lock(0, 0, v, D3DLOCK_DISCARD);
	}
	void lock(ubyte**v, uint FVF, int SIZE){
		fvf = FVF;
		vsize = SIZE;
		buffer->Lock(0, 0, v, D3DLOCK_DISCARD);
	}
	void unlock();
	void lost(){
		if(buffer)buffer->Release();
		size = 0;
		buffer = NULL;
	}
	void draw(_D3DPRIMITIVETYPE TYPE, int num);
	uint fvf;
	int vsize;
	int size;
	IDirect3DVertexBuffer8 *buffer;
};

extern dynamic_buffer render_buffer;

#endif // !NO_DIRECT3D

#endif //_GRD3DINTERNAL_H
