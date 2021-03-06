/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Sound/ds3d.cpp $
 * $Revision: 2.11 $
 * $Date: 2006-03-15 17:30:47 $
 * $Author: taylor $
 *
 * C file for interface to DirectSound3D
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.10  2006/02/25 21:47:19  Goober5000
 * spelling
 *
 * Revision 2.9  2005/04/05 11:48:23  taylor
 * remove acm-unix.cpp, replaced by acm-openal.cpp since it's properly cross-platform now
 * better error handling for OpenAL functions
 * Windows can now build properly with OpenAL
 * extra check to make sure we don't try and use too many hardware bases sources
 * fix memory error from OpenAL extension list in certain instances
 *
 * Revision 2.8  2005/04/05 05:53:25  taylor
 * s/vector/vec3d/g, better support for different compilers (Jens Granseuer)
 *
 * Revision 2.7  2005/04/01 07:33:08  taylor
 * fix hanging on exit with OpenAL
 * some better error handling on OpenAL init and make it more Windows friendly too
 * basic 3d sound stuff for OpenAL, not working right yet
 *
 * Revision 2.6  2005/03/27 06:14:30  taylor
 * update for streaming support and platform independance
 *
 * Revision 2.5  2004/07/26 20:47:52  Kazan
 * remove MCD complete
 *
 * Revision 2.4  2004/07/12 16:33:06  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 2.3  2003/03/02 06:37:24  penguin
 * Use multimedia headers in local dir, not system's (headers are not present in MinGW distribution)
 *  - penguin
 *
 * Revision 2.2  2002/08/01 01:41:10  penguin
 * The big include file move
 *
 * Revision 2.1  2002/07/07 19:56:00  penguin
 * Back-port to MSVC
 *
 * Revision 2.0  2002/06/03 04:02:29  penguin
 * Warpcore CVS sync
 *
 * Revision 1.1  2002/05/02 18:03:13  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 12    8/05/99 4:45p Alanl
 * the FINAL tweak to rolloffs!
 * 
 * 11    8/05/99 4:34p Alanl
 * change rolloff factors again
 * 
 * 10    8/05/99 4:27p Danw
 * would you believe we're still tweaking the EAX?? :)
 * 
 * 9     8/05/99 4:04p Danw
 * tweak rolloffs for EAX
 * 8     8/05/99 2:54p Danw
 * tweak rolloffs for A3D and EAX
 * 7     8/05/99 10:54a Alanl
 * change EAX rolloff to 3.0
 * 
 * 6     8/04/99 11:51a Danw
 * tweak rolloffs for A3D and EAX
 * 5     8/04/99 11:42a Danw
 * tweak rolloffs for A3D and EAX
 * 
 * 4     8/01/99 2:06p Alanl
 * increase the rolloff for A3D
 * 
 * 3     5/23/99 8:11p Alanl
 * Added support for EAX
 * 
 * 2     10/07/98 10:54a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:51a Dave
 * 
 * 15    5/06/98 2:16p Dan
 * 
 * 14    5/05/98 4:49p Lawrance
 * Put in code to authenticate A3D, improve A3D support
 * 
 * 13    4/19/98 9:30p Lawrance
 * Use Aureal_enabled flag
 * 
 * 12    9/09/97 3:39p Sandeep
 * warning level 4 bugs
 * 
 * 11    8/01/97 10:40a Lawrance
 * decrease rolloff for DirectSound3D sounds
 * 
 * 10    7/29/97 2:54p Lawrance
 * 
 * 9     7/28/97 11:39a Lawrance
 * allow individual volume scaling on 3D buffers
 * 
 * 8     7/17/97 9:32a John
 * made all directX header files name start with a v
 * 
 * 7     6/09/97 11:50p Lawrance
 * integrating DirectSound3D
 * 
 * 6     6/09/97 8:53a Lawrance
 * remove warning
 * 
 * 5     6/08/97 5:59p Lawrance
 * integrate DirectSound3D into sound system
 * 
 * 4     6/02/97 1:45p Lawrance
 * implementing hardware mixing
 * 
 * 3     5/29/97 4:02p Lawrance
 * listener interface in place
 * 
 * 2     5/29/97 12:03p Lawrance
 * creation of file to hold DirectSound3D specific code
 *
 * $NoKeywords: $
 */

