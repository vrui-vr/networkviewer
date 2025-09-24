/***********************************************************************
NetworkViewerClientShowLabelTool - Tool class to show or hide the
property label of a selected node.
Copyright (c) 2023 Oliver Kreylos

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

#include "NetworkViewerClientShowLabelTool.h"

#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

/******************************************************************
Static elements of class CollaborativeNetworkViewer::ShowLabelTool:
******************************************************************/

CollaborativeNetworkViewer::ShowLabelTool::Factory* CollaborativeNetworkViewer::ShowLabelTool::factory=0;

/**********************************************************
Methods of class CollaborativeNetworkViewer::ShowLabelTool:
**********************************************************/

void CollaborativeNetworkViewer::ShowLabelTool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("ShowLabelTool","Show Node Properties",Tool::factory,*Vrui::getToolManager());
	
	/* Set the tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Show/Hide");
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

CollaborativeNetworkViewer::ShowLabelTool::ShowLabelTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment)
	{
	}

const Vrui::ToolFactory* CollaborativeNetworkViewer::ShowLabelTool::getFactory(void) const
	{
	return factory;
	}

void CollaborativeNetworkViewer::ShowLabelTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
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
			/* Check if the node already has its label displayed: */
			if(application->nodeLabels.isEntry(pickedNodeIndex))
				{
				/* Hide the node's label: */
				application->nvClient->displayLabel(pickedNodeIndex,2);
				application->hideNodeLabel(pickedNodeIndex);
				}
			else
				{
				/* Show the node's label: */
				application->nvClient->displayLabel(pickedNodeIndex,1);
				application->showNodeLabel(pickedNodeIndex);
				}
			}
		}
	}
