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

#include "NetworkViewer.h"

#include <string.h>
#include <deque>
#include <Misc/FunctionCalls.h>
#include <Realtime/Time.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Box.h>
#include <Geometry/Random.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLModels.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/CascadeButton.h>
#include <Vrui/Vrui.h>
#include <Vrui/DisplayState.h>

#include "Network.h"
#include "ParticleSystem.icpp"
#include "JsonEntity.h"
#include "JsonBoolean.h"
#include "JsonNumber.h"
#include "JsonString.h"
#include "JsonMap.h"
#include "NetworkViewerTool.h"
#include "SelectAndDragTool.h"
#include "AddSelectTool.h"
#include "SubtractSelectTool.h"
#include "ShowPropertiesTool.h"

#define TIMING 1
#if TIMING
#include <iostream>
#include <Realtime/Time.h>
#endif

/*
To do:

- Brushing selection tool that adds or subtracts

- Draw only selected nodes, or only links between selected nodes

*/

/****************************************
Methods of class NetworkViewer::DataItem:
****************************************/

NetworkViewer::DataItem::DataItem(void)
	:sphereDisplayList(glGenLists(1))
	{
	}

NetworkViewer::DataItem::~DataItem(void)
	{
	glDeleteLists(sphereDisplayList,1);
	}

/******************************
Methods of class NetworkViewer:
******************************/

GLMotif::Widget* NetworkViewer::createNodeCallout(unsigned int nodeIndex)
	{
	GLMotif::PopupWindow* calloutPopup=new GLMotif::PopupWindow("CalloutPopup",Vrui::getWidgetManager(),"Node Properties");
	
	/* Create a callout container: */
	GLMotif::RowColumn* callout=new GLMotif::RowColumn("Callout",calloutPopup,false);
	callout->setOrientation(GLMotif::RowColumn::VERTICAL);
	callout->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	callout->setNumMinorWidgets(2);
	
	/* Create pairs of name / value labels for the given node's properties: */
	const JsonMap::Map& nodeProperties=network->getNodeProperties(nodeIndex);
	unsigned int propertyIndex=0;
	for(JsonMap::Map::ConstIterator npIt=nodeProperties.begin();!npIt.isFinished();++npIt,++propertyIndex)
		{
		/* Create a label with the property's name: */
		char labelName[16];
		memcpy(labelName,"PropName",8);
		unsigned int pi=propertyIndex;
		labelName[15]='\0';
		for(int pos=14;pos>=8;--pos,pi/=10)
			labelName[pos]=pi%10+'0';
		new GLMotif::Label(labelName,callout,npIt->getSource().c_str());
		
		/* Create a label with the property's value: */
		memcpy(labelName,"PropValue",9);
		pi=propertyIndex;
		labelName[15]='\0';
		for(int pos=14;pos>=9;--pos,pi/=10)
			labelName[pos]=pi%10+'0';
		switch(npIt->getDest()->getType())
			{
			case JsonEntity::BOOLEAN:
				new GLMotif::Label(labelName,callout,getBoolean(npIt->getDest())?"true":"false");
				break;
			
			case JsonEntity::NUMBER:
				{
				char number[64];
				snprintf(number,sizeof(number),"%f",getNumber(npIt->getDest()));
				new GLMotif::Label(labelName,callout,number);
				break;
				}
			
			case JsonEntity::STRING:
				new GLMotif::Label(labelName,callout,getString(npIt->getDest()).c_str());
				break;
			
			default:
				new GLMotif::Label(labelName,callout,"(unsupported type)");
			}
		}
	
	callout->manageChild();
	
	return calloutPopup;
	}

void NetworkViewer::showNodeProperties(unsigned int nodeIndex)
	{
	/* Delete the current callout: */
	delete nodeCallout;
	
	/* Create a new callout for the given node: */
	calloutNodeIndex=nodeIndex;
	nodeCallout=createNodeCallout(calloutNodeIndex);
	Vrui::popupPrimaryWidget(nodeCallout);
	}

