/**
 * @file sel_layer.cpp
 * @brief minor dialogs for one layer selection and a layer pair selection.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <confirm.h>
#include <pcb_base_frame.h>
#include <widgets/layer_box_selector.h>
#include <board.h>
#include <dialogs/dialog_layer_selection_base.h>
#include <router/router_tool.h>
#include <settings/color_settings.h>


// Column position by function:
#define SELECT_COLNUM       0
#define COLOR_COLNUM        1
#define LAYERNAME_COLNUM    2


/*
 * Display a layer list using a wxGrid.
 */
class PCB_LAYER_SELECTOR: public LAYER_SELECTOR
{
public:
    PCB_LAYER_SELECTOR( PCB_BASE_FRAME* aFrame ) :
        LAYER_SELECTOR()
    {
        m_frame = aFrame;
    }

protected:
    PCB_BASE_FRAME*  m_frame;

    ///< @return true if the layer id is enabled (i.e. is it should be displayed).
    bool isLayerEnabled( int aLayer ) const override
    {
        return m_frame->GetBoard()->IsLayerEnabled( PCB_LAYER_ID( aLayer ) );
    }

    // Return the color index from the layer ID.
    COLOR4D getLayerColor( int aLayer ) const override
    {
        return m_frame->GetColorSettings()->GetColor( aLayer );
    }

    // Return the name of the layer ID.
    wxString getLayerName( int aLayer ) const override
    {
        return m_frame->GetBoard()->GetLayerName( ToLAYER_ID( aLayer ) );
    }
};


/**
 * Display a PCB layers list in a dialog to select one layer from this list.
 */
class PCB_ONE_LAYER_SELECTOR : public PCB_LAYER_SELECTOR, public DIALOG_LAYER_SELECTION_BASE
{
public:
    PCB_ONE_LAYER_SELECTOR( PCB_BASE_FRAME* aParent, BOARD * aBrd, PCB_LAYER_ID aDefaultLayer,
                            LSET aNotAllowedLayersMask, bool aHideCheckBoxes = false );
    ~PCB_ONE_LAYER_SELECTOR();

    int GetLayerSelection()   { return m_layerSelected; }

private:
    // Event handlers
    void OnLeftGridCellClick( wxGridEvent& aEvent ) override;
    void OnRightGridCellClick( wxGridEvent& aEvent ) override;
    void OnMouseMove( wxUpdateUIEvent& aEvent ) override;

    // Will close the dialog on ESC key
    void onCharHook( wxKeyEvent& event );

    void buildList();

    PCB_LAYER_ID              m_layerSelected;
    LSET                      m_notAllowedLayersMask;
    BOARD*                    m_brd;
    std::vector<PCB_LAYER_ID> m_layersIdLeftColumn;
    std::vector<PCB_LAYER_ID> m_layersIdRightColumn;
};


PCB_ONE_LAYER_SELECTOR::PCB_ONE_LAYER_SELECTOR( PCB_BASE_FRAME* aParent, BOARD* aBrd,
                                                PCB_LAYER_ID aDefaultLayer,
                                                LSET aNotAllowedLayersMask,
                                                bool aHideCheckBoxes ) :
        PCB_LAYER_SELECTOR( aParent ),
        DIALOG_LAYER_SELECTION_BASE( aParent )
{
    m_useCalculatedSize = true;

    m_layerSelected = aDefaultLayer;
    m_notAllowedLayersMask = aNotAllowedLayersMask;
    m_brd = aBrd;

    m_leftGridLayers->SetCellHighlightPenWidth( 0 );
    m_rightGridLayers->SetCellHighlightPenWidth( 0 );
    m_leftGridLayers->SetColFormatBool( SELECT_COLNUM );
    m_rightGridLayers->SetColFormatBool( SELECT_COLNUM );
    buildList();

    if( aHideCheckBoxes )
    {
        m_leftGridLayers->HideCol( SELECT_COLNUM );
        m_rightGridLayers->HideCol( SELECT_COLNUM );
    }

    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( PCB_ONE_LAYER_SELECTOR::onCharHook ) );

    Layout();
    GetSizer()->SetSizeHints( this );
    SetFocus();
}


