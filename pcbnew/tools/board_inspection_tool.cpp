/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <pcb_group.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/edit_tool.h>
#include <pcb_painter.h>
#include <connectivity/connectivity_data.h>
#include <dialogs/wx_html_report_box.h>
#include <drc/drc_engine.h>
#include <dialogs/panel_setup_rules_base.h>
#include <dialogs/dialog_constraints_reporter.h>
#include <string_utils.h>
#include "board_inspection_tool.h"
#include <pcbnew_settings.h>
#include <widgets/appearance_controls.h>
#include <drc/drc_item.h>
#include <pad.h>


BOARD_INSPECTION_TOOL::BOARD_INSPECTION_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InspectionTool" ),
        m_frame( nullptr )
{
    m_probingSchToPcb = false;
    m_dynamicData     = nullptr;
}


class NET_CONTEXT_MENU : public ACTION_MENU
{
public:
    NET_CONTEXT_MENU() : ACTION_MENU( true )
    {
        SetIcon( BITMAPS::show_ratsnest );
        SetTitle( _( "Net Tools" ) );

        Add( PCB_ACTIONS::showNet );
        Add( PCB_ACTIONS::hideNet );
        Add( PCB_ACTIONS::highlightNetSelection );
        Add( PCB_ACTIONS::clearHighlight );
    }

private:
    ACTION_MENU* create() const override
    {
        return new NET_CONTEXT_MENU();
    }
};


bool BOARD_INSPECTION_TOOL::Init()
{
    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    auto netSubMenu = std::make_shared<NET_CONTEXT_MENU>();
    netSubMenu->SetTool( this );

    static KICAD_T connectedTypes[] = { PCB_TRACE_T, PCB_VIA_T, PCB_ARC_T, PCB_PAD_T, PCB_ZONE_T,
                                        EOT };

    CONDITIONAL_MENU& menu = selectionTool->GetToolMenu().GetMenu();

    selectionTool->GetToolMenu().AddSubMenu( netSubMenu );

    menu.AddMenu( netSubMenu.get(), SELECTION_CONDITIONS::OnlyTypes( connectedTypes ), 200 );
    menu.AddItem( PCB_ACTIONS::inspectClearance, SELECTION_CONDITIONS::Count( 2 ), 200 );

    return true;
}


void BOARD_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


int BOARD_INSPECTION_TOOL::ShowStatisticsDialog( const TOOL_EVENT& aEvent )
{
    DIALOG_BOARD_STATISTICS dialog( m_frame );
    dialog.ShowModal();
    return 0;
}


DRC_ENGINE BOARD_INSPECTION_TOOL::makeDRCEngine( bool* aCompileError, bool* aCourtyardError )
{
    DRC_ENGINE engine( m_frame->GetBoard(), &m_frame->GetBoard()->GetDesignSettings() );

    try
    {
        engine.InitEngine( m_frame->GetDesignRulesPath() );
    }
    catch( PARSE_ERROR& )
    {
        if( aCompileError )
            *aCompileError = true;
    }

    for( ZONE* zone : m_frame->GetBoard()->Zones() )
        zone->CacheBoundingBox();

    for( FOOTPRINT* footprint : m_frame->GetBoard()->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
            zone->CacheBoundingBox();

        footprint->BuildPolyCourtyards();

        if( aCourtyardError && ( footprint->GetFlags() & MALFORMED_COURTYARDS ) != 0 )
            *aCourtyardError = true;
    }

    return engine;
}


wxString BOARD_INSPECTION_TOOL::getItemDescription( BOARD_ITEM* aItem )
{
    // Null items have no description
    if( !aItem )
        return wxString();

    wxString s = aItem->GetSelectMenuText( m_frame->GetUserUnits() );

    if( aItem->IsConnected() )
    {
        BOARD_CONNECTED_ITEM* cItem = static_cast<BOARD_CONNECTED_ITEM*>( aItem );
        s += wxS( " " ) + wxString::Format( _( "[netclass %s]" ),
                                            cItem->GetNetClass()->GetName() );
    }

    return s;
};


void BOARD_INSPECTION_TOOL::reportCompileError( REPORTER* r )
{
    r->Report( "" );
    r->Report( _( "Report incomplete: could not compile custom design rules.  " )
               + "<a href='boardsetup'>" + _( "Show design rules." ) + "</a>" );
}


void BOARD_INSPECTION_TOOL::reportHeader( const wxString& aTitle, BOARD_ITEM* a, REPORTER* r )
{
    r->Report( "<h7>" + EscapeHTML( aTitle ) + "</h7>" );
    r->Report( "<ul><li>" + EscapeHTML( getItemDescription( a ) ) + "</li></ul>" );
}


void BOARD_INSPECTION_TOOL::reportHeader( const wxString& aTitle, BOARD_ITEM* a, BOARD_ITEM* b,
                                          REPORTER* r )
{
    r->Report( "<h7>" + EscapeHTML( aTitle ) + "</h7>" );
    r->Report( "<ul><li>" + EscapeHTML( getItemDescription( a ) ) + "</li>"
                 + "<li>" + EscapeHTML( getItemDescription( b ) ) + "</li></ul>" );
}


void BOARD_INSPECTION_TOOL::reportHeader( const wxString& aTitle, BOARD_ITEM* a, BOARD_ITEM* b,
                                          PCB_LAYER_ID aLayer, REPORTER* r )
{
    wxString layerStr = _( "Layer" ) + wxS( " " ) + m_frame->GetBoard()->GetLayerName( aLayer );

    r->Report( "<h7>" + EscapeHTML( aTitle ) + "</h7>" );
    r->Report( "<ul><li>" + EscapeHTML( layerStr ) + "</li>"
                 + "<li>" + EscapeHTML( getItemDescription( a ) ) + "</li>"
                 + "<li>" + EscapeHTML( getItemDescription( b ) ) + "</li></ul>" );
}