bool NetworkViewer::lockSelection(void)
	{
	/* Return success if the selection is not locked: */
	bool result=!selectionLocked;
	
	/* Lock the selection: */
	selectionLocked=true;
	
	return result;
	}

void NetworkViewer::unlockSelection(void)
	{
	/* Unlock the selection (assumes that caller actually held the lock): */
	selectionLocked=false;
	}

void NetworkViewer::objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest)
	{
	/* Snap against all particles: */
	for(Index index=0;index<particles.getNumParticles();++index)
		snapRequest.snapPoint(Vrui::Point(particles.getParticlePosition(index)));
	}

void NetworkViewer::attenuationValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	particles.setAttenuation(Scalar(cbData->value));
	}

void NetworkViewer::repellingForceModeValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	switch(cbData->newSelectedItem)
		{
		case 0:
			repellingForceMode=Linear;
			break;
		
		case 1:
			repellingForceMode=Quadratic;
			break;
		}
	}

void NetworkViewer::linkStrengthValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	linkStrength=Scalar(cbData->value);
	
	/* Set the overall distance constraint scale in the particle system: */
	particles.setDistConstraintScale(linkStrength);
	}

GLMotif::PopupWindow* NetworkViewer::createParametersDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getUiStyleSheet();
	
	GLMotif::PopupWindow* parametersDialog=new GLMotif::PopupWindow("ParametersDialog",Vrui::getWidgetManager(),"Simulation Parameters");
	parametersDialog->setHideButton(true);
	parametersDialog->setCloseButton(true);
	parametersDialog->popDownOnClose();
	parametersDialog->setResizableFlags(true,false);
	
	GLMotif::RowColumn* parameters=new GLMotif::RowColumn("Parameters",parametersDialog,false);
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
	attenuationSlider->setValue(particles.getAttenuation());
	attenuationSlider->getValueChangedCallbacks().add(this,&NetworkViewer::attenuationValueChangedCallback);
	
	new GLMotif::Label("CentralForceLabel",parameters,"Central Force Strength");
	
	GLMotif::TextFieldSlider* centralForceSlider=new GLMotif::TextFieldSlider("CentralForceSlider",parameters,8,ss.fontHeight*10.0f);
	centralForceSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	centralForceSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	centralForceSlider->getTextField()->setPrecision(2);
	centralForceSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	centralForceSlider->setValueRange(0.0,50.0,0.01);
	centralForceSlider->track(centralForce);
	
	new GLMotif::Label("RepellingForceModeLabel",parameters,"Repelling Force Mode");
	
	GLMotif::DropdownBox* repellingForceModeBox=new GLMotif::DropdownBox("RepellingForceModeBox",parameters);
	repellingForceModeBox->addItem("Linear");
	repellingForceModeBox->addItem("Quadratic");
	switch(repellingForceMode)
		{
		case Linear:
			repellingForceModeBox->setSelectedItem(0);
			break;
		
		case Quadratic:
			repellingForceModeBox->setSelectedItem(1);
			break;
		}
	repellingForceModeBox->getValueChangedCallbacks().add(this,&NetworkViewer::repellingForceModeValueChangedCallback);
	
	new GLMotif::Label("RepellingForceLabel",parameters,"Repelling Force Strength");
	
	GLMotif::TextFieldSlider* repellingForceSlider=new GLMotif::TextFieldSlider("RepellingForceSlider",parameters,8,ss.fontHeight*10.0f);
	repellingForceSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	repellingForceSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	repellingForceSlider->getTextField()->setPrecision(2);
	repellingForceSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	repellingForceSlider->setValueRange(0.0,50.0,0.01);
	repellingForceSlider->track(repellingForce);
	
	new GLMotif::Label("RepellingForceThetaLabel",parameters,"Repelling Force Theta");
	
	GLMotif::TextFieldSlider* repellingForceThetaSlider=new GLMotif::TextFieldSlider("RepellingForceThetaSlider",parameters,8,ss.fontHeight*10.0f);
	repellingForceThetaSlider->setSliderMapping(GLMotif::TextFieldSlider::GAMMA);
	repellingForceThetaSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	repellingForceThetaSlider->getTextField()->setFieldWidth(7);
	repellingForceThetaSlider->getTextField()->setPrecision(5);
	repellingForceThetaSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	repellingForceThetaSlider->setValueRange(0.0,1.0,0.001);
	repellingForceThetaSlider->setGammaExponent(0.5,0.25);
	repellingForceThetaSlider->track(repellingForceTheta);
	
	new GLMotif::Label("RepellingForceCutoffLabel",parameters,"Repelling Force Cutoff");
	
	GLMotif::TextFieldSlider* repellingForceCutoffSlider=new GLMotif::TextFieldSlider("RepellingForceCutoffSlider",parameters,8,ss.fontHeight*10.0f);
	repellingForceCutoffSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	repellingForceCutoffSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	repellingForceCutoffSlider->getTextField()->setPrecision(3);
	repellingForceCutoffSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	repellingForceCutoffSlider->setValueRange(0.0,1.0,0.001);
	repellingForceCutoffSlider->track(repellingForceCutoff);
	
	new GLMotif::Label("LinkStrengthLabel",parameters,"Link Strength");
	
	GLMotif::TextFieldSlider* linkStrengthSlider=new GLMotif::TextFieldSlider("LinkStrengthSlider",parameters,8,ss.fontHeight*10.0f);
	linkStrengthSlider->setSliderMapping(GLMotif::TextFieldSlider::GAMMA);
	linkStrengthSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	linkStrengthSlider->getTextField()->setFieldWidth(7);
	linkStrengthSlider->getTextField()->setPrecision(5);
	linkStrengthSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	linkStrengthSlider->setValueRange(0.0,1.0,0.001);
	linkStrengthSlider->setGammaExponent(0.5,0.1);
	linkStrengthSlider->setValue(linkStrength);
	linkStrengthSlider->getValueChangedCallbacks().add(this,&NetworkViewer::linkStrengthValueChangedCallback);
	
	parameters->manageChild();
	
	return parametersDialog;
	}

