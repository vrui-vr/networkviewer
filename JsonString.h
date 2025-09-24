/***********************************************************************
JsonString - Class for json entities representing strings.
Copyright (c) 2018-2020 Oliver Kreylos

This file is part of the Network Viewer.

The Network Viewer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Network Viewer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Network Viewer; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef JSONSTRING_INCLUDED
#define JSONSTRING_INCLUDED

#include <string>

#include "JsonEntity.h"

class JsonString:public JsonEntity
	{
	/* Elements: */
	private:
	std::string string; // The represented string
	
	/* Constructors and destructors: */
	public:
	JsonString(const char* sString)
		:string(sString)
		{
		}
	JsonString(const std::string& sString)
		:string(sString)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const
		{
		return STRING;
		}
	virtual std::string getTypeName(void) const
		{
		return "String";
		}
	virtual void print(std::ostream& os) const
		{
		os<<'"'<<string<<'"';
		}
	
	/* New methods: */
	const std::string& getString(void) const // Returns the represented string
		{
		return string;
		}
	};

typedef Misc::Autopointer<JsonString> JsonStringPointer; // Type for pointers to json strings

/****************
Helper functions:
****************/

inline const std::string& getString(JsonPointer entity) // Returns the string represented by the given json entity; throws exception if entity is not a string
	{
	JsonStringPointer string(entity);
	if(string==0)
		throw std::runtime_error("getString: Entity is not a string");
	
	return string->getString();
	}

#endif
