/***********************************************************************
NetworkViewerClientToggleToggleSelectTool - Tool class to toggle the selection
state of an individual node.
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

#include "NetworkViewerClientToggleSelectTool.h"

#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

/*********************************************************************
Static elements of class CollaborativeNetworkViewer::ToggleSelectTool:
*********************************************************************/

CollaborativeNetworkViewer::ToggleSelectTool::Factory* CollaborativeNetworkViewer::ToggleSelectTool::factory=0;

/*************************************************************
Methods of class CollaborativeNetworkViewer::ToggleSelectTool:
*************************************************************/

void CollaborativeNetworkViewer::ToggleSelectTool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("ToggleSelectTool","Select/Deselect Nodes",Tool::factory,*Vrui::getToolManager());
	
	/* Set the tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Select/Deselect");
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

CollaborativeNetworkViewer::ToggleSelectTool::ToggleSelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment)
	{
	}

const Vrui::ToolFactory* CollaborativeNetworkViewer::ToggleSelectTool::getFactory(void) const
	{
	return factory;
	}

void CollaborativeNetworkViewer::ToggleSelectTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Bail out if there is no collaboration client: */
	if(application->nvClient==0)
		return;
	
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Pick a node: */
		unsigned int pickedNodeIndex=pickNode(buttonSlotIndex);
		if(pickedNodeIndex!=~0x0U)
			{
			/* Select the picked node: */
			application->nvClient->selectNode(pickedNodeIndex,2);
			}
		}
	}
