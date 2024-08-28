/*************************************************************************************************/
/**
	controlflow.cpp


	Copyright (C) Rich Talbot-Watkins, Charles Reilly 2007 - 2024

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

#include <string>
#include <iostream>

#include "controlflow.h"
#include "asmexception.h"
#include "symboltable.h"
#include "sourcefile.h"
#include "globaldata.h"

using namespace std;


ControlFlow::ControlFlow()
	:	m_forStackPtr( 0 ),
		m_ifStackPtr( 0 ),
		m_currentMacro( NULL ),
		m_sourceCode( NULL ),
		m_nextForId(0)
{
}


void ControlFlow::IncludeFile( std::string filename )
{
	if ( ShouldOutputAsm() )
	{
		cerr << "Including file " << filename << endl;
	}

	SourceFile input( filename.c_str(), m_sourceCode );
	Process( &input );
}


void ControlFlow::Process( SourceCode* sourceCode )
{
	SourceCode* previousSource = m_sourceCode;
	m_sourceCode = sourceCode;
	sourceCode->Process(this);
	m_sourceCode = previousSource;
}


void ControlFlow::CheckMismatches( const std::string& filename, int m_initialForStackPtr, int m_initialIfStackPtr )
{
	// Check that we have no FOR / braces mismatch

	if ( m_forStackPtr != m_initialForStackPtr )
	{
		For& mismatchedFor = m_forStack[ m_forStackPtr - 1 ];

		if ( mismatchedFor.m_step == 0.0 )
		{
			AsmException_SyntaxError_MismatchedBraces e( mismatchedFor.m_line, mismatchedFor.m_column );
			e.SetFilename( filename );
			e.SetLineNumber( mismatchedFor.m_lineNumber );
			throw e;
		}
		else
		{
			AsmException_SyntaxError_ForWithoutNext e( mismatchedFor.m_line, mismatchedFor.m_column );
			e.SetFilename( filename );
			e.SetLineNumber( mismatchedFor.m_lineNumber );
			throw e;
		}
	}

	// Check that we have no IF / MACRO mismatch

	if ( m_ifStackPtr != m_initialIfStackPtr )
	{
		If& mismatchedIf = m_ifStack[ m_ifStackPtr - 1 ];

		if ( mismatchedIf.m_isMacroDefinition )
		{
			AsmException_SyntaxError_NoEndMacro e( mismatchedIf.m_line, mismatchedIf.m_column );
			e.SetFilename( filename );
			e.SetLineNumber( mismatchedIf.m_lineNumber );
			throw e;
		}
		else
		{
			AsmException_SyntaxError_IfWithoutEndif e( mismatchedIf.m_line, mismatchedIf.m_column );
			e.SetFilename( filename );
			e.SetLineNumber( mismatchedIf.m_lineNumber );
			throw e;
		}
	}
}


/*************************************************************************************************/
/**
	ControlFlow::AddFor()
*/
/*************************************************************************************************/
void ControlFlow::AddFor( const ScopedSymbolName& varName,
						 double start,
						 double end,
						 double step,
						 const string& line,
						 int variableColumn,
						 int bodyColumn )
{
	if ( m_forStackPtr == MAX_FOR_LEVELS )
	{
		throw AsmException_SyntaxError_TooManyFORs( line, variableColumn );
	}

	// Add symbol to table

	SymbolTable::Instance().AddSymbol( varName, start );

	// Fill in FOR block

	m_forStack[ m_forStackPtr ].m_varName		= varName;
	m_forStack[ m_forStackPtr ].m_current		= start;
	m_forStack[ m_forStackPtr ].m_end			= end;
	m_forStack[ m_forStackPtr ].m_step			= step;
	m_forStack[ m_forStackPtr ].m_filePtr		= m_sourceCode->GetLineStartPointer() + bodyColumn;
	m_forStack[ m_forStackPtr ].m_id			= m_nextForId++;
	m_forStack[ m_forStackPtr ].m_count			= 0;
	m_forStack[ m_forStackPtr ].m_line			= line;
	m_forStack[ m_forStackPtr ].m_column		= variableColumn;
	m_forStack[ m_forStackPtr ].m_lineNumber	= m_sourceCode->GetLineNumber();

	SymbolTable::Instance().PushFor(m_forStack[ m_forStackPtr ].m_varName, m_forStack[ m_forStackPtr ].m_current);
	m_forStackPtr++;
}

/*************************************************************************************************/
/**
	ControlFlow::OpenBrace()

	Braces for scoping variables are just FORs in disguise...
*/
/*************************************************************************************************/
void ControlFlow::OpenBrace( const string& line, int column )
{
	if ( m_forStackPtr == MAX_FOR_LEVELS )
	{
		throw AsmException_SyntaxError_TooManyFORs( line, column );
	}

	// Fill in FOR block

	m_forStack[ m_forStackPtr ].m_varName		= ScopedSymbolName();
	m_forStack[ m_forStackPtr ].m_current		= 1.0;
	m_forStack[ m_forStackPtr ].m_end			= 0.0;
	m_forStack[ m_forStackPtr ].m_step			= 0.0;
	m_forStack[ m_forStackPtr ].m_filePtr		= 0;
	m_forStack[ m_forStackPtr ].m_id			= m_nextForId++;
	m_forStack[ m_forStackPtr ].m_count			= 0;
	m_forStack[ m_forStackPtr ].m_line			= line;
	m_forStack[ m_forStackPtr ].m_column		= column;
	m_forStack[ m_forStackPtr ].m_lineNumber	= m_sourceCode->GetLineNumber();

	SymbolTable::Instance().PushBrace();
	m_forStackPtr++;
}



