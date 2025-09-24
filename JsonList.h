/***********************************************************************
JsonString - Class for json entities representing lists of json
entities.
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

#ifndef JSONLIST_INCLUDED
#define JSONLIST_INCLUDED

#include <vector>

#include "JsonEntity.h"

class JsonList:public JsonEntity
	{
	/* Embedded classes: */
	public:
	typedef std::vector<JsonPointer> List; // Type for lists of json entities
	
	/* Elements: */
	private:
	List list; // The represented list
	
	/* Methods from class JsonEntity: */
	public:
	virtual EntityType getType(void) const
		{
		return LIST;
		}
	virtual std::string getTypeName(void) const
		{
		return "List";
		}
	virtual void print(std::ostream& os) const
		{
		os<<'[';
		for(List::const_iterator lIt=list.begin();lIt!=list.end();++lIt)
			{
			if(lIt!=list.begin())
				os<<',';
			(*lIt)->print(os);
			}
		os<<']';
		}
	
	/* New methods: */
	const List& getList(void) const // Returns the represented list
		{
		return list;
		}
	List& getList(void) // Ditto
		{
		return list;
		}
	bool empty(void) const // Returns true if the list is empty
		{
		return list.empty();
		}
	size_t size(void) const // Returns the number of elements in the list
		{
		return list.size();
		}
	JsonPointer getItem(size_t index) // Returns a list item
		{
		return list[index];
		}
	};

typedef Misc::Autopointer<JsonList> JsonListPointer; // Type for pointers to json lists

#endif