void BOARD_INSPECTION_TOOL::InspectDRCError( const std::shared_ptr<RC_ITEM>& aDRCItem )
{
    BOARD_ITEM*  a = m_frame->GetBoard()->GetItem( aDRCItem->GetMainItemID() );
    BOARD_ITEM*  b = m_frame->GetBoard()->GetItem( aDRCItem->GetAuxItemID() );
    PCB_LAYER_ID layer = m_frame->GetActiveLayer();

    if( !a || !b )
        return;

    if( m_inspectClearanceDialog == nullptr )
    {
        m_inspectClearanceDialog = std::make_unique<DIALOG_CONSTRAINTS_REPORTER>( m_frame );
        m_inspectClearanceDialog->SetTitle( _( "Clearance Report" ) );

        m_inspectClearanceDialog->Connect( wxEVT_CLOSE_WINDOW,
                    wxCommandEventHandler( BOARD_INSPECTION_TOOL::onInspectClearanceDialogClosed ),
                    nullptr, this );
    }

    WX_HTML_REPORT_BOX* r = nullptr;
    bool                compileError = false;
    DRC_ENGINE          drcEngine = makeDRCEngine( &compileError );
    DRC_CONSTRAINT      constraint;
    int                 clearance = 0;
    wxString            clearanceStr;

    switch( aDRCItem->GetErrorCode() )
    {
    case DRCE_EDGE_CLEARANCE:
        r = m_inspectClearanceDialog->AddPage( _( "Clearance" ) );
        reportHeader( _( "Edge clearance resolution for:" ), a, b, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine.EvalRules( EDGE_CLEARANCE_CONSTRAINT, a, b, layer, r );
        clearance = constraint.m_Value.Min();
        clearanceStr = StringFromValue( r->GetUnits(), clearance, true );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved clearance: %s." ), clearanceStr ) );
        break;

    case DRCE_CLEARANCE:
        if( a->Type() == PCB_TRACE_T || a->Type() == PCB_ARC_T )
        {
            layer = a->GetLayer();
        }
        else if( b->Type() == PCB_TRACE_T || b->Type() == PCB_ARC_T )
        {
            layer = b->GetLayer();
        }
        else if( a->Type() == PCB_PAD_T && static_cast<PAD*>( a )->GetAttribute() == PAD_ATTRIB::SMD )
        {
            PAD* pad = static_cast<PAD*>( a );

            if( pad->IsOnLayer( F_Cu ) )
                layer = F_Cu;
            else
                layer = B_Cu;
        }
        else if( b->Type() == PCB_PAD_T && static_cast<PAD*>( a )->GetAttribute() == PAD_ATTRIB::SMD )
        {
            PAD* pad = static_cast<PAD*>( b );

            if( pad->IsOnLayer( F_Cu ) )
                layer = F_Cu;
            else
                layer = B_Cu;
        }

        r = m_inspectClearanceDialog->AddPage( _( "Clearance" ) );
        reportHeader( _( "Clearance resolution for:" ), a, b, layer, r );

        if( compileError )
            reportCompileError( r );

        constraint = drcEngine.EvalRules( CLEARANCE_CONSTRAINT, a, a, layer, r );
        clearance = constraint.m_Value.Min();
        clearanceStr = StringFromValue( r->GetUnits(), clearance, true );

        r->Report( "" );
        r->Report( wxString::Format( _( "Resolved clearance: %s." ), clearanceStr ) );
        break;

    default:
        return;
    }

    r->Flush();

    m_inspectClearanceDialog->Raise();
    m_inspectClearanceDialog->Show( true );
}


bool hasHole( BOARD_ITEM* aItem )
{
    PAD* pad = dynamic_cast<PAD*>( aItem );

    if( pad && pad->GetDrillSizeX() > 0 && pad->GetDrillSizeY() > 0 )
        return true;

    PCB_VIA* via = dynamic_cast<PCB_VIA*>( aItem );

    if( via )
        return true;

    return false;
};


int BOARD_INSPECTION_TOOL::InspectClearance( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() != 2 )
    {
        m_frame->ShowInfoBarError( _( "Select two items for a clearance resolution report." ) );
        return 0;
    }

    BOARD_ITEM* a = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) );
    BOARD_ITEM* b = static_cast<BOARD_ITEM*>( selection.GetItem( 1 ) );

    wxCHECK( a && b, 0 );

    if( a->Type() == PCB_GROUP_T )
    {
        PCB_GROUP* ag = static_cast<PCB_GROUP*>( a );

        if( ag->GetItems().empty() )
        {
            m_frame->ShowInfoBarError( _( "Cannot generate clearance report on empty group." ) );
            return 0;
        }

        a = *ag->GetItems().begin();
    }

    if( b->Type() == PCB_GROUP_T )
    {
        PCB_GROUP* bg = static_cast<PCB_GROUP*>( b );

        if( bg->GetItems().empty() )
        {
            m_frame->ShowInfoBarError( _( "Cannot generate clearance report on empty group." ) );
            return 0;
        }

        b = *bg->GetItems().begin();
    }

    // a and b could be null after group tests above.
    wxCHECK( a && b, 0 );

    if( m_inspectClearanceDialog == nullptr )
    {
        m_inspectClearanceDialog = std::make_unique<DIALOG_CONSTRAINTS_REPORTER>( m_frame );
        m_inspectClearanceDialog->SetTitle( _( "Clearance Report" ) );

        m_inspectClearanceDialog->Connect( wxEVT_CLOSE_WINDOW,
                    wxCommandEventHandler( BOARD_INSPECTION_TOOL::onInspectClearanceDialogClosed ),
                    nullptr, this );
    }

    m_inspectClearanceDialog->DeleteAllPages();

    if( a->Type() != PCB_ZONE_T && b->Type() == PCB_ZONE_T )
        std::swap( a, b );
    else if( !a->IsConnected() && b->IsConnected() )
        std::swap( a, b );

    WX_HTML_REPORT_BOX*   r = nullptr;
    EDA_UNITS             units = m_frame->GetUserUnits();

    PCB_LAYER_ID          active = m_frame->GetActiveLayer();
    LSET                  layerIntersection = a->GetLayerSet() & b->GetLayerSet();
    LSET                  copperIntersection = layerIntersection & LSET::AllCuMask();
    BOARD_CONNECTED_ITEM* ac = dynamic_cast<BOARD_CONNECTED_ITEM*>( a );
    BOARD_CONNECTED_ITEM* bc = dynamic_cast<BOARD_CONNECTED_ITEM*>( b );
    ZONE*                 zone = dynamic_cast<ZONE*>( a );
    PAD*                  pad = dynamic_cast<PAD*>( b );
    FOOTPRINT*            aFP = dynamic_cast<FOOTPRINT*>( a );
    FOOTPRINT*            bFP = dynamic_cast<FOOTPRINT*>( b );

    bool                  compileError = false;
    DRC_ENGINE            drcEngine = makeDRCEngine( &compileError );
    DRC_CONSTRAINT        constraint;
    int                   clearance = 0;

    if( copperIntersection.any() && zone && pad && zone->GetNetCode() == pad->GetNetCode() )
    {
        PCB_LAYER_ID layer = active;

        if( !zone->IsOnLayer( active ) )
            layer = zone->GetLayerSet().Seq().front();

        r = m_inspectClearanceDialog->AddPage( _( "Zone" ) );
        reportHeader( _( "Zone connection resolution for:" ), a, b, layer, r );

        constraint = drcEngine.EvalZoneConnection( pad, zone, layer, r );

        if( constraint.m_ZoneConnection == ZONE_CONNECTION::THERMAL )
        {
            constraint = drcEngine.EvalRules( THERMAL_RELIEF_GAP_CONSTRAINT, pad, zone, layer, r );
            int gap = constraint.m_Value.Min();

            r->Report( wxString::Format( _( "Resolved thermal relief gap: %s." ),
                                         StringFromValue( units, gap, true ) ) );

            constraint = drcEngine.EvalRules( THERMAL_SPOKE_WIDTH_CONSTRAINT, pad, zone, layer, r );
            int width = constraint.m_Value.Opt();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved thermal spoke width: %s." ),
                                         StringFromValue( units, width, true ) ) );
        }
        else if( constraint.m_ZoneConnection == ZONE_CONNECTION::NONE )
        {
            clearance = zone->GetLocalClearance();
            r->Report( wxString::Format( _( "Zone clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );

            constraint = drcEngine.EvalRules( THERMAL_RELIEF_GAP_CONSTRAINT, pad, zone, layer, r );

            if( constraint.m_Value.Min() > clearance )
            {
                clearance = constraint.m_Value.Min();
                r->Report( wxString::Format( _( "Overridden by larger thermal relief from %s;"
                                                "clearance: %s." ),
                                             EscapeHTML( constraint.GetName() ),
                                             StringFromValue( units, clearance, true ) ) );
            }

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Clearance: %s." ),
                                         StringFromValue( units, 0, true ) ) );
        }
        else
        {
            if( compileError )
                reportCompileError( r );

            // Report a 0 clearance for solid connections
            r->Report( "" );
            r->Report( wxString::Format( _( "Clearance: %s." ),
                                         StringFromValue( units, 0, true ) ) );
        }

        r->Flush();
    }
    else if( copperIntersection.any() && !aFP && !bFP )
    {
        PCB_LAYER_ID layer = active;

        if( !copperIntersection.test( layer ) )
            layer = copperIntersection.Seq().front();

        r = m_inspectClearanceDialog->AddPage( m_frame->GetBoard()->GetLayerName( layer ) );
        reportHeader( _( "Clearance resolution for:" ), a, b, layer, r );

        if( ac && bc && ac->GetNetCode() > 0 && ac->GetNetCode() == bc->GetNetCode() )
        {
            // Same nets....
            r->Report( _( "Items belong to the same net. Clearance is 0." ) );
        }
        else
        {
            // Different nets (or one or both unconnected)....
            constraint = drcEngine.EvalRules( CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );
        }

        r->Flush();
    }

    for( PCB_LAYER_ID layer : { F_SilkS, B_SilkS } )
    {
        PCB_LAYER_ID correspondingMask = IsFrontLayer( layer ) ? F_Mask : B_Mask;

        if( ( a->IsOnLayer( layer ) && b->IsOnLayer( layer ) )
                || ( a->IsOnLayer( layer ) && b->IsOnLayer( correspondingMask ) )
                || ( b->IsOnLayer( layer ) && a->IsOnLayer( correspondingMask ) ) )
        {
            r = m_inspectClearanceDialog->AddPage( m_frame->GetBoard()->GetLayerName( layer ) );
            reportHeader( _( "Silkscreen clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine.EvalRules( SILK_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );

            r->Flush();
        }
    }

    for( PCB_LAYER_ID layer : { F_CrtYd, B_CrtYd } )
    {
        bool aCourtyard = aFP && !aFP->GetPolyCourtyard( layer ).IsEmpty();
        bool bCourtyard = bFP && !bFP->GetPolyCourtyard( layer ).IsEmpty();

        if( aCourtyard && bCourtyard )
        {
            r = m_inspectClearanceDialog->AddPage( m_frame->GetBoard()->GetLayerName( layer ) );
            reportHeader( _( "Courtyard clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine.EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );

            r->Flush();
        }
    }

    if( hasHole( a ) || hasHole( b ) )
    {
        PCB_LAYER_ID layer = UNDEFINED_LAYER;

        if( hasHole( a ) && b->IsOnLayer( active ) && IsCopperLayer( active ) )
            layer = active;
        else if( hasHole( b ) && a->IsOnLayer( active ) && IsCopperLayer( active ) )
            layer = active;
        else if( hasHole( a ) && IsCopperLayer( b->GetLayer() ) )
            layer = b->GetLayer();
        else if( hasHole( b ) && IsCopperLayer( a->GetLayer() ) )
            layer = a->GetLayer();

        if( layer >= 0 )
        {
            r = m_inspectClearanceDialog->AddPage( _( "Hole" ) );
            reportHeader( _( "Hole clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine.EvalRules( HOLE_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );

            r->Flush();
        }
    }

    for( PCB_LAYER_ID edgeLayer : { Edge_Cuts, Margin } )
    {
        PCB_LAYER_ID layer = UNDEFINED_LAYER;

        if( a->IsOnLayer( edgeLayer ) && b->IsOnLayer( active ) && IsCopperLayer( active ) )
            layer = active;
        else if( b->IsOnLayer( edgeLayer ) && a->IsOnLayer( active ) && IsCopperLayer( active ) )
            layer = active;
        else if( a->IsOnLayer( edgeLayer ) && IsCopperLayer( b->GetLayer() ) )
            layer = b->GetLayer();
        else if( b->IsOnLayer( edgeLayer ) && IsCopperLayer( a->GetLayer() ) )
            layer = a->GetLayer();

        if( layer >= 0 )
        {
            wxString layerName = m_frame->GetBoard()->GetLayerName( edgeLayer );
            r = m_inspectClearanceDialog->AddPage( layerName + wxS( " " ) + _( "Clearance" ) );
            reportHeader( _( "Edge clearance resolution for:" ), a, b, layer, r );

            constraint = drcEngine.EvalRules( EDGE_CLEARANCE_CONSTRAINT, a, b, layer, r );
            clearance = constraint.m_Value.Min();

            if( compileError )
                reportCompileError( r );

            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );

            r->Flush();
        }
    }

    r = m_inspectClearanceDialog->AddPage( _( "Mechanical" ) );

    if( layerIntersection.any() )
    {
        PCB_LAYER_ID layer = active;

        if( !layerIntersection.test( layer ) )
            layer = layerIntersection.Seq().front();

        reportHeader( _( "Mechanical clearance resolution for:" ), a, b, layer, r );

        constraint = drcEngine.EvalRules( MECHANICAL_CLEARANCE_CONSTRAINT, a, b, layer, r );
        clearance = constraint.m_Value.Min();

        if( compileError )
        {
            reportCompileError( r );
        }
        else if( !drcEngine.HasRulesForConstraintType( MECHANICAL_CLEARANCE_CONSTRAINT ) )
        {
            r->Report( "" );
            r->Report( _( "No 'mechanical_clearance' constraints defined." ) );
        }
        else
        {
            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );
        }

        r->Report( "" );
        r->Report( "" );
        r->Report( "" );
    }

    if( hasHole( a ) || hasHole( b ) )
    {
        PCB_LAYER_ID layer;

        if( hasHole( a ) && b->IsOnLayer( active ) )
            layer = active;
        else if( hasHole( b ) && a->IsOnLayer( active ) )
            layer = active;
        else if( hasHole( a ) )
            layer = b->GetLayer();
        else
            layer = a->GetLayer();

        reportHeader( _( "Mechanical hole clearance resolution for:" ), a, b, layer, r );

        constraint = drcEngine.EvalRules( MECHANICAL_HOLE_CLEARANCE_CONSTRAINT, a, b, layer, r );
        clearance = constraint.m_Value.Min();

        if( compileError )
        {
            reportCompileError( r );
        }
        else if( !drcEngine.HasRulesForConstraintType( MECHANICAL_HOLE_CLEARANCE_CONSTRAINT ) )
        {
            r->Report( "" );
            r->Report( _( "No 'mechanical_hole_clearance' constraints defined." ) );
        }
        else
        {
            r->Report( "" );
            r->Report( wxString::Format( _( "Resolved clearance: %s." ),
                                         StringFromValue( units, clearance, true ) ) );
        }
    }

    r->Flush();

    m_inspectClearanceDialog->Raise();
    m_inspectClearanceDialog->Show( true );
    return 0;
}


wxString reportMin( EDA_UNITS aUnits, DRC_CONSTRAINT& aConstraint )
{
    if( aConstraint.m_Value.HasMin() )
        return StringFromValue( aUnits, aConstraint.m_Value.Min(), true );
    else
        return wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
}


wxString reportOpt( EDA_UNITS aUnits, DRC_CONSTRAINT& aConstraint )
{
    if( aConstraint.m_Value.HasOpt() )
        return StringFromValue( aUnits, aConstraint.m_Value.Opt(), true );
    else
        return wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
}


wxString reportMax( EDA_UNITS aUnits, DRC_CONSTRAINT& aConstraint )
{
    if( aConstraint.m_Value.HasMax() )
        return StringFromValue( aUnits, aConstraint.m_Value.Max(), true );
    else
        return wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
}


int BOARD_INSPECTION_TOOL::InspectConstraints( const TOOL_EVENT& aEvent )
{
#define EVAL_RULES( constraint, a, b, layer, r ) drcEngine.EvalRules( constraint, a, b, layer, r )

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() != 1 )
    {
        m_frame->ShowInfoBarError( _( "Select an item for a constraints resolution report." ) );
        return 0;
    }

    if( m_inspectConstraintsDialog == nullptr )
    {
        m_inspectConstraintsDialog = std::make_unique<DIALOG_CONSTRAINTS_REPORTER>( m_frame );
        m_inspectConstraintsDialog->SetTitle( _( "Constraints Report" ) );

        m_inspectConstraintsDialog->Connect( wxEVT_CLOSE_WINDOW,
                wxCommandEventHandler( BOARD_INSPECTION_TOOL::onInspectConstraintsDialogClosed ),
                nullptr, this );
    }

    m_inspectConstraintsDialog->DeleteAllPages();

    BOARD_ITEM*    item = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) );
    bool           compileError = false;
    bool           courtyardError = false;
    DRC_ENGINE     drcEngine = makeDRCEngine( &compileError, &courtyardError );
    DRC_CONSTRAINT constraint;

    WX_HTML_REPORT_BOX* r = nullptr;
    
    if( item->Type() == PCB_TRACE_T )
    {
        r = m_inspectConstraintsDialog->AddPage( _( "Track Width" ) );
        reportHeader( _( "Track width resolution for:" ), item, r );

        constraint = EVAL_RULES( TRACK_WIDTH_CONSTRAINT, item, nullptr, item->GetLayer(), r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Width constraints: min %s; opt %s; max %s." ),
                                     reportMin( r->GetUnits(),  constraint ),
                                     reportOpt( r->GetUnits(),  constraint ),
                                     reportMax( r->GetUnits(),  constraint ) ) );

        r->Flush();
    }

    if( item->Type() == PCB_VIA_T )
    {
        r = m_inspectConstraintsDialog->AddPage( _( "Via Diameter" ) );
        reportHeader( _( "Via diameter resolution for:" ), item, r );

        // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
        constraint = EVAL_RULES( VIA_DIAMETER_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Diameter constraints: min %s; opt %s; max %s." ),
                                     reportMin( r->GetUnits(),  constraint ),
                                     reportOpt( r->GetUnits(),  constraint ),
                                     reportMax( r->GetUnits(),  constraint ) ) );

        r->Flush();

        r = m_inspectConstraintsDialog->AddPage( _( "Via Annular Width" ) );
        reportHeader( _( "Via annular width resolution for:" ), item, r );

        // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
        constraint = EVAL_RULES( ANNULAR_WIDTH_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        r->Report( "" );
        r->Report( wxString::Format( _( "Annular width constraints: min %s; opt %s; max %s." ),
                                     reportMin( r->GetUnits(),  constraint ),
                                     reportOpt( r->GetUnits(),  constraint ),
                                     reportMax( r->GetUnits(),  constraint ) ) );

        r->Flush();
    }

    if( ( item->Type() == PCB_PAD_T && static_cast<PAD*>( item )->GetDrillSize().x > 0 )
            || item->Type() == PCB_VIA_T )
    {
        r = m_inspectConstraintsDialog->AddPage( _( "Hole Size" ) );
        reportHeader( _( "Hole diameter resolution for:" ), item, r );

        // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
        constraint = EVAL_RULES( HOLE_SIZE_CONSTRAINT, item, nullptr, UNDEFINED_LAYER, r );

        if( compileError )
            reportCompileError( r );

        wxString min = _( "undefined" );

        if( constraint.m_Value.HasMin() )
            min = StringFromValue( r->GetUnits(), constraint.m_Value.Min(), true );

        r->Report( "" );
        r->Report( wxString::Format( _( "Hole constraint: min %s." ), min ) );

        r->Flush();
    }

    r = m_inspectConstraintsDialog->AddPage( _( "Keepouts" ) );
    reportHeader( _( "Keepout resolution for:" ), item, r );

    constraint = EVAL_RULES( DISALLOW_CONSTRAINT, item, nullptr, item->GetLayer(), r );

    if( compileError )
        reportCompileError( r );

    if( courtyardError )
    {
        r->Report( "" );
        r->Report( _( "Report may be incomplete: some footprint courtyards are malformed." )
                   + "  <a href='drc'>" + _( "Run DRC for a full analysis." ) + "</a>" );
    }

    r->Report( "" );

    if( constraint.m_DisallowFlags )
        r->Report( _( "Item <b>disallowed</b> at current location." ) );
    else
        r->Report( _( "Item allowed at current location." ) );

    r->Flush();

    r = m_inspectConstraintsDialog->AddPage( _( "Assertions" ) );
    reportHeader( _( "Assertions for:" ), item, r );

    if( compileError )
        reportCompileError( r );

    if( courtyardError )
    {
        r->Report( "" );
        r->Report( _( "Report may be incomplete: some footprint courtyards are malformed." )
                   + "  <a href='drc'>" + _( "Run DRC for a full analysis." ) + "</a>" );
    }

    drcEngine.ProcessAssertions( item, []( const DRC_CONSTRAINT* c ){}, r );
    r->Flush();

    m_inspectConstraintsDialog->FinishInitialization();
    m_inspectConstraintsDialog->Raise();
    m_inspectConstraintsDialog->Show( true );
    return 0;
}