#include "globalincs/pstypes.h"

#ifndef USE_OPENAL
#include <windows.h>
#include "directx/vdsound.h"
#include "sound/channel.h"
#else
#if !(defined(__APPLE__) || defined(_WIN32))
	#include <AL/al.h>
	#include <AL/alc.h>
#else
	#include "al.h"
	#include "alc.h"
#endif // !__APPLE__ && !_WIN32
#endif // USE_OPENAL

#include "sound/ds3d.h"
#include "sound/ds.h"
#include "sound/sound.h"
#include "object/object.h"



#ifndef USE_OPENAL
typedef enum 
{
	DSPROPERTY_VMANAGER_MODE = 0,
	DSPROPERTY_VMANAGER_PRIORITY,
	DSPROPERTY_VMANAGER_STATE
} DSPROPERTY_VMANAGER;


typedef enum 
{
	DSPROPERTY_VMANAGER_MODE_DEFAULT = 0,
	DSPROPERTY_VMANAGER_MODE_AUTO,
	DSPROPERTY_VMANAGER_MODE_REPORT,
	DSPROPERTY_VMANAGER_MODE_USER
} VmMode;


typedef enum 
{
	DSPROPERTY_VMANAGER_STATE_PLAYING3DHW = 0,
	DSPROPERTY_VMANAGER_STATE_SILENT,
	DSPROPERTY_VMANAGER_STATE_BUMPED,
	DSPROPERTY_VMANAGER_STATE_PLAYFAILED
} VmState;


extern LPDIRECTSOUND pDirectSound;

LPDIRECTSOUND3DLISTENER	pDS3D_listener = NULL;

GUID DSPROPSETID_VoiceManager_Def = {0x62a69bae, 0xdf9d, 0x11d1, {0x99, 0xa6, 0x0, 0xc0, 0x4f, 0xc9, 0x9d, 0x46}};
#endif	// !USE_OPENAL


int DS3D_inited = FALSE;


// ---------------------------------------------------------------------------------------
// ds3d_update_buffer()
//
//	parameters:		channel	=> identifies the 3D sound to update
//						min		=>	the distance at which sound doesn't get any louder
//						max		=>	the distance at which sound doesn't attenuate any further
//						pos		=> world position of sound
//						vel		=> velocity of the objects producing the sound
//
//	returns:		0		=>		success
//					-1		=>		failure
//
//
int ds3d_update_buffer(int channel, float min, float max, vec3d *pos, vec3d *vel)
{
	if (DS3D_inited == FALSE)
		return 0;

	if ( channel == -1 )
		return 0;

#ifdef USE_OPENAL
	// as used by DS3D
//	OpenAL_ErrorPrint( alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED) );

	// set the min distance
	OpenAL_ErrorPrint( alSourcef(Channels[channel].source_id, AL_REFERENCE_DISTANCE, min) );

	// set the max distance
//	OpenAL_ErrorPrint( alSourcef(Channels[channel].source_id, AL_MAX_DISTANCE, max) );
	OpenAL_ErrorPrint( alSourcef(Channels[channel].source_id, AL_MAX_DISTANCE, 40000.0f) );
	
	// set rolloff factor
	OpenAL_ErrorPrint( alSourcef(Channels[channel].source_id, AL_ROLLOFF_FACTOR, 1.0f) );
		
	// set doppler
	OpenAL_ErrorPrint( alDopplerVelocity(10000.0f) );
	OpenAL_ErrorPrint( alDopplerFactor(0.0f) );  // TODO: figure out why using a value of 1 sounds bad

	// set the buffer position
	if ( pos != NULL ) {
		ALfloat alpos[] = { pos->xyz.x, pos->xyz.y, pos->xyz.z };
		OpenAL_ErrorPrint( alSourcefv(Channels[channel].source_id, AL_POSITION, alpos) );
	}

	// set the buffer velocity
	if ( vel != NULL ) {
		ALfloat alvel[] = { vel->xyz.x, vel->xyz.y, vel->xyz.z };
		OpenAL_ErrorPrint( alSourcefv(Channels[channel].source_id, AL_VELOCITY, alvel) );
	} else {
		ALfloat alvel[] = { 0.0f, 0.0f, 0.0f };
		OpenAL_ErrorPrint( alSourcefv(Channels[channel].source_id, AL_VELOCITY, alvel) );
	}

#else

	HRESULT						hr;
	LPDIRECTSOUND3DBUFFER	pds3db;
	float							max_dist, min_dist;

	pds3db = Channels[channel].pds3db;
	Assert( pds3db != NULL);

	// set the buffer position
	if ( pos != NULL ) {
		hr = pds3db->SetPosition(pos->xyz.x, pos->xyz.y, pos->xyz.z, DS3D_DEFERRED);
	}

	// set the buffer veclocity
	if ( vel != NULL ) {
		hr = pds3db->SetVelocity(vel->xyz.x, vel->xyz.y, vel->xyz.z, DS3D_DEFERRED);
	}
	else {
		hr = pds3db->SetVelocity(0.0f, 0.0f, 0.0f, DS3D_DEFERRED);
	}

	// set the min distance
	hr = pds3db->GetMinDistance(&min_dist);
	hr = pds3db->SetMinDistance( min, DS3D_DEFERRED );
	// set the max distance
	hr = pds3db->GetMaxDistance(&max_dist);
//	hr = pds3db->SetMaxDistance( max, DS3D_DEFERRED );
	hr = pds3db->SetMaxDistance( 100000.0f, DS3D_DEFERRED );
#endif

	return 0;
}


