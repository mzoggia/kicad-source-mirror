///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class COLOR_SWATCH;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/gbsizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_SHAPE_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_SHAPE_PROPERTIES_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_onFill( wxCommandEvent& event ){ onFill( event ); }


	protected:
		enum
		{
			NO_FILL = 1000,
			FILLED_SHAPE,
			FILLED_WITH_BG_BODYCOLOR,
			FILLED_WITH_COLOR
		};

		wxStaticText* m_widthLabel;
		wxTextCtrl* m_widthCtrl;
		wxStaticText* m_widthUnits;
		wxStaticText* m_helpLabel;
		wxRadioButton* m_rbFillNone;
		wxRadioButton* m_rbFillOutline;
		wxRadioButton* m_rbFillBackground;
		wxRadioButton* m_rbFillCustom;
		COLOR_SWATCH* m_colorSwatch;
		wxCheckBox* m_checkApplyToAllUnits;
		wxCheckBox* m_checkApplyToAllConversions;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onFill( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LIB_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Drawing Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LIB_SHAPE_PROPERTIES_BASE();

};

