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

#ifndef SELECTANDDRAGTOOL_INCLUDED
#define SELECTANDDRAGTOOL_INCLUDED

#include <vector>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/GenericToolFactory.h>

#include "NetworkViewerTool.h"

/*****************************************************
Declaration of class NetworkViewer::SelectAndDragTool:
*****************************************************/

class NetworkViewer::SelectAndDragTool:public NetworkViewer::Tool
	{
	/* Embedded classes: */
	private:
	typedef Geometry::OrthonormalTransformation<Scalar,3> DragTransform; // Type for dragging transformations
	typedef Vrui::GenericToolFactory<SelectAndDragTool> Factory; // Factory class for this tool class
	friend class Vrui::GenericToolFactory<SelectAndDragTool>;
	
	struct DraggedParticle // Structure to represent a dragged particle
		{
		/* Element: */
		public:
		Index particleIndex; // Index of the dragged particle
		Scalar particleInvMass; // Original mass of the dragged particle
		Point dragPosition; // Position of the dragged particle relative to tool's drag transformation
		};
	
	/* Elements: */
	private:
	static Factory* factory; // Pointer to the factory object for this class
	bool draggingSelection; // Flag whether this tool is currently dragging the set of selected nodes
	std::vector<DraggedParticle> draggedParticles; // List of currently dragged particles
	
	/* Private methods: */
	void beginDrag(unsigned int pickedNodeIndex,const DragTransform& initialDragTransform); // Prepares for dragging the currently selected nodes, or the picked node if the selection is locked
	void drag(const DragTransform& dragTransform); // Drags the selected nodes
	void endDrag(void);
	
	/* Constructors and destructors: */
	public:
	static void initClass(void); // Initializes the tool's factory class
	SelectAndDragTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* Methods from class Vrui::Tool: */
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

#endif
