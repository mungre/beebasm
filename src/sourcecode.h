/*************************************************************************************************/
/**
	sourcecode.h


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

#ifndef SOURCECODE_H_
#define SOURCECODE_H_

#include <string>

#include "scopedsymbolname.h"
#include "value.h"

class Macro;
class ControlFlow;

class SourceCode
{
public:

	// Constructor/destructor

	SourceCode( const std::string& filename, int lineNumber, const std::string& text, const SourceCode* parent );
	~SourceCode();

	// Process the file

	virtual void Process( ControlFlow* controlFlow );

	// Accessors

	inline const std::string&	GetFilename() const				{ return m_filename; }
	inline int				GetLineNumber() const			{ return m_lineNumber; }
	inline const SourceCode*GetParent() const				{ return m_parent; }
	inline int				GetLineStartPointer() const		{ return m_lineStartPointer; }

	virtual bool			GetLine( std::string& lineFromFile );
	virtual int				GetFilePointer() { return m_textPointer; }
	void					SetPosition( int filePointer, int lineNumber );
	virtual bool			IsAtEnd() { return m_textPointer == static_cast<int>(m_text.length()); }

	inline int 				GetInitialForStackPtr() const { return m_initialForStackPtr; }
	// For SOURCELINE
	void					SetLineNumber(int line) { m_lineNumber = line; }
	void					SetFileName(const std::string& name) { m_filename = name; }

protected:

	int						m_initialForStackPtr;
	int						m_initialIfStackPtr;
	std::string				m_filename;
	int						m_lineNumber;
	const SourceCode*		m_parent;
	int						m_lineStartPointer;
	std::string				m_text;
	int						m_textPointer;
};


#endif // SOURCECODE_H_