int BOARD_INSPECTION_TOOL::CrossProbePcbToSch( const TOOL_EVENT& aEvent )
{
    // Don't get in an infinite loop PCB -> SCH -> PCB -> SCH -> ...
    if( m_probingSchToPcb )
        return 0;

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() == 1 )
        m_frame->SendMessageToEESCHEMA( static_cast<BOARD_ITEM*>( selection.Front() ) );
    else
        m_frame->SendMessageToEESCHEMA( nullptr );

    // Update 3D viewer highlighting
    m_frame->Update3DView( false, frame()->GetDisplayOptions().m_Live3DRefresh );

    return 0;
}


int BOARD_INSPECTION_TOOL::HighlightItem( const TOOL_EVENT& aEvent )
{
    BOARD_ITEM* item = aEvent.Parameter<BOARD_ITEM*>();

    m_probingSchToPcb = true;   // recursion guard
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        if( item )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, (void*) item );
    }
    m_probingSchToPcb = false;

    bool request3DviewRedraw = frame()->GetDisplayOptions().m_Live3DRefresh;

    if( item && item->Type() != PCB_FOOTPRINT_T )
        request3DviewRedraw = false;

    // Update 3D viewer highlighting
    if( request3DviewRedraw )
        m_frame->Update3DView( false, true );

    return 0;
}


 bool BOARD_INSPECTION_TOOL::highlightNet( const VECTOR2D& aPosition, bool aUseSelection )
{
    BOARD*                  board         = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* settings      = getView()->GetPainter()->GetSettings();
    PCB_SELECTION_TOOL*     selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    int net = -1;
    bool enableHighlight = false;

    if( aUseSelection )
    {
        const PCB_SELECTION& selection = selectionTool->GetSelection();
        std::set<int> netcodes;

        for( EDA_ITEM* item : selection )
        {
            if( BOARD_CONNECTED_ITEM* ci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                netcodes.insert( ci->GetNetCode() );
        }

        enableHighlight = !netcodes.empty();

        if( enableHighlight && netcodes.size() > 1 )
        {
            // If we are doing a multi-highlight, cross-probing back and other stuff is not
            // yet supported
            settings->SetHighlight( netcodes );
            board->ResetNetHighLight();

            for( int multiNet : netcodes )
                board->SetHighLightNet( multiNet, true );

            board->HighLightON();
            m_toolMgr->GetView()->UpdateAllLayersColor();
            m_currentlyHighlighted = netcodes;
            return true;
        }
        else if( enableHighlight )
        {
            net = *netcodes.begin();
        }
    }

    // If we didn't get a net to highlight from the selection, use the cursor
    if( net < 0 )
    {
        GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
        guide.SetIgnoreZoneFills( false );

        PCB_LAYER_ID activeLayer = static_cast<PCB_LAYER_ID>( view()->GetTopLayer() );
        guide.SetPreferredLayer( activeLayer );

        GENERAL_COLLECTOR collector;
        collector.Collect( board, GENERAL_COLLECTOR::PadsOrTracks, (wxPoint) aPosition, guide );

        if( collector.GetCount() == 0 )
            collector.Collect( board, GENERAL_COLLECTOR::Zones, (wxPoint) aPosition, guide );

        // Apply the active selection filter, except we want to allow picking locked items for
        // highlighting even if the user has disabled them for selection
        SELECTION_FILTER_OPTIONS& filter = selectionTool->GetFilter();

        bool saved         = filter.lockedItems;
        filter.lockedItems = true;

        selectionTool->FilterCollectedItems( collector, true );

        filter.lockedItems = saved;

        // Clear the previous highlight
        m_frame->SendMessageToEESCHEMA( nullptr );

        bool         highContrast  = settings->GetHighContrast();
        PCB_LAYER_ID contrastLayer = settings->GetPrimaryHighContrastLayer();

        for( int i = collector.GetCount() - 1; i >= 0; i-- )
        {
            LSET itemLayers = collector[i]->GetLayerSet();

            if( ( itemLayers & LSET::AllCuMask() ).none() ||
                ( highContrast && !itemLayers.Contains( contrastLayer ) ) )
            {
                collector.Remove( i );
                continue;
            }
        }

        enableHighlight = ( collector.GetCount() > 0 );

        // Obtain net code for the clicked item
        if( enableHighlight )
        {
            BOARD_CONNECTED_ITEM* targetItem = static_cast<BOARD_CONNECTED_ITEM*>( collector[0] );

            if( targetItem->Type() == PCB_PAD_T )
                m_frame->SendMessageToEESCHEMA( targetItem );

            net = targetItem->GetNetCode();
        }
    }

    auto& netcodes = settings->GetHighlightNetCodes();

    // Toggle highlight when the same net was picked
    if( net > 0 && netcodes.count( net ) )
        enableHighlight = !settings->IsHighlightEnabled();

    if( enableHighlight != settings->IsHighlightEnabled() || !netcodes.count( net ) )
    {
        if( !netcodes.empty() )
            m_lastHighlighted = netcodes;

        settings->SetHighlight( enableHighlight, net );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }

    // Store the highlighted netcode in the current board (for dialogs for instance)
    if( enableHighlight && net >= 0 )
    {
        m_currentlyHighlighted = netcodes;
        board->SetHighLightNet( net );
        board->HighLightON();

        NETINFO_ITEM* netinfo = board->FindNet( net );

        if( netinfo )
        {
            std::vector<MSG_PANEL_ITEM> items;
            netinfo->GetMsgPanelInfo( m_frame, items );
            m_frame->SetMsgPanel( items );
            m_frame->SendCrossProbeNetName( netinfo->GetNetname() );
        }
    }
    else
    {
        m_currentlyHighlighted.clear();
        board->ResetNetHighLight();
        m_frame->SetMsgPanel( board );
        m_frame->SendCrossProbeNetName( "" );
    }

    return true;
}


int BOARD_INSPECTION_TOOL::HighlightNet( const TOOL_EVENT& aEvent )
{
    int                     netcode     = aEvent.Parameter<intptr_t>();
    KIGFX::RENDER_SETTINGS* settings    = m_toolMgr->GetView()->GetPainter()->GetSettings();
    const std::set<int>&    highlighted = settings->GetHighlightNetCodes();

    if( netcode > 0 )
    {
        m_lastHighlighted = highlighted;
        settings->SetHighlight( true, netcode );
        m_toolMgr->GetView()->UpdateAllLayersColor();
        m_currentlyHighlighted.clear();
        m_currentlyHighlighted.insert( netcode );
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::highlightNetSelection ) )
    {
        // Highlight selection (cursor position will be ignored)
        highlightNet( getViewControls()->GetMousePosition(), true );
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::toggleLastNetHighlight ) )
    {
        std::set<int> temp = highlighted;
        settings->SetHighlight( m_lastHighlighted );
        m_toolMgr->GetView()->UpdateAllLayersColor();
        m_currentlyHighlighted = m_lastHighlighted;
        m_lastHighlighted      = temp;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::toggleNetHighlight ) )
    {
        bool turnOn = highlighted.empty() && !m_currentlyHighlighted.empty();
        settings->SetHighlight( m_currentlyHighlighted, turnOn );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }
    else    // Highlight the net belonging to the item under the cursor
    {
        highlightNet( getViewControls()->GetMousePosition(), false );
    }

    return 0;
}


