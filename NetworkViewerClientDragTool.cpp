/***********************************************************************
NetworkViewerClientDragTool - Tool class to drag an individual node or
the current selection.
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

#include "NetworkViewerClientDragTool.h"

#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Collaboration2/Plugins/VruiCoreClient.h>

/*************************************************************
Static elements of class CollaborativeNetworkViewer::DragTool:
*************************************************************/

CollaborativeNetworkViewer::DragTool::Factory* CollaborativeNetworkViewer::DragTool::factory=0;

/*****************************************************
Methods of class CollaborativeNetworkViewer::DragTool:
*****************************************************/

void CollaborativeNetworkViewer::DragTool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("DragTool","Drag Nodes",Tool::factory,*Vrui::getToolManager());
	
	/* Set the tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Drag");
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

CollaborativeNetworkViewer::DragTool::DragTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment),
	 inputDeviceId(0),
	 dragId(0)
	{
	}

const Vrui::ToolFactory* CollaborativeNetworkViewer::DragTool::getFactory(void) const
	{
	return factory;
	}

void CollaborativeNetworkViewer::DragTool::initialize(void)
	{
	/* Retrieve the Vrui Core ID of the input device to which this tool is attached: */
	if(application->vruiCoreClient!=0)
		inputDeviceId=application->vruiCoreClient->getInputDeviceId(getButtonDevice(0));
	}

void CollaborativeNetworkViewer::DragTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
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
			/* Start a drag operation: */
			dragId=application->nvClient->startDrag(inputDeviceId,pickedNodeIndex);
			}
		}
	else // Button has just been released
		{
		/* Finish a current drag operation: */
		if(dragId!=0)
			application->nvClient->stopDrag(dragId);
		dragId=0;
		}
	}

void CollaborativeNetworkViewer::DragTool::frame(void)
	{
	/* Bail out if there is no collaboration client: */
	if(application->nvClient==0)
		return;
	
	/* Send a drag update if there is an active drag: */
	if(dragId!=0)
		application->nvClient->drag(dragId);
	}
