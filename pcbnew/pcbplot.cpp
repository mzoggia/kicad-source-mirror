/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcbnew/pcbplot.cpp
 */

#include <plotters/plotter.h>
#include <pcbplot.h>
#include <base_units.h>
#include <locale_io.h>
#include <reporter.h>
#include <board.h>
#include <board_design_settings.h>
#include <plotcontroller.h>
#include <pcb_plot_params.h>
#include <wx/ffile.h>
#include <dialog_plot.h>
#include <build_version.h>
#include <gbr_metadata.h>
#include <render_settings.h>


const wxString GetGerberProtelExtension( int aLayer )
{
    if( IsCopperLayer( aLayer ) )
    {
        if( aLayer == F_Cu )
            return wxT( "gtl" );
        else if( aLayer == B_Cu )
            return wxT( "gbl" );
        else
        {
            return wxString::Format( wxT( "g%d" ), aLayer+1 );
        }
    }
    else
    {
        switch( aLayer )
        {
        case B_Adhes:       return wxT( "gba" );
        case F_Adhes:       return wxT( "gta" );

        case B_Paste:       return wxT( "gbp" );
        case F_Paste:       return wxT( "gtp" );

        case B_SilkS:       return wxT( "gbo" );
        case F_SilkS:       return wxT( "gto" );

        case B_Mask:        return wxT( "gbs" );
        case F_Mask:        return wxT( "gts" );

        case Edge_Cuts:     return wxT( "gm1" );

        case Dwgs_User:
        case Cmts_User:
        case Eco1_User:
        case Eco2_User:
        default:            return wxT( "gbr" );
        }
    }
}


const wxString GetGerberFileFunctionAttribute( const BOARD* aBoard, int aLayer )
{
    wxString attrib;

    switch( aLayer )
    {
    case F_Adhes:
        attrib = "Glue,Top";
        break;

    case B_Adhes:
        attrib = "Glue,Bot";
        break;

    case F_SilkS:
        attrib = "Legend,Top";
        break;

    case B_SilkS:
        attrib = "Legend,Bot";
        break;

    case F_Mask:
        attrib = "Soldermask,Top";
        break;

    case B_Mask:
        attrib = "Soldermask,Bot";
        break;

    case F_Paste:
        attrib = "Paste,Top";
        break;

    case B_Paste:
        attrib = "Paste,Bot";
        break;

    case Edge_Cuts:
        // Board outline.
        // Can be "Profile,NP" (Not Plated: usual) or "Profile,P"
        // This last is the exception (Plated)
        attrib = "Profile,NP";
        break;

    case Dwgs_User:
        attrib = "OtherDrawing,Comment";
        break;

    case Cmts_User:
        attrib = "Other,Comment";
        break;

    case Eco1_User:
        attrib = "Other,ECO1";
        break;

    case Eco2_User:
        attrib = "Other,ECO2";
        break;

    case B_Fab:
        // This is actually a assembly layer
        attrib = "AssemblyDrawing,Bot";
        break;

    case F_Fab:
        // This is actually a assembly layer
        attrib = "AssemblyDrawing,Top";
        break;

    case B_Cu:
        attrib.Printf( wxT( "Copper,L%d,Bot" ), aBoard->GetCopperLayerCount() );
        break;

    case F_Cu:
        attrib = "Copper,L1,Top";
        break;

    default:
        if( IsCopperLayer( aLayer ) )
            attrib.Printf( wxT( "Copper,L%d,Inr" ), aLayer+1 );
        else
            attrib.Printf( wxT( "Other,User" ), aLayer+1 );
        break;
    }

    // This code adds a optional parameter: the type of copper layers.
    // Because it is not used by Pcbnew (it can be used only by external autorouters)
    // user do not really set this parameter.
    // Therefore do not add it.
    // However, this code is left here, for perhaps a future usage.
#if 0
    // Add the signal type of the layer, if relevant
    if( IsCopperLayer( aLayer ) )
    {
        LAYER_T type = aBoard->GetLayerType( ToLAYER_ID( aLayer ) );

        switch( type )
        {
        case LT_SIGNAL:
            attrib += ",Signal";
            break;
        case LT_POWER:
            attrib += ",Plane";
            break;
        case LT_MIXED:
            attrib += ",Mixed";
            break;
        default:
            break;   // do nothing (but avoid a warning for unhandled LAYER_T values from GCC)
        }
    }
#endif

    wxString fileFct;
    fileFct.Printf( "%%TF.FileFunction,%s*%%", attrib );

    return fileFct;
}