int BOARD_INSPECTION_TOOL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    BOARD*                  board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* settings = m_toolMgr->GetView()->GetPainter()->GetSettings();

    m_currentlyHighlighted.clear();
    m_lastHighlighted.clear();

    board->ResetNetHighLight();
    settings->SetHighlight( false );
    m_toolMgr->GetView()->UpdateAllLayersColor();
    m_frame->SetMsgPanel( board );
    m_frame->SendCrossProbeNetName( "" );
    return 0;
}

#if 0
int BOARD_INSPECTION_TOOL::HighlightNetTool( const TOOL_EVENT& aEvent )
{
    std::string      tool = aEvent.GetCommandStr().get();
    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    // If the keyboard hotkey was triggered and we are already in the highlight tool, behave
    // the same as a left-click.  Otherwise highlight the net of the selected item(s), or if
    // there is no selection, then behave like a ctrl-left-click.
    if( aEvent.IsAction( &PCB_ACTIONS::highlightNetSelection ) )
    {
        bool use_selection = m_frame->IsCurrentTool( PCB_ACTIONS::highlightNetTool );
        highlightNet( getViewControls()->GetMousePosition(), use_selection );
    }

    picker->SetClickHandler(
        [this] ( const VECTOR2D& pt ) -> bool
        {
            highlightNet( pt, false );
            return true;
        } );

    picker->SetLayerSet( LSET::AllCuMask() );
    picker->SetSnapping( false );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}
