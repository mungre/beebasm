/*************************************************************************************************/
/**
	sourcecode.cpp

	Represents a piece of source code, whether from a file, or a macro definition.


	Copyright (C) Rich Talbot-Watkins 2007 - 2012

	This file is part of BeebAsm.

	BeebAsm is free software: you can redistribute it and/or modify it under the terms of the GNU
	General Public License as published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	BeebAsm is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
	even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with BeebAsm, as
	COPYING.txt.  If not, see <http://www.gnu.org/licenses/>.
*/
/*************************************************************************************************/

#include "sourcecode.h"
#include "asmexception.h"
#include "stringutils.h"
#include "globaldata.h"
#include "lineparser.h"
#include "symboltable.h"
#include "macro.h"
#include "controlflow.h"

using namespace std;



/*************************************************************************************************/
/**
	SourceCode::SourceCode()

	Constructor for SourceCode

	@param		filename		Filename of source file to open
	@param		lineNumber		Line number
	@param		parent  		Parent SourceCode object (or null)

	The supplied file will be opened.  If there is a problem, an AsmException will be thrown.
*/
/*************************************************************************************************/
SourceCode::SourceCode( const string& filename, int lineNumber, const std::string& text, const SourceCode* parent )
	:	m_initialForStackPtr( 0 ),
		m_initialIfStackPtr( 0 ),
		m_filename( filename ),
		m_lineNumber( lineNumber ),
		m_parent( parent ),
		m_lineStartPointer( 0 ),
		m_text( text ),
		m_textPointer( 0 )
{
	// Double-check the supplied text came with a '\n' sentinel
	if (m_text.empty() || m_text.back() != '\n')
	{
		assert(false);
		m_text.push_back('\n');
	}
}



/*************************************************************************************************/
/**
	SourceCode::~SourceCode()

	Destructor for SourceCode

	The associated source file will be closed.
*/
/*************************************************************************************************/
SourceCode::~SourceCode()
{
}



/*************************************************************************************************/
/**
	SourceCode::Process()

	Process the associated source file
*/
/*************************************************************************************************/
void SourceCode::Process( ControlFlow* controlFlow )
{
	// Remember the FOR and IF stack initial pointer values

	m_initialForStackPtr = controlFlow->GetForStackPtr();
	m_initialIfStackPtr = controlFlow->GetIfStackPtr();

	// Reuse the parser because it's a big object and expensive to construct/destruct
	LineParser parser( controlFlow );

	// Iterate through the file line-by-line

	string lineFromFile;

	while ( GetLine( lineFromFile ) )
	{
//		// Display and process
//
//		if ( GlobalData::Instance().IsFirstPass() )
//		{
//			cout << setw( 5 ) << m_lineNumber << ": " << lineFromFile << endl;
//		}

		try
		{
			parser.Process( lineFromFile );
		}
		catch ( AsmException_SyntaxError& e )
		{
			// Augment exception with more details
			e.SetFilename( m_filename );
			e.SetLineNumber( m_lineNumber );
			throw;
		}

		m_lineNumber++;
		m_lineStartPointer = GetFilePointer();
	}

	// Check whether we aborted prematurely

	if ( !IsAtEnd() )
	{
		throw AsmException_FileError_ReadSourceFile( m_filename );
	}

	controlFlow->CheckMismatches( m_filename, m_initialForStackPtr, m_initialIfStackPtr );
}



/*************************************************************************************************/
/**
	SourceCode::GetLine()

	Reads a line from the source code and returns it into lineFromFile
*/
/*************************************************************************************************/
bool SourceCode::GetLine( string& lineFromFile )
{
	if (IsAtEnd())
	{
		return false;
	}
	// Check there is always a trailing '\n' (the constructor should ensure this)
	assert(m_text.back() == '\n');
	int begin = m_textPointer;
	while (m_text[m_textPointer++] != '\n')
	{
	}
	// Adding the line in one go rather than character by character is very much faster
	lineFromFile.assign(m_text.cbegin() + begin, m_text.cbegin() + m_textPointer - 1);
	return true;
}



/*************************************************************************************************/
/**
	SourceCode::SetPosition()

	Sets the current file pointer and line number
*/
/*************************************************************************************************/
void SourceCode::SetPosition( int filePointer, int lineNumber )
{
	if (filePointer > static_cast<int>(m_text.length()))
	{
		assert(false);
		filePointer = m_text.length();
	}
	m_lineStartPointer = filePointer;
	m_textPointer = filePointer;
	m_lineNumber = lineNumber;
}
