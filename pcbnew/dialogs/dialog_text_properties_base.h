///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/bmpcbox.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TEXT_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TEXT_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_MultiLineSizer;
		wxStyledTextCtrl* m_MultiLineText;
		wxBoxSizer* m_SingleLineSizer;
		wxStaticText* m_TextLabel;
		wxTextCtrl* m_SingleLineText;
		wxCheckBox* m_cbLocked;
		wxCheckBox* m_Visible;
		wxCheckBox* m_KeepUpright;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		BITMAP_BUTTON* m_separator0;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_alignLeft;
		BITMAP_BUTTON* m_alignCenter;
		BITMAP_BUTTON* m_alignRight;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_mirrored;
		BITMAP_BUTTON* m_separator3;
		wxStaticText* m_SizeXLabel;
		wxTextCtrl* m_SizeXCtrl;
		wxStaticText* m_SizeXUnits;
		wxStaticText* m_SizeYLabel;
		wxTextCtrl* m_SizeYCtrl;
		wxStaticText* m_SizeYUnits;
		wxStaticText* m_ThicknessLabel;
		wxTextCtrl* m_ThicknessCtrl;
		wxStaticText* m_ThicknessUnits;
		wxStaticText* m_PositionXLabel;
		wxTextCtrl* m_PositionXCtrl;
		wxStaticText* m_PositionXUnits;
		wxStaticText* m_PositionYLabel;
		wxTextCtrl* m_PositionYCtrl;
		wxStaticText* m_PositionYUnits;
		wxStaticText* m_OrientLabel;
		wxComboBox* m_OrientCtrl;
		wxStaticText* m_statusLine;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void onMultiLineTCLostFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSetFocusText( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onAlignButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU );
		~DIALOG_TEXT_PROPERTIES_BASE();

};

