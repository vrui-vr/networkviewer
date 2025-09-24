/***********************************************************************
SimulationParameters - Structure to hold parameters for a force-
directed network layout simulation algorithm.
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

#ifndef SIMULATIONPARAMETERS_INCLUDED
#define SIMULATIONPARAMETERS_INCLUDED

#include <stddef.h>
#include <Misc/SizedTypes.h>

#include "ParticleTypes.h"

struct SimulationParameters
	{
	/* Embedded classes: */
	public:
	enum ForceMode // Enumerated type for n-body repelling force formulas
		{
		Linear, // Inverse linear force law
		Quadratic // Inverse quadratic force law
		};
	
	/* Elements: */
	static const size_t size=2*sizeof(Scalar)+sizeof(Misc::UInt8)+3*sizeof(Scalar)+sizeof(Misc::UInt8)+sizeof(Scalar); // Size of simulation parameters when read from/written to a binary source/sink
	Scalar attenuation; // Velocity attenuation factor
	Scalar centralForce; // Coefficient of central force pulling particles towards the center of the display
	Misc::UInt8 repellingForceMode; // Repelling force calculation mode
	Scalar repellingForce; // Coefficient of repelling n-body force
	Scalar repellingForceTheta; // Approximation threshold for Barnes-Hut n-body force calculation
	Scalar repellingForceCutoff; // Cutoff distance for inverse repelling force law
	Misc::UInt8 numRelaxationIterations; // Number of iterations for the constraint solver
	Scalar linkStrength; // Strength parameter for node links
	
	/* Constructors and destructors: */
	SimulationParameters(void); // Creates default set of simulation parameters
	
	/* Methods: */
	template <class SourceParam>
	void read(SourceParam& source) // Reads simulation parameters from a binary source
		{
		source.read(attenuation);
		source.read(centralForce);
		source.read(repellingForceMode);
		source.read(repellingForce);
		source.read(repellingForceTheta);
		source.read(repellingForceCutoff);
		source.read(numRelaxationIterations);
		source.read(linkStrength);
		}
	template <class SinkParam>
	void write(SinkParam& sink) const // Writes simulation parameters to a binary sink
		{
		sink.write(attenuation);
		sink.write(centralForce);
		sink.write(repellingForceMode);
		sink.write(repellingForce);
		sink.write(repellingForceTheta);
		sink.write(repellingForceCutoff);
		sink.write(numRelaxationIterations);
		sink.write(linkStrength);
		}
	};

#endif
