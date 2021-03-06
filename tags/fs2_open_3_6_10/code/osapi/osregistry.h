/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

#ifndef _FS2_REGISTRY_HEADER_FILE
#define _FS2_REGISTRY_HEADER_FILE


#include <stdlib.h>

// ------------------------------------------------------------------------------------------------------------
// REGISTRY DEFINES/VARS
//

// exectuable defines
extern char *Osreg_company_name;
extern char *Osreg_class_name;
extern char *Osreg_app_name;
extern char *Osreg_title;
#ifdef SCP_UNIX
extern char *Osreg_user_dir;
#endif


// ------------------------------------------------------------------------------------------------------------
// REGISTRY FUNCTIONS
//


// initialize the registry. setup default keys to use
void os_init_registry_stuff(char *company, char *app, char *version);

// Removes a value from to the INI file.  Passing
// name=NULL will delete the section.
void os_config_remove( char *section, char *name );

// Writes a string to the registry
void os_config_write_string( char *section, char *name, char *value );

// same as previous function except we don't use the application name to build up the keyname
void os_config_write_string2( char *section, char *name, char *value );

// Writes an unsigned int to the INI file.  
void os_config_write_uint( char *section, char *name, unsigned int value );

// Reads a string from the INI file.  If default is passed,
// and the string isn't found, returns ptr to default otherwise
// returns NULL;    Copy the return value somewhere before
// calling os_read_string again, because it might reuse the
// same buffer.
char * os_config_read_string( char *section, char *name, char *default_value=0 /*NULL*/ );

// same as previous function except we don't use the application name to build up the keyname
char * os_config_read_string2( char *section, char *name, char *default_value=0 /*NULL*/ );

// Reads a string from the INI file.  Default_value must 
// be passed, and if 'name' isn't found, then returns default_value
unsigned int  os_config_read_uint( char *section, char *name, unsigned int default_value );

// uses Ex versions of Windows registry functions
char * os_config_read_string_ex( char *keyname, char *name, char *default_value );

#endif