// ---------------------------------------------------------------------------------------
// ds3d_update_listener()
//
//	returns:		0		=>		success
//					-1		=>		failure
//
int ds3d_update_listener(vec3d *pos, vec3d *vel, matrix *orient)
{
	if (DS3D_inited == FALSE)
		return 0;

#ifdef USE_OPENAL
	// set the listener position
	if ( pos != NULL ) {
		OpenAL_ErrorPrint( alListener3f(AL_POSITION, pos->xyz.x, pos->xyz.y, pos->xyz.z) );
	}

	// set the listener velocity
	if ( vel != NULL ) {
		OpenAL_ErrorPrint( alListener3f(AL_VELOCITY, vel->xyz.x, vel->xyz.y, vel->xyz.z) );
	}

	// set the listener orientation
	if ( orient != NULL ) {
		// uvec is up/top vector, fvec is at/front vector
		ALfloat list_orien[] = { orient->vec.fvec.xyz.x, orient->vec.fvec.xyz.y, orient->vec.fvec.xyz.z,
									orient->vec.uvec.xyz.x, orient->vec.uvec.xyz.y, orient->vec.uvec.xyz.z };
		OpenAL_ErrorPrint( alListenerfv(AL_ORIENTATION, list_orien) );
	}

#else

	HRESULT			hr;

	if ( pDS3D_listener == NULL )
		return -1;
	
	// set the listener position
	if ( pos != NULL ) {
		hr = pDS3D_listener->SetPosition(pos->xyz.x, pos->xyz.y, pos->xyz.z, DS3D_DEFERRED); 
	}

	// set the listener veclocity
	if ( vel != NULL ) {
		hr = pDS3D_listener->SetVelocity(vel->xyz.x, vel->xyz.y, vel->xyz.z, DS3D_DEFERRED); 
	}

	if ( orient != NULL ) {
		hr = pDS3D_listener->SetOrientation(	orient->vec.fvec.xyz.x, orient->vec.fvec.xyz.y, orient->vec.fvec.xyz.z,
															orient->vec.uvec.xyz.x, orient->vec.uvec.xyz.y, orient->vec.uvec.xyz.z,
															DS3D_DEFERRED );
	}

	float rolloff_factor = 1.0f;
	if (ds_using_a3d() == true) {
		rolloff_factor = 3.0f;		// A3D rolloff
	} else {
		rolloff_factor = 3.0f;		// EAX rolloff
	}

	hr = pDS3D_listener->SetRolloffFactor( rolloff_factor, DS3D_DEFERRED );
	hr = pDS3D_listener->SetDopplerFactor( 1.0f, DS3D_DEFERRED );
	
	hr = pDS3D_listener->CommitDeferredSettings();
	if ( hr != DS_OK ) {
		nprintf(("SOUND","Error in pDS3D_listener->CommitDeferredSettings(): %s\n", get_DSERR_text(hr) ));
		return -1;
	}
#endif

	return 0;
}

