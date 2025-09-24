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

#include "CollaborativeNetworkViewer.h"

#include <Misc/FunctionCalls.h>
#include <Misc/MessageLogger.h>
#include <GL/gl.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/ONTransformNode.h>
#include <SceneGraph/OGTransformNode.h>
#include <SceneGraph/ShapeNode.h>
#include <Vrui/SceneGraphManager.h>

#include "Network.h"
#include "CreateNodeLabel.h"
#include "NetworkViewerClientTool.h"
#include "NetworkViewerClientSelectTool.h"
#include "NetworkViewerClientDeselectTool.h"
#include "NetworkViewerClientToggleSelectTool.h"
#include "NetworkViewerClientShowLabelTool.h"
#include "NetworkViewerClientDragTool.h"

/*****************************************************
Methods of class CollaborativeNetworkViewer::DataItem:
*****************************************************/

CollaborativeNetworkViewer::DataItem::DataItem(void)
	:vertexBuffer(0),indexBuffer(0),
	 networkVersion(0),vertexVersion(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBVertexBufferObject::initExtension();
	
	/* Create buffer objects: */
	glGenBuffersARB(1,&vertexBuffer);
	glGenBuffersARB(1,&indexBuffer);
	}

CollaborativeNetworkViewer::DataItem::~DataItem(void)
	{
	/* Destroy the buffer objects: */
	glDeleteBuffersARB(1,&vertexBuffer);
	glDeleteBuffersARB(1,&indexBuffer);
	}

/*******************************************
Methods of class CollaborativeNetworkViewer:
*******************************************/

void CollaborativeNetworkViewer::shutdownClient(void)
	{
	/* Call the base class method: */
	CollaborativeVruiApplication::shutdownClient();
	
	/* Delete the client object: */
	nvClient=0;
	}

void CollaborativeNetworkViewer::objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest)
	{
	/* Snap against all particles: */
	const NVPointList& ps=positions.getLockedValue();
	for(NVPointList::const_iterator pIt=ps.begin();pIt!=ps.end();++pIt)
		snapRequest.snapPoint(Vrui::Point(*pIt));
	}

void CollaborativeNetworkViewer::loadNetworkFileCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Load the selected file: */
	nvClient->loadNetwork(cbData->getSelectedPath().c_str());
	}

void CollaborativeNetworkViewer::changeSelectionCallback(Misc::CallbackData* cbData,const unsigned int& command)
	{
	nvClient->changeSelection(command);
	}

void CollaborativeNetworkViewer::showSimulationParametersDialogCallback(Misc::CallbackData* cbData)
	{
	/* Pop up the simulation parameters dialog: */
	Vrui::popupPrimaryWidget(simulationParametersDialog);
	}

void CollaborativeNetworkViewer::showRenderingDialogCallback(Misc::CallbackData* cbData)
	{
	/* Pop up the rendering dialog: */
	Vrui::popupPrimaryWidget(renderingDialog);
	}

GLMotif::PopupMenu* CollaborativeNetworkViewer::createSelectionMenu(void)
	{
	GLMotif::PopupMenu* selectionMenu=new GLMotif::PopupMenu("SelectionMenu",Vrui::getWidgetManager());
	
	GLMotif::Button* clearButton=selectionMenu->addEntry("Clear Selection");
	clearButton->getSelectCallbacks().add(this,&CollaborativeNetworkViewer::changeSelectionCallback,0U);
	
	GLMotif::Button* growButton=selectionMenu->addEntry("Grow Selection");
	growButton->getSelectCallbacks().add(this,&CollaborativeNetworkViewer::changeSelectionCallback,1U);
	
	GLMotif::Button* shrinkButton=selectionMenu->addEntry("Shrink Selection");
	shrinkButton->getSelectCallbacks().add(this,&CollaborativeNetworkViewer::changeSelectionCallback,2U);
	
	selectionMenu->manageMenu();
	return selectionMenu;
	}

