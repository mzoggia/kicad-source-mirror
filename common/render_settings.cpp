/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <render_settings.h>

using namespace KIGFX;


RENDER_SETTINGS::RENDER_SETTINGS() :
        m_highlightNetcodes(),
        m_printDC( nullptr )
{
    // Set the default initial values
    m_activeLayer           = F_Cu;
    m_highlightFactor       = 0.5f;
    m_selectFactor          = 0.5f;
    m_highlightEnabled      = false;
    m_hiContrastEnabled     = false;
    m_hiContrastFactor      = 0.2f;
    m_outlineWidth          = 1;
    m_drawingSheetLineWidth = 100000;
    m_defaultPenWidth       = 0;
    m_minPenWidth           = 0;
    m_isPrinting            = false;
}


RENDER_SETTINGS::~RENDER_SETTINGS()
{
}


constexpr double visualCorrection = 0.8;


double RENDER_SETTINGS::GetDashLength( int aLineWidth ) const
{
    return std::max( m_dashLengthRatio - visualCorrection, 1.0 ) * aLineWidth;
}


double RENDER_SETTINGS::GetDotLength( int aLineWidth ) const
{
    return ( 1.0 - visualCorrection ) * aLineWidth;
}


double RENDER_SETTINGS::GetGapLength( int aLineWidth ) const
{
    return std::max( m_gapLengthRatio + visualCorrection, 1.0 ) * aLineWidth;
}


void RENDER_SETTINGS::update()
{
    // Calculate darkened/highlighted variants of layer colors
    for( int i = 0; i < LAYER_ID_COUNT; i++ )
    {
        m_hiContrastColor[i] = m_layerColors[i].Mix( m_layerColors[LAYER_PCB_BACKGROUND],
                                                     m_hiContrastFactor );

        m_layerColorsHi[i]   = m_layerColors[i].Brightened( m_highlightFactor );
        m_layerColorsDark[i] = m_layerColors[i].Darkened( 1.0 - m_highlightFactor );

        if( IsNetnameLayer( i ) || IsHoleLayer( i ) )
        {
            m_layerColorsSel[i] = m_layerColors[i];
            continue;
        }

        // Linear brightening doesn't work well for colors near white
        double factor = ( m_selectFactor * 0.5 ) + pow( m_layerColors[i].GetBrightness(), 3 );
        factor = std::min( 1.0, factor );

        m_layerColorsSel[i] = m_layerColors[i].Brightened( factor );

        // If we are maxed out on brightening as a highlight, fallback to darkening but keep
        // the blue that acts as a "glowing" color
        if( std::fabs( m_layerColorsSel[i].GetBrightness() - m_layerColors[i].GetBrightness() )
                < 0.05 )
        {
            m_layerColorsSel[i] = m_layerColors[i].Darkened( m_selectFactor * 0.4 );
            m_layerColorsSel[i].b = m_layerColors[i].b * ( 1.0 - factor ) + factor;
        }

    }
}