GLMotif::PopupWindow* NetworkViewer::createRenderingDialog(void)
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
	nodeRadiusSlider->track(nodeRadius);
	nodeRadiusSlider->getSlider()->addNotch(Math::log10(nodeRadius));
	
	#if 0
	
	new GLMotif::Blind("Blind1",rendering);
	
	GLMotif::ToggleButton* useNodeSizeToggle=new GLMotif::ToggleButton("UseNodeSizeToggle",rendering,"Use Node Size");
	useNodeSizeToggle->setBorderWidth(0.0f);
	useNodeSizeToggle->setHAlignment(GLFont::HAlignment::Left);
	useNodeSizeToggle->track(useNodeSize);
	
	#endif
	
	new GLMotif::Label("NodeSizeExponentLabel",rendering,"Node Size Exponent");
	
	GLMotif::TextFieldSlider* nodeSizeExponentSlider=new GLMotif::TextFieldSlider("NodeSizeExponentSlider",rendering,8,ss.fontHeight*10.0f);
	nodeSizeExponentSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	nodeSizeExponentSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	nodeSizeExponentSlider->getTextField()->setFieldWidth(7);
	nodeSizeExponentSlider->getTextField()->setPrecision(5);
	nodeSizeExponentSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	nodeSizeExponentSlider->setValueRange(0.0,1.0,0.001);
	nodeSizeExponentSlider->track(nodeSizeExponent);
	nodeSizeExponentSlider->getSlider()->addNotch(1.0/3.0);
	nodeSizeExponentSlider->getSlider()->addNotch(1.0/2.0);
	
	rendering->manageChild();
	
	return renderingDialog;
	}

void NetworkViewer::clearSelectionCallback(Misc::CallbackData* cbData)
	{
	/* Try locking the selection: */
	if(lockSelection())
		{
		/* Clear the selection: */
		network->clearSelection();
		unlockSelection();
		}
	}

