/*
 * Created by Ian "Goober5000" Warfield for the Freespace2 Source Code Project.
 * You may not sell or otherwise commercially exploit the source or things you
 * create based on the source.
 *
 */ 

/*
 * $Logfile: /Freespace2/code/wxFRED2/VoiceActingManagerEditor.h $
 * $Revision: 1.1 $
 * $Date: 2005-05-12 14:00:14 $
 * $Author: Goober5000 $
 *
 * Voice acting manager editor
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.0  2005/05/12 07:30:00  Goober5000
 * Addition to CVS repository
 *
 */

#ifndef _VOICE_ACTING_MANAGER_EDITOR_H
#define _VOICE_ACTING_MANAGER_EDITOR_H

class dlgVoiceActingManagerEditor : public wxDialog
{
	public:
		// constructor/destructors
		dlgVoiceActingManagerEditor(wxWindow *parent);
		~dlgVoiceActingManagerEditor();

		// event handlers
		void OnGenerateFileNames(wxCommandEvent &WXUNUSED(event));

	protected:		
		// events
		DECLARE_EVENT_TABLE()

	private:
		// dialog control stuff goes here
};

#endif
