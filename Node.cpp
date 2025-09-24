/***********************************************************************
Node - Class for network nodes.
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

#include "Node.h"

#include <Math/Random.h>

#include "ParticleSystem.h"
#include "JsonNumber.h"
#include "JsonString.h"

namespace {

/****************
Helper functions:
****************/

int getHex(char hexChar)
	{
	if(hexChar>='0'&&hexChar<='9')
		return hexChar-'0';
	else if(hexChar>='A'&&hexChar<='F')
		return hexChar-'A'+10;
	else if(hexChar>='a'&&hexChar<='f')
		return hexChar-'a'+10;
	else
		throw std::runtime_error("getHex: Character is not a hexadecimal digit");
	}

}

/*********************
Methods of class Node:
*********************/

Node::Node(JsonMapPointer jsonMap,Index sParticleIndex)
	:id(getString(jsonMap->getProperty("id"))),
	 size(1),
	 color(128U,128U,128U),
	 particleIndex(sParticleIndex)
	{
	/* Assign optional properties: */
	if(jsonMap->hasProperty("size"))
		{
		JsonNumberPointer number(jsonMap->getProperty("size"));
		if(number!=0)
			size=number->getNumber();
		#if 0
		else
			{
			std::cout<<"Size property in node is of type "<<jsonMap->getProperty("size")->getType()<<" and value ";
			jsonMap->getProperty("size")->print(std::cout);
			std::cout<<std::endl;
			}
		#endif
		}
	if(jsonMap->hasProperty("color"))
		{
		/* Retrieve the color name: */
		const std::string& colorName=getString(jsonMap->getProperty("color"));
		
		/* Convert the color name into an RGB color: */
		if(colorName[0]!='#')
			throw std::runtime_error("Node::Node: Invalid color name");
		for(int i=0;i<3;++i)
			color[i]=Color::Scalar(getHex(colorName[1+i*2+0])*16+getHex(colorName[1+i*2+1]));
		color[3]=Misc::ColorComponentTraits<Color::Scalar>::one;
		}
	}

void Node::createParticle(ParticleSystem& particles,Scalar domainSize)
	{
	/* Create a particle at a random position inside the domain: */
	Point pos;
	for(int i=0;i<3;++i)
		pos[i]=Scalar(Math::randUniformCO(-domainSize,domainSize));
	particleIndex=particles.addParticle(Scalar(1),pos,Vector::zero);
	}
