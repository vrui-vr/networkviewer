/***********************************************************************
SelectAndDragTool - Tool class to select single nodes from a network and
drag the currently selected nodes.
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

#include "SelectAndDragTool.h"

#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/PointPicker.h>
#include <Geometry/RayPicker.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#include "Node.h"
#include "Network.h"

/*********************************************************
Static elements of class NetworkViewer::SelectAndDragTool:
*********************************************************/

NetworkViewer::SelectAndDragTool::Factory* NetworkViewer::SelectAndDragTool::factory=0;

/*************************************************
Methods of class NetworkViewer::SelectAndDragTool:
*************************************************/

void NetworkViewer::SelectAndDragTool::beginDrag(unsigned int pickedNodeIndex,const NetworkViewer::SelectAndDragTool::DragTransform& initialDragTransform)
	{
	Network* network=application->network;
	const Network::NodeList& nodes=network->getNodes();
	ParticleSystem& particles=application->particles;
	
	/* Try locking the set of selected nodes: */
	draggingSelection=application->lockSelection();
	if(draggingSelection)
		{
		/* Check if the picked node is not currently selected: */
		if(!network->isSelected(pickedNodeIndex))
			{
			/* Set the selection to the picked node: */
			network->setSelection(pickedNodeIndex);
			}
		
		/* Begin dragging the set of selected nodes: */
		const Network::Selection& selection=network->getSelection();
		draggedParticles.reserve(selection.getNumEntries());
		for(Network::Selection::ConstIterator sIt=selection.begin();!sIt.isFinished();++sIt)
			{
			DraggedParticle dp;
			dp.particleIndex=nodes[sIt->getSource()].getParticleIndex();
			
			/* Remember the particle's original mass and set it to infinity during dragging: */
			dp.particleInvMass=application->particles.getParticleInvMass(dp.particleIndex);
			particles.setParticleInvMass(dp.particleIndex,Scalar(0));
			
			/* Calculate the particle's position in drag space: */
			dp.dragPosition=initialDragTransform.inverseTransform(particles.getParticlePosition(dp.particleIndex));
			
			draggedParticles.push_back(dp);
			}
		}
	else if(!network->isSelected(pickedNodeIndex))
		{
		/* Begin dragging the picked node: */
		DraggedParticle dp;
		dp.particleIndex=nodes[pickedNodeIndex].getParticleIndex();
		
		/* Remember the particle's original mass and set it to infinity during dragging: */
		dp.particleInvMass=particles.getParticleInvMass(dp.particleIndex);
		particles.setParticleInvMass(dp.particleIndex,Scalar(0));
		
		/* Calculate the particle's position in drag space: */
		dp.dragPosition=initialDragTransform.inverseTransform(particles.getParticlePosition(dp.particleIndex));
		
		draggedParticles.push_back(dp);
		}
	}

void NetworkViewer::SelectAndDragTool::drag(const NetworkViewer::SelectAndDragTool::DragTransform& dragTransform)
	{
	/* Move all dragged particles: */
	ParticleSystem& particles=application->particles;
	for(std::vector<DraggedParticle>::iterator dpIt=draggedParticles.begin();dpIt!=draggedParticles.end();++dpIt)
		particles.setParticlePosition(dpIt->particleIndex,dragTransform.transform(dpIt->dragPosition));
	}

void NetworkViewer::SelectAndDragTool::endDrag(void)
	{
	/* Reset the mass of all dragged particles: */
	ParticleSystem& particles=application->particles;
	for(std::vector<DraggedParticle>::iterator dpIt=draggedParticles.begin();dpIt!=draggedParticles.end();++dpIt)
		particles.setParticleInvMass(dpIt->particleIndex,dpIt->particleInvMass);
	
	/* Clear the list of dragged particles: */
	draggedParticles.clear();
	
	/* Unlock the set of selected node if it was being dragged: */
	if(draggingSelection)
		application->unlockSelection();
	draggingSelection=false;
	}

void NetworkViewer::SelectAndDragTool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("SelectAndDragTool","Select & Drag Nodes",Tool::factory,*Vrui::getToolManager());
	
	/* Set the tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Select & Drag");
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

NetworkViewer::SelectAndDragTool::SelectAndDragTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment),
	 draggingSelection(false)
	{
	}

const Vrui::ToolFactory* NetworkViewer::SelectAndDragTool::getFactory(void) const
	{
	return factory;
	}

void NetworkViewer::SelectAndDragTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Pick a node: */
		unsigned int pickedNodeIndex=pickNode(buttonSlotIndex);
		if(pickedNodeIndex!=~0x0U)
			{
			/* Calculate the initial drag transformation for 6-DOF dragging: */
			Vrui::NavTransform devTrans=Vrui::getInverseNavigationTransformation()*Vrui::NavTransform(getButtonDeviceTransformation(buttonSlotIndex));
			DragTransform initialDragTransform(DragTransform::Vector(devTrans.getTranslation()),DragTransform::Rotation(devTrans.getRotation()));
			
			/* Check whether the tool is ray-based: */
			if(!getButtonDevice(buttonSlotIndex)->is6DOFDevice())
				{
				/* Adjust the initial drag transformation for ray-based dragging: */
				initialDragTransform.leftMultiply(DragTransform::translate(pickRay(pickRayLambda)-initialDragTransform.getOrigin()));
				}
			
			/* Begin dragging nodes: */
			beginDrag(pickedNodeIndex,initialDragTransform);
			}
		else if(application->lockSelection())
			{
			/* Clear the node selection: */
			application->network->clearSelection();
			application->unlockSelection();
			}
		}
	else // Button has just been released
		{
		/* Stop dragging: */
		endDrag();
		}
	}

void NetworkViewer::SelectAndDragTool::frame(void)
	{
	/* Check if the tool is dragging particles: */
	if(!draggedParticles.empty())
		{
		/* Calculate the drag transformation: */
		Vrui::NavTransform devTrans=Vrui::getInverseNavigationTransformation()*Vrui::NavTransform(getButtonDeviceTransformation(0));
		DragTransform dragTransform(DragTransform::Vector(devTrans.getTranslation()),DragTransform::Rotation(devTrans.getRotation()));
		
		/* Check whether the tool is ray-based: */
		if(!getButtonDevice(0)->is6DOFDevice())
			{
			/* Offset the drag transformation along the tool's current selection ray: */
			Vrui::Ray pickRay(getButtonDeviceRay(0));
			pickRay.transform(Vrui::getInverseNavigationTransformation());
			dragTransform.leftMultiply(DragTransform::translate(DragTransform::Point(pickRay(pickRayLambda))-dragTransform.getOrigin()));
			}
		
		/* Drag the dragged particles: */
		drag(dragTransform);
		}
	}
