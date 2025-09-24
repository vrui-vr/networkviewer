/***********************************************************************
JsonNumber - Class for json entities representing numbers.
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

#ifndef JSONNUMBER_INCLUDED
#define JSONNUMBER_INCLUDED

#include "JsonEntity.h"

class JsonNumber:public JsonEntity
	{
	/* Elements: */
	private:
	double number; // The represented number
	
	/* Constructors and destructors: */
	public:
	JsonNumber(double sNumber)
		:number(sNumber)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const
		{
		return NUMBER;
		}
	virtual std::string getTypeName(void) const
		{
		return "Number";
		}
	virtual void print(std::ostream& os) const
		{
		os<<number;
		}
	
	/* New methods: */
	double getNumber(void) const // Returns the represented number
		{
		return number;
		}
	};

typedef Misc::Autopointer<JsonNumber> JsonNumberPointer; // Type for pointers to json numbers

/****************
Helper functions:
****************/

inline double getNumber(JsonPointer entity) // Returns the number represented by the given json entity; throws exception if entity is not a number
	{
	JsonNumberPointer number(entity);
	if(number==0)
		throw std::runtime_error("getNumber: Entity is not a number");
	
	return number->getNumber();
	}

#endif