static const wxString GetGerberFilePolarityAttribute( int aLayer )
{
    /* build the string %TF.FilePolarity,Positive*%
     * or  %TF.FilePolarity,Negative*%
     * an emply string for layers which do not use a polarity
     *
     * The value of the .FilePolarity specifies whether the image represents the
     * presence or absence of material.
     * This attribute can only be used when the file represents a pattern in a material layer,
     * e.g. copper, solder mask, legend.
     * Together with.FileFunction it defines the role of that image in
     * the layer structure of the PCB.
     * Note that the .FilePolarity attribute does not change the image -
     * no attribute does.
     * It changes the interpretation of the image.
     * For example, in a copper layer in positive polarity a round flash generates a copper pad.
     * In a copper layer in negative polarity it generates a clearance.
     * Solder mask images usually represent solder mask openings and are then negative.
     * This may be counter-intuitive.
     */
    int polarity = 0;

    switch( aLayer )
    {
    case F_Adhes:
    case B_Adhes:
    case F_SilkS:
    case B_SilkS:
    case F_Paste:
    case B_Paste:
        polarity = 1;
        break;

    case F_Mask:
    case B_Mask:
        polarity = -1;
        break;

    default:
        if( IsCopperLayer( aLayer ) )
            polarity = 1;
        break;
    }

    wxString filePolarity;

    if( polarity == 1 )
        filePolarity = "%TF.FilePolarity,Positive*%";
    if( polarity == -1 )
        filePolarity = "%TF.FilePolarity,Negative*%";

    return filePolarity;
}


/* Add some X2 attributes to the file header, as defined in the
 * Gerber file format specification J4 and "Revision 2015.06"
 */


// A helper function to convert a X2 attribute string to a X1 structured comment:
static wxString& makeStringCompatX1( wxString& aText, bool aUseX1CompatibilityMode )
{
    if( aUseX1CompatibilityMode )
    {
        aText.Replace( "%", "" );
        aText.Prepend( "G04 #@! " );
    }

    return aText;
}