GLMotif::PopupMenu* CollaborativeNetworkViewer::createMainMenu(void)
	{
	GLMotif::PopupMenu* mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Network Viewer");
	
	/* Create a button to load a network file: */
	GLMotif::Button* loadNetworkFileButton=new GLMotif::Button("LoadNetworkFileButton",mainMenu,"Load Network File...");
	loadNetworkFileHelper.addLoadCallback(loadNetworkFileButton,Misc::createFunctionCall(this,&CollaborativeNetworkViewer::loadNetworkFileCallback));
	
	/* Create a sub-menu to manipulate the selection: */
	GLMotif::CascadeButton* selectionCascade=new GLMotif::CascadeButton("SelectionCascade",mainMenu,"Selection");
	selectionCascade->setPopup(createSelectionMenu());
	
	#if 0
	/* Create a sub-menu to select node color mappings: */
	GLMotif::CascadeButton* colorMappingCascade=new GLMotif::CascadeButton("ColorMappingCascade",mainMenu,"Color Mapping");
	colorMappingCascade->setPopup(createColorMappingMenu());
	#endif
	
	/* Create a button to show the simulation parameters dialog: */
	GLMotif::Button* showParametersDialogButton=new GLMotif::Button("ShowParametersDialogButton",mainMenu,"Show Simulation Parameters");
	showParametersDialogButton->getSelectCallbacks().add(this,&CollaborativeNetworkViewer::showSimulationParametersDialogCallback);
	
	/* Create a button to show the rendering dialog: */
	GLMotif::Button* showRenderingDialogButton=new GLMotif::Button("ShowRenderingDialogButton",mainMenu,"Show Rendering Settings");
	showRenderingDialogButton->getSelectCallbacks().add(this,&CollaborativeNetworkViewer::showRenderingDialogCallback);
	
	mainMenu->manageMenu();
	return mainMenu;
	}

void CollaborativeNetworkViewer::updateNetwork(Network* newNetwork)
	{
	/* Mark cached network state as outdated : */
	++networkVersion;
	}

void CollaborativeNetworkViewer::updateSimulationParameters(const SimulationParameters& newSimulationParameters)
	{
	simulationParameters=newSimulationParameters;
	
	/* Update the UI: */
	simulationParametersDialog->updateVariables();
	}

void CollaborativeNetworkViewer::clearNodeLabels(void)
	{
	/* Remove all current node labels: */
	for(NodeLabelMap::Iterator nlIt=nodeLabels.begin();!nlIt.isFinished();++nlIt)
		{
		/* Remove the node label scene graph from Vrui's scene graph: */
		Vrui::getSceneGraphManager()->removeNavigationalNode(*nlIt->getDest());
		}
	
	/* Clear the node label set: */
	nodeLabels.clear();
	}

void CollaborativeNetworkViewer::showNodeLabel(unsigned int nodeIndex)
	{
	/* Check if the node is not yet labeled: */
	if(!nodeLabels.isEntry(nodeIndex))
		{
		/* Access the given node's properties: */
		const JsonMap::Map& nodeProperties=nvClient->getNetwork().getNodeProperties(nodeIndex);
		
		/* Retrieve the node's number of outgoing links: */
		size_t numLinks=nvClient->getNetwork().getNodes()[nodeIndex].getLinkedNodes().size();
		
		/* Create a label for the node's properties: */
		SceneGraph::OGTransformNodePointer labelRoot=createNodeLabel(nodeProperties,numLinks,*labelFontStyle);
		
		/* Show the node label and add it to the label set: */
		Vrui::getSceneGraphManager()->addNavigationalNode(*labelRoot);
		nodeLabels.setEntry(NodeLabelMap::Entry(nodeIndex,labelRoot));
		}
	}

