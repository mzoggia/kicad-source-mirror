/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_TEXT_PROPERTIES_H
#define DIALOG_TEXT_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <sch_text.h>
#include <dialog_text_properties_base.h>


class SCH_EDIT_FRAME;
class SCH_TEXT;
class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;


class DIALOG_TEXT_PROPERTIES : public DIALOG_TEXT_PROPERTIES_BASE
{
public:
    DIALOG_TEXT_PROPERTIES( SCH_EDIT_FRAME* parent, SCH_TEXT* aTextItem );
    ~DIALOG_TEXT_PROPERTIES();

private:
    void onScintillaCharAdded( wxStyledTextEvent &aEvent );
    void onSpinButton( wxCommandEvent &aEvent );

    void OnFormattingHelp( wxHyperlinkEvent& aEvent ) override;
    void onMultiLineTCLostFocus( wxFocusEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    SCH_EDIT_FRAME*       m_Parent;
    SCH_TEXT*             m_currentText;
    UNIT_BINDER           m_textSize;
    SCINTILLA_TRICKS*     m_scintillaTricks;

    HTML_MESSAGE_BOX*     m_helpWindow;
};



#endif // DIALOG_TEXT_PROPERTIES_H
