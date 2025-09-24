/***********************************************************************
NetworkViewerTool - Base class for tools working with the NetworkViewer
application.
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

#include "NetworkViewerTool.h"

#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/PointPicker.h>
#include <Geometry/RayPicker.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#include "Node.h"
#include "Network.h"

/********************************************
Static elements of class NetworkViewer::Tool:
********************************************/

NetworkViewer::Tool::Factory* NetworkViewer::Tool::factory=0;

/************************************
Methods of class NetworkViewer::Tool:
************************************/

unsigned int NetworkViewer::Tool::pickNode(int buttonSlotIndex)
	{
	unsigned int result=~0x0U;
	
	/* Check whether the tool is position-based or ray-based: */
	if(getButtonDevice(buttonSlotIndex)->is6DOFDevice())
		{
		/* Issue a point pick request against all nodes: */
		Point pickPoint(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(buttonSlotIndex)));
		Scalar pickDist(Vrui::getPointPickDistance());
		Geometry::PointPicker<Scalar,3> picker(pickPoint,pickDist);
		const Network::NodeList& nodes=application->network->getNodes();
		for(Network::NodeList::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt)
			picker(application->particles.getParticlePosition(nIt->getParticleIndex()));
		
		/* Check if a node was picked: */
		if(picker.havePickedPoint())
			{
			/* Remember the picked node's index: */
			result=picker.getPickIndex();
			}
		}
	else
		{
		/* Issue a ray pick request: */
		pickRay=getButtonDeviceRay(buttonSlotIndex);
		pickRay.transform(Vrui::getInverseNavigationTransformation());
		Scalar maxPickCos(Vrui::getRayPickCosine());
		Geometry::RayPicker<Scalar,3> picker(Geometry::RayPicker<Scalar,3>::Ray(pickRay),maxPickCos);
		const Network::NodeList& nodes=application->network->getNodes();
		for(Network::NodeList::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt)
			picker(application->particles.getParticlePosition(nIt->getParticleIndex()));
		
		/* Check if a node was picked: */
		if(picker.havePickedPoint())
			{
			/* Remember the picked node's index: */
			result=picker.getPickIndex();
			
			/* Remember the picked node's selection ray parameter: */
			pickRayLambda=picker.getLambda();
			}
		}
	
	return result;
	}

void NetworkViewer::Tool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("NetworkViewerTool","Network Viewer",0,*Vrui::getToolManager());
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addAbstractClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

NetworkViewer::Tool::Tool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment)
	{
	}

const Vrui::ToolFactory* NetworkViewer::Tool::getFactory(void) const
	{
	return factory;
	}
