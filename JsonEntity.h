/***********************************************************************
JsonEntity - Base class for entities parsed from json files.
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

#ifndef JSONENTITY_INCLUDED
#define JSONENTITY_INCLUDED

#include <iostream>
#include <Misc/Autopointer.h>
#include <Threads/RefCounted.h>

class JsonEntity:public Threads::RefCounted
	{
	/* Embedded classes: */
	public:
	enum EntityType // Enumerated type for json entity types
		{
		BOOLEAN,NUMBER,STRING,LIST,MAP
		};
	
	/* Methods: */
	public:
	virtual EntityType getType(void) const =0; // Returns the entity's type
	virtual std::string getTypeName(void) const =0; // Returns the entity's type as a string
	virtual void print(std::ostream& os) const =0; // Prints an entity to the given output stream
	};

typedef Misc::Autopointer<JsonEntity> JsonPointer; // Type for pointers to json entities

/* Helper function to print json entities: */
inline std::ostream& operator<<(std::ostream& os,const JsonEntity& entity)
	{
	entity.print(os);
	return os;
	}

#endif
