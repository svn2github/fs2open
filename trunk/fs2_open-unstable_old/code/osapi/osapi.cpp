/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/* 
 * $Logfile: /Freespace2/code/OsApi/OsApi.cpp $
 * $Revision: 2.39 $
 * $Date: 2007-09-03 22:19:29 $
 * $Author: Goober5000 $
 *
 * Low level Windows code
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.38  2007/01/07 13:15:42  taylor
 * fix up Windows window/cursor handling so that it's up-to-date and more compatible
 * make sure that we handle background clearing ourselves, fixes several little issues
 * fix popup warnings to work a bit better and not screw up the game so much or make it difficult to interact with them
 *
 * Revision 2.37  2006/12/28 00:59:39  wmcoolmon
 * WMC codebase commit. See pre-commit build thread for details on changes.
 *
 * Revision 2.36  2006/11/16 00:54:15  taylor
 * new window creation code for Windows (gets rid of some stupid :V: voodoo for the software/glide stuff)
 * use detect_home() on Windows too (needed for now outwnd code)
 * some bits of minor cleanup
 *
 * Revision 2.35  2006/11/06 06:23:27  taylor
 * grrr ... fix dos EOL chars
 *
 * Revision 2.34  2006/07/13 22:15:02  taylor
 * handle non-MVE movies a bit better in OpenGL (don't get freaky with the window, don't lose input, etc.)
 * some cleanup to OpenGL window handling, to fix min/max/full issues, and try to make shutdown a little nicer
 *
 * Revision 2.33  2006/01/26 03:23:30  Goober5000
 * pare down the pragmas some more
 * --Goober5000
 *
 * Revision 2.32  2006/01/10 18:37:46  randomtiger
 * Improvements to voice recognition system.
 * Also function put on -voicer launcher option.
 *
 * Revision 2.31  2006/01/03 17:07:11  randomtiger
 * Added voice recognition functionality for Visual C6 project only.
 * Currently still a work in progress.
 *
 * Revision 2.30  2005/07/31 01:30:48  taylor
 * print file and line info for Int3() calls to the logfile so that they are easier to debug with basic error reports
 *
 * Revision 2.29  2005/06/03 06:39:27  taylor
 * better audio pause/unpause support when game window loses focus or is minimized
 *
 * Revision 2.28  2005/05/01 23:17:46  phreak
 * when creating the window take into account the top border
 *
 * Revision 2.27  2005/04/24 03:06:29  phreak
 * centered the window on the desktop if the -window command line is used.
 *
 * Revision 2.26  2005/03/08 03:50:24  Goober5000
 * edited for language ;)
 * --Goober5000
 *
 * Revision 2.25  2005/03/03 16:18:19  taylor
 * lockup fixes, and it's Linux friendly too :)
 *
 * Revision 2.24  2005/03/01 06:55:42  bobboau
 * oh, hey look I've commited something :D
 * animation system, weapon models detail box alt-tab bug, probly other stuff
 *
 * Revision 2.23  2005/02/04 20:06:06  taylor
 * merge with Linux/OSX tree - p0204-2
 *
 * Revision 2.22  2004/07/26 20:47:46  Kazan
 * remove MCD complete
 *
 * Revision 2.21  2004/07/25 00:31:29  Kazan
 * i have absolutely nothing to say about that subject
 *
 * Revision 2.20  2004/07/12 16:33:00  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 2.19  2004/04/07 03:32:45  righteous1
 * Updated to call alt_tab_pause() function to draw pause screen and discontinue sounds when the game is minimized. -R1
 *
 * Revision 2.18  2004/03/31 05:42:28  Goober5000
 * got rid of all those nasty warnings from xlocale and so forth; also added comments
 * to indicate which warnings were being disabled
 * --Goober5000
 *
 * Revision 2.17  2004/03/05 09:02:08  Goober5000
 * Uber pass at reducing #includes
 * --Goober5000
 *
 * Revision 2.16  2004/02/16 16:15:04  phreak
 * grr fixed a warning message i didn't see
 *
 * Revision 2.15  2004/02/16 16:09:59  phreak
 * made the window command line tag recognize non-standard resolutions
 *
 * Revision 2.14  2004/02/14 00:18:35  randomtiger
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
 * Revision 2.13  2004/01/24 15:52:26  randomtiger
 * I have submitted the new movie playing code despite the fact in D3D it sometimes plays behind the main window.
 * In OGL it works perfectly and in both API's it doesnt switch to the desktop anymore so hopefully people will not experience the crashes etc that the old system used to suffer from.
 *
 * Revision 2.12  2004/01/17 21:59:54  randomtiger
 * Some small changes to the main codebase that allow Fred_open OGL to compile.
 *
 * Revision 2.11  2003/10/26 00:31:59  randomtiger
 * Fixed hulls not drawing (with Phreaks advise).
 * Put my 32bit PCX loading under PCX_32 compile flag until its working.
 * Fixed a bug with res 640x480 I introduced with my non standard mode code.
 * Changed JPG and TGA loading command line param to "-t32"
 *
 * Revision 2.10  2003/10/16 00:17:18  randomtiger
 * Added incomplete code to allow selection of non-standard modes in D3D (requires new launcher).
 * As well as initialised in a different mode, bitmaps are stretched and for these modes
 * previously point filtered textures now use linear to keep them smooth.
 * I also had to shuffle some of the GR_1024 a bit.
 * Put my HT&L flags in ready for my work to sort out some of the render order issues.
 * Tided some other stuff up.
 *
 * Revision 2.9  2003/08/21 20:54:38  randomtiger
 * Fixed switching - RT
 *
 * Revision 2.8  2003/07/06 00:19:25  randomtiger
 * Random Tiger 6/7/2003
 *
 * fs2_open now uses the registry entry 'VideocardFs2open' instead of 'Videocard' to store its video settings. To run fs2_open now you MUST use the launcher I have provided.
 *
 * Launcher binary:      http://mysite.freeserve.com/thomaswhittaker/c_code/freespace/Launcher.rar
 * Launcher source code: http://mysite.freeserve.com/thomaswhittaker/c_code/freespace/Launcher_code.rar
 *
 * I have also taken the opertunity to fix a few bugs in the launcher and add a new feature to make selecting mods a bit easier.
 *
 * The launcher now uses some files in the freespace project so it should be put into CVS with the rest of the code inside the 'code' directory (still in its 'Launcher' dir of course). Currently the launcher wont compile since speech.cpp and speech.h arent in cvs yet. But once Roee has checked in that will be sorted.
 *
 * I have also removed the internal launcher from the D3D8 module.
 * Please contact me if you have any problems.
 *
 * When trying to run the exe after updating I get an error parsing 'rank.tbl' but im fairly sure thats nothing to do with me so I'll just have to leave it for now because I'm still using a 56K modem and cant afford to find out.
 *
 * Revision 2.7  2003/03/18 10:07:05  unknownplayer
 * The big DX/main line merge. This has been uploaded to the main CVS since I can't manage to get it to upload to the DX branch. Apologies to all who may be affected adversely, but I'll work to debug it as fast as I can.
 *
 * Revision 2.6  2003/03/02 06:01:55  penguin
 * Added #ifdef WIN32 and #ifdef MSC_VER
 *  - penguin
 * Revision 2.2.2.2  2002/10/03 08:32:08  unknownplayer
 *
 * Hacked in a windowed mode so we can properly debug this without using
 * monitors (although I drool at the concept of having that!)
 *
 * Revision 2.2.2.1  2002/08/28 12:39:36  randomtiger
 * OK, this should be a commit to the DX branch or Im going to be in a lot of trouble.
 * The movie and dx8show files have been cleaned up big time.
 * My debug system is in but has NO EFFECT at all unless a compiler flag is turned on, check h file for details.
 * Aside from that a few changes to help the movie code work properly.
 * Works on most things including GF4 and Voodoo 3. However may not work properly on a voodoo 2.
 * Im going to leave this as a bug for now, serves you right for buying voodoo!
 *
 * Revision 2.2  2002/08/01 01:41:09  penguin
 * The big include file move
 *
 * Revision 2.1  2002/07/07 19:55:59  penguin
 * Back-port to MSVC
 *
 * Revision 2.5  2003/02/22 04:14:21  wmcoolmon
 * Added "os_app_activate_set"
 *
 * Revision 2.4  2003/01/07 00:02:08  phreak
 * fixed problem in win32_create_window() that disabled OpenGL from running in windowed mode
 *
 * Revision 2.3  2002/11/14 04:18:17  bobboau
 * added warp model and type 1 glow points
 * and well as made the new glow file type,
 * some general improvement to fighter beams,
 * 
 * 
 * 7     6/30/99 5:53p Dave
 * Put in new anti-camper code.
 * 
 * 6     6/03/99 6:37p Dave
 * More TNT fun. Made perspective bitmaps more flexible.
 * 
 * 5     6/02/99 6:18p Dave
 * Fixed TNT lockup problems! Wheeeee!
 * 
 * 4     12/18/98 1:13a Dave
 * Rough 1024x768 support for Direct3D. Proper detection and usage through
 * the launcher.
 * 
 * 3     10/09/98 2:57p Dave
 * Starting splitting up OS stuff.
 * 
 * 2     10/08/98 2:38p Dave
 * Cleanup up OsAPI code significantly. Removed old functions, centralized
 * registry functions.
 * 
 * 118   7/10/98 5:04p Dave
 * Fix connection speed bug on standalone server.
 * 
 * 117   5/24/98 2:28p Hoffoss
 * Because we never really care about if the left or the right shift or
 * alt key was used, but rather than either shift or alt was used, made
 * both map to the left one.  Solves some problems, causes none.
 * 
 * 116   5/18/98 9:22p John
 * Took out the annoying tab check.
 * 
 * 115   5/18/98 11:17a John
 * Fixed some bugs with software window and output window.
 * 
 * 114   5/16/98 2:20p John
 * Changed the os_suspend and resume to use a critical section to prevent
 * threads from executing rather than just suspending the thread.  Had to
 * make sure gr_close was called before os_close.
 * 
 * 113   5/15/98 4:49p John
 * 
 * 112   5/15/98 3:36p John
 * Fixed bug with new graphics window code and standalone server.  Made
 * hwndApp not be a global anymore.
 * 
 * 111   5/14/98 5:42p John
 * Revamped the whole window position/mouse code for the graphics windows.
 * 
 * 110   5/04/98 11:08p Hoffoss
 * Expanded on Force Feedback code, and moved it all into Joy_ff.cpp.
 * Updated references everywhere to it.
 *
 * $NoKeywords: $
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <winsock.h>
#include <stdarg.h>
#include <direct.h>

#include "globalincs/pstypes.h"
#include "io/key.h"
#include "palman/palman.h"
#include "io/mouse.h"
#include "osapi/outwnd.h"
#include "sound/sound.h"
#include "freespace2/freespaceresource.h"
#include "playerman/managepilot.h"
#include "io/joy.h"
#include "io/joy_ff.h"
#include "gamesequence/gamesequence.h"
#include "freespace2/freespace.h"
#include "osapi/osregistry.h"
#include "cmdline/cmdline.h"
#include "sound/voicerec.h"
#include "graphics/2d.h"

#define THREADED	// to use the proper set of macros
#include "osapi/osapi.h"


// used to be a THREADED define but only use multiple process threads if this is defined
// NOTE: may hang if set
//#define THREADED_PROCESS


// ----------------------------------------------------------------------------------------------------
// OSAPI DEFINES/VARS
//

// os-wide globals
static HINSTANCE	hInstApp;
static HWND			hwndApp = NULL;
static int			fAppActive = 0;
static int			fOldAppActive = 0;
static int			main_window_inited = 0;
static char			szWinTitle[128];
static char			szWinClass[128];
static HANDLE		hThread=NULL;
static DWORD		ThreadID;
static int			WinX, WinY, WinW, WinH;
static int			Os_inited = 0;

static CRITICAL_SECTION Os_lock;

int Os_debugger_running = 0;

// ----------------------------------------------------------------------------------------------------
// OSAPI FORWARD DECLARATIONS
//

#ifdef THREADED_PROCESS
	// thread handler for the main message thread
	DWORD win32_process(DWORD lparam);
#else
	DWORD win32_process1(DWORD lparam);
	DWORD win32_process2(DWORD lparam);
#endif

// Fills in the Os_debugger_running with non-zero if debugger detected.
void os_check_debugger();

// called at shutdown. Makes sure all thread processing terminates.
void os_deinit();

// go through all windows and try and find the one that matches the search string
BOOL __stdcall os_enum_windows( HWND hwnd, char * search_string );

// message handler for the main thread
LRESULT CALLBACK win32_message_handler(HWND hwnd,UINT msg,WPARAM wParam, LPARAM lParam);


// ----------------------------------------------------------------------------------------------------
// OSAPI FUNCTIONS
//

// detect home/base directory  (placeholder for possible future Win32 userdir support, just returns current directory for now)
char Cur_path[MAX_PATH_LEN];
const char *detect_home(void)
{
	if ( strlen(Cfile_root_dir) )
		return Cfile_root_dir;

	memset( Cur_path, 0, MAX_PATH_LEN );
	GetCurrentDirectory( MAX_PATH_LEN-1, Cur_path );

	return Cur_path;
}

// initialization/shutdown functions -----------------------------------------------

// If app_name is NULL or ommited, then TITLE is used
// for the app name, which is where registry keys are stored.
void os_init(char * wclass, char * title, char *app_name, char *version_string )
{
#ifndef NDEBUG
	outwnd_init(1);
#endif

	os_init_registry_stuff(Osreg_company_name, title, version_string);

	strcpy( szWinTitle, title );
	strcpy( szWinClass, wclass );	

	INITIALIZE_CRITICAL_SECTION( Os_lock );

	#ifdef THREADED_PROCESS
		// Create an even to signal that the window is created, 
		// so that we don't return from this function until 
		// the window is all properly created.
		HANDLE Window_created = CreateEvent( NULL, FALSE, FALSE, NULL );
		hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)win32_process, Window_created, 0, &ThreadID );
		if ( WaitForSingleObject( Window_created, 5000 )==WAIT_TIMEOUT)	{			//INFINITE );
			mprintf(( "Wait timeout!\n" ));
		}
		CloseHandle(Window_created);
		Window_created = NULL;
	#endif // THREADED

	// initialized
	Os_inited = 1;

	// check to see if we're running under msdev
	os_check_debugger();

	atexit(os_deinit);
}

// set the main window title
void os_set_title( char * title )
{
	strcpy( szWinTitle, title );
	SetWindowText( hwndApp, szWinTitle );
}

extern void gr_opengl_shutdown();

// call at program end
void os_cleanup()
{
	if (gr_screen.mode == GR_OPENGL)
		gr_opengl_shutdown();

	// destroy the window (takes care of a lot of window related cleanup and sys messages)
	DestroyWindow( hwndApp );

#ifndef NDEBUG
	outwnd_close();
#endif
}


// window management -----------------------------------------------------------------

// Returns 1 if app is not the foreground app.
int os_foreground()
{
	return fAppActive;
}

// Returns the handle to the main window
uint os_get_window()
{
	return (uint)hwndApp;
}

// Returns the handle to the main window
void os_set_window(uint new_handle)
{
	hwndApp = (HWND) new_handle;
}


// process management -----------------------------------------------------------------

// Sleeps for n milliseconds or until app becomes active.
void os_sleep(int ms)
{
	Sleep(ms);
}

// Used to stop message processing
void os_suspend()
{
	ENTER_CRITICAL_SECTION( Os_lock );	
}

// resume message processing
void os_resume()
{
	LEAVE_CRITICAL_SECTION( Os_lock );	
}


// ----------------------------------------------------------------------------------------------------
// OSAPI FORWARD DECLARATIONS
//

#ifdef THREADED_PROCESS

// thread handler for the main message thread
DWORD win32_process(DWORD lparam)
{
/*	MSG msg;
	HANDLE Window_created = (HANDLE)lparam;

	if ( !win32_create_window() )
		return 0;

	// Let the app continue once the window is created
	SetEvent(Window_created);

	while (1)	{	
		if (WaitMessage() == TRUE)	{
			ENTER_CRITICAL_SECTION( Os_lock );			
			while(PeekMessage(&msg,0,0,0,PM_REMOVE))	{
				if ( msg.message == WM_DESTROY )	{
					LEAVE_CRITICAL_SECTION( Os_lock );

					// cleanup and exit this thread!!
					DELETE_CRITICAL_SECTION( Os_lock );
					return 0;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			LEAVE_CRITICAL_SECTION( Os_lock );
		} 
	}*/

	return 0;
}