void AddGerberX2Header( PLOTTER* aPlotter, const BOARD* aBoard, bool aUseX1CompatibilityMode )
{
    wxString text;

    // Creates the TF,.GenerationSoftware. Format is:
    // %TF,.GenerationSoftware,<vendor>,<application name>[,<application version>]*%
    text.Printf( wxT( "%%TF.GenerationSoftware,KiCad,Pcbnew,%s*%%" ), GetBuildVersion() );
    aPlotter->AddLineToHeader( makeStringCompatX1( text, aUseX1CompatibilityMode ) );

    // creates the TF.CreationDate attribute:
    text = GbrMakeCreationDateAttributeString( aUseX1CompatibilityMode ?
                                                    GBR_NC_STRING_FORMAT_X1 :
                                                    GBR_NC_STRING_FORMAT_X2 );
    aPlotter->AddLineToHeader( text );

    // Creates the TF,.ProjectId. Format is (from Gerber file format doc):
    // %TF.ProjectId,<project id>,<project GUID>,<revision id>*%
    // <project id> is the name of the project, restricted to basic ASCII symbols only,
    // Rem: <project id> accepts only ASCII 7 code (only basic ASCII codes are allowed in
    // gerber files) and comma not accepted.
    // All illegal chars will be replaced by underscore.
    //
    // <project GUID> is a string which is an unique id of a project.
    // However Kicad does not handle such a project GUID, so it is built from the board name
    wxFileName fn = aBoard->GetFileName();
    wxString msg = fn.GetFullName();

    // Build a <project GUID>, from the board name
    wxString guid = GbrMakeProjectGUIDfromString( msg );

    // build the <project id> string: this is the board short filename (without ext)
    // and all non ASCII chars and comma are replaced by '_'
    msg = fn.GetName();
    msg.Replace( wxT( "," ), wxT( "_" ) );

    // build the <revision id> string. All non ASCII chars and comma are replaced by '_'
    wxString rev = ExpandTextVars( aBoard->GetTitleBlock().GetRevision(), aBoard->GetProject() );
    rev.Replace( wxT( "," ), wxT( "_" ) );

    if( rev.IsEmpty() )
        rev = wxT( "rev?" );

    text.Printf( wxT( "%%TF.ProjectId,%s,%s,%s*%%" ), msg.ToAscii(), guid, rev.ToAscii() );
    aPlotter->AddLineToHeader( makeStringCompatX1( text, aUseX1CompatibilityMode ) );

    // Add the TF.SameCoordinates, that specify all gerber files uses the same
    // origin and orientation, and the registration between files is OK.
    // The parameter of TF.SameCoordinates is a string that is common
    // to all files using the same registration and has no special meaning:
    // this is just a key
    // Because there is no mirroring/rotation in Kicad, only the plot offset origin
    // can create incorrect registration.
    // So we create a key from plot offset options.
    // and therefore for a given board, all Gerber files having the same key have the same
    // plot origin and use the same registration
    //
    // Currently the key is "Original" when using absolute Pcbnew coordinates,
    // and the PY and PY position of auxiliary axis, when using it.
    // Please, if absolute Pcbnew coordinates, one day, are set by user, change the way
    // the key is built to ensure file only using the *same* axis have the same key.
    wxString registration_id = "Original";
    wxPoint auxOrigin = aBoard->GetDesignSettings().GetAuxOrigin();

    if( aBoard->GetPlotOptions().GetUseAuxOrigin() && auxOrigin.x && auxOrigin.y )
        registration_id.Printf( "PX%xPY%x", auxOrigin.x, auxOrigin.y );

    text.Printf( "%%TF.SameCoordinates,%s*%%", registration_id.GetData() );
    aPlotter->AddLineToHeader( makeStringCompatX1( text, aUseX1CompatibilityMode ) );
}


void AddGerberX2Attribute( PLOTTER* aPlotter, const BOARD* aBoard, int aLayer,
                           bool aUseX1CompatibilityMode )
{
    AddGerberX2Header( aPlotter, aBoard, aUseX1CompatibilityMode );

    wxString text;

    // Add the TF.FileFunction
    text = GetGerberFileFunctionAttribute( aBoard, aLayer );
    aPlotter->AddLineToHeader( makeStringCompatX1( text, aUseX1CompatibilityMode ) );

    // Add the TF.FilePolarity (for layers which support that)
    text = GetGerberFilePolarityAttribute( aLayer );

    if( !text.IsEmpty() )
        aPlotter->AddLineToHeader( makeStringCompatX1( text, aUseX1CompatibilityMode ) );
}


