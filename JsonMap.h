/***********************************************************************
JsonMap - Class for json entities representing a table of associations
of json entities with strings.
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

#ifndef JSONMAP_INCLUDED
#define JSONMAP_INCLUDED

#include <string>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>

#include "JsonEntity.h"

class JsonMap:public JsonEntity
	{
	/* Embedded classes: */
	public:
	typedef Misc::HashTable<std::string,JsonPointer> Map; // Type for maps mapping strings to json entities
	
	/* Elements: */
	private:
	Map map; // The represented map
	
	/* Constructors and destructors: */
	public:
	JsonMap(void) // Creates an empty map
		:map(17)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const
		{
		return MAP;
		}
	virtual std::string getTypeName(void) const
		{
		return "Map";
		}
	virtual void print(std::ostream& os) const
		{
		os<<'{';
		for(Map::ConstIterator mIt=map.begin();!mIt.isFinished();++mIt)
			{
			if(mIt!=map.begin())
				os<<',';
			os<<'"'<<mIt->getSource()<<'"'<<':';
			mIt->getDest()->print(os);
			}
		os<<'}';
		}
	
	/* New methods: */
	const Map& getMap(void) const // Returns the represented map
		{
		return map;
		}
	Map& getMap(void) // Ditto
		{
		return map;
		}
	bool hasProperty(const std::string& name) const // Returns true if an entity is associated with the given name
		{
		return map.isEntry(name);
		}
	JsonPointer getProperty(const std::string& name) // Returns the json entity associated with the given name
		{
		/* Return the entity associated with the given name, throwing an exception if it is not there: */
		return map.getEntry(name).getDest();
		}
	};

typedef Misc::Autopointer<JsonMap> JsonMapPointer; // Type for pointers to json maps

#endif