void CollaborativeNetworkViewer::hideNodeLabel(unsigned int nodeIndex)
	{
	/* Check if the node is already labeled: */
	NodeLabelMap::Iterator nlIt=nodeLabels.findEntry(nodeIndex);
	if(!nlIt.isFinished())
		{
		/* Hide and delete the node's label: */
		Vrui::getSceneGraphManager()->removeNavigationalNode(*nlIt->getDest());
		nodeLabels.removeEntry(nlIt);
		}
	}

void CollaborativeNetworkViewer::updateRenderingParameters(const RenderingParameters& newRenderingParameters)
	{
	renderingParameters=newRenderingParameters;
	
	/* Update the UI: */
	renderingDialog->updateVariables();
	}

void CollaborativeNetworkViewer::simulationParametersChangedCallback(Misc::CallbackData* cbData)
	{
	/* Notify a potential Network Viewer client that simulation parameters have changed: */
	if(nvClient!=0)
		nvClient->updateSimulationParameters(simulationParameters);
	}

GLMotif::PopupWindow* CollaborativeNetworkViewer::createSimulationParametersDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getUiStyleSheet();
	
	GLMotif::PopupWindow* simulationParametersDialog=new GLMotif::PopupWindow("ParametersDialog",Vrui::getWidgetManager(),"Simulation Parameters");
	simulationParametersDialog->setHideButton(true);
	simulationParametersDialog->setCloseButton(true);
	simulationParametersDialog->popDownOnClose();
	simulationParametersDialog->setResizableFlags(true,false);
	
	GLMotif::RowColumn* parameters=new GLMotif::RowColumn("Parameters",simulationParametersDialog,false);
	parameters->setOrientation(GLMotif::RowColumn::VERTICAL);
	parameters->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	parameters->setNumMinorWidgets(2);
	
	new GLMotif::Label("AttenuationLabel",parameters,"Attenuation");
	
	GLMotif::TextFieldSlider* attenuationSlider=new GLMotif::TextFieldSlider("AttenuationSlider",parameters,8,ss.fontHeight*10.0f);
	attenuationSlider->setSliderMapping(GLMotif::TextFieldSlider::GAMMA);
	attenuationSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	attenuationSlider->getTextField()->setFieldWidth(7);
	attenuationSlider->getTextField()->setPrecision(5);
	attenuationSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	attenuationSlider->setValueRange(0.0,1.0,0.001);
	attenuationSlider->setGammaExponent(0.5,0.9);
	attenuationSlider->track(simulationParameters.attenuation);
	attenuationSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::simulationParametersChangedCallback);
	
	new GLMotif::Label("CentralForceLabel",parameters,"Central Force Strength");
	
	GLMotif::TextFieldSlider* centralForceSlider=new GLMotif::TextFieldSlider("CentralForceSlider",parameters,8,ss.fontHeight*10.0f);
	centralForceSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	centralForceSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	centralForceSlider->getTextField()->setPrecision(2);
	centralForceSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	centralForceSlider->setValueRange(0.0,50.0,0.01);
	centralForceSlider->track(simulationParameters.centralForce);
	centralForceSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::simulationParametersChangedCallback);
	
	new GLMotif::Label("RepellingForceModeLabel",parameters,"Repelling Force Mode");
	
	GLMotif::DropdownBox* repellingForceModeBox=new GLMotif::DropdownBox("RepellingForceModeBox",parameters);
	repellingForceModeBox->addItem("Linear");
	repellingForceModeBox->addItem("Quadratic");
	repellingForceModeBox->track(simulationParameters.repellingForceMode);
	repellingForceModeBox->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::simulationParametersChangedCallback);
	
	new GLMotif::Label("RepellingForceLabel",parameters,"Repelling Force Strength");
	
	GLMotif::TextFieldSlider* repellingForceSlider=new GLMotif::TextFieldSlider("RepellingForceSlider",parameters,8,ss.fontHeight*10.0f);
	repellingForceSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	repellingForceSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	repellingForceSlider->getTextField()->setPrecision(2);
	repellingForceSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	repellingForceSlider->setValueRange(0.0,50.0,0.01);
	repellingForceSlider->track(simulationParameters.repellingForce);
	repellingForceSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::simulationParametersChangedCallback);
	
	new GLMotif::Label("RepellingForceThetaLabel",parameters,"Repelling Force Theta");
	
	GLMotif::TextFieldSlider* repellingForceThetaSlider=new GLMotif::TextFieldSlider("RepellingForceThetaSlider",parameters,8,ss.fontHeight*10.0f);
	repellingForceThetaSlider->setSliderMapping(GLMotif::TextFieldSlider::GAMMA);
	repellingForceThetaSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	repellingForceThetaSlider->getTextField()->setFieldWidth(7);
	repellingForceThetaSlider->getTextField()->setPrecision(5);
	repellingForceThetaSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	repellingForceThetaSlider->setValueRange(0.0,1.0,0.001);
	repellingForceThetaSlider->setGammaExponent(0.5,0.25);
	repellingForceThetaSlider->track(simulationParameters.repellingForceTheta);
	repellingForceThetaSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::simulationParametersChangedCallback);
	
	new GLMotif::Label("RepellingForceCutoffLabel",parameters,"Repelling Force Cutoff");
	
	GLMotif::TextFieldSlider* repellingForceCutoffSlider=new GLMotif::TextFieldSlider("RepellingForceCutoffSlider",parameters,8,ss.fontHeight*10.0f);
	repellingForceCutoffSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	repellingForceCutoffSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	repellingForceCutoffSlider->getTextField()->setPrecision(3);
	repellingForceCutoffSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	repellingForceCutoffSlider->setValueRange(0.0,1.0,0.001);
	repellingForceCutoffSlider->track(simulationParameters.repellingForceCutoff);
	repellingForceCutoffSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::simulationParametersChangedCallback);
	
	new GLMotif::Label("LinkStrengthLabel",parameters,"Link Strength");
	
	GLMotif::TextFieldSlider* linkStrengthSlider=new GLMotif::TextFieldSlider("LinkStrengthSlider",parameters,8,ss.fontHeight*10.0f);
	linkStrengthSlider->setSliderMapping(GLMotif::TextFieldSlider::GAMMA);
	linkStrengthSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	linkStrengthSlider->getTextField()->setFieldWidth(7);
	linkStrengthSlider->getTextField()->setPrecision(5);
	linkStrengthSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	linkStrengthSlider->setValueRange(0.0,1.0,0.001);
	linkStrengthSlider->setGammaExponent(0.5,0.1);
	linkStrengthSlider->track(simulationParameters.linkStrength);
	linkStrengthSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::simulationParametersChangedCallback);
	
	parameters->manageChild();
	
	return simulationParametersDialog;
	}