#endif

int BOARD_INSPECTION_TOOL::LocalRatsnestTool( const TOOL_EVENT& aEvent )
{
    std::string       tool = aEvent.GetCommandStr().get();
    PCB_PICKER_TOOL*  picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    BOARD*            board = getModel<BOARD>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetClickHandler(
        [this, board]( const VECTOR2D& pt ) -> bool
        {
            const PCB_DISPLAY_OPTIONS& opt = displayOptions();
            PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
            m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true, EDIT_TOOL::PadFilter );
            PCB_SELECTION& selection = selectionTool->GetSelection();

            if( selection.Empty() )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true,
                                      EDIT_TOOL::FootprintFilter );
                selection = selectionTool->GetSelection();
            }

            if( selection.Empty() )
            {
                // Clear the previous local ratsnest if we click off all items
                for( FOOTPRINT* fp : board->Footprints() )
                {
                    for( PAD* pad : fp->Pads() )
                        pad->SetLocalRatsnestVisible( opt.m_ShowGlobalRatsnest );
                }
            }
            else
            {
                for( EDA_ITEM* item : selection )
                {
                    if( PAD* pad = dyn_cast<PAD*>( item) )
                    {
                        pad->SetLocalRatsnestVisible( !pad->GetLocalRatsnestVisible() );
                    }
                    else if( FOOTPRINT* fp = dyn_cast<FOOTPRINT*>( item) )
                    {
                        if( !fp->Pads().empty() )
                        {
                            bool enable = !fp->Pads()[0]->GetLocalRatsnestVisible();

                            for( PAD* childPad : fp->Pads() )
                                childPad->SetLocalRatsnestVisible( enable );
                        }
                    }
                }
            }

            m_toolMgr->GetView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );

            return true;
        } );

    picker->SetFinalizeHandler(
        [this, board]( int aCondition )
        {
            const PCB_DISPLAY_OPTIONS& opt = displayOptions();

            if( aCondition != PCB_PICKER_TOOL::END_ACTIVATE )
            {
                for( FOOTPRINT* fp : board->Footprints() )
                {
                    for( PAD* pad : fp->Pads() )
                        pad->SetLocalRatsnestVisible( opt.m_ShowGlobalRatsnest );
                }
            }
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


int BOARD_INSPECTION_TOOL::UpdateSelectionRatsnest( const TOOL_EVENT& aEvent )
{
    VECTOR2I  delta;

    // If we have passed the simple move vector, we can update without recalculation
    if( aEvent.Parameter<VECTOR2I*>() )
    {
        delta = *aEvent.Parameter<VECTOR2I*>();
        delete aEvent.Parameter<VECTOR2I*>();
    }
    else
    {
        // We can delete the existing map to force a recalculation
        delete m_dynamicData;
        m_dynamicData = nullptr;
    }

    auto selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    auto& selection = selectionTool->GetSelection();
    auto connectivity = getModel<BOARD>()->GetConnectivity();

    if( selection.Empty() )
    {
        connectivity->ClearDynamicRatsnest();
        delete m_dynamicData;
        m_dynamicData = nullptr;
    }
    else
    {
        calculateSelectionRatsnest( delta );
    }

    return 0;
}


int BOARD_INSPECTION_TOOL::HideDynamicRatsnest( const TOOL_EVENT& aEvent )
{
    getModel<BOARD>()->GetConnectivity()->ClearDynamicRatsnest();
    delete m_dynamicData;
    m_dynamicData = nullptr;

    return 0;
}


void BOARD_INSPECTION_TOOL::calculateSelectionRatsnest( const VECTOR2I& aDelta )
{
    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    SELECTION&          selection = selectionTool->GetSelection();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();
    std::vector<BOARD_ITEM*> items;
    std::deque<EDA_ITEM*>    queued_items( selection.begin(), selection.end() );

    for( std::size_t i = 0; i < queued_items.size(); ++i )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( queued_items[i] );

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
            {
                if( pad->GetLocalRatsnestVisible() || displayOptions().m_ShowModuleRatsnest )
                    items.push_back( pad );
            }
        }
        else if( item->Type() == PCB_GROUP_T )
        {
            PCB_GROUP *group = static_cast<PCB_GROUP*>( item );
            group->RunOnDescendants( [ &queued_items ]( BOARD_ITEM *aItem )
                                     {
                                         queued_items.push_back( aItem );
                                     } );
        }
        else if( BOARD_CONNECTED_ITEM* boardItem = dyn_cast<BOARD_CONNECTED_ITEM*>( item ) )
        {
            if( boardItem->GetLocalRatsnestVisible() || displayOptions().m_ShowModuleRatsnest )
                items.push_back( boardItem );
        }
    }

    if( items.empty() || std::none_of( items.begin(), items.end(),
                                       []( const BOARD_ITEM* aItem )
                                       {
                                           return( aItem->Type() == PCB_TRACE_T
                                                    || aItem->Type() == PCB_PAD_T
                                                    || aItem->Type() == PCB_ARC_T
                                                    || aItem->Type() == PCB_ZONE_T
                                                    || aItem->Type() == PCB_FOOTPRINT_T
                                                    || aItem->Type() == PCB_VIA_T );
                                       } ) )
    {
        return;
    }

    if( !m_dynamicData )
    {
        m_dynamicData = new CONNECTIVITY_DATA( items, true );
        connectivity->BlockRatsnestItems( items );
    }
    else
    {
        m_dynamicData->Move( aDelta );
    }

    connectivity->ComputeDynamicRatsnest( items, m_dynamicData );
}