/*************************************************************************************************/
/**
	ControlFlow::UpdateFor()
*/
/*************************************************************************************************/
void ControlFlow::UpdateFor( const string& line, int column )
{
	if ( m_forStackPtr == 0 )
	{
		throw AsmException_SyntaxError_NextWithoutFor( line, column );
	}

	For& thisFor = m_forStack[ m_forStackPtr - 1 ];

	// step of 0.0 here means that the 'for' is in fact an open brace, so throw an error

	if ( thisFor.m_step == 0.0 )
	{
		throw AsmException_SyntaxError_NextWithoutFor( line, column );
	}

	thisFor.m_current += thisFor.m_step;

	if ( ( thisFor.m_step > 0.0 && thisFor.m_current > thisFor.m_end ) ||
		 ( thisFor.m_step < 0.0 && thisFor.m_current < thisFor.m_end ) )
	{
		// we have reached the end of the FOR
		SymbolTable::Instance().RemoveSymbol( thisFor.m_varName );
		SymbolTable::Instance().PopScope();
		m_forStackPtr--;
	}
	else
	{
		// reloop
		SymbolTable::Instance().ChangeSymbol( thisFor.m_varName, thisFor.m_current );
		SymbolTable::Instance().PopScope();
		SymbolTable::Instance().PushFor(thisFor.m_varName, thisFor.m_current);
		thisFor.m_count++;
		m_sourceCode->SetPosition( thisFor.m_filePtr, thisFor.m_lineNumber - 1);
	}
}



/*************************************************************************************************/
/**
	ControlFlow::CloseBrace()

	Braces for scoping variables are just FORs in disguise...
*/
/*************************************************************************************************/
void ControlFlow::CloseBrace( const string& line, int column )
{
	// Instead of comparing against 0, I compare with the initial value of the stack ptr when
	// SourceCode::Process() was called.
	// This is because macros start wih a copy of the parent FOR stack frame, with an extra set of
	// braces pushed so they are in their own scope.  Without this amendment, it'd be possible to
	// close the 'hidden' braces started by the macro instantiation - with hilarious* consequences!
	//
	// * for unfunny values of hilarious

	if ( m_forStackPtr == m_sourceCode->GetInitialForStackPtr() )
	{
		throw AsmException_SyntaxError_MismatchedBraces( line, column );
	}

	For& thisFor = m_forStack[ m_forStackPtr - 1 ];

	// step of non-0.0 here means that this a real 'for', so throw an error

	if ( thisFor.m_step != 0.0 )
	{
		throw AsmException_SyntaxError_MismatchedBraces( line, column );
	}

	SymbolTable::Instance().PopScope();
	m_forStackPtr--;
}


/*************************************************************************************************/
/**
	ControlFlow::GetScopedSymbolName()
*/
/*************************************************************************************************/
ScopedSymbolName ControlFlow::GetScopedSymbolName( const string& symbolName, int level ) const
{
	if ( level == -1 )
	{
		level = m_forStackPtr;
	}

	int i = level - 1;
	if ( i >= 0 )
	{
		return ScopedSymbolName(symbolName, m_forStack[ i ].m_id, m_forStack[ i ].m_count);
	}
	else
	{
		return ScopedSymbolName(symbolName);
	}
}



/*************************************************************************************************/
/**
	ControlFlow::IsIfConditionTrue()
*/
/*************************************************************************************************/
bool ControlFlow::IsIfConditionTrue() const
{
	for ( int i = 0; i < m_ifStackPtr; i++ )
	{
		if ( !m_ifStack[ i ].m_condition )
		{
			return false;
		}
	}

	return true;
}



/*************************************************************************************************/
/**
	ControlFlow::AddIfLevel()
*/
/*************************************************************************************************/
void ControlFlow::AddIfLevel( const string& line, int column )
{
	if ( m_ifStackPtr == MAX_IF_LEVELS )
	{
		throw AsmException_SyntaxError_TooManyIFs( line, column );
	}

	m_ifStack[ m_ifStackPtr ].m_condition			= true;
	m_ifStack[ m_ifStackPtr ].m_passed				= false;
	m_ifStack[ m_ifStackPtr ].m_hadElse				= false;
	m_ifStack[ m_ifStackPtr ].m_isMacroDefinition	= false;
	m_ifStack[ m_ifStackPtr ].m_line				= line;
	m_ifStack[ m_ifStackPtr ].m_column				= column;
	m_ifStack[ m_ifStackPtr ].m_lineNumber			= m_sourceCode->GetLineNumber();
	m_ifStackPtr++;
}



