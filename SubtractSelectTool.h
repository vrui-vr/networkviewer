/***********************************************************************
SubtractSelectTool - Tool class to remove single nodes from a network
from the current selection
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

#ifndef SUBTRACTSELECTTOOL_INCLUDED
#define SUBTRACTSELECTTOOL_INCLUDED

#include <Vrui/GenericToolFactory.h>

#include "NetworkViewerTool.h"

/******************************************************
Declaration of class NetworkViewer::SubtractSelectTool:
******************************************************/

class NetworkViewer::SubtractSelectTool:public NetworkViewer::Tool
	{
	/* Embedded classes: */
	private:
	typedef Vrui::GenericToolFactory<SubtractSelectTool> Factory; // Factory class for this tool class
	friend class Vrui::GenericToolFactory<SubtractSelectTool>;
	
	/* Elements: */
	private:
	static Factory* factory; // Pointer to the factory object for this class
	
	/* Constructors and destructors: */
	public:
	static void initClass(void); // Initializes the tool's factory class
	SubtractSelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* Methods from class Vrui::Tool: */
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
	};

#endif
