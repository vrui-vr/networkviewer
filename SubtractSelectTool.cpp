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

#include "SubtractSelectTool.h"

#include <Vrui/ToolManager.h>

#include "Network.h"

/**********************************************************
Static elements of class NetworkViewer::SubtractSelectTool:
**********************************************************/

NetworkViewer::SubtractSelectTool::Factory* NetworkViewer::SubtractSelectTool::factory=0;

/**************************************************
Methods of class NetworkViewer::SubtractSelectTool:
**************************************************/

void NetworkViewer::SubtractSelectTool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("SubtractSelectTool","Deselect Nodes",Tool::factory,*Vrui::getToolManager());
	
	/* Set the tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Deselect");
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

NetworkViewer::SubtractSelectTool::SubtractSelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment)
	{
	}

const Vrui::ToolFactory* NetworkViewer::SubtractSelectTool::getFactory(void) const
	{
	return factory;
	}

void NetworkViewer::SubtractSelectTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Pick a node: */
		unsigned int pickedNodeIndex=pickNode(buttonSlotIndex);
		if(pickedNodeIndex!=~0x0U&&application->lockSelection())
			{
			/* Select the picked node: */
			application->network->deselectNode(pickedNodeIndex);
			application->unlockSelection();
			}
		}
	}