PCB_ONE_LAYER_SELECTOR::~PCB_ONE_LAYER_SELECTOR()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( PCB_ONE_LAYER_SELECTOR::onCharHook ) );
}


void PCB_ONE_LAYER_SELECTOR::OnMouseMove( wxUpdateUIEvent& aEvent )
{
    /// We have to assign this in UpdateUI events because the wxGrid is not properly receiving
    /// MouseMove events.  It seems to only get them on the edges.  So, for now we use this
    /// workaround

    wxPoint mouse_pos = wxGetMousePosition();
    wxPoint left_pos = m_leftGridLayers->ScreenToClient( mouse_pos );
    wxPoint right_pos = m_rightGridLayers->ScreenToClient( mouse_pos );

    if( m_leftGridLayers->HitTest( left_pos ) == wxHT_WINDOW_INSIDE )
    {
        int row = m_leftGridLayers->YToRow( left_pos.y );

        if( row != wxNOT_FOUND && row < static_cast<int>( m_layersIdLeftColumn.size() ) )
        {
            m_layerSelected = m_layersIdLeftColumn[ row ];
            m_leftGridLayers->SelectBlock( row, LAYERNAME_COLNUM, row, LAYERNAME_COLNUM);
            return;
        }
    }

    if( m_rightGridLayers->HitTest( right_pos ) == wxHT_WINDOW_INSIDE )
    {
        int row = m_rightGridLayers->YToRow( right_pos.y );

        if( row == wxNOT_FOUND || row >= static_cast<int>( m_layersIdRightColumn.size() ) )
            return;

        m_layerSelected = m_layersIdRightColumn[ row ];
        m_rightGridLayers->SelectBlock( row, LAYERNAME_COLNUM, row, LAYERNAME_COLNUM);
    }
}


void PCB_ONE_LAYER_SELECTOR::onCharHook( wxKeyEvent& event )
{
    if( event.GetKeyCode() == WXK_ESCAPE )
        Close();
}


void PCB_ONE_LAYER_SELECTOR::buildList()
{
    wxColour bg = getLayerColor( LAYER_PCB_BACKGROUND ).ToColour();
    int      left_row = 0;
    int      right_row = 0;
    wxString layername;

    for( LSEQ ui_seq = m_brd->GetEnabledLayers().UIOrder(); ui_seq; ++ui_seq )
    {
        PCB_LAYER_ID  layerid = *ui_seq;

        if( m_notAllowedLayersMask[layerid] )
            continue;

        wxColour fg = getLayerColor( layerid ).ToColour();
        wxColour color( wxColour::AlphaBlend( fg.Red(), bg.Red(), fg.Alpha() / 255.0 ),
                        wxColour::AlphaBlend( fg.Green(), bg.Green(), fg.Alpha() / 255.0 ),
                        wxColour::AlphaBlend( fg.Blue(), bg.Blue(), fg.Alpha() / 255.0 ) );

        layername = wxT( " " ) + getLayerName( layerid );

        if( IsCopperLayer( layerid ) )
        {
            if( left_row )
                m_leftGridLayers->AppendRows( 1 );

            m_leftGridLayers->SetCellBackgroundColour ( left_row, COLOR_COLNUM, color );
            m_leftGridLayers->SetCellValue( left_row, LAYERNAME_COLNUM, layername );

            if( m_layerSelected == layerid )
                m_leftGridLayers->SetCellValue( left_row, SELECT_COLNUM, "1" );

            m_layersIdLeftColumn.push_back( layerid );
            left_row++;
        }
        else
        {
            if( right_row )
                m_rightGridLayers->AppendRows( 1 );

            m_rightGridLayers->SetCellBackgroundColour( right_row, COLOR_COLNUM, color );
            m_rightGridLayers->SetCellValue( right_row, LAYERNAME_COLNUM, layername );

            if( m_layerSelected == layerid )
                m_rightGridLayers->SetCellValue( right_row, SELECT_COLNUM, "1" );

            m_layersIdRightColumn.push_back( layerid );
            right_row++;
        }
    }

    // Show only populated lists:
    if( left_row <= 0 )
        m_leftGridLayers->Show( false );

    if( right_row <= 0 )
        m_rightGridLayers->Show( false );

    // Now fix min grid column size (it also sets a minimal size)
    m_leftGridLayers->AutoSizeColumns();
    m_rightGridLayers->AutoSizeColumns();
}