int BOARD_INSPECTION_TOOL::ListNets( const TOOL_EVENT& aEvent )
{
    if( m_listNetsDialog == nullptr )
    {
        m_listNetsDialog =
                std::make_unique<DIALOG_NET_INSPECTOR>( m_frame, m_listNetsDialogSettings );

        m_listNetsDialog->Connect( wxEVT_CLOSE_WINDOW,
                wxCommandEventHandler( BOARD_INSPECTION_TOOL::onListNetsDialogClosed ), nullptr,
                this );

        m_listNetsDialog->Connect( wxEVT_BUTTON,
                wxCommandEventHandler( BOARD_INSPECTION_TOOL::onListNetsDialogClosed ), nullptr,
                this );
    }

    m_listNetsDialog->Raise();
    m_listNetsDialog->Show( true );
    return 0;
}


void BOARD_INSPECTION_TOOL::onListNetsDialogClosed( wxCommandEvent& event )
{
    m_listNetsDialogSettings = m_listNetsDialog->Settings();

    m_listNetsDialog->Disconnect( wxEVT_CLOSE_WINDOW,
            wxCommandEventHandler( BOARD_INSPECTION_TOOL::onListNetsDialogClosed ), nullptr, this );

    m_listNetsDialog->Disconnect( wxEVT_BUTTON,
            wxCommandEventHandler( BOARD_INSPECTION_TOOL::onListNetsDialogClosed ), nullptr, this );

    m_listNetsDialog->Destroy();
    m_listNetsDialog.release();
}