/*************************************************************************************************/
/**
	ControlFlow::SetCurrentIfAsMacroDefinition()
*/
/*************************************************************************************************/
void ControlFlow::SetCurrentIfAsMacroDefinition()
{
	assert( m_ifStackPtr > 0 );
	m_ifStack[ m_ifStackPtr - 1 ].m_isMacroDefinition = true;
}



/*************************************************************************************************/
/**
	ControlFlow::SetCurrentIfCondition()
*/
/*************************************************************************************************/
void ControlFlow::SetCurrentIfCondition( bool b )
{
	assert( m_ifStackPtr > 0 );
	m_ifStack[ m_ifStackPtr - 1 ].m_condition = b;
	if ( b )
	{
		m_ifStack[ m_ifStackPtr - 1 ].m_passed = true;
	}
}



/*************************************************************************************************/
/**
	ControlFlow::StartElse()
*/
/*************************************************************************************************/
void ControlFlow::StartElse( const string& line, int column )
{
	if ( m_ifStack[ m_ifStackPtr - 1 ].m_hadElse )
	{
		throw AsmException_SyntaxError_ElseWithoutIf( line, column );
	}

	m_ifStack[ m_ifStackPtr - 1 ].m_hadElse = true;

	m_ifStack[ m_ifStackPtr - 1 ].m_condition = !m_ifStack[ m_ifStackPtr - 1 ].m_passed;
}



/*************************************************************************************************/
/**
	ControlFlow::StartElif()
*/
/*************************************************************************************************/
void ControlFlow::StartElif( const string& line, int column )
{
	if ( m_ifStack[ m_ifStackPtr - 1 ].m_hadElse )
	{
		throw AsmException_SyntaxError_ElifWithoutIf( line, column );
	}

	m_ifStack[ m_ifStackPtr - 1 ].m_condition = !m_ifStack[ m_ifStackPtr - 1 ].m_passed;
}



/*************************************************************************************************/
/**
	ControlFlow::RemoveIfLevel()
*/
/*************************************************************************************************/
void ControlFlow::RemoveIfLevel( const string& line, int column )
{
	if ( m_ifStackPtr == 0 )
	{
		throw AsmException_SyntaxError_EndifWithoutIf( line, column );
	}

	m_ifStackPtr--;
}



/*************************************************************************************************/
/**
	ControlFlow::StartMacro()
*/
/*************************************************************************************************/
void ControlFlow::StartMacro( const string& line, int column )
{
	if ( GlobalData::Instance().IsFirstPass() )
	{
		if ( m_currentMacro == NULL )
		{
			m_currentMacro = new Macro( m_sourceCode->GetFilename(), m_sourceCode->GetLineNumber() );
		}
		else
		{
			throw AsmException_SyntaxError_NoNestedMacros( line, column );
		}
	}

	AddIfLevel( line, column );
	SetCurrentIfAsMacroDefinition();
}



/*************************************************************************************************/
/**
	ControlFlow::EndMacro()
*/
/*************************************************************************************************/
void ControlFlow::EndMacro( const string& line, int column )
{
	if ( GlobalData::Instance().IsFirstPass() &&
		 m_currentMacro == NULL )
	{
		throw AsmException_SyntaxError_EndMacroUnexpected( line, column - 8 );
	}

	RemoveIfLevel( line, column );

	if ( GlobalData::Instance().IsFirstPass() )
	{
		MacroTable::Instance().Add( m_currentMacro );
		m_currentMacro = NULL;
	}
}



/*************************************************************************************************/
/**
	ControlFlow::IsRealForLevel()

	Is the relevant level in the for stack a real for loop or one of the special ones used
        to implement braces?
*/
/*************************************************************************************************/
bool ControlFlow::IsRealForLevel( int level ) const
{
        assert( level > 0 );
        assert( level <= m_forStackPtr );
        return m_forStack[ level - 1 ].m_step != 0.0;
}



/*************************************************************************************************/
/**
	ControlFlow::GetSymbolValue()

	Search up the stack for a value for a symbol.  N.B. This is dynamic scoping.
*/
/*************************************************************************************************/
bool ControlFlow::GetSymbolValue(const std::string& name, Value& value)
{
	for ( int forLevel = GetForLevel(); forLevel >= 0; forLevel-- )
	{
		ScopedSymbolName fullSymbolName = GetScopedSymbolName( name, forLevel );

		if ( SymbolTable::Instance().IsSymbolDefined( fullSymbolName ) )
		{
			value = SymbolTable::Instance().GetSymbol( fullSymbolName );
			return true;
		}
	}
	return false;
}



/*************************************************************************************************/
/**
	ControlFlow::ShouldOutputAsm()

	Return true if verbose output is required.
*/
/*************************************************************************************************/
bool ControlFlow::ShouldOutputAsm()
{
	if (!GlobalData::Instance().IsSecondPass())
		return false;

	if (GlobalData::Instance().IsVerboseSet())
	{
		return GlobalData::Instance().IsVerbose();
	}

	Value value;
	if ( GetSymbolValue("VERBOSE", value) )
	{
		if (value.GetType() != Value::NumberValue)
			return false;
		return value.GetNumber() != 0;
	}

	return false;
}