void CollaborativeNetworkViewer::renderingParametersChangedCallback(Misc::CallbackData* cbData)
	{
	/* Notify a potential Network Viewer client that rendering parameters have changed: */
	if(nvClient!=0)
		nvClient->updateRenderingParameters(renderingParameters);
	}

GLMotif::PopupWindow* CollaborativeNetworkViewer::createRenderingDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getUiStyleSheet();
	
	GLMotif::PopupWindow* renderingDialog=new GLMotif::PopupWindow("RenderingDialog",Vrui::getWidgetManager(),"Rendering Settings");
	renderingDialog->setHideButton(true);
	renderingDialog->setCloseButton(true);
	renderingDialog->popDownOnClose();
	renderingDialog->setResizableFlags(true,false);
	
	GLMotif::RowColumn* rendering=new GLMotif::RowColumn("Rendering",renderingDialog,false);
	rendering->setOrientation(GLMotif::RowColumn::VERTICAL);
	rendering->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	rendering->setNumMinorWidgets(2);
	
	new GLMotif::Label("NodeRadiusLabel",rendering,"Node Radius");
	
	GLMotif::TextFieldSlider* nodeRadiusSlider=new GLMotif::TextFieldSlider("NodeRadiusSlider",rendering,8,ss.fontHeight*10.0f);
	nodeRadiusSlider->setSliderMapping(GLMotif::TextFieldSlider::EXP10);
	nodeRadiusSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	nodeRadiusSlider->getTextField()->setFieldWidth(7);
	nodeRadiusSlider->getTextField()->setPrecision(5);
	nodeRadiusSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	nodeRadiusSlider->setValueRange(0.01,100.0,0.001);
	nodeRadiusSlider->track(renderingParameters.nodeRadius);
	nodeRadiusSlider->getSlider()->addNotch(Math::log10(renderingParameters.nodeRadius));
	nodeRadiusSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::renderingParametersChangedCallback);
	
	#if 0
	
	new GLMotif::Blind("Blind1",rendering);
	
	GLMotif::ToggleButton* useNodeSizeToggle=new GLMotif::ToggleButton("UseNodeSizeToggle",rendering,"Use Node Size");
	useNodeSizeToggle->setBorderWidth(0.0f);
	useNodeSizeToggle->setHAlignment(GLFont::HAlignment::Left);
	useNodeSizeToggle->track(useNodeSize);
	
	#endif
	
	new GLMotif::Label("NodeRadiusLabel",rendering,"Node Size Exponent");
	
	GLMotif::TextFieldSlider* nodeSizeExponentSlider=new GLMotif::TextFieldSlider("NodeSizeExponentSlider",rendering,8,ss.fontHeight*10.0f);
	nodeSizeExponentSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	nodeSizeExponentSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	nodeSizeExponentSlider->getTextField()->setFieldWidth(7);
	nodeSizeExponentSlider->getTextField()->setPrecision(5);
	nodeSizeExponentSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	nodeSizeExponentSlider->setValueRange(0.0,1.0,0.001);
	nodeSizeExponentSlider->track(renderingParameters.nodeSizeExponent);
	nodeSizeExponentSlider->getSlider()->addNotch(1.0/3.0);
	nodeSizeExponentSlider->getSlider()->addNotch(1.0/2.0);
	nodeSizeExponentSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::renderingParametersChangedCallback);
	
	new GLMotif::Label("LinkLineWidthLabel",rendering,"Link Line Width");
	
	GLMotif::TextFieldSlider* linkLineWidthSlider=new GLMotif::TextFieldSlider("LinkLineWidthSlider",rendering,8,ss.fontHeight*10.0f);
	linkLineWidthSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	linkLineWidthSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	linkLineWidthSlider->getTextField()->setFieldWidth(7);
	linkLineWidthSlider->getTextField()->setPrecision(5);
	linkLineWidthSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	linkLineWidthSlider->setValueRange(0.5,11.0,0.5);
	linkLineWidthSlider->track(renderingParameters.linkLineWidth);
	linkLineWidthSlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::renderingParametersChangedCallback);
	
	new GLMotif::Label("LinkOpacityLabel",rendering,"Link Opacity");
	
	GLMotif::TextFieldSlider* linkOpacitySlider=new GLMotif::TextFieldSlider("LinkOpacitySlider",rendering,8,ss.fontHeight*10.0f);
	linkOpacitySlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	linkOpacitySlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	linkOpacitySlider->getTextField()->setFieldWidth(7);
	linkOpacitySlider->getTextField()->setPrecision(5);
	linkOpacitySlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	linkOpacitySlider->setValueRange(0.0,1.0,0.001);
	linkOpacitySlider->track(renderingParameters.linkOpacity);
	linkOpacitySlider->getValueChangedCallbacks().add(this,&CollaborativeNetworkViewer::renderingParametersChangedCallback);
	
	rendering->manageChild();
	
	return renderingDialog;
	}