void NetworkViewer::growSelectionCallback(Misc::CallbackData* cbData)
	{
	/* Try locking the selection: */
	if(lockSelection())
		{
		/* Grow the selection: */
		network->growSelection();
		unlockSelection();
		}
	}

void NetworkViewer::shrinkSelectionCallback(Misc::CallbackData* cbData)
	{
	/* Try locking the selection: */
	if(lockSelection())
		{
		/* Shrink the selection: */
		network->shrinkSelection();
		unlockSelection();
		}
	}

GLMotif::PopupMenu* NetworkViewer::createSelectionMenu(void)
	{
	GLMotif::PopupMenu* selectionMenu=new GLMotif::PopupMenu("SelectionMenu",Vrui::getWidgetManager());
	
	GLMotif::Button* clearButton=selectionMenu->addEntry("Clear Selection");
	clearButton->getSelectCallbacks().add(this,&NetworkViewer::clearSelectionCallback);
	
	GLMotif::Button* growButton=selectionMenu->addEntry("Grow Selection");
	growButton->getSelectCallbacks().add(this,&NetworkViewer::growSelectionCallback);
	
	GLMotif::Button* shrinkButton=selectionMenu->addEntry("Shrink Selection");
	shrinkButton->getSelectCallbacks().add(this,&NetworkViewer::shrinkSelectionCallback);
	
	selectionMenu->manageMenu();
	return selectionMenu;
	}

void NetworkViewer::colorMapPropertySelectedCallback(GLMotif::Button::SelectCallbackData* cbData)
	{
	/* Color nodes by the property whose name is the label of the selected button: */
	network->mapNodeColorsFromNodeProperty(cbData->button->getString(),numericalPropertyValueMap);
	}

GLMotif::PopupMenu* NetworkViewer::createColorMappingMenu(void)
	{
	GLMotif::PopupMenu* colorMappingMenu=new GLMotif::PopupMenu("ColorMappingMenu",Vrui::getWidgetManager());
	
	/* Add buttons for all defined node properties: */
	const Network::StringList& names=network->getNodePropertyNames();
	for(Network::StringList::const_iterator nIt=names.begin();nIt!=names.end();++nIt)
		{
		GLMotif::Button* button=colorMappingMenu->addEntry(nIt->c_str());
		button->getSelectCallbacks().add(this,&NetworkViewer::colorMapPropertySelectedCallback);
		}
	
	colorMappingMenu->manageMenu();
	return colorMappingMenu;
	}

void NetworkViewer::showParametersDialogCallback(Misc::CallbackData* cbData)
	{
	Vrui::popupPrimaryWidget(parametersDialog);
	}

void NetworkViewer::showRenderingDialogCallback(Misc::CallbackData* cbData)
	{
	Vrui::popupPrimaryWidget(renderingDialog);
	}

GLMotif::PopupMenu* NetworkViewer::createMainMenu(void)
	{
	GLMotif::PopupMenu* mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Network Viewer");
	
	/* Create a sub-menu to manipulate the selection: */
	GLMotif::CascadeButton* selectionCascade=new GLMotif::CascadeButton("SelectionCascade",mainMenu,"Selection");
	selectionCascade->setPopup(createSelectionMenu());
	
	/* Create a sub-menu to select node color mappings: */
	GLMotif::CascadeButton* colorMappingCascade=new GLMotif::CascadeButton("ColorMappingCascade",mainMenu,"Color Mapping");
	colorMappingCascade->setPopup(createColorMappingMenu());
	
	/* Create a button to show the simulation properties dialog: */
	GLMotif::Button* showParametersDialogButton=new GLMotif::Button("ShowParametersDialogButton",mainMenu,"Show Simulation Parameters");
	showParametersDialogButton->getSelectCallbacks().add(this,&NetworkViewer::showParametersDialogCallback);
	
	/* Create a button to show the rendering settings dialog: */
	GLMotif::Button* showRenderingDialogButton=new GLMotif::Button("ShowRenderingDialogButton",mainMenu,"Show Rendering Settings");
	showRenderingDialogButton->getSelectCallbacks().add(this,&NetworkViewer::showRenderingDialogCallback);
	
	mainMenu->manageMenu();
	return mainMenu;
	}

