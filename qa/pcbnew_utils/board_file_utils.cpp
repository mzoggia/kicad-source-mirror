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

#include <pcbnew_utils/board_file_utils.h>

// For PCB parsing
#include <plugins/kicad/pcb_plugin.h>
#include <plugins/kicad/pcb_parser.h>
#include <richio.h>

#include <board.h>

#include <qa_utils/stdstream_line_reader.h>

namespace KI_TEST
{

#ifndef QA_PCBNEW_DATA_LOCATION
    #define QA_PCBNEW_DATA_LOCATION "???"
#endif

std::string GetPcbnewTestDataDir()
{
    const char* env = std::getenv( "KICAD_TEST_PCBNEW_DATA_DIR" );
    std::string fn;

    if( !env )
    {
        // Use the compiled-in location of the data dir (i.e. where the files were at build time)
        fn = QA_PCBNEW_DATA_LOCATION;
    }
    else
    {
        // Use whatever was given in the env var
        fn = env;
    }

    // Ensure the string ends in / to force a directory interpretation
    fn += "/";

    return fn;
}


void DumpBoardToFile( BOARD& board, const std::string& aFilename )
{
    PCB_PLUGIN io;
    io.Save( aFilename, &board );
}


std::unique_ptr<BOARD_ITEM> ReadBoardItemFromStream( std::istream& aStream )
{
    // Take input from stdin
    STDISTREAM_LINE_READER reader;
    reader.SetStream( aStream );

    PCB_PARSER                  parser( &reader );
    std::unique_ptr<BOARD_ITEM> board;

    try
    {
        board.reset( parser.Parse() );
    }
    catch( const IO_ERROR& )
    {
    }

    return board;
}


std::unique_ptr<BOARD> ReadBoardFromFileOrStream( const std::string& aFilename,
                                                  std::istream& aFallback )
{
    std::istream* in_stream = nullptr;
    std::ifstream file_stream;

    if( aFilename.empty() )
    {
        // no file, read stdin
        in_stream = &aFallback;
    }
    else
    {
        file_stream.open( aFilename );
        in_stream = &file_stream;
    }

    return ReadItemFromStream<BOARD>( *in_stream );
}



} // namespace KI_TEST