CollaborativeNetworkViewer::CollaborativeNetworkViewer(int& argc,char**& argv)
	:CollaborativeVruiApplication(argc,argv),
	 nvClient(0),
	 loadNetworkFileHelper(Vrui::getWidgetManager(),"NetworkFile.json",".json"),
	 mainMenu(0),simulationParametersDialog(0),renderingDialog(0),
	 startupNetworkFileName(0),networkVersion(1),networkPositionVersion(0),positionVersion(0),
	 nodeLabels(5)
	{
	/* Parse the command line: */
	for(int argi=1;argi<argc;++argi)
		{
		if(startupNetworkFileName==0)
			startupNetworkFileName=argv[argi];
		else
			Misc::formattedUserWarning("CollaborativeNetworkViewer: Ignoring command line argument %s",argv[argi]);
		}
	
	/* Register the network viewer client: */
	nvClient=new Collab::Plugins::NetworkViewerClient(this,&client);
	client.addPluginProtocol(nvClient);
	
	/* Start the collaboration back end: */
	startClient();
	
	/* Register a callback with the object snapper tool class: */
	Vrui::ObjectSnapperTool::addSnapCallback(Misc::createFunctionCall(this,&CollaborativeNetworkViewer::objectSnapCallback));
	
	/* Create the UI: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	simulationParametersDialog=createSimulationParametersDialog();
	renderingDialog=createRenderingDialog();
	
	/* Create the custom tool classes: */
	Tool::initClass();
	SelectTool::initClass();
	DeselectTool::initClass();
	ToggleSelectTool::initClass();
	ShowLabelTool::initClass();
	DragTool::initClass();
	
	/* Create a font style for node labels: */
	labelFontStyle=new SceneGraph::FancyFontStyleNode;
	labelFontStyle->family.setValue("SANS");
	labelFontStyle->style.setValue("PLAIN");
	labelFontStyle->size.setValue(Vrui::getUiStyleSheet()->fontHeight);
	labelFontStyle->spacing.setValue(1);
	labelFontStyle->justify.appendValue("BEGIN");
	labelFontStyle->precision.setValue(1);
	labelFontStyle->update();
	
	#if CONFIG_USE_IMPOSTORSPHERES
	
	/* Initialize the node renderer: */
	if(renderingParameters.useNodeSize)
		nodeRenderer.setVariableRadius();
	else
		nodeRenderer.setFixedRadius(renderingParameters.nodeRadius);
	nodeRenderer.setColorMaterial(true);
	
	#endif
	}

