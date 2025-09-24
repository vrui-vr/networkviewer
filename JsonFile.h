/***********************************************************************
JsonFile - Class to represent json files and parse json entities.
Copyright (c) 2018-2023 Oliver Kreylos

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

#ifndef JSONFILE_INCLUDED
#define JSONFILE_INCLUDED

#include <IO/ValueSource.h>

#include "JsonEntity.h"

/* Forward declarations: */
namespace IO {
class File;
}

class JsonFile
	{
	/* Elements: */
	private:
	IO::ValueSource file; // The represented json file
	
	/* Constructors and destructors: */
	public:
	JsonFile(const char* fileName); // Opens the json file of the given name
	JsonFile(IO::File& baseFile); // Prepares to parse the given already opened file
	
	/* Methods: */
	bool eof(void) const // Returns true if the json file has been completely parsed
		{
		return file.eof();
		}
	JsonPointer parseEntity(void); // Parses the next entity from the json file; throws exception at end of file or on syntax error
	};

#endif