void BuildPlotFileName( wxFileName* aFilename, const wxString& aOutputDir,
                        const wxString& aSuffix, const wxString& aExtension )
{
    // aFilename contains the base filename only (without path and extension)
    // when calling this function.
    // It is expected to be a valid filename (this is usually the board filename)
    aFilename->SetPath( aOutputDir );

    // Set the file extension
    aFilename->SetExt( aExtension );

    // remove leading and trailing spaces if any from the suffix, if
    // something survives add it to the name;
    // also the suffix can contain some not allowed chars in filename (/ \ . : and some others),
    // so change them to underscore
    // Remember it can be called from a python script, so the illegal chars
    // have to be filtered here.
    wxString suffix = aSuffix;
    suffix.Trim( true );
    suffix.Trim( false );

    wxString badchars = wxFileName::GetForbiddenChars(wxPATH_DOS);
    badchars.Append( "%." );

    for( unsigned ii = 0; ii < badchars.Len(); ii++ )
        suffix.Replace( badchars[ii], wxT("_") );

    if( !suffix.IsEmpty() )
        aFilename->SetName( aFilename->GetName() + wxT( "-" ) + suffix );
}


PLOT_CONTROLLER::PLOT_CONTROLLER( BOARD* aBoard )
{
    m_plotter = nullptr;
    m_board = aBoard;
    m_plotLayer = UNDEFINED_LAYER;
}


PLOT_CONTROLLER::~PLOT_CONTROLLER()
{
    ClosePlot();
}


/* IMPORTANT THING TO KNOW: the locale during plots *MUST* be kept as
 * C/POSIX using a LOCALE_IO object on the stack. This even when
 * opening/closing the plotfile, since some drivers do I/O even then */
void PLOT_CONTROLLER::ClosePlot()
{
    LOCALE_IO toggle;

    if( m_plotter )
    {
        m_plotter->EndPlot();

        delete m_plotter->RenderSettings();
        delete m_plotter;

        m_plotter = nullptr;
    }
}


bool PLOT_CONTROLLER::OpenPlotfile( const wxString& aSuffix, PLOT_FORMAT aFormat,
                                    const wxString& aSheetDesc )
{
    LOCALE_IO toggle;

    /* Save the current format: sadly some plot routines depends on this
       but the main reason is that the StartPlot method uses it to
       dispatch the plotter creation */
    GetPlotOptions().SetFormat( aFormat );

    // Ensure that the previous plot is closed
    ClosePlot();

    // Now compute the full filename for the output and start the plot
    // (after ensuring the output directory is OK)
    wxString outputDirName = GetPlotOptions().GetOutputDirectory() ;
    wxFileName outputDir = wxFileName::DirName( outputDirName );
    wxString boardFilename = m_board->GetFileName();

    if( EnsureFileDirectoryExists( &outputDir, boardFilename ) )
    {
        // outputDir contains now the full path of plot files
        m_plotFile = boardFilename;
        m_plotFile.SetPath( outputDir.GetPath() );
        wxString fileExt = GetDefaultPlotExtension( aFormat );

        // Gerber format can use specific file ext, depending on layers
        // (now not a good practice, because the official file ext is .gbr)
        if( GetPlotOptions().GetFormat() == PLOT_FORMAT::GERBER
                && GetPlotOptions().GetUseGerberProtelExtensions() )
            fileExt = GetGerberProtelExtension( GetLayer() );

        // Build plot filenames from the board name and layer names:
        BuildPlotFileName( &m_plotFile, outputDir.GetPath(), aSuffix, fileExt );

        m_plotter = StartPlotBoard( m_board, &GetPlotOptions(), ToLAYER_ID( GetLayer() ),
                                    m_plotFile.GetFullPath(), aSheetDesc );
    }

    return ( m_plotter != nullptr );
}


bool PLOT_CONTROLLER::PlotLayer()
{
    LOCALE_IO toggle;

    // No plot open, nothing to do...
    if( !m_plotter )
        return false;

    // Fully delegated to the parent
    PlotOneBoardLayer( m_board, m_plotter, ToLAYER_ID( GetLayer() ), GetPlotOptions() );

    return true;
}


void PLOT_CONTROLLER::SetColorMode( bool aColorMode )
{
    if( !m_plotter )
        return;

    m_plotter->SetColorMode( aColorMode );
}


bool PLOT_CONTROLLER::GetColorMode()
{
    if( !m_plotter )
        return false;

    return m_plotter->GetColorMode();
}