CollaborativeNetworkViewer::~CollaborativeNetworkViewer(void)
	{
	delete mainMenu;
	delete simulationParametersDialog;
	delete renderingDialog;
	}

void CollaborativeNetworkViewer::frame(void)
	{
	/* Call the base class method: */
	CollaborativeVruiApplication::frame();
	
	/* Lock the most recent network node positions: */
	const NVPointList& positions=lockAndGetPositions();
	
	if(nvClient!=0&&networkVersion==networkPositionVersion)
		{
		/* Get a scale factor to scale the labels back to their physical-space sizes: */
		SceneGraph::OGTransformNode::OGTransform::Scalar scaling(Vrui::getInverseNavigationTransformation().getScaling());
		
		/* Update the positions of all active node labels: */
		for(NodeLabelMap::Iterator nlIt=nodeLabels.begin();!nlIt.isFinished();++nlIt)
			{
			/* Update the label root node's transformation: */
			SceneGraph::OGTransformNode::OGTransform::Vector translation(positions[nlIt->getSource()]-NVPoint::origin);
			SceneGraph::OGTransformNode::OGTransform::Rotation rotation;
			nlIt->getDest()->setTransform(SceneGraph::OGTransformNode::OGTransform(translation,rotation,scaling));
			
			/* Update the label's label transform to place the point of the label bubble onto the node's sphere: */
			SceneGraph::ONTransformNodePointer labelTransform(SceneGraph::GroupNodePointer(nlIt->getDest()->getChildren()[0])->getChildren()[0]);
			SceneGraph::ShapeNodePointer bubbleShape=(labelTransform->getChildren()[1]);
			SceneGraph::Box bbox=bubbleShape->calcBoundingBox();
			SceneGraph::Point bubblePoint(Math::mid(bbox.min[0],bbox.max[0]),bbox.min[1],Math::mid(bbox.min[2],bbox.max[2]));
			bubblePoint[1]-=getNodeRadius(nlIt->getSource())/scaling;
			labelTransform->setTransform(SceneGraph::ONTransformNode::ONTransform::translateToOriginFrom(bubblePoint));
			}
		}
	}

