/***********************************************************************
ParticleTest - Test program for particle system simulator.
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

#include <string.h>
#include <vector>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Random.h>
#include <Geometry/LinearUnit.h>
#include <GL/gl.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLMaterial.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/TextFieldSlider.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/TransparentObject.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/ToolManager.h>

#include "ParticleSystem.h"
#include "ParticleMesh.h"
#include "Body.h"
#include "Whip.h"
#include "Figure.h"
#include "ParticleGrabber.h"

// DEBUGGING
#include <iostream>
#include <Geometry/OutputOperators.h>

class ParticleTest:public Vrui::Application,public Vrui::TransparentObject
	{
	/* Elements: */
	private:
	ParticleSystem particles; // A particle system
	std::vector<ParticleMesh*> meshes; // A list of particle meshes
	std::vector<GLMaterial> meshMaterials; // Material properties to render the particle meshes
	std::vector<Body*> bodies; // A list of bodies, each composed of multiple particles
	GLMotif::PopupWindow* particleParameterDialog; // Dialog window to control parameters of the particle system
	std::vector<Index> atoms; // List of atoms in the environment
	
	/* Private methods: */
	void gravityValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void attenuationValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void bounceValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void frictionValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void minParticleDistValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void numRelaxationIterationsValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	GLMotif::PopupWindow* createParticleParameterDialog(void); // Creates a dialog window to change particle system parameters
	
	/* Constructors and destructors: */
	public:
	ParticleTest(int& argc,char**& argv);
	virtual ~ParticleTest(void);
	
	/* Methods from class Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* Methods from class Vrui::TransparentObject: */
	virtual void glRenderActionTransparent(GLContextData& contextData) const;
	};

/*****************************
Methods of class ParticleTest:
*****************************/

void ParticleTest::gravityValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	particles.setGravity(Vector(0,0,Scalar(-cbData->value)));
	}

void ParticleTest::attenuationValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	particles.setAttenuation(Scalar(cbData->value));
	}

void ParticleTest::bounceValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	particles.setBounce(Scalar(cbData->value));
	}

void ParticleTest::frictionValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	particles.setFriction(Scalar(cbData->value));
	}

void ParticleTest::minParticleDistValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	particles.setMinParticleDist(Scalar(cbData->value));
	}

void ParticleTest::numRelaxationIterationsValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	particles.setNumRelaxationIterations((unsigned int)(Math::floor(cbData->value+0.5)));
	}

