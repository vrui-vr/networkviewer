/***********************************************************************
CollaborativeNetworkViewer - Client application for collaborative
network viewer.
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

#ifndef COLLABORATIVENETWORKVIEWER_INCLUDED
#define COLLABORATIVENETWORKVIEWER_INCLUDED

#include "Config.h"

#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/TripleBuffer.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#if CONFIG_USE_IMPOSTORSPHERES
#include <GL/GLSphereRenderer.h>
#endif
#include <GL/GLGeometryVertex.icpp>
#include <GLMotif/DropdownBox.h>
#include <GLMotif/TextFieldSlider.h>
#include <GLMotif/FileSelectionDialog.h>
#include <GLMotif/FileSelectionHelper.h>
#include <SceneGraph/OGTransformNode.h>
#include <SceneGraph/FancyFontStyleNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/TransparentObject.h>
#include <Vrui/ObjectSnapperTool.h>
#include <Collaboration2/CollaborativeVruiApplication.h>

#include "ParticleTypes.h"
#include "Network.h"
#include "SimulationParameters.h"
#include "RenderingParameters.h"
#include "NetworkViewerClient.h"

/* Forward declarations: */
namespace GLMotif {
class PopupMenu;
class PopupWindow;
}
class Network;

class CollaborativeNetworkViewer:public Collab::CollaborativeVruiApplication,public Vrui::TransparentObject,public GLObject
	{
	friend class Collab::Plugins::NetworkViewerClient;
	
	/* Embedded classes: */
	private:
	typedef Collab::Plugins::NetworkViewerClient::NVScalar NVScalar;
	typedef Collab::Plugins::NetworkViewerClient::NVPoint NVPoint;
	typedef Collab::Plugins::NetworkViewerClient::NVPointList NVPointList;
	typedef Misc::HashTable<unsigned int,SceneGraph::OGTransformNodePointer> NodeLabelMap; // Type for maps from node IDs to node label scene graphs
	
	class Tool; // Base class for tools working with the NetworkViewerClient application
	class SelectTool; // Tool class to select individual nodes
	class DeselectTool; // Tool class to deselect individual nodes
	class ToggleSelectTool; // Tool class to toggle the selection state of individual nodes
	class ShowLabelTool; // Tool class to show/hide labels displaying a node's properties
	class DragTool; // Tool class to drag an individual node or the current selection
	
	friend class Tool;
	friend class SelectTool;
	friend class DeselectTool;
	friend class ToggleSelectTool;
	friend class ShowLabelTool;
	friend class DragTool;
	
	struct DataItem:public GLObject::DataItem
		{
		/* Embedded classes: */
		public:
		typedef GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,4> Vertex; // Type for vertices held in vertex buffers
		
		/* Elements: */
		GLuint vertexBuffer; // Buffer holding node positions and colors
		GLuint indexBuffer; // Buffer holding indices of linked nodes
		unsigned int networkVersion; // Version number of the visualized network
		unsigned int vertexVersion; // Version number of the vertex buffer
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	Collab::Plugins::NetworkViewerClient* nvClient; // Network viewer plug-in protocol client
	GLMotif::FileSelectionHelper loadNetworkFileHelper; // Helper object to load/save network files
	GLMotif::PopupMenu* mainMenu;
	GLMotif::PopupWindow* simulationParametersDialog;
	GLMotif::PopupWindow* renderingDialog; // Dialog window to control rendering settings
	const char* startupNetworkFileName; // Name of a network file to load on application start-up, or 0
	SimulationParameters simulationParameters; // Current network simulation parameters
	unsigned int networkVersion; // Version number of the visualized network
	Threads::TripleBuffer<NVPointList> positions; // Triple buffer of network node positions
	unsigned int networkPositionVersion; // Version number of the visualized network reflected in the locked position array
	unsigned int positionVersion; // Version number of the locked position array
	SceneGraph::FancyFontStyleNodePointer labelFontStyle; // Font style to use for node labels
	NodeLabelMap nodeLabels; // Set of currently displayed node labels
	RenderingParameters renderingParameters; // Current set of rendering parameters
	#if CONFIG_USE_IMPOSTORSPHERES
	GLSphereRenderer nodeRenderer; // Renderer for node spheres
	#endif
	
	/* Protected methods from class Collab::CollaborativeVruiApplication: */
	virtual void shutdownClient(void);
	
	/* Private methods: */
	Scalar getNodeRadius(unsigned int nodeIndex) const // Returns the radius of the given node based on current rendering settings
		{
		if(renderingParameters.useNodeSize)
			return renderingParameters.nodeRadius*Math::pow(nvClient->getNetwork().getNodes()[nodeIndex].getSize(),renderingParameters.nodeSizeExponent);
		else
			return renderingParameters.nodeRadius;
		}
	Scalar getNodeRadius(const Node& node) const // Ditto
		{
		if(renderingParameters.useNodeSize)
			return renderingParameters.nodeRadius*Math::pow(node.getSize(),renderingParameters.nodeSizeExponent);
		else
			return renderingParameters.nodeRadius;
		}
	void objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest); // Callback called when an object snapper tool issues a snap request
	void loadNetworkFileCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void changeSelectionCallback(Misc::CallbackData* cbData,const unsigned int& command);
	void showSimulationParametersDialogCallback(Misc::CallbackData* cbData);
	void showRenderingDialogCallback(Misc::CallbackData* cbData);
	GLMotif::PopupMenu* createSelectionMenu(void);
	GLMotif::PopupMenu* createMainMenu(void);
	void updateNetwork(Network* newNetwork); // Notifies the application that a new network has been loaded
	void updateSimulationParameters(const SimulationParameters& newSimulationParameters);
	void clearNodeLabels(void); // Removes all node labels
	void showNodeLabel(unsigned int nodeIndex); // Shows a label for the node of the given index
	void hideNodeLabel(unsigned int nodeIndex); // Hides the label for the node of the given index
	void updateRenderingParameters(const RenderingParameters& newRenderingParameters);
	void simulationParametersChangedCallback(Misc::CallbackData* cbData);
	GLMotif::PopupWindow* createSimulationParametersDialog(void); // Creates a dialog window to change network simulation settings
	void renderingParametersChangedCallback(Misc::CallbackData* cbData);
	GLMotif::PopupWindow* createRenderingDialog(void); // Creates a dialog window to change rendering settings
	const NVPointList& lockAndGetPositions(void) // Returns the most recently updated list of particle positions
		{
		/* Check if there is a new positions list: */
		if(positions.lockNewValue())
			++positionVersion;
		
		/* Return the locked positions list: */
		return positions.getLockedValue();
		}
	
	/* Constructors and destructors: */
	public:
	CollaborativeNetworkViewer(int& argc,char**& argv);
	virtual ~CollaborativeNetworkViewer(void);
	
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