NetworkViewer::NetworkViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 network(0),
	 centralForce(5),
	 repellingForceMode(Linear),repellingForce(2),repellingForceTheta(0.25),repellingForceCutoff(0.01),
	 linkStrength(0.01),
	 nodeRadius(0.05),useNodeSize(true),nodeSizeExponent(0.0),
	 selectionLocked(false),
	 parametersDialog(0),renderingDialog(0),mainMenu(0),
	 nodeCallout(0)
	{
	if(argc<2)
		throw std::runtime_error("NetworkViewer: No mineral network file name provided");
	
	/* Load the mineral network json file given on the command line: */
	network=new Network(*IO::openFile(argv[1]));
	
	/* Initialize the particle system: */
	particles.setGravity(Vector::zero);
	particles.setAttenuation(Scalar(0.1));
	particles.setDistConstraintScale(linkStrength);
	particles.setNumRelaxationIterations(20);
	network->createParticles(particles,Scalar(1));
	particles.finishUpdate();
	
	/* Initialize the tool classes: */
	Tool::initClass();
	SelectAndDragTool::initClass();
	AddSelectTool::initClass();
	SubtractSelectTool::initClass();
	ShowPropertiesTool::initClass();
	
	/* Register a callback with the object snapper tool class: */
	Vrui::ObjectSnapperTool::addSnapCallback(Misc::createFunctionCall(this,&NetworkViewer::objectSnapCallback));
	
	/* Create the simulation parameters control dialog: */
	parametersDialog=createParametersDialog();
	
	/* Create the rendering control dialog: */
	renderingDialog=createRenderingDialog();
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	#if CONFIG_USE_IMPOSTORSPHERES
	
	/* Initialize the node renderer: */
	if(useNodeSize)
		nodeRenderer.setVariableRadius();
	else
		nodeRenderer.setFixedRadius(nodeRadius);
	nodeRenderer.setColorMaterial(true);
	
	#endif
	
	/* Create a color map for numerical property values: */
	static const GLColorMap::Color npvColors[6]=
		{
		GLColorMap::Color(1.0f,0.0f,0.0f),
		GLColorMap::Color(1.0f,1.0f,0.0f),
		GLColorMap::Color(0.0f,1.0f,0.0f),
		GLColorMap::Color(0.0f,1.0f,1.0f),
		GLColorMap::Color(0.0f,0.0f,1.0f),
		GLColorMap::Color(1.0f,0.0f,1.0f)
		};
	GLdouble npvKeys[6];
	for(int i=0;i<6;++i)
		npvKeys[i]=double(i);
	numericalPropertyValueMap.setColors(6,npvColors,npvKeys);
	}

NetworkViewer::~NetworkViewer(void)
	{
	delete network;
	
	delete parametersDialog;
	delete renderingDialog;
	delete mainMenu;
	delete nodeCallout;
	}

class LocalRepulsiveForceFunctor
	{
	/* Elements: */
	private:
	ParticleSystem& particles; // Reference to particle system
	Index index; // Index of source particle
	const Point& position; // Source particle's position
	Scalar dt2; // Squared time step
	
	/* Constructors and destructors: */
	public:
	LocalRepulsiveForceFunctor(ParticleSystem& sParticles,Index sIndex,Scalar sDt2)
		:particles(sParticles),
		 index(sIndex),position(particles.getParticlePosition(index)),
		 dt2(sDt2)
		{
		}
	
	/* Methods: */
	const Point& getCenterPosition(void) const
		{
		return position;
		}
	Scalar getMaxDist2(void) const
		{
		return Scalar(4); // Two times link length, squared
		}
	void operator()(Index particleIndex,const Point& particlePosition,Scalar dist2)
		{
		if(index<particleIndex)
			{
			/* Calculate the force: */
			if(dist2>Scalar(0))
				{
				/* Calculate force vector: */
				Vector force=(particles.getParticlePosition(particleIndex)-position)*((Scalar(10)-Scalar(5)*Math::sqrt(dist2))/dist2);
				
				/* Apply the force vector to both particles: */
				particles.forceParticle(index,-force,dt2);
				particles.forceParticle(particleIndex,force,dt2);
				}
			else
				{
				/* Do something clever... */
				}
			}
		}
	};