GLMotif::PopupWindow* ParticleTest::createParticleParameterDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getUiStyleSheet();
	
	GLMotif::PopupWindow* particleParametersDialog=new GLMotif::PopupWindow("ParticleParametersDialog",Vrui::getWidgetManager(),"Particle System Parameters");
	particleParametersDialog->setHideButton(true);
	particleParametersDialog->setResizableFlags(true,false);
	
	GLMotif::RowColumn* particleParameters=new GLMotif::RowColumn("ParticleParameters",particleParametersDialog,false);
	particleParameters->setOrientation(GLMotif::RowColumn::VERTICAL);
	particleParameters->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	particleParameters->setNumMinorWidgets(2);
	
	new GLMotif::Label("GravityLabel",particleParameters,"Gravity");
	
	GLMotif::TextFieldSlider* gravitySlider=new GLMotif::TextFieldSlider("GravitySlider",particleParameters,8,ss.fontHeight*10.0f);
	gravitySlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	gravitySlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	gravitySlider->getTextField()->setPrecision(2);
	gravitySlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	gravitySlider->setValueRange(0.0,9.81*2.0,0.01);
	gravitySlider->getSlider()->addNotch(9.81);
	gravitySlider->setValue(-particles.getGravity()[2]);
	gravitySlider->getValueChangedCallbacks().add(this,&ParticleTest::gravityValueChangedCallback);
	
	new GLMotif::Label("AttenuationLabel",particleParameters,"Attenuation");
	
	GLMotif::TextFieldSlider* attenuationSlider=new GLMotif::TextFieldSlider("AttenuationSlider",particleParameters,8,ss.fontHeight*10.0f);
	attenuationSlider->setSliderMapping(GLMotif::TextFieldSlider::GAMMA);
	attenuationSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	attenuationSlider->getTextField()->setFieldWidth(7);
	attenuationSlider->getTextField()->setPrecision(5);
	attenuationSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	attenuationSlider->setValueRange(0.0,1.0,0.001);
	attenuationSlider->setGammaExponent(0.5,0.9);
	attenuationSlider->setValue(particles.getAttenuation());
	attenuationSlider->getValueChangedCallbacks().add(this,&ParticleTest::attenuationValueChangedCallback);
	
	new GLMotif::Label("BounceLabel",particleParameters,"Bounce");
	
	GLMotif::TextFieldSlider* bounceSlider=new GLMotif::TextFieldSlider("BounceSlider",particleParameters,8,ss.fontHeight*10.0f);
	bounceSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	bounceSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	bounceSlider->getTextField()->setPrecision(2);
	bounceSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	bounceSlider->setValueRange(0.0,1.0,0.01);
	bounceSlider->setValue(particles.getBounce());
	bounceSlider->getValueChangedCallbacks().add(this,&ParticleTest::bounceValueChangedCallback);
	
	new GLMotif::Label("FrictionLabel",particleParameters,"Friction");
	
	GLMotif::TextFieldSlider* frictionSlider=new GLMotif::TextFieldSlider("FrictionSlider",particleParameters,8,ss.fontHeight*10.0f);
	frictionSlider->setSliderMapping(GLMotif::TextFieldSlider::GAMMA);
	frictionSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	frictionSlider->getTextField()->setFieldWidth(7);
	frictionSlider->getTextField()->setPrecision(5);
	frictionSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	frictionSlider->setValueRange(0.0,100.0,0.001);
	frictionSlider->setGammaExponent(0.5,1.0);
	frictionSlider->setValue(particles.getFriction());
	frictionSlider->getValueChangedCallbacks().add(this,&ParticleTest::frictionValueChangedCallback);
	
	new GLMotif::Label("MinParticleDistLabel",particleParameters,"Min Distance");
	
	GLMotif::TextFieldSlider* minParticleDistSlider=new GLMotif::TextFieldSlider("MinParticleDistSlider",particleParameters,8,ss.fontHeight*10.0f);
	minParticleDistSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	minParticleDistSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	minParticleDistSlider->getTextField()->setFieldWidth(7);
	minParticleDistSlider->getTextField()->setPrecision(3);
	minParticleDistSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	minParticleDistSlider->setValueRange(0.0,1.0,0.001);
	minParticleDistSlider->setValue(particles.getMinParticleDist());
	minParticleDistSlider->getValueChangedCallbacks().add(this,&ParticleTest::minParticleDistValueChangedCallback);
	
	new GLMotif::Label("NumRelaxationIterationsLabel",particleParameters,"# Iterations");
	
	GLMotif::TextFieldSlider* numRelaxationIterationsSlider=new GLMotif::TextFieldSlider("NumRelaxationIterationsSlider",particleParameters,8,ss.fontHeight*10.0f);
	numRelaxationIterationsSlider->setSliderMapping(GLMotif::TextFieldSlider::EXP10);
	numRelaxationIterationsSlider->setValueType(GLMotif::TextFieldSlider::UINT);
	numRelaxationIterationsSlider->getTextField()->setPrecision(0);
	numRelaxationIterationsSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	numRelaxationIterationsSlider->setValueRange(1.0,1000.0,0.01);
	numRelaxationIterationsSlider->setValue(particles.getNumRelaxationIterations());
	numRelaxationIterationsSlider->getValueChangedCallbacks().add(this,&ParticleTest::numRelaxationIterationsValueChangedCallback);
	
	particleParameters->manageChild();
	
	return particleParametersDialog;
	}