void PCB_ONE_LAYER_SELECTOR::OnLeftGridCellClick( wxGridEvent& event )
{
    m_layerSelected = m_layersIdLeftColumn[ event.GetRow() ];

    if( IsQuasiModal() )
        EndQuasiModal( 1 );
    else
        EndDialog( 1 );
}


void PCB_ONE_LAYER_SELECTOR::OnRightGridCellClick( wxGridEvent& event )
{
    m_layerSelected = m_layersIdRightColumn[ event.GetRow() ];

    if( IsQuasiModal() )
        EndQuasiModal( 2 );
    else
        EndDialog( 2 );
}


PCB_LAYER_ID PCB_BASE_FRAME::SelectOneLayer( PCB_LAYER_ID aDefaultLayer, LSET aNotAllowedLayersMask,
                                             wxPoint aDlgPosition )
{
    PCB_ONE_LAYER_SELECTOR dlg( this, GetBoard(), aDefaultLayer, aNotAllowedLayersMask, true );

    if( aDlgPosition != wxDefaultPosition )
    {
        wxSize dlgSize = dlg.GetSize();
        aDlgPosition.x -= dlgSize.x/2;
        aDlgPosition.y -= dlgSize.y/2;
        dlg.SetPosition( aDlgPosition );
    }

    if( dlg.ShowModal() != wxID_CANCEL )
        return ToLAYER_ID( dlg.GetLayerSelection() );
    else
        return UNDEFINED_LAYER;
}


/**
 * Display a pair PCB copper layers list in a dialog to select a layer pair from these lists.
 */
class SELECT_COPPER_LAYERS_PAIR_DIALOG: public PCB_LAYER_SELECTOR,
                                        public DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE
{
public:
    SELECT_COPPER_LAYERS_PAIR_DIALOG( PCB_BASE_FRAME* aParent, BOARD* aPcb,
                                      PCB_LAYER_ID aFrontLayer, PCB_LAYER_ID aBackLayer );

    void GetLayerPair( PCB_LAYER_ID& aFrontLayer, PCB_LAYER_ID& aBackLayer )
    {
        aFrontLayer = m_frontLayer;
        aBackLayer = m_backLayer;
    }

private:
    void OnLeftGridCellClick( wxGridEvent& event ) override;
    void OnRightGridCellClick( wxGridEvent& event ) override;

    void buildList();

    BOARD*       m_brd;
    PCB_LAYER_ID m_frontLayer;
    PCB_LAYER_ID m_backLayer;
    int          m_leftRowSelected;
    int          m_rightRowSelected;

    std::vector<PCB_LAYER_ID> m_layersId;
};


int ROUTER_TOOL::SelectCopperLayerPair( const TOOL_EVENT& aEvent )
{
    PCB_SCREEN* screen = frame()->GetScreen();

    SELECT_COPPER_LAYERS_PAIR_DIALOG dlg( frame(), frame()->GetBoard(), screen->m_Route_Layer_TOP,
                                          screen->m_Route_Layer_BOTTOM );

    if( dlg.ShowModal() == wxID_OK )
    {
        dlg.GetLayerPair( screen->m_Route_Layer_TOP, screen->m_Route_Layer_BOTTOM );

        // select the same layer for both layers is allowed (normal in some boards)
        // but could be a mistake. So display an info message
        if( screen->m_Route_Layer_TOP == screen->m_Route_Layer_BOTTOM )
            DisplayInfoMessage( frame(), _( "Warning: top and bottom layers are same." ) );
    }

    return 0;
}