#else

DWORD win32_process(DWORD lparam)
{
	MSG msg;

	while ( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
#endif // THREADED_PROCESS

// Fills in the Os_debugger_running with non-zero if debugger detected.
void os_check_debugger()
{
	HMODULE hMod;
	char search_string[256];
	char myname[128];
	int namelen;
	char * p;

	Os_debugger_running = 0;		// Assume its not

	// Find my EXE file name
	hMod = GetModuleHandle(NULL);
	if ( !hMod ) return;
	namelen = GetModuleFileName( hMod, myname, 127 );	
	if ( namelen < 1 ) return;
	
	// Strip off the .EXE
	p = strstr( myname, ".exe" );
	if (!p) return;
	*p = '\0';

	// Move p to point to first letter of EXE filename
	while( (*p!='\\') && (*p!='/') && (*p!=':') )
		p--;
	p++;	
	if ( strlen(p) < 1 ) return;

	// Build what the debugger's window title would be if the debugger is running...
	sprintf( search_string, "[run] - %s -", p );

	// ... and then search for it.
	EnumWindows( (int (__stdcall *)(struct HWND__ *,long))os_enum_windows, (long)&search_string );
}

// called at shutdown. Makes sure all thread processing terminates.
void os_deinit()
{
	if (hThread)	{
		CloseHandle(hThread);
		hThread = NULL;
	}
}

// go through all windows and try and find the one that matches the search string
BOOL __stdcall os_enum_windows( HWND hwnd, char * search_string )
{
	char tmp[128];
	int len;

	len = GetWindowText( hwnd, tmp, 127 );
	 
	if ( len )	{
		if ( strstr( tmp, search_string ))	{
			Os_debugger_running = 1;		// found the search string!
			return FALSE;	// stop enumerating windows
		}
	}

	return TRUE;	// continue enumeration
}

void change_window_active_state()
{
	if (fAppActive != fOldAppActive) {
		if (fAppActive) {
			// maximize it
			joy_reacquire_ff();

			game_unpause();

#ifdef THREADED_PROCESS
			SetThreadPriority( hThread, THREAD_PRIORITY_HIGHEST );
#endif

			disableWindowsKey();
		} else {
			joy_unacquire_ff();

			if (Mouse_hidden)
				Mouse_hidden = 0;

			// Pause sounds and put up pause screen if necessary
			game_pause();

#ifdef THREADED_PROCESS
			SetThreadPriority( hThread, THREAD_PRIORITY_NORMAL );
#endif

			enableWindowsKey();
		}

		gr_activate(fAppActive);

		fOldAppActive = fAppActive;
	}
}

int Got_message = 0;
extern bool Messagebox_active;
// message handler for the main thread
LRESULT CALLBACK win32_message_handler(HWND hwnd,UINT msg,WPARAM wParam, LPARAM lParam)
{
	// Got_message++;

	switch(msg)	{

	case WM_QUERYNEWPALETTE:
		// mprintf(( "WM: QueryNewPalette\n" ));
		return TRUE;	// Say that I've realized my own palette
		break;
	case WM_PALETTECHANGED:
		// mprintf(( "WM: PaletteChanged\n" ));
		break;
	case WM_PALETTEISCHANGING:
		// mprintf(( "WM: PaletteIsChanging\n" ));
		break;

	case WM_DISPLAYCHANGE:
		// mprintf(( "WM: DisplayChange\n" ));
		break;

	case WM_LBUTTONDOWN:
		mouse_mark_button( MOUSE_LEFT_BUTTON, 1 );
		break;

	case WM_LBUTTONUP:
		mouse_mark_button( MOUSE_LEFT_BUTTON, 0 );
		break;

	case WM_RBUTTONDOWN:
		mouse_mark_button( MOUSE_RIGHT_BUTTON, 1 );
		break;

	case WM_RBUTTONUP:
		mouse_mark_button( MOUSE_RIGHT_BUTTON, 0 );
		break;

	case WM_MBUTTONDOWN:
		mouse_mark_button( MOUSE_MIDDLE_BUTTON, 1 );
		break;

	case WM_MBUTTONUP:
		mouse_mark_button( MOUSE_MIDDLE_BUTTON, 0 );
		break;

	case WM_TIMER:
		break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:	{		
			int nVirtKey;
			uint lKeyData;

			int latency;
			latency = timeGetTime() - GetMessageTime();
			if ( latency < 0 )
				latency=0;

			nVirtKey = (int)wParam;    // virtual-key code 
			lKeyData = (lParam>>16) & 255;          // key data 
			if ( (lParam>>16) & 256 ) lKeyData += 0x80;

			// Fix up print screen, whose OEM code is wrong under 95.
			if ( nVirtKey == VK_SNAPSHOT )	{
				lKeyData = KEY_PRINT_SCRN;	
			}

			if (lKeyData == KEY_RSHIFT)  // either shift is just a shift to us..
				lKeyData = KEY_LSHIFT;

			if (lKeyData == KEY_RALT)  // Same with alt keys..
				lKeyData = KEY_LALT;

//			mprintf(( "Key down = 0x%x|%x\n", lKeyData, nVirtKey ));
			key_mark( lKeyData, 1, latency );
//			mprintf(( "Key down = 0x%x\n", lKeyData ));
			//Warning( LOCATION, "Key = 0x%x", lKeyData );			
		}
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:	 {		
			int nVirtKey;
			uint lKeyData;

			int latency;
			latency = timeGetTime() - GetMessageTime();
			if ( latency < 0 )
				latency=0;

			nVirtKey = (int) wParam;    // virtual-key code 
			lKeyData = (lParam>>16) & 255;          // key data 
			if ( (lParam>>16) & 256 ) lKeyData += 0x80;

			// Fix up print screen, whose OEM code is wrong under 95.
			if ( nVirtKey == VK_SNAPSHOT )	{
				lKeyData = KEY_PRINT_SCRN;	
			}

			if (lKeyData == KEY_RSHIFT)  // either shift is just a shift to us..
				lKeyData = KEY_LSHIFT;

			if (lKeyData == KEY_RALT)  // Same with alt keys..
				lKeyData = KEY_LALT;

//			mprintf(( "Key up = 0x%x|%x\n", lKeyData, nVirtKey ));
			if ( lKeyData == 0xB7 )	{
				// Hack for PrintScreen which only sends one up message!
				key_mark( lKeyData, 1, latency );		
				key_mark( lKeyData, 0, latency );

			} else {
				key_mark( lKeyData, 0, latency );
			}			
		}		
		break;

	case WM_KILLFOCUS:
		{
			if (Messagebox_active)
				break;

			key_lost_focus();
			gr_activate(0);
			break;
		}

	case WM_SETFOCUS:
		{
			if (Messagebox_active)
				break;

			key_got_focus();
			gr_activate(1);
			break;
		}


	case WM_ACTIVATE:		   
	{
		if (Messagebox_active)
			break;

		int flag = LOWORD(wParam);
		fAppActive = (( flag == WA_ACTIVE) || (flag==WA_CLICKACTIVE)) ? TRUE : FALSE;
		change_window_active_state();
		break;
	}

	case WM_ACTIVATEAPP:
		if (Messagebox_active)
			break;

		fAppActive = (BOOL)wParam;
		change_window_active_state();
		break;

	case WM_DESTROY:
		// mprintf(( "WM_DESTROY called\n" ));
		PostQuitMessage(0);
		break;

	case WM_CLOSE:
		gameseq_post_event(GS_EVENT_QUIT_GAME);
		break;

	case WM_SYSCOMMAND:
		// mprintf(( "Sys command called '%x'\n", wParam ));
		 if ( wParam != SC_SCREENSAVE ){
			 return DefWindowProc(hwnd, msg, wParam, lParam);
		 }
		 break;

/*
	case MM_WIM_DATA:
		rtvoice_stream_data((uint)hwnd, (uint)wParam, (uint)lParam);
		break;
*/
#ifdef FS2_VOICER
    case WM_RECOEVENT:
		if ( Game_mode & GM_IN_MISSION && Cmdline_voice_recognition)
		{
			VOICEREC_process_event( hwnd );
		}
        break;
#endif

	// report back that we handle this ourselves (with gr_clear()) in order to
	// prevent flickering (especially with movies)
	case WM_ERASEBKGND:
		return TRUE;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
		break;
	}

	return 0;
}

// create the main window
void win32_create_window(int width, int height)
{
	WNDCLASSEX wclass;							// Huh?
	HINSTANCE hInst = GetModuleHandle(NULL);

	memset( &wclass, 0, sizeof(WNDCLASSEX) );

	wclass.hInstance 		= hInst;
	wclass.lpszClassName	= szWinClass;
	wclass.lpfnWndProc		= (WNDPROC)win32_message_handler;	  

//	if (Cmdline_window) {
//		wclass.style			= CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
//	} else {
//		wclass.style			= CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW;
//	}
	wclass.style			= CS_OWNDC;	// using CS_OWNDC for better Win9x/WinME support (I think it's implied with WinNT+)

	wclass.cbSize			= sizeof(WNDCLASSEX);
	wclass.hIcon			= LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP_ICON) );
	wclass.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wclass.lpszMenuName		= NULL;	//"FreeSpaceMenu";
	wclass.cbClsExtra		= 0;
	wclass.cbWndExtra		= 0;
	// set background to erase/clear with a black brush
	// (NULL means that we had to do it ourselves, and created a white-screen problem)
	wclass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);

	if ( !RegisterClassEx(&wclass) ) {
		Error( LOCATION, "FATAL ERROR:  Unable to register window class!!" );
	}

	int style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;

	if (Cmdline_window)
		style |= (WS_CAPTION | WS_SYSMENU | WS_BORDER);

	int x_add, y_add;
	int start_x, start_y;

	if (Cmdline_window) {
		RECT my_rect;

		// make sure we adjust for the actual window border	
		x_add = GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
		y_add = 2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);

		GetWindowRect( GetDesktopWindow(), &my_rect );

		start_x = (my_rect.right - width - x_add) / 2;
		start_y = (my_rect.bottom - height - y_add) / 2;

		if (start_x < 0)
			start_x = 0;
		if (start_y < 0)
			start_y = 0;
	} else {
		x_add = y_add = 0;
		start_x = start_y = 0;
	}

	// we don't sicky TOPMOST for windowed mode since we wouldn't be able to bring
	// the debug window (or anything else) to the true foreground otherwise
	hwndApp = CreateWindowEx( (Cmdline_window) ? 0 : WS_EX_TOPMOST,
								szWinClass, szWinTitle,
								style,   
								start_x,		// x
								start_y,		// y
								width + x_add,	// w
								height + y_add,	// h
								NULL, (HMENU)NULL, hInst,
								(LPSTR)NULL );

	if ( !hwndApp ) {
		Error( LOCATION, "FATAL ERROR:  Unable to create game window!!" );
	}

	main_window_inited = 1;