class GlobalRepulsiveForceFunctor // Base class for n-body force functors
	{
	/* Elements: */
	protected:
	Scalar theta; // Barnes-Hut approximation threshold
	Scalar minDist,minDist2; // Squared minimum particle distance for inverse force law
	Index particleIndex; // Index of particle for which forces are accumulated
	Point particlePosition; // Position of particle for which forces are accumulated
	Vector force; // Accumulated force
	
	/* Constructors and destructors: */
	public:
	GlobalRepulsiveForceFunctor(Scalar sTheta,Scalar sMinDist)
		:theta(sTheta),minDist(sMinDist),minDist2(Math::sqr(minDist))
		{
		}
	
	/* Methods: */
	void prepareParticle(Index newParticleIndex,const Point& newParticlePosition) // Prepares to accumulate force for a new particle
		{
		/* Remember the new particle: */
		particleIndex=newParticleIndex;
		particlePosition=newParticlePosition;
		
		/* Reset the force accumulator: */
		force=Vector::zero;
		}
	Index getParticleIndex(void) const
		{
		return particleIndex;
		}
	const Point& getParticlePosition(void) const
		{
		return particlePosition;
		}
	Scalar getTheta(void) const
		{
		return theta;
		}
	const Vector& getForce(void) const // Returns the accumulated force
		{
		return force;
		}
	};

class GlobalRepulsiveForceFunctorLinear:public GlobalRepulsiveForceFunctor // Functor for inverse linear n-body force
	{
	/* Constructors and destructors: */
	public:
	GlobalRepulsiveForceFunctorLinear(Scalar sTheta,Scalar sMinDist)
		:GlobalRepulsiveForceFunctor(sTheta,sMinDist)
		{
		}
	
	/* Methods: */
	void operator()(const Vector& dist,Scalar distLen2,Scalar mass)
		{
		/* Check the distance against the inverse force law cutoff: */
		if(distLen2>=minDist2)
			{
			/* Use inverse force law: */
			force-=dist*(mass/distLen2);
			}
		else if(distLen2>Scalar(0))
			{
			/* Use cut-off inverse force law: */
			force-=dist*(mass/Math::sqrt(minDist2*distLen2));
			}
		else
			{
			/* Use a random distance vector: */
			Vector d=Geometry::randVectorUniform<Scalar,3>(minDist);
			force-=d*(mass/minDist2);
			}
		}
	};

class GlobalRepulsiveForceFunctorQuadratic:public GlobalRepulsiveForceFunctor // Functor for inverse quadratic n-body force
	{
	/* Constructors and destructors: */
	public:
	GlobalRepulsiveForceFunctorQuadratic(Scalar sTheta,Scalar sMinDist)
		:GlobalRepulsiveForceFunctor(sTheta,sMinDist)
		{
		}
	
	/* Methods: */
	void operator()(const Vector& dist,Scalar distLen2,Scalar mass)
		{
		/* Check the distance against the inverse force law cutoff: */
		if(distLen2>=minDist2)
			{
			/* Use inverse force law: */
			force-=dist*(mass/(distLen2*Math::sqrt(distLen2)));
			}
		else if(distLen2>Scalar(0))
			{
			/* Use cut-off inverse force law: */
			force-=dist*(mass/(minDist2*Math::sqrt(distLen2)));
			}
		else
			{
			/* Use a random distance vector: */
			Vector d=Geometry::randVectorUniform<Scalar,3>(minDist);
			force-=d*(mass/(minDist2*minDist));
			}
		}
	};