SELECT_COPPER_LAYERS_PAIR_DIALOG::SELECT_COPPER_LAYERS_PAIR_DIALOG(
        PCB_BASE_FRAME* aParent, BOARD * aPcb, PCB_LAYER_ID aFrontLayer, PCB_LAYER_ID aBackLayer) :
    PCB_LAYER_SELECTOR( aParent ),
    DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( aParent )
{
    m_frontLayer = aFrontLayer;
    m_backLayer = aBackLayer;
    m_leftRowSelected = 0;
    m_rightRowSelected = 0;
    m_brd = aPcb;

    m_leftGridLayers->SetCellHighlightPenWidth( 0 );
    m_rightGridLayers->SetCellHighlightPenWidth( 0 );
    m_leftGridLayers->SetColFormatBool( SELECT_COLNUM );
    m_rightGridLayers->SetColFormatBool( SELECT_COLNUM );
    buildList();

    SetFocus();

    GetSizer()->SetSizeHints( this );
    Center();
}


void SELECT_COPPER_LAYERS_PAIR_DIALOG::buildList()
{
    wxColour bg = getLayerColor( LAYER_PCB_BACKGROUND ).ToColour();
    int      row = 0;
    wxString layername;

    for( LSEQ ui_seq = m_brd->GetEnabledLayers().UIOrder(); ui_seq; ++ui_seq )
    {
        PCB_LAYER_ID layerid = *ui_seq;

        if( !IsCopperLayer( layerid ) )
            continue;

        wxColour fg = getLayerColor( layerid ).ToColour();
        wxColour color( wxColour::AlphaBlend( fg.Red(), bg.Red(), fg.Alpha() / 255.0 ),
                        wxColour::AlphaBlend( fg.Green(), bg.Green(), fg.Alpha() / 255.0 ),
                        wxColour::AlphaBlend( fg.Blue(), bg.Blue(), fg.Alpha() / 255.0 ) );

        layername = wxT( " " ) + getLayerName( layerid );

        if( row )
            m_leftGridLayers->AppendRows( 1 );

        m_leftGridLayers->SetCellBackgroundColour( row, COLOR_COLNUM, color );
        m_leftGridLayers->SetCellValue( row, LAYERNAME_COLNUM, layername );
        m_layersId.push_back( layerid );

        if( m_frontLayer == layerid )
        {
            m_leftGridLayers->SetCellValue( row, SELECT_COLNUM, "1" );
            m_leftGridLayers->SetGridCursor( row, COLOR_COLNUM );
            m_leftRowSelected = row;
        }

        if( row )
            m_rightGridLayers->AppendRows( 1 );

        m_rightGridLayers->SetCellBackgroundColour( row, COLOR_COLNUM, color );
        m_rightGridLayers->SetCellValue( row, LAYERNAME_COLNUM, layername );

        if( m_backLayer == layerid )
        {
            m_rightGridLayers->SetCellValue( row, SELECT_COLNUM, "1" );
            m_rightRowSelected = row;
        }

        row++;
    }

    // Now fix min grid layer name column size (it also sets a minimal size)
    m_leftGridLayers->AutoSizeColumn( LAYERNAME_COLNUM );
    m_rightGridLayers->AutoSizeColumn( LAYERNAME_COLNUM );
}


void SELECT_COPPER_LAYERS_PAIR_DIALOG::OnLeftGridCellClick( wxGridEvent& event )
{
    int          row = event.GetRow();
    PCB_LAYER_ID layer = m_layersId[row];

    if( m_frontLayer == layer )
        return;

    m_leftGridLayers->SetCellValue( m_leftRowSelected, SELECT_COLNUM, wxEmptyString );
    m_frontLayer = layer;
    m_leftRowSelected = row;
    m_leftGridLayers->SetCellValue( m_leftRowSelected, SELECT_COLNUM, "1" );
}


void SELECT_COPPER_LAYERS_PAIR_DIALOG::OnRightGridCellClick( wxGridEvent& event )
{
    int          row = event.GetRow();
    PCB_LAYER_ID layer = m_layersId[row];

    if( m_backLayer == layer )
        return;

    m_rightGridLayers->SetCellValue( m_rightRowSelected, SELECT_COLNUM, wxEmptyString );
    m_backLayer = layer;
    m_rightRowSelected = row;
    m_rightGridLayers->SetCellValue( m_rightRowSelected, SELECT_COLNUM, "1" );
}
