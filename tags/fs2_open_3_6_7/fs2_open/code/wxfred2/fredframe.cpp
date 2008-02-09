/*
 * Created by Ian "Goober5000" Warfield for the Freespace2 Source Code Project.
 * You may not sell or otherwise commercially exploit the source or things you
 * create based on the source.
 *
 */ 

/*
 * $Logfile: /Freespace2/code/wxFRED2/FREDFrame.cpp $
 * $Revision: 1.4 $
 * $Date: 2005-05-12 14:00:14 $
 * $Author: Goober5000 $
 *
 * FRED app frame
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2005/04/25 12:14:21  Goober5000
 * -made the XRC file parsed on load instead of embedded
 * -migrated wxFRED to wxWidgets 2.6.0
 * --Goober5000
 *
 * Revision 1.2  2005/04/24 14:42:27  Goober5000
 * wxFRED now uses XRC-based resources
 * --Goober5000
 *
 * Revision 1.1  2005/03/29 13:35:53  Goober5000
 * set up the main menu options
 * --Goober5000
 *
 * Revision 1.0  2005/03/29 07:55:19  Goober5000
 * Addition to CVS repository
 *
 */

// precompiled header for compilers that support it
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "fredframe.h"
#include <wx/xrc/xmlres.h>
#include "asteroidfieldeditor.h"
#include "missionspecseditor.h"
#include "debriefingeditor.h"
#include "shieldsystemeditor.h"
#include "commandbriefingeditor.h"
#include "setglobalshipflagseditor.h"
#include "voiceactingmanagereditor.h"
#include "campaigneditor.h"
#include "aboutbox.h"


BEGIN_EVENT_TABLE(FREDFrame, wxFrame)
	// file menu
	EVT_MENU(XRCID("mnuFileNew"), FREDFrame::OnFileNew)
	EVT_MENU(XRCID("mnuFileOpen"), FREDFrame::OnFileOpen)
	EVT_MENU(XRCID("mnuFileSave"), FREDFrame::OnFileSave)
	EVT_MENU(XRCID("mnuFileSaveAs"), FREDFrame::OnFileSaveAs)
	EVT_MENU(XRCID("mnuFileExit"), FREDFrame::OnFileExit)

	// editor menu
	EVT_MENU(XRCID("mnuEditorsAsteroidField"), FREDFrame::OnEditorsAsteroidField)
	EVT_MENU(XRCID("mnuEditorsMissionSpecs"), FREDFrame::OnEditorsMissionSpecs)
	EVT_MENU(XRCID("mnuEditorsDebriefing"), FREDFrame::OnEditorsDebriefing)
	EVT_MENU(XRCID("mnuEditorsShieldSystem"), FREDFrame::OnEditorsShieldSystem)
	EVT_MENU(XRCID("mnuEditorsCommandBriefing"), FREDFrame::OnEditorsCommandBriefing)
	EVT_MENU(XRCID("mnuEditorsSetGlobalShipFlags"), FREDFrame::OnEditorsSetGlobalShipFlags)
	EVT_MENU(XRCID("mnuEditorsVoiceActingManager"), FREDFrame::OnEditorsVoiceActingManager)
	EVT_MENU(XRCID("mnuEditorsCampaign"), FREDFrame::OnEditorsCampaign)

	// help menu
	EVT_MENU(XRCID("mnuHelpAboutFRED2"), FREDFrame::OnHelpAboutFRED2)
END_EVENT_TABLE()


FREDFrame::FREDFrame(const wxChar *title, int xpos, int ypos, int width, int height)
	: wxFrame(NULL, -1, title, wxPoint(xpos, ypos), wxSize(width, height))
{
	myMenuBar = wxXmlResource::Get()->LoadMenuBar(_T("FREDMenu"));
	SetMenuBar(myMenuBar);

	CreateStatusBar(5);
	SetStatusText("For Help, press F1", 0);
}

FREDFrame::~FREDFrame()
{}

void FREDFrame::OnFileNew(wxCommandEvent &WXUNUSED(event))
{
	currentFilename = _T("Untitled.fs2");
}

void FREDFrame::OnFileOpen(wxCommandEvent &WXUNUSED(event))
{
	// standard Open File dialog
	wxFileDialog *dlg = new wxFileDialog(this, "Open", "", "",
		_T("FreesSpace2 Missions (*.fs2)|*.fs2|All files (*.*)|*.*"), wxOPEN, wxDefaultPosition);

	// display it and maybe open the file
	if (dlg->ShowModal() == wxID_OK)
	{
		currentFilename = dlg->GetFilename();
    }
}

void FREDFrame::OnFileSave(wxCommandEvent &WXUNUSED(event))
{
	SetStatusText(currentFilename, 0);
}

void FREDFrame::OnFileSaveAs(wxCommandEvent &WXUNUSED(event))
{
	// standard Save File As dialog
	wxFileDialog *dlg = new wxFileDialog(this, "Save As", "", currentFilename,
		_T("FreesSpace2 Missions (*.fs2)|*.fs2|All files (*.*)|*.*"), wxSAVE|wxOVERWRITE_PROMPT, wxDefaultPosition);

	// display it and maybe save the file
	if (dlg->ShowModal() == wxID_OK)
	{
    }
}

void FREDFrame::OnFileExit(wxCommandEvent &WXUNUSED(event))
{
	Close(true);
}

void FREDFrame::OnEditorsAsteroidField(wxCommandEvent &WXUNUSED(event))
{
	dlgAsteroidFieldEditor *dlg = new dlgAsteroidFieldEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnEditorsMissionSpecs(wxCommandEvent &WXUNUSED(event))
{
	dlgMissionSpecsEditor *dlg = new dlgMissionSpecsEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnEditorsDebriefing(wxCommandEvent &WXUNUSED(event))
{
	dlgDebriefingEditor *dlg = new dlgDebriefingEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnEditorsShieldSystem(wxCommandEvent &WXUNUSED(event))
{
	dlgShieldSystemEditor *dlg = new dlgShieldSystemEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnEditorsCommandBriefing(wxCommandEvent &WXUNUSED(event))
{
	dlgCommandBriefingEditor *dlg = new dlgCommandBriefingEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnEditorsSetGlobalShipFlags(wxCommandEvent &WXUNUSED(event))
{
	dlgSetGlobalShipFlagsEditor *dlg = new dlgSetGlobalShipFlagsEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnEditorsVoiceActingManager(wxCommandEvent &WXUNUSED(event))
{
	dlgVoiceActingManagerEditor *dlg = new dlgVoiceActingManagerEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnEditorsCampaign(wxCommandEvent &WXUNUSED(event))
{
	dlgCampaignEditor *dlg = new dlgCampaignEditor(this);
	dlg->ShowModal();
	dlg->Destroy();
}

void FREDFrame::OnHelpAboutFRED2(wxCommandEvent &WXUNUSED(event))
{
	dlgAboutBox *dlg = new dlgAboutBox(this);
	dlg->ShowModal();
	dlg->Destroy();
}
