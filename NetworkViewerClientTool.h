/***********************************************************************
NetworkViewerClientTool - Base class for tools working with the
NetworkViewerClient application.
Copyright (c) 2019-2023 Oliver Kreylos

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

#ifndef NETWORKVIEWERCLIENTTOOL_INCLUDED
#define NETWORKVIEWERCLIENTTOOL_INCLUDED

#include <Vrui/Tool.h>
#include <Vrui/GenericAbstractToolFactory.h>

#include "CollaborativeNetworkViewer.h"

/**********************************************
Declaration of class NetworkViewerClient::Tool:
**********************************************/

class CollaborativeNetworkViewer::Tool:public Vrui::Tool,public Vrui::Application::Tool<CollaborativeNetworkViewer>
	{
	/* Embedded classes: */
	protected:
	typedef Vrui::GenericAbstractToolFactory<Tool> Factory; // Factory class for this tool class
	friend class Vrui::GenericAbstractToolFactory<Tool>;
	
	/* Elements: */
	protected:
	static Factory* factory; // Pointer to the factory object for this class
	Vrui::Ray pickRay; // Most recent picking ray
	Scalar pickRayLambda; // Ray parameter of the most recently picked point along the picking ray
	
	/* Protected methods: */
	unsigned int pickNode(int buttonSlotIndex); // Returns the index of the node being picked by the tool, or ~0x0U if no node was picked
	
	/* Constructors and destructors: */
	public:
	static void initClass(void); // Initializes the tool's factory class
	Tool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* Methods from class Vrui::Tool: */
	virtual const Vrui::ToolFactory* getFactory(void) const;
	};

#endif
