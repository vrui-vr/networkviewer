/***********************************************************************
SimulationParameters - Structure to hold parameters for a force-
directed network layout simulation algorithm.
Copyright (c) 2020-2023 Oliver Kreylos

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

#include "SimulationParameters.h"

/**************************************
Methods of struct SimulationParameters:
**************************************/

SimulationParameters::SimulationParameters(void)
	:attenuation(0.5),
	 centralForce(5),
	 repellingForceMode(Linear),
	 repellingForce(2),
	 repellingForceTheta(0.25),
	 repellingForceCutoff(0.01),
	 numRelaxationIterations(20),
	 linkStrength(0.5)
	{
	}