void BOARD_INSPECTION_TOOL::onInspectClearanceDialogClosed( wxCommandEvent& event )
{
    m_inspectClearanceDialog->Disconnect( wxEVT_CLOSE_WINDOW,
            wxCommandEventHandler( BOARD_INSPECTION_TOOL::onInspectClearanceDialogClosed ),
                                          nullptr, this );

    m_inspectClearanceDialog->Destroy();
    m_inspectClearanceDialog.release();
}


void BOARD_INSPECTION_TOOL::onInspectConstraintsDialogClosed( wxCommandEvent& event )
{
    m_inspectConstraintsDialog->Disconnect( wxEVT_CLOSE_WINDOW,
            wxCommandEventHandler( BOARD_INSPECTION_TOOL::onInspectConstraintsDialogClosed ),
                                            nullptr, this );

    m_inspectConstraintsDialog->Destroy();
    m_inspectConstraintsDialog.release();
}


int BOARD_INSPECTION_TOOL::HideNet( const TOOL_EVENT& aEvent )
{
    doHideNet( aEvent.Parameter<intptr_t>(), true );
    return 0;
}


int BOARD_INSPECTION_TOOL::ShowNet( const TOOL_EVENT& aEvent )
{
    doHideNet( aEvent.Parameter<intptr_t>(), false );
    return 0;
}