void CollaborativeNetworkViewer::display(GLContextData& contextData) const
	{
	/* Call the base class method: */
	CollaborativeVruiApplication::display(contextData);
	
	/* Check if the network and node positions are in synch: */
	if(nvClient!=0&&networkVersion==networkPositionVersion)
		{
		/* Retrieve the context data item: */
		DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
		
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT);
		#if CONFIG_USE_IMPOSTORSPHERES
		glEnable(GL_LIGHTING);
		
		/* Set material properties: */
		glMaterialSpecular(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.5f,0.5f,0.5f));
		glMaterialShininess(GLMaterialEnums::FRONT,64.0f);
		#else
		glDisable(GL_LIGHTING);
		glPointSize(3.0f);
		#endif
		
		/* Bind the vertex and index buffers: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBuffer);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBuffer);
		
		/* Check if the network version changed: */
		const NVPointList& points=positions.getLockedValue();
		if(dataItem->networkVersion!=networkVersion)
			{
			/* Resize the vertex buffer: */
			glBufferDataARB(GL_ARRAY_BUFFER_ARB,points.size()*sizeof(DataItem::Vertex),0,GL_DYNAMIC_DRAW_ARB);
			
			/* Update the index buffer to render node links: */
			const Network::LinkList& links=nvClient->network->getLinks();
			glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,links.size()*2*sizeof(GLuint),0,GL_STATIC_DRAW_ARB);
			GLuint* iPtr=static_cast<GLuint*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
			for(Network::LinkList::const_iterator lIt=links.begin();lIt!=links.end();++lIt,iPtr+=2)
				for(int i=0;i<2;++i)
					iPtr[i]=lIt->getNode(i)->getParticleIndex();
			glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
			
			/* Mark the network as up-to-date: */
			dataItem->networkVersion=networkVersion;
			}
		
		if(dataItem->vertexVersion!=positionVersion)
			{
			/* Upload the newest node positions, sizes, and colors: */
			GLubyte opacity=renderingParameters.linkOpacity<Scalar(1)?GLubyte(Math::floor(renderingParameters.linkOpacity*Scalar(256))):GLubyte(255);
			Network::ColorList::const_iterator ncIt=nvClient->network->getNodeColors().begin();
			DataItem::Vertex* vPtr=static_cast<DataItem::Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
			#if CONFIG_USE_IMPOSTORSPHERES
			if(renderingParameters.useNodeSize)
				{
				/* Upload nodes as variable-radius spheres: */
				Network::NodeList::const_iterator nIt=nvClient->network->getNodes().begin();
				for(NVPointList::const_iterator pIt=points.begin();pIt!=points.end();++pIt,++nIt,++ncIt,++vPtr)
					{
					for(int i=0;i<4;++i)
						vPtr->color[i]=(*ncIt)[i];
					vPtr->color[3]=opacity;
					for(int i=0;i<3;++i)
						vPtr->position[i]=(*pIt)[i];
					vPtr->position[3]=GLfloat(getNodeRadius(*nIt));
					}
				}
			else
			#endif
				{
				/* Upload nodes as fixed-radius spheres: */
				for(NVPointList::const_iterator pIt=points.begin();pIt!=points.end();++pIt,++ncIt,++vPtr)
					{
					for(int i=0;i<4;++i)
						vPtr->color[i]=(*ncIt)[i];
					vPtr->color[3]=opacity;
					for(int i=0;i<3;++i)
						vPtr->position[i]=(*pIt)[i];
					vPtr->position[3]=1.0f;
					}
				}
			glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			
			/* Mark the node positions as up-to-date: */
			dataItem->vertexVersion=positionVersion;
			}
		
		/* Enable vertex arrays: */
		GLVertexArrayParts::enable(DataItem::Vertex::getPartsMask());
		
		/* Draw all nodes as spheres: */
		#if CONFIG_USE_IMPOSTORSPHERES
		nodeRenderer.enable(GLfloat(Vrui::getNavigationTransformation().getScaling()),contextData);
		#endif
		glVertexPointer(static_cast<const DataItem::Vertex*>(0));
		glDrawArrays(GL_POINTS,0,points.size());
		#if CONFIG_USE_IMPOSTORSPHERES
		nodeRenderer.disable(contextData);
		#endif
		
		/* Disable vertex arrays: */
		GLVertexArrayParts::disable(DataItem::Vertex::getPartsMask());
		
		/* Unbind the vertex and index buffers: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
		
		glPopAttrib();
		}
	}

void CollaborativeNetworkViewer::resetNavigation(void)
	{
	if(nvClient!=0&&networkVersion==networkPositionVersion)
		{
		/* Get the most recent network node positions: */
		const NVPointList& points=lockAndGetPositions();
		
		/* Calculate the network graph's bounding box: */
		Geometry::Box<NVScalar,3> bbox=Geometry::Box<NVScalar,3>::empty;
		for(NVPointList::const_iterator pIt=points.begin();pIt!=points.end();++pIt)
			bbox.addPoint(*pIt);
		
		/* Center and scale the network graph: */
		Vrui::Point center(Geometry::mid(bbox.min,bbox.max));
		Vrui::Scalar size(Geometry::dist(bbox.min,bbox.max));
		Vrui::setNavigationTransformation(center,size);
		}
	}

