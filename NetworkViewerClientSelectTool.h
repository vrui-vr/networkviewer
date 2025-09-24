/***********************************************************************
NetworkViewerClientSelectTool - Tool class to select an individual node.
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

#ifndef NETWORKVIEWERCLIENTSELECTTOOL_INCLUDED
#define NETWORKVIEWERCLIENTSELECTTOOL_INCLUDED

#include <Vrui/GenericToolFactory.h>

#include "NetworkViewerClientTool.h"

/***********************************************************
Declaration of class CollaborativeNetworkViewer::SelectTool:
***********************************************************/

class CollaborativeNetworkViewer::SelectTool:public CollaborativeNetworkViewer::Tool
	{
	/* Embedded classes: */
	private:
	typedef Vrui::GenericToolFactory<SelectTool> Factory; // Factory class for this tool class
	friend class Vrui::GenericToolFactory<SelectTool>;
	
	/* Elements: */
	private:
	static Factory* factory; // Pointer to the factory object for this class
	
	/* Constructors and destructors: */
	public:
	static void initClass(void); // Initializes the tool's factory class
	SelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* Methods from class Vrui::Tool: */
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
	};

#endif