void NetworkViewer::frame(void)
	{
	/* Advance the particle system's state: */
	Scalar dt(1.0/60.0); // Vrui::getFrameTime());
	if(dt>Scalar(0))
		{
		#if TIMING
		Realtime::TimePointMonotonic timer0;
		#endif
		Scalar dt2=Math::sqr(dt);
		particles.moveParticles(dt);
		#if TIMING
		std::cout<<double(timer0.setAndDiff())*1000.0<<"ms";
		#endif
		
		/* Pull all particles towards the network's center of gravity: */
		#if TIMING
		Realtime::TimePointMonotonic timer1;
		#endif
		const ParticleOctree& octree=particles.getOctree();
		Point center=octree.getCenterOfGravity();
		// center=Point::origin;
		for(Index index=0;index<particles.getNumParticles();++index)
			{
			const Point& pos=particles.getParticlePosition(index);
			Vector d=center-pos;
			particles.forceParticle(index,d,dt2*centralForce);
			}
		#if TIMING
		std::cout<<", "<<double(timer1.setAndDiff())*1000.0<<"ms";
		#endif
		
		/* Apply a repelling n-body force to all particles: */
		#if TIMING
		Realtime::TimePointMonotonic timer2;
		#endif
		switch(repellingForceMode)
			{
			case Linear:
				{
				GlobalRepulsiveForceFunctorLinear grff(repellingForceTheta,repellingForceCutoff);
				for(Index index=0;index<particles.getNumParticles();++index)
					{
					grff.prepareParticle(index,particles.getParticlePosition(index));
					octree.calcForce(grff);
					particles.forceParticle(index,grff.getForce(),dt2*repellingForce);
					}
				
				break;
				}
			
			case Quadratic:
				{
				GlobalRepulsiveForceFunctorQuadratic grff(repellingForceTheta,repellingForceCutoff);
				for(Index index=0;index<particles.getNumParticles();++index)
					{
					grff.prepareParticle(index,particles.getParticlePosition(index));
					octree.calcForce(grff);
					particles.forceParticle(index,grff.getForce(),dt2*repellingForce);
					}
				
				break;
				}
			}
		#if TIMING
		std::cout<<", "<<double(timer2.setAndDiff())*1000.0<<"ms";
		#endif
		
		#if 0
		/* Calculate interaction forces between linked particles: */
		for(std::vector<Link>::iterator lIt=links.begin();lIt!=links.end();++lIt)
			{
			/* Calculate force vector: */
			Index i0=lIt->getNode(0)->getParticleIndex();
			Index i1=lIt->getNode(1)->getParticleIndex();
			Vector d=particles.getParticlePosition(i1)-particles.getParticlePosition(i0);
			Scalar dLen2=d.sqr();
			d*=Scalar(0.5)*(Scalar(1)-Scalar(1)/d.mag())*linkStrength;
			
			/* Apply the force vector to both particles: */
			particles.forceParticle(i0,d,Scalar(1));
			particles.forceParticle(i1,d,Scalar(-1));
			}
		#endif
		
		#if 0
		/* Calculate interactions between all pairs of near particles: */
		for(Index index=0;index<particles.getNumParticles();++index)
			{
			LocalRepulsiveForceFunctor lrff(particles,index,dt2);
			particles.processCloseParticles(lrff);
			}
		#endif
		
		/* Finish the particle system's update step: */
		#if TIMING
		Realtime::TimePointMonotonic timer3;
		#endif
		particles.enforceConstraints(dt);
		#if TIMING
		std::cout<<", "<<double(timer3.setAndDiff())*1000.0<<"ms"<<std::endl;
		#endif
		}
	
	/* Request another frame: */
	Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
	}

