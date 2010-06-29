/*
 * Created by Ian "Goober5000" Warfield for the FreeSpace2 Source Code Project.
 * You may not sell or otherwise commercially exploit the source or things you
 * create based on the source.
 */ 



// precompiled header for compilers that support it
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "aboutbox.h"
#include <wx/xrc/xmlres.h>


BEGIN_EVENT_TABLE(dlgAboutBox, wxDialog)
END_EVENT_TABLE()


dlgAboutBox::dlgAboutBox(wxWindow *parent)
	: wxDialog()
{
	wxInitAllImageHandlers();
	wxXmlResource::Get()->LoadDialog(this, parent, "dlgAboutBox");
}

dlgAboutBox::~dlgAboutBox()
{}