#ifndef NDEBUG
	extern void outwnd_init_debug_window(int);
	outwnd_init_debug_window(1);
#endif

	ShowWindow( hwndApp, SW_SHOWNORMAL );

	SetForegroundWindow( hwndApp );
	SetActiveWindow( hwndApp );
	SetFocus( hwndApp );

	// Hack!! Turn off Window's cursor.
//	ShowCursor(false);
//	ClipCursor(NULL);

	return;// TRUE;
}

void os_poll()
{
#ifndef THREADED_PROCESS
	win32_process(0);
#else
	MSG msg;
	ENTER_CRITICAL_SECTION( Os_lock );
	while(PeekMessage(&msg,0,0,0,PM_NOREMOVE))	{		
		if ( msg.message == WM_DESTROY )	{
			break;
		}
		if (PeekMessage(&msg,0,0,0,PM_REMOVE))	{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}		
		Got_message++;
	}
	LEAVE_CRITICAL_SECTION( Os_lock );
#endif
}

void debug_int3(char *file, int line)
{
	mprintf(("Int3(): From %s at line %d\n", file, line));

	gr_activate(0);
 
#ifdef _WIN32
#if defined _MSC_VER
	_asm { int 3 };
#elif defined __GNUC__
	asm("int $3");
#else
#error debug_int3: unknown compiler
#endif

#else
#error debug_int3: unknown OS
#endif

	gr_activate(1);

}


// Goober5000 - code provided by jr2 to disable windows key when FSO is in the foreground

#ifdef _WIN32

static HHOOK g_hKeyboardHook = NULL;

// ugh
#ifndef WH_KEYBOARD_LL
  #define WH_KEYBOARD_LL	13
#endif

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0 || nCode != HC_ACTION)  // do not process message 
		return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam); 

	// hack!
	// this is because the KBDLLHOOKSTRUCT type requires a mess of #includes,
	// but all we need from it is the first field
	DWORD vkCode = *( (DWORD *) lParam );

	// determine key event
	switch (wParam) 
	{
		case WM_KEYDOWN:  
		case WM_KEYUP:    
			if ( (vkCode == VK_LWIN) || (vkCode == VK_RWIN) )
				return 1;
	}

	return CallNextHookEx( g_hKeyboardHook, nCode, wParam, lParam );
}

void disableWindowsKey()
{
	if (g_hKeyboardHook != NULL)
		return;

	g_hKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0 );
}

void enableWindowsKey()
{
	if (g_hKeyboardHook == NULL)
		return;

	UnhookWindowsHookEx( g_hKeyboardHook );
	g_hKeyboardHook = NULL;
}

#endif // _WIN32
