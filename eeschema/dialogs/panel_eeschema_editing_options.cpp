/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <eeschema_settings.h>
#include "panel_eeschema_editing_options.h"


PANEL_EESCHEMA_EDITING_OPTIONS::PANEL_EESCHEMA_EDITING_OPTIONS( wxWindow* aWindow,
                                                                EDA_BASE_FRAME* aUnitsProvider ) :
        PANEL_EESCHEMA_EDITING_OPTIONS_BASE( aWindow ),
        m_hPitch( aUnitsProvider, m_hPitchLabel, m_hPitchCtrl, m_hPitchUnits ),
        m_vPitch( aUnitsProvider, m_vPitchLabel, m_vPitchCtrl, m_vPitchUnits )
{
    // Make the color swatch show "Clear Color" instead
    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_backgroundColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

#ifdef __WXOSX_MAC__
    m_leftClickCmdsBook->SetSelection( 1 );
#else
    m_leftClickCmdsBook->SetSelection( 0 );
#endif
}


void PANEL_EESCHEMA_EDITING_OPTIONS::loadEEschemaSettings( EESCHEMA_SETTINGS* aCfg )
{
    m_hPitch.SetValue( Mils2iu( aCfg->m_Drawing.default_repeat_offset_x ) );
    m_vPitch.SetValue( Mils2iu( aCfg->m_Drawing.default_repeat_offset_y ) );
    m_spinLabelRepeatStep->SetValue( aCfg->m_Drawing.repeat_label_increment );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COLOR_SETTINGS*   settings = mgr.GetColorSettings();
    COLOR4D           schematicBackground = settings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetSwatchBackground( schematicBackground );
    m_borderColorSwatch->SetSwatchColor( aCfg->m_Drawing.default_sheet_border_color, false );

    m_backgroundColorSwatch->SetSwatchBackground( schematicBackground );
    m_backgroundColorSwatch->SetSwatchColor( aCfg->m_Drawing.default_sheet_background_color, false );

    m_checkHVOrientation->SetValue( aCfg->m_Drawing.hv_lines_only );
    m_footprintPreview->SetValue( aCfg->m_Appearance.footprint_preview );
    m_navigatorStaysOpen->SetValue( aCfg->m_Appearance.navigator_stays_open );

    m_checkAutoplaceFields->SetValue( aCfg->m_AutoplaceFields.enable );
    m_checkAutoplaceJustify->SetValue( aCfg->m_AutoplaceFields.allow_rejustify );
    m_checkAutoplaceAlign->SetValue( aCfg->m_AutoplaceFields.align_to_grid );

    m_mouseDragIsDrag->SetValue( !aCfg->m_Input.drag_is_move );
    m_cbPinSelectionOpt->SetValue( aCfg->m_Selection.select_pin_selects_symbol );

    m_cbAutoStartWires->SetValue( aCfg->m_Drawing.auto_start_wires );
}


bool PANEL_EESCHEMA_EDITING_OPTIONS::TransferDataToWindow()
{
    SETTINGS_MANAGER&  mgr = Pgm().GetSettingsManager();
    EESCHEMA_SETTINGS* cfg = mgr.GetAppSettings<EESCHEMA_SETTINGS>();

    loadEEschemaSettings( cfg );

    return true;
}


bool PANEL_EESCHEMA_EDITING_OPTIONS::TransferDataFromWindow()
{
    SETTINGS_MANAGER&  mgr = Pgm().GetSettingsManager();
    EESCHEMA_SETTINGS* cfg = mgr.GetAppSettings<EESCHEMA_SETTINGS>();

    cfg->m_Drawing.default_sheet_border_color = m_borderColorSwatch->GetSwatchColor();
    cfg->m_Drawing.default_sheet_background_color = m_backgroundColorSwatch->GetSwatchColor();

    cfg->m_Drawing.default_repeat_offset_x = Iu2Mils( (int) m_hPitch.GetValue() );
    cfg->m_Drawing.default_repeat_offset_y = Iu2Mils( (int) m_vPitch.GetValue() );
    cfg->m_Drawing.repeat_label_increment = m_spinLabelRepeatStep->GetValue();

    cfg->m_Drawing.hv_lines_only = m_checkHVOrientation->GetValue();
    cfg->m_Appearance.footprint_preview = m_footprintPreview->GetValue();
    cfg->m_Appearance.navigator_stays_open = m_navigatorStaysOpen->GetValue();

    cfg->m_AutoplaceFields.enable = m_checkAutoplaceFields->GetValue();
    cfg->m_AutoplaceFields.allow_rejustify = m_checkAutoplaceJustify->GetValue();
    cfg->m_AutoplaceFields.align_to_grid = m_checkAutoplaceAlign->GetValue();

    cfg->m_Input.drag_is_move = !m_mouseDragIsDrag->GetValue();
    cfg->m_Selection.select_pin_selects_symbol = m_cbPinSelectionOpt->GetValue();

    cfg->m_Drawing.auto_start_wires = m_cbAutoStartWires->GetValue();

    return true;
}


void PANEL_EESCHEMA_EDITING_OPTIONS::ResetPanel()
{
    EESCHEMA_SETTINGS cfg;
    cfg.Load();             // Loading without a file will init to defaults

    loadEEschemaSettings( &cfg );
}


