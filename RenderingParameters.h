/***********************************************************************
RenderingParameters - Structure to hold parameters to render a network.
Copyright (c) 2020 Oliver Kreylos

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

#ifndef RENDERINGPARAMETERS_INCLUDED
#define RENDERINGPARAMETERS_INCLUDED

#include <stddef.h>
#include <Misc/SizedTypes.h>

#include "ParticleTypes.h"

struct RenderingParameters
	{
	/* Elements: */
	public:
	static const size_t size=sizeof(Scalar)+sizeof(Misc::UInt8)+sizeof(Scalar)+2*sizeof(Misc::Float32); // Size of rendering parameters when read from/written to a binary source/sink
	Scalar nodeRadius; // Radius factor for node spheres
	bool useNodeSize; // Flag to scale node spheres by their size fields, or use node radius factor directly
	Scalar nodeSizeExponent; // Exponent to calculate node sphere radius from node size field
	Misc::Float32 linkLineWidth; // Cosmetic line width to render link lines
	Misc::Float32 linkOpacity; // Opacity value to render link lines
	
	/* Constructors and destructors: */
	RenderingParameters(void); // Creates default set of rendering parameters
	
	/* Methods: */
	template <class SourceParam>
	void read(SourceParam& source) // Reads rendering parameters from a binary source
		{
		source.read(nodeRadius);
		useNodeSize=source.template read<Misc::UInt8>()!=0U;
		source.read(nodeSizeExponent);
		source.read(linkLineWidth);
		source.read(linkOpacity);
		}
	template <class SinkParam>
	void write(SinkParam& sink) const // Writes rendering parameters to a binary sink
		{
		sink.write(nodeRadius);
		sink.write(Misc::UInt8(useNodeSize?1U:0U));
		sink.write(nodeSizeExponent);
		sink.write(linkLineWidth);
		sink.write(linkOpacity);
		}
	};

#endif