void BOARD_INSPECTION_TOOL::doHideNet( int aNetCode, bool aHide )
{
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_toolMgr->GetView()->GetPainter()->GetSettings() );

    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    SELECTION&          selection     = selectionTool->GetSelection();

    if( aNetCode <= 0 && !selection.Empty() )
    {
        for( EDA_ITEM* item : selection )
        {
            if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            {
                if( bci->GetNetCode() > 0 )
                    doHideNet( bci->GetNetCode(), aHide );
            }
        }

        return;
    }

    if( aHide )
        rs->GetHiddenNets().insert( aNetCode );
    else
        rs->GetHiddenNets().erase( aNetCode );

    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();

    m_frame->GetAppearancePanel()->OnNetVisibilityChanged( aNetCode, !aHide );
}


void BOARD_INSPECTION_TOOL::setTransitions()
{
    Go( &BOARD_INSPECTION_TOOL::CrossProbePcbToSch,     EVENTS::SelectedEvent );
    Go( &BOARD_INSPECTION_TOOL::CrossProbePcbToSch,     EVENTS::UnselectedEvent );
    Go( &BOARD_INSPECTION_TOOL::CrossProbePcbToSch,     EVENTS::ClearedEvent );

    Go( &BOARD_INSPECTION_TOOL::LocalRatsnestTool,
        PCB_ACTIONS::localRatsnestTool.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HideDynamicRatsnest,
        PCB_ACTIONS::hideDynamicRatsnest.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::UpdateSelectionRatsnest,
        PCB_ACTIONS::updateLocalRatsnest.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::ListNets,             PCB_ACTIONS::listNets.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ShowStatisticsDialog, PCB_ACTIONS::boardStatistics.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::InspectClearance,     PCB_ACTIONS::inspectClearance.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::InspectConstraints,   PCB_ACTIONS::inspectConstraints.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::HighlightNet,   PCB_ACTIONS::highlightNet.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,   PCB_ACTIONS::highlightNetSelection.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,   PCB_ACTIONS::toggleLastNetHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ClearHighlight, PCB_ACTIONS::clearHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightNet,   PCB_ACTIONS::toggleNetHighlight.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::HighlightItem,  PCB_ACTIONS::highlightItem.MakeEvent() );

    Go( &BOARD_INSPECTION_TOOL::HideNet,        PCB_ACTIONS::hideNet.MakeEvent() );
    Go( &BOARD_INSPECTION_TOOL::ShowNet,        PCB_ACTIONS::showNet.MakeEvent() );
}