// ---------------------------------------------------------------------------------------
// ds3d_init_listener()
//
//
//	returns:		0		=>		success
//					-1		=>		failure
//
int ds3d_init_listener()
{
#ifdef USE_OPENAL
	// this is already setup in ds_init()
#else
	HRESULT			hr;

	if ( pDS3D_listener != NULL )
		return 0;

	hr = pPrimaryBuffer->QueryInterface(IID_IDirectSound3DListener, (void**)&pDS3D_listener);
	if (hr != DS_OK) {
		nprintf(("Sound","SOUND => Fatal error calling pPrimaryBuffer->QueryInterface(): %s\n", get_DSERR_text(hr) ));
		return -1;
	}
#endif

	return 0;		
}

// ---------------------------------------------------------------------------------------
// ds3d_close_listener()
//
//
void ds3d_close_listener()
{
#ifndef USE_OPENAL
	if ( pDS3D_listener != NULL ) {
		pDS3D_listener->Release();
		pDS3D_listener = NULL;
	}
#endif
}


// ---------------------------------------------------------------------------------------
// ds3d_init()
//
// Initialize the DirectSound3D system.  Call the initialization for the pDS3D_listener
// 
// returns:     -1	=> init failed
//              0		=> success
int ds3d_init(int voice_manager_required)
{
	if ( DS3D_inited == TRUE )
		return 0;

#ifndef USE_OPENAL
	if (voice_manager_required == 1) {
		LPKSPROPERTYSET pset;
		pset = (LPKSPROPERTYSET)ds_get_property_set_interface();

		if (pset == NULL) {
			nprintf(("Sound", "Disabling DirectSound3D since unable to get property set interface\n"));
			return -1;
		}

		HRESULT hr;
		unsigned long driver_support = 0;

		hr = pset->QuerySupport(DSPROPSETID_VoiceManager_Def, DSPROPERTY_VMANAGER_MODE, &driver_support);
		if (FAILED(hr)) {
			nprintf(("Sound", "Driver does not support Voice Manager extension, so abort DirectSound3D initialization\n"));
			return -1;
		}

		if ((driver_support & KSPROPERTY_SUPPORT_SET|KSPROPERTY_SUPPORT_GET) != (KSPROPERTY_SUPPORT_SET|KSPROPERTY_SUPPORT_GET)) {
			nprintf(("Sound", "Driver does not support Voice Manager extension, so abort DirectSound3D initialization\n"));
			return -1;
		}

		VmMode vmode = DSPROPERTY_VMANAGER_MODE_AUTO;
		hr = pset->Set(DSPROPSETID_VoiceManager_Def, DSPROPERTY_VMANAGER_MODE, NULL, 0, &vmode, sizeof(float));
		if (FAILED(hr)) {
			nprintf(("Sound", "Driver does not support Voice Manager extension, so abort DirectSound3D initialization\n"));
			return -1;
		}
	}
#endif

	if (ds3d_init_listener() != 0) {
		return -1;
	}
	
	DS3D_inited = TRUE;
	return 0;
}


// ---------------------------------------------------------------------------------------
// ds3d_close()
//
// De-initialize the DirectSound3D system
// 
void ds3d_close()
{
	if ( DS3D_inited == FALSE )
		return;

	ds3d_close_listener();
	DS3D_inited = FALSE;
}
