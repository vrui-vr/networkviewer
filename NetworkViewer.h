/***********************************************************************
NetworkViewer - Vrui application to interactively explore mineral
networks or other graphs laid out in 3D space.
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

#ifndef NETWORKVIEWER_INCLUDED
#define NETWORKVIEWER_INCLUDED

#include "Config.h"

#include <vector>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLColorMap.h>
#if CONFIG_USE_IMPOSTORSPHERES
#include <GL/GLSphereRenderer.h>
#endif
#include <GLMotif/Button.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/DropdownBox.h>
#include <GLMotif/TextFieldSlider.h>
#include <Vrui/Application.h>
#include <Vrui/TransparentObject.h>
#include <Vrui/ObjectSnapperTool.h>

#include "ParticleTypes.h"
#include "ParticleSystem.h"

/* Forward declarations: */
namespace GLMotif {
class PopupWindow;
class PopupMenu;
}
class Network;

class NetworkViewer:public Vrui::Application,public Vrui::TransparentObject,public GLObject
	{
	/* Embedded classes: */
	private:
	enum ForceMode // Enumerated type for n-body repelling force formulas
		{
		Linear, // Inverse linear force law
		Quadratic // Inverse quadratic force law
		};
	
	class Tool; // Base class for tools working with the NetworkViewer application
	friend class Tool;
	
	class SelectAndDragTool;
	class AddSelectTool;
	class SubtractSelectTool;
	class ShowPropertiesTool;
	friend class SelectAndDragTool;
	friend class AddSelectTool;
	friend class SubtractSelectTool;
	friend class ShowPropertiesTool;
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBuffer; // Buffer holding node positions and colors
		GLuint indexBuffer; // Buffer holding indices of linked nodes
		GLenum sphereDisplayList; // ID of a display list to render a small sphere
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	Network* network; // The visualized network
	ParticleSystem particles; // Particle system simulating the interaction between linked nodes
	Scalar centralForce; // Coefficient of central force pulling particles towards the center of the display
	ForceMode repellingForceMode; // Repelling force calculation mode
	Scalar repellingForce; // Coefficient of repelling n-body force
	Scalar repellingForceTheta; // Approximation threshold for Barnes-Hut n-body force calculation
	Scalar repellingForceCutoff; // Cutoff distance for inverse repelling force law
	Scalar linkStrength; // Strength parameter for node links
	Scalar nodeRadius; // Radius factor for node spheres
	bool useNodeSize; // Flag to scale node spheres by their size fields
	Scalar nodeSizeExponent; // Exponent to calculate node sphere radius from node size field
	bool selectionLocked; // Flag if the set of selected nodes is currently locked, e.g., while being dragged
	GLMotif::PopupWindow* parametersDialog; // Dialog window to control simulation parameters
	GLMotif::PopupWindow* renderingDialog; // Dialog window to control rendering settings
	GLMotif::PopupMenu* mainMenu; // The application's main menu
	#if CONFIG_USE_IMPOSTORSPHERES
	GLSphereRenderer nodeRenderer; // Renderer for node spheres
	#endif
	GLColorMap numericalPropertyValueMap; // Color map mapping numerical node property values to colors
	unsigned int calloutNodeIndex; // Index of the node whose properties are currently being displayed
	GLMotif::Widget* nodeCallout; // A widget displaying the most recently picked node's property values
	
	/* Private methods: */
	void loadNetworkFile(const char* networkFileName); // Loads a network from a JSON file of the given name
	GLMotif::Widget* createNodeCallout(unsigned int nodeIndex); // Creates a display showing the properties and values of the node of the given node index
	void showNodeProperties(unsigned int nodeIndex); // Displays the given node's property values
	bool lockSelection(void); // Locks the selection; returns true if the selection was not already locked
	void unlockSelection(void); // Unlocks the selection; assumes that caller successfully locked it before
	void objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest); // Callback called when an object snapper tool issues a snap request
	void attenuationValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void repellingForceModeValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void linkStrengthValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	GLMotif::PopupWindow* createParametersDialog(void); // Creates a dialog window to change simulation parameters
	GLMotif::PopupWindow* createRenderingDialog(void); // Creates a dialog window to change rendering settings
	void clearSelectionCallback(Misc::CallbackData* cbData);
	void growSelectionCallback(Misc::CallbackData* cbData);
	void shrinkSelectionCallback(Misc::CallbackData* cbData);
	GLMotif::PopupMenu* createSelectionMenu(void); // Creates the selection sub-menu
	void colorMapPropertySelectedCallback(GLMotif::Button::SelectCallbackData* cbData);
	GLMotif::PopupMenu* createColorMappingMenu(void); // Creates the color mapping sub-menu
	void showParametersDialogCallback(Misc::CallbackData* cbData);
	void showRenderingDialogCallback(Misc::CallbackData* cbData);
	GLMotif::PopupMenu* createMainMenu(void); // Creates the application's main menu
	
	/* Constructors and destructors: */
	public:
	NetworkViewer(int& argc,char**& argv);
	virtual ~NetworkViewer(void);
	
	/* Methods from class Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* Methods from class Vrui::TransparentObject: */
	virtual void glRenderActionTransparent(GLContextData& contextData) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

#endif
