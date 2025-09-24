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

#include "NetworkViewerClientTool.h"

#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#include "Network.h"
#include "PointSpherePicker.h"
#include "RaySpherePicker.h"

/*********************************************************
Static elements of class CollaborativeNetworkViewer::Tool:
*********************************************************/

CollaborativeNetworkViewer::Tool::Factory* CollaborativeNetworkViewer::Tool::factory=0;

/*************************************************
Methods of class CollaborativeNetworkViewer::Tool:
*************************************************/

unsigned int CollaborativeNetworkViewer::Tool::pickNode(int buttonSlotIndex)
	{
	unsigned int result=~0x0U;
	
	/* Get the network nodes and their most recent positions: */
	const Network::NodeList& nodes=application->nvClient->getNetwork().getNodes();
	const NVPointList& points=application->lockAndGetPositions();
	NVPointList::const_iterator pIt=points.begin();
	
	/* Check whether the tool is position-based or ray-based: */
	if(getButtonDevice(buttonSlotIndex)->is6DOFDevice())
		{
		/* Issue a point pick request against all nodes: */
		Point pickPoint(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(buttonSlotIndex)));
		Scalar pickDist(Vrui::getPointPickDistance());
		PointSpherePicker picker(pickPoint,pickDist);
		
		/* Pick using node radii based on current rendering settings: */
		for(Network::NodeList::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt,++pIt)
			picker(*pIt,application->getNodeRadius(*nIt));
		
		/* Check if a node was picked: */
		if(picker.havePickedSphere())
			{
			/* Remember the picked node's index: */
			result=picker.getPickIndex();
			}
		}
	else
		{
		/* Issue a ray pick request against all nodes: */
		pickRay=getButtonDeviceRay(buttonSlotIndex);
		pickRay.transform(Vrui::getInverseNavigationTransformation());
		Scalar maxPickCos(Vrui::getRayPickCosine());
		RaySpherePicker picker(pickRay,maxPickCos);
		
		/* Pick using node radii based on current rendering settings: */
		for(Network::NodeList::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt,++pIt)
			picker(*pIt,application->getNodeRadius(*nIt));
		
		/* Check if a node was picked: */
		if(picker.havePickedSphere())
			{
			/* Remember the picked node's index: */
			result=picker.getPickIndex();
			
			/* Remember the picked node's selection ray parameter: */
			pickRayLambda=Scalar(pickRay.getDirection()*(Vrui::Point(points[result])-pickRay.getOrigin()));
			}
		}
	
	return result;
	}

void CollaborativeNetworkViewer::Tool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("NetworkViewerClientTool","Network Viewer",0,*Vrui::getToolManager());
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addAbstractClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

CollaborativeNetworkViewer::Tool::Tool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment)
	{
	}

const Vrui::ToolFactory* CollaborativeNetworkViewer::Tool::getFactory(void) const
	{
	return factory;
	}
