/*************************************************************************************************/
/**
	controlflow.h


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

#ifndef CONTROLFLOW_H_
#define CONTROLFLOW_H_

#include "macro.h"
#include "scopedsymbolname.h"

class ControlFlow
{
public:
	ControlFlow();

	// For loop / if related stuff
	// Should use a std::vector here, but I can't really be bothered to change it now

	#define MAX_FOR_LEVELS	256
	#define MAX_IF_LEVELS	256

protected:

	struct For
	{
		ScopedSymbolName	m_varName;
		double				m_current;
		double				m_end;
		double				m_step;
		int					m_filePtr;
		int					m_id;
		int					m_count;
		std::string			m_line;
		int					m_column;
		int					m_lineNumber;
	};

	For						m_forStack[ MAX_FOR_LEVELS ];
	int						m_forStackPtr;

	struct If
	{
		bool				m_condition;
		bool                m_hadElse;
		bool				m_passed;
		bool				m_isMacroDefinition;
		std::string			m_line;
		int					m_column;
		int					m_lineNumber;
	};

	int						m_ifStackPtr;
	If						m_ifStack[ MAX_IF_LEVELS ];

	Macro*					m_currentMacro;

public:
	void					OpenBrace( const std::string& line, int column );
	void					CloseBrace( const std::string& line, int column );

	void					AddFor( const ScopedSymbolName& varName,
									double start,
									double end,
									double step,
									const std::string& line,
									int variableColumn,
									int bodyColumn );

	void					UpdateFor( const std::string& line, int column );

	inline int 				GetForLevel() const { return m_forStackPtr; }
	inline SourceCode*		GetCurrentSource() const { return m_sourceCode; }
	inline Macro*			GetCurrentMacro() { return m_currentMacro; }

	bool					GetSymbolValue(const std::string& name, Value& value);
	ScopedSymbolName		GetScopedSymbolName( const std::string& symbolName, int level = -1 ) const;
	int						GetForStackPtr() const { return m_forStackPtr; };
	int						GetIfStackPtr() const { return m_ifStackPtr; };

	bool					ShouldOutputAsm();

	bool					IsIfConditionTrue() const;
	void					AddIfLevel( const std::string& line, int column );
	void					SetCurrentIfAsMacroDefinition();
	void					SetCurrentIfCondition( bool b );
	void					StartElse( const std::string& line, int column );
	void					StartElif( const std::string& line, int column );
	void					ToggleCurrentIfCondition( const std::string& line, int column );
	void					RemoveIfLevel( const std::string& line, int column );
	void					StartMacro( const std::string& line, int column );
	void					EndMacro( const std::string& line, int column );
	bool					IsRealForLevel( int level ) const;

	void					IncludeFile( std::string filename );
	void					Process( SourceCode* sourceCode );
	void					CheckMismatches( const std::string& filename, int m_initialForStackPtr, int m_initialIfStackPtr );

	// For SOURCELINE
	void					SetLineNumber(int line) { m_sourceCode->SetLineNumber( line ); }
	void					SetFileName(const std::string& name) { m_sourceCode->SetFileName( name ); }

private:
	SourceCode* m_sourceCode;
	int m_nextForId;
};

#endif // CONTROLFLOW_H_