ParticleTest::ParticleTest(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 particleParameterDialog(0)
	{
	/* Enforce minimum particle distance: */
	// particles.setMinParticleDist(Scalar(2.0/50));
	
	/* Create a domain box constraint: */
	particles.addBoxConstraint(true,Point(0,0,0),Point(10,10,10));
	// particles.addSphereConstraint(true,Point(5,5,5),Scalar(7.07));
	
	/* Create a table: */
	// particles.addBoxConstraint(false,Point(3.5,7.5,0),Point(4.5,8.5,1.5));
	particles.addBoxConstraint(false,Point(7.5,1,-1),Point(9.5,3,1.5));
	
	/* Create a wart: */
	// particles.addSphereConstraint(false,Point(8.5,2,0.75),1);
	particles.addSphereConstraint(false,Point(4.25,7.25,3),1);
	
	/* Create a whipping post: */
	particles.addBoxConstraint(false,Point(2.0,2.0,-1),Point(2.1,2.1,2.5));
	
	// particles.setGravity(Vector(0,0,-9.81));
	// particles.setGravity(Vector(0,0,-1));
	particles.setGravity(Vector::zero);
	particles.setAttenuation(Scalar(1));
	particles.setBounce(Scalar(0));
	particles.setFriction(Scalar(0.5));
	// particles.setNumRelaxationIterations(500);
	particles.setNumRelaxationIterations(10);
	
	#if 0
	/* Create a test object: */
	atoms.push_back(particles.addParticle(Scalar(1),Point(4.0,4.0,5.0),Vector::zero));
	atoms.push_back(particles.addParticle(Scalar(1),Point(6.0,4.0,5.0),Vector::zero));
	atoms.push_back(particles.addParticle(Scalar(1),Point(4.0,6.0,5.0),Vector::zero));
	atoms.push_back(particles.addParticle(Scalar(1),Point(6.0,6.0,5.0),Vector::zero));
	
	particles.addDistConstraint(atoms[0],atoms[1],Scalar(2),Scalar(1));
	particles.addDistConstraint(atoms[1],atoms[3],Scalar(2),Scalar(1));
	particles.addDistConstraint(atoms[3],atoms[2],Scalar(2),Scalar(1));
	particles.addDistConstraint(atoms[2],atoms[0],Scalar(2),Scalar(1));
	
	particles.addDistConstraint(atoms[0],atoms[3],Scalar(4),Scalar(1));
	particles.addDistConstraint(atoms[1],atoms[2],Scalar(4),Scalar(1));
	#endif
	
	/* Load figures from files passed on the command line: */
	for(int argi=1;argi<argc;++argi)
		{
		if(argv[argi][0]=='-')
			{
			if(strcasecmp(argv[argi],"-rag")==0)
				{
				/* Get the mesh size: */
				int size=20;
				if(argi+1<argc)
					{
					/* Attempt to parse the next argument as a number: */
					int s=0;
					int i;
					for(i=0;isdigit(argv[argi+1][i]);++i)
						s=s*10+int(argv[argi+1][i])-int('0');
					if(i>0&&argv[argi+1][i]=='\0')
						{
						/* It was actually a number: */
						size=s;
						++argi;
						}
					}
				
				/* Create a piece of cloth: */
				double scale=2.5/double(size);
				Scalar bondStrength(0.5);
				// Scalar bondStrength(1);
				ParticleMesh* rag=new ParticleMesh(particles);
				for(int y=0;y<size;++y)
					for(int x=0;x<size;++x)
						rag->addVertex(particles.addParticle(1.0,Point(3.0+double(x)*scale,6.0+double(y)*scale,7.0),Vector::zero));
				for(int x=1;x<size;++x)
					particles.addDistConstraint(rag->getVertexIndex((x-1)),rag->getVertexIndex((x-0)),scale);
				for(int y=1;y<size;++y)
					particles.addDistConstraint(rag->getVertexIndex((y-1)*size),rag->getVertexIndex((y-0)*size),scale);
				for(int y=1;y<size;++y)
					for(int x=1;x<size;++x)
						{
						particles.addDistConstraint(rag->getVertexIndex((y-1)*size+(x-0)),rag->getVertexIndex((y-0)*size+(x-0)),scale,bondStrength);
						particles.addDistConstraint(rag->getVertexIndex((y-0)*size+(x-1)),rag->getVertexIndex((y-0)*size+(x-0)),scale,bondStrength);
						// particles.addDistConstraint(rag->getVertexIndex((y-0)*size+(x-1)),rag->getVertexIndex((y-1)*size+(x-0)),scale,bondStrength);
						particles.addDistConstraint(rag->getVertexIndex((y-0)*size+(x-1)),rag->getVertexIndex((y-1)*size+(x-0)),scale*Math::sqrt(Scalar(2)),bondStrength);
						// particles.addDistConstraint(rag->getVertexIndex((y-1)*size+(x-1)),rag->getVertexIndex((y-0)*size+(x-0)),scale*Math::sqrt(Scalar(2)),bondStrength);
						
						rag->addTriangle((y-1)*size+(x-1),(y-1)*size+(x-0),(y-0)*size+(x-1));
						rag->addTriangle((y-0)*size+(x-1),(y-1)*size+(x-0),(y-0)*size+(x-0));
						}
				#if 1
				for(int y=0;y<size;++y)
					for(int x=2;x<size;++x)
						particles.addDistConstraint(rag->getVertexIndex(y*size+(x-2)),rag->getVertexIndex(y*size+(x-0)),scale*Scalar(2),bondStrength*Scalar(0.5));
				for(int x=0;x<size;++x)
					for(int y=2;y<size;++y)
						particles.addDistConstraint(rag->getVertexIndex((y-2)*size+x),rag->getVertexIndex((y-0)*size+x),scale*Scalar(2),bondStrength*Scalar(0.5));
				#endif
				
				rag->setFrontMaterial(GLMaterial(GLMaterial::Color(0.3f,0.5f,1.0f),GLMaterial::Color(0.5f,0.5f,0.5f),32.0f));
				rag->setBackMaterial(GLMaterial(GLMaterial::Color(0.7f,0.8f,0.3f),GLMaterial::Color(0.5f,0.5f,0.5f),32.0f));
				meshes.push_back(rag);
				}
			else if(strcasecmp(argv[argi],"-whip")==0)
				{
				/* Create a whip: */
				bodies.push_back(new Whip(particles,Point(3,0.5,5),Vector(0.2,1.0,0)));
				}
			else if(strcasecmp(argv[argi],"-tet")==0)
				{
				/* Create a tetrahedron: */
				static const Point tetVertices[4]=
					{
					Point(4,4,4),
					Point(5,4,4),
					Point(4,5,4),
					Point(4,4,5)
					};
				static const int tetFaces[4][3]=
					{
					{0,2,1},{0,1,3},{1,2,3},{2,0,3}
					};
				Index tetVertexIndices[4];
				for(int i=0;i<4;++i)
					tetVertexIndices[i]=particles.addParticle(1,tetVertices[i],Vector::zero);
				for(int i=0;i<4;++i)
					for(int j=i+1;j<4;++j)
						particles.addDistConstraint(tetVertexIndices[i],tetVertexIndices[j],1,Scalar(1));
				
				ParticleMesh* tetrahedron=new ParticleMesh(particles);
				for(int face=0;face<4;++face)
					{
					for(int v=0;v<3;++v)
						tetrahedron->addVertex(tetVertexIndices[tetFaces[face][v]]);
					tetrahedron->addTriangle(face*3+0,face*3+1,face*3+2);
					}
				
				tetrahedron->setFrontMaterial(GLMaterial(GLMaterial::Color(1.0f,0.3f,0.3f),GLMaterial::Color(0.5f,0.5f,0.5f),32.0f));
				meshes.push_back(tetrahedron);
				}
			}
		else
			{
			try
				{
				bodies.push_back(new Figure(particles,argv[argi],Body::GrabTransform::translateFromOriginTo(Point(5,5,5))));
				}
			catch(const std::runtime_error& err)
				{
				Misc::formattedUserError("ParticleTest: Cannot load figure from file %s due to exception %s",argv[argi],err.what());
				}
			}
		}
	
	/* Use a single thread to simulate the particle system: */
	particles.setNumThreads(1,*static_cast<Threads::Barrier*>(0));
	
	/* Initialize the particle grabber tool class: */
	ParticleGrabber::initClass(particles,&bodies);
	
	/* Create the parameter control dialog: */
	particleParameterDialog=createParticleParameterDialog();
	Vrui::popupPrimaryWidget(particleParameterDialog);
	
	/* Set coordinate unit: */
	Vrui::getCoordinateManager()->setUnit(Geometry::LinearUnit(Geometry::LinearUnit::METER,1.0));
	}

ParticleTest::~ParticleTest(void)
	{
	/* Delete the parameter control dialog: */
	delete particleParameterDialog;
	
	/* Delete all meshes: */
	for(std::vector<ParticleMesh*>::iterator mIt=meshes.begin();mIt!=meshes.end();++mIt)
		delete *mIt;
	
	/* Delete all bodies: */
	for(std::vector<Body*>::iterator bIt=bodies.begin();bIt!=bodies.end();++bIt)
		delete *bIt;
	}

void ParticleTest::frame(void)
	{
	/* Get the time delta of the previous frame: */
	Scalar frameTime(Vrui::getFrameTime());
	if(frameTime>Scalar(0))
		{
		/* Move the particles by their velocities from the previous frame: */
		particles.moveParticles(frameTime);
		
		/* Let all bodies apply forces to the particles: */
		Scalar frameTime2=Math::sqr(frameTime);
		for(std::vector<Body*>::iterator bIt=bodies.begin();bIt!=bodies.end();++bIt)
			(*bIt)->applyForces(frameTime,frameTime2);
		
		/* Enforce the particle system's constraints: */
		particles.enforceConstraints(frameTime);
		}
	
	/* Update all meshes: */
	for(std::vector<ParticleMesh*>::iterator mIt=meshes.begin();mIt!=meshes.end();++mIt)
		(*mIt)->update();
	
	/* Update all bodies: */
	for(std::vector<Body*>::iterator bIt=bodies.begin();bIt!=bodies.end();++bIt)
		(*bIt)->update(frameTime);
	
	/* Request another frame: */
	Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
	}

void ParticleTest::display(GLContextData& contextData) const
	{
	/* Render the particle system's boundary constraints: */
	glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.6f,0.6f,0.6f));
	glMaterialSpecular(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.0f,0.0f,0.0f));
	glMaterialShininess(GLMaterialEnums::FRONT,0.0f);
	particles.glRenderAction(false);
	
	/* Render all meshes: */
	for(std::vector<ParticleMesh*>::const_iterator mIt=meshes.begin();mIt!=meshes.end();++mIt)
		(*mIt)->glRenderAction(contextData);
	
	/* Render all bodies: */
	for(std::vector<Body*>::const_iterator bIt=bodies.begin();bIt!=bodies.end();++bIt)
		(*bIt)->glRenderAction(contextData);
	
	/* Render all atoms: */
	glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glPointSize(5.0f);
	
	glBegin(GL_POINTS);
	glColor3f(1.0f,0.0f,1.0f);
	for(std::vector<Index>::const_iterator aIt=atoms.begin();aIt!=atoms.end();++aIt)
		glVertex(particles.getParticlePosition(*aIt));
	glEnd();
	
	glPopAttrib();
	
	#if 0
	/* Render the test object: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	glPointSize(5.0f);
	
	glBegin(GL_POINTS);
	glColor3f(1.0f,0.0f,0.0f);
	glVertex(particles.getParticlePosition(c0));
	glVertex(particles.getParticlePosition(c1));
	glVertex(particles.getParticlePosition(c2));
	glVertex(particles.getParticlePosition(c3));
	glEnd();
	
	glBegin(GL_LINE_LOOP);
	glColor3f(0.0f,0.0f,1.0f);
	glVertex(particles.getParticlePosition(c0));
	glVertex(particles.getParticlePosition(c1));
	glVertex(particles.getParticlePosition(c3));
	glVertex(particles.getParticlePosition(c2));
	glEnd();
	
	glPopAttrib();
	#endif
	}

void ParticleTest::resetNavigation(void)
	{
	/* Show the entire domain: */
	Vrui::setNavigationTransformation(Vrui::Point(5,5,5),Vrui::Scalar(10),Vrui::Vector(0,0,1));
	}

void ParticleTest::glRenderActionTransparent(GLContextData& contextData) const
	{
	/* Go to navigational coordinates: */
	Vrui::goToNavigationalSpace(contextData);
	
	/* Render the particle system: */
	// glBlendFunc(GL_ONE,GL_ONE);
	// glColor3f(0.05f,0.05f,0.05f);
	glColor4f(1.0f,1.0f,1.0f,0.05f);
	particles.glRenderAction(true);
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	/* Return to physical coordinates: */
	glPopMatrix();
	}

VRUI_APPLICATION_RUN(ParticleTest)
