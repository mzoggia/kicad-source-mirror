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
class BITMAP_BUTTON;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/stc/stc.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_TEXT_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_TEXT_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_textLabel;
		wxTextCtrl* m_TextCtrl;
		wxStyledTextCtrl* m_StyledTextCtrl;
		wxBitmapButton* m_TextValueSelectButton;
		wxStaticText* m_note;
		wxCheckBox* m_visible;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_horizontal;
		BITMAP_BUTTON* m_vertical;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator3;
		BITMAP_BUTTON* m_hAlignLeft;
		BITMAP_BUTTON* m_hAlignCenter;
		BITMAP_BUTTON* m_hAlignRight;
		BITMAP_BUTTON* m_separator4;
		BITMAP_BUTTON* m_vAlignTop;
		BITMAP_BUTTON* m_vAlignCenter;
		BITMAP_BUTTON* m_vAlignBottom;
		BITMAP_BUTTON* m_separator5;
		wxStaticText* m_xPosLabel;
		wxTextCtrl* m_xPosCtrl;
		wxStaticText* m_xPosUnits;
		wxCheckBox* m_CommonUnit;
		wxStaticText* m_yPosLabel;
		wxTextCtrl* m_yPosCtrl;
		wxStaticText* m_yPosUnits;
		wxCheckBox* m_CommonConvert;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseDialog( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnSetFocusText( wxFocusEvent& event ) { event.Skip(); }
		virtual void onMultiLineTCLostFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnTextValueSelectButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LIB_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Item Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LIB_TEXT_PROPERTIES_BASE();

};