void NetworkViewer::display(GLContextData& contextData) const
	{
	#if 0 // Not currently needed
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	#endif
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT);
	glEnable(GL_LIGHTING);
	
	/* Set material properties: */
	glMaterialSpecular(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.5f,0.5f,0.5f));
	glMaterialShininess(GLMaterialEnums::FRONT,64.0f);
	
	#if CONFIG_USE_IMPOSTORSPHERES
	
	/* Enable the node renderer: */
	nodeRenderer.enable(GLfloat(Vrui::getNavigationTransformation().getScaling()),contextData);
	
	#else
	
	/* Draw nodes as fat points: */
	glPointSize(3.0f);
	
	#endif
	
	/* Draw all nodes: */
	glBegin(GL_POINTS);
	Network::ColorList::const_iterator ncIt=network->getNodeColors().begin();
	const Network::NodeList& nodes=network->getNodes();
	if(useNodeSize)
		{
		for(Network::NodeList::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt,++ncIt)
			{
			/* Draw the node: */
			glColor<4>(ncIt->getComponents());
			const Point& pos=particles.getParticlePosition(nIt->getParticleIndex());
			glVertex4f(pos[0],pos[1],pos[2],nodeRadius*Math::pow(nIt->getSize(),nodeSizeExponent));
			}
		}
	else
		{
		for(Network::NodeList::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt,++ncIt)
			{
			/* Draw the node: */
			glColor<4>(ncIt->getComponents());
			glVertex(particles.getParticlePosition(nIt->getParticleIndex()));
			}
		}
	glEnd();
	
	#if CONFIG_USE_IMPOSTORSPHERES
	
	/* Disable the node renderer: */
	nodeRenderer.disable(contextData);
	
	#endif
	
	glPopAttrib();
	}

void NetworkViewer::resetNavigation(void)
	{
	/* Calculate the network graph's bounding box: */
	Geometry::Box<Scalar,3> bbox=Geometry::Box<Scalar,3>::empty;
	const Network::NodeList& nodes=network->getNodes();
	for(Network::NodeList::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt)
		bbox.addPoint(particles.getParticlePosition(nIt->getParticleIndex()));
	
	/* Center and scale the network graph: */
	Vrui::Point center(Geometry::mid(bbox.min,bbox.max));
	Vrui::Scalar size(Geometry::dist(bbox.min,bbox.max));
	Vrui::setNavigationTransformation(center,size);
	}

void NetworkViewer::glRenderActionTransparent(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	// glColor3f(0.05f,0.05f,0.05f);
	// glColor4f(1.0f,1.0f,1.0f,0.1f);
	
	/* Go to navigational space: */
	Vrui::goToNavigationalSpace(contextData);
	
	/* Draw all links: */
	glBegin(GL_LINES);
	const Network::ColorList& nodeColors=network->getNodeColors();
	const Network::LinkList& links=network->getLinks();
	for(Network::LinkList::const_iterator lIt=links.begin();lIt!=links.end();++lIt)
		{
		GLubyte linkAlpha;
		if(lIt->getValue()<=Scalar(0))
			linkAlpha=GLubyte(0U);
		else if(lIt->getValue()>=Scalar(1))
			linkAlpha=GLubyte(255U);
		else
			linkAlpha=GLubyte(Math::floor(lIt->getValue()*256.0));
		// linkAlpha=51U;
		for(int i=0;i<2;++i)
			{
			const Node::Color& nc=nodeColors[lIt->getNodeIndex(i)];
			glColor4ub(nc[0],nc[1],nc[2],linkAlpha);
			glVertex(particles.getParticlePosition(lIt->getNode(i)->getParticleIndex()));
			}
		}
	glEnd();

	/* Return to physical space: */
	glPopMatrix();
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	glPopAttrib();
	}

void NetworkViewer::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create a display list to render a sphere: */
	glNewList(dataItem->sphereDisplayList,GL_COMPILE);
	
	glDrawSphereIcosahedron(1.0f,6);
	
	glEndList();
	}

VRUI_APPLICATION_RUN(NetworkViewer)
