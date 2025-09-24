/***********************************************************************
JsonBoolean - Class for json entities representing boolean values.
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

#ifndef JSONBOOLEAN_INCLUDED
#define JSONBOOLEAN_INCLUDED

#include "JsonEntity.h"

class JsonBoolean:public JsonEntity
	{
	/* Elements: */
	private:
	bool value; // The represented boolean value
	
	/* Constructors and destructors: */
	public:
	JsonBoolean(bool sValue)
		:value(sValue)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const
		{
		return BOOLEAN;
		}
	virtual std::string getTypeName(void) const
		{
		return "Boolean";
		}
	virtual void print(std::ostream& os) const
		{
		os<<(value?"true":"false");
		}
	
	/* New methods: */
	bool getBoolean(void) const // Returns the represented boolean value
		{
		return value;
		}
	};

typedef Misc::Autopointer<JsonBoolean> JsonBooleanPointer; // Type for pointers to json booleans

/****************
Helper functions:
****************/

inline bool getBoolean(JsonPointer entity) // Returns the boolean value represented by the given json entity; throws exception if entity is not a boolean
	{
	JsonBooleanPointer boolean(entity);
	if(boolean==0)
		throw std::runtime_error("getBoolean: Entity is not a boolean");
	
	return boolean->getBoolean();
	}

#endif
