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

#include "JsonFile.h"

#include "JsonBoolean.h"
#include "JsonNumber.h"
#include "JsonString.h"
#include "JsonList.h"
#include "JsonMap.h"

#include <IO/OpenFile.h>

/*************************
Methods of class JsonFile:
*************************/

JsonFile::JsonFile(const char* fileName)
	:file(IO::openFile(fileName))
	{
	/* Set up the json file syntax: */
	file.setWhitespace('\n',true);
	file.setWhitespace('\r',true);
	file.setPunctuation("{}[]:,");
	file.setQuote('"',true);
	
	/* Prepare for reading: */
	file.skipWs();
	}

JsonFile::JsonFile(IO::File& baseFile)
	:file(&baseFile)
	{
	/* Set up the json file syntax: */
	file.setWhitespace('\n',true);
	file.setWhitespace('\r',true);
	file.setPunctuation("{}[]:,");
	file.setQuote('"',true);
	
	/* Prepare for reading: */
	file.skipWs();
	}

JsonPointer JsonFile::parseEntity(void)
	{
	/* Determine the type of the next entity: */
	switch(file.peekc())
		{
		case '"': // String
			{
			/* Parse a string: */
			std::string string=file.readString();
			return new JsonString(string);
			}
		
		case '[': // List
			{
			/* Skip the opening bracket: */
			file.skipString();
			
			/* Create a new list entity: */
			JsonList* list=new JsonList;
			
			/* Parse list items until the closing bracket: */
			while(true)
				{
				/* Parse the next list item: */
				JsonPointer item=parseEntity();
				list->getList().push_back(item);
				
				/* Check for comma or closing bracket: */
				if(file.peekc()==',')
					{
					/* Skip the comma: */
					file.skipString();
					}
				else if(file.peekc()==']')
					{
					/* Skip the closing bracket and end the list: */
					file.skipString();
					break;
					}
				else
					throw std::runtime_error("JsonFile::parseEntity: Illegal token in list");
				}
			
			return list;
			}
		
		case '{': // Map
			{
			/* Skip the opening brace: */
			file.skipString();
			
			/* Create a new map entity: */
			JsonMap* map=new JsonMap;
			
			/* Parse map items until the closing brace: */
			while(true)
				{
				/* Parse the next entity name: */
				if(file.peekc()!='"')
					throw std::runtime_error("JsonFile::parseEntity: No name in map item");
				std::string name=file.readString();
				
				/* Check for the colon: */
				if(!file.isLiteral(':'))
					throw std::runtime_error("JsonFile::parseEntity: Missing colon in map item");
				
				/* Parse the next entity: */
				JsonPointer entity=parseEntity();
				
				/* Store the association: */
				map->getMap()[name]=entity;
				
				/* Check for comma or closing brace: */
				if(file.peekc()==',')
					{
					/* Skip the comma: */
					file.skipString();
					}
				else if(file.peekc()=='}')
					{
					/* Skip the closing brace and end the map: */
					file.skipString();
					break;
					}
				else
					throw std::runtime_error("JsonFile::parseEntity: Illegal token in map");
				}
			
			return map;
			}
		
		case 'F': // Boolean literal
		case 'f':
		case 'T':
		case 't':
			{
			std::string value=file.readString();
			if(strcasecmp(value.c_str(),"true")==0)
				return new JsonBoolean(true);
			else if(strcasecmp(value.c_str(),"false")==0)
				return new JsonBoolean(false);
			else
				throw std::runtime_error("JsonFile::parseEntity: Illegal boolean literal");
			}
		
		case 'n': // NULL value
		case 'N':
			{
			std::string null=file.readString();
			if(strcasecmp(null.c_str(),"null")==0)
				return 0;
			else
				throw std::runtime_error("JsonFile::parseEntity: Illegal null value");
			}
		
		case '+': // Number
		case '-':
		case '.':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
			/* Parse a number: */
			double number=file.readNumber();
			return new JsonNumber(number);
			}
		
		default:
			throw std::runtime_error("JsonFile::parseEntity: Illegal token");
		}
	}