void CollaborativeNetworkViewer::glRenderActionTransparent(GLContextData& contextData) const
	{
	/* Check if the network and node positions are in synch: */
	if(nvClient!=0&&networkVersion==networkPositionVersion)
		{
		/* Retrieve the context data item: */
		DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
		
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(renderingParameters.linkLineWidth);
		
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		// glColor3f(0.05f,0.05f,0.05f);
		glColor4f(1.0f,1.0f,1.0f,0.1f);
		
		/* Go to navigational coordinates: */
		Vrui::goToNavigationalSpace(contextData);
		
		/* Bind the vertex and index buffers: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBuffer);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBuffer);
		
		/* Enable vertex arrays: */
		GLVertexArrayParts::enable(DataItem::Vertex::getPartsMask());
		
		/* Use only the first three components of the vertex position: */
		glInterleavedArrays(GL_C4UB_V3F,sizeof(DataItem::Vertex),0);
		// glVertexPointer(static_cast<const DataItem::Vertex*>(0));
		
		/* Draw all links as lines: */
		glDrawElements(GL_LINES,nvClient->network->getLinks().size()*2,GL_UNSIGNED_INT,0);
		
		/* Disable vertex arrays: */
		GLVertexArrayParts::disable(DataItem::Vertex::getPartsMask());
		
		/* Unbind the vertex and index buffers: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
		
		/* Return to physical coordinates: */
		glPopMatrix();
		
		// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		
		glPopAttrib();
		}
	}

void CollaborativeNetworkViewer::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	
	/* Associate the data item with this object in the OpenGL context: */
	contextData.addDataItem(this,dataItem);
	}

/****************
Main entry point:
****************/

VRUI_APPLICATION_RUN(CollaborativeNetworkViewer)
