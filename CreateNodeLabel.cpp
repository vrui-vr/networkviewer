/***********************************************************************
CreateNodeLabel - Function to create a label displaying the properties
of a network node.
Copyright (c) 2023 Oliver Kreylos

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

#include "CreateNodeLabel.h"

#include <stdio.h>
#include <Math/Math.h>
#include <SceneGraph/ONTransformNode.h>
#include <SceneGraph/OGTransformNode.h>
#include <SceneGraph/BillboardNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/FancyFontStyleNode.h>
#include <SceneGraph/FancyTextNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>

#include "JsonEntity.h"
#include "JsonBoolean.h"
#include "JsonNumber.h"
#include "JsonString.h"

namespace {

/****************
Helper functions:
****************/

typedef SceneGraph::Scalar Scalar;
typedef SceneGraph::Point Point;
typedef SceneGraph::Vector Vector;

void makeCorner(Scalar centerX,Scalar centerY,Scalar z,Scalar radius,int corner,int numSegments,SceneGraph::MFPoint::ValueList& points)
	{
	/* Generate a quarter circle of vertices: */
	for(int i=0;i<=numSegments;++i)
		{
		Scalar angle=Math::rad(Scalar(90)*(Scalar(corner)+Scalar(i)/Scalar(numSegments)));
		points.push_back(Point(centerX+Math::cos(angle)*radius,centerY+Math::sin(angle)*radius,z));
		}
	}

SceneGraph::IndexedFaceSetNodePointer makeBubble(SceneGraph::ShapeNode& textShape)
	{
	/* Get the text shape's extents: */
	SceneGraph::Box bbox=textShape.calcBoundingBox();
	Scalar xMin=bbox.min[0];
	Scalar xMax=bbox.max[0];
	Scalar yMin=bbox.min[1];
	Scalar yMax=bbox.max[1];
	Scalar z=bbox.min[2];
	
	/* Set up bubble parameters: */
	SceneGraph::FancyTextNodePointer text(textShape.geometry.getValue());
	SceneGraph::FancyFontStyleNodePointer fontStyle(text->fontStyle.getValue());
	Scalar marginWidth=fontStyle->size.getValue()*Scalar(0.333);
	Scalar borderWidth=fontStyle->size.getValue()*Scalar(0.125);
	Scalar borderDepth=borderWidth*Scalar(0.5);
	Scalar pointWidth=fontStyle->size.getValue()*Scalar(1.25);
	Scalar pointHeight=fontStyle->size.getValue()*Scalar(1.75);
	Scalar thickness=fontStyle->size.getValue()*Scalar(0.125);
	int numSegments=8;
	SceneGraph::Color backgroundColor(0.5f,0.5f,0.5f);
	SceneGraph::Color borderColor(0.0f,0.125f,0.5f);
	
	SceneGraph::IndexedFaceSetNodePointer faceSet=new SceneGraph::IndexedFaceSetNode;
	
	/* Generate the bubble's vertices: */
	SceneGraph::CoordinateNodePointer coord=new SceneGraph::CoordinateNode;
	SceneGraph::MFPoint::ValueList& points=coord->point.getValues();
	
	/* Generate the vertices of the interior rectangle: */
	points.push_back(Point(xMin,yMin,z));
	points.push_back(Point(xMax,yMin,z));
	points.push_back(Point(xMax,yMax,z));
	points.push_back(Point(xMin,yMax,z));
	
	/* Generate the vertices of the lower inner border corners: */
	makeCorner(xMin,yMin,z,marginWidth,2,numSegments,points);
	makeCorner(xMax,yMin,z,marginWidth,3,numSegments,points);
	makeCorner(xMax,yMax,z,marginWidth,0,numSegments,points);
	makeCorner(xMin,yMax,z,marginWidth,1,numSegments,points);
	
	/* Generate the vertices of the top inner border corners: */
	makeCorner(xMin,yMin,z+borderDepth,marginWidth,2,numSegments,points);
	makeCorner(xMax,yMin,z+borderDepth,marginWidth,3,numSegments,points);
	makeCorner(xMax,yMax,z+borderDepth,marginWidth,0,numSegments,points);
	makeCorner(xMin,yMax,z+borderDepth,marginWidth,1,numSegments,points);
	
	/* Generate the vertices of the top outer border corners: */
	makeCorner(xMin,yMin,z+borderDepth,marginWidth+borderWidth,2,numSegments,points);
	makeCorner(xMax,yMin,z+borderDepth,marginWidth+borderWidth,3,numSegments,points);
	makeCorner(xMax,yMax,z+borderDepth,marginWidth+borderWidth,0,numSegments,points);
	makeCorner(xMin,yMax,z+borderDepth,marginWidth+borderWidth,1,numSegments,points);
	
	/* Generate the vertices of the bottom outer border corners: */
	makeCorner(xMin,yMin,z-thickness,marginWidth+borderWidth,2,numSegments,points);
	makeCorner(xMax,yMin,z-thickness,marginWidth+borderWidth,3,numSegments,points);
	makeCorner(xMax,yMax,z-thickness,marginWidth+borderWidth,0,numSegments,points);
	makeCorner(xMin,yMax,z-thickness,marginWidth+borderWidth,1,numSegments,points);
	
	/* Add the special vertices for the bubble point: */
	Scalar xMid=Math::mid(xMin,xMax);
	Scalar bpx0=Math::max(xMid-Math::div2(pointHeight)-pointWidth,xMin);
	Scalar bpx1=Math::max(xMid-Math::div2(pointHeight),bpx0+pointWidth);
	points.push_back(Point(bpx0,yMin-marginWidth-borderWidth,z+borderDepth));
	points.push_back(Point(bpx1,yMin-marginWidth-borderWidth,z+borderDepth));
	points.push_back(Point(xMid,yMin-marginWidth-borderWidth-pointHeight,z+borderDepth));
	points.push_back(Point(bpx0,yMin-marginWidth-borderWidth,z-thickness));
	points.push_back(Point(bpx1,yMin-marginWidth-borderWidth,z-thickness));
	points.push_back(Point(xMid,yMin-marginWidth-borderWidth-pointHeight,z-thickness));
	
	coord->update();
	faceSet->coord.setValue(coord);
	
	/* Generate normal vectors: */
	SceneGraph::NormalNodePointer normal=new SceneGraph::NormalNode;
	SceneGraph::MFVector::ValueList& normals=normal->vector.getValues();
	
	normals.push_back(Vector(0,0,1));
	for(int i=0;i<4*numSegments;++i)
		{
		Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(4*numSegments);
		normals.push_back(Vector(Math::cos(angle),Math::sin(angle),0));
		}
	normals.push_back(Vector(0,0,-1));
	normals.push_back(Geometry::normalize(Vector(-pointHeight,bpx0-xMid,0)));
	normals.push_back(Geometry::normalize(Vector(pointHeight,xMid-bpx1,0)));
	
	normal->update();
	faceSet->normal.setValue(normal);
	
	/* Generate faces: */
	SceneGraph::ColorNodePointer color=new SceneGraph::ColorNode;
	SceneGraph::MFColor::ValueList& colors=color->color.getValues();
	SceneGraph::MFInt::ValueList& normalIndex=faceSet->normalIndex.getValues();
	SceneGraph::MFInt::ValueList& coordIndex=faceSet->coordIndex.getValues();
	
	/* Generate the inside box: */
	for(int i=0;i<4;++i)
		{
		normalIndex.push_back(0);
		coordIndex.push_back(i);
		}
	normalIndex.push_back(-1);
	coordIndex.push_back(-1);
	colors.push_back(backgroundColor);
	
	/* Generate the margin: */
	int marginBase=4;
	for(int corner=0;corner<4;++corner)
		{
		/* Generate a face for the rounded corner: */
		normalIndex.push_back(0);
		coordIndex.push_back(corner);
		for(int i=0;i<=numSegments;++i)
			{
			normalIndex.push_back(0);
			coordIndex.push_back(marginBase+corner*(numSegments+1)+i);
			}
		normalIndex.push_back(-1);
		coordIndex.push_back(-1);
		colors.push_back(backgroundColor);
		
		/* Generate a face for the margin rectangle: */
		normalIndex.push_back(0);
		coordIndex.push_back(corner);
		normalIndex.push_back(0);
		coordIndex.push_back(marginBase+corner*(numSegments+1)+numSegments);
		normalIndex.push_back(0);
		coordIndex.push_back(marginBase+((corner+1)%4)*(numSegments+1));
		normalIndex.push_back(0);
		coordIndex.push_back((corner+1)%4);
		normalIndex.push_back(-1);
		coordIndex.push_back(-1);
		colors.push_back(backgroundColor);
		}
	
	/* Generate the inner border: */
	int innerBorderBase=marginBase+4*(numSegments+1);
	for(int corner=0;corner<4;++corner)
		{
		/* Generate faces for the rounded corner: */
		for(int i=0;i<numSegments;++i)
			{
			normalIndex.push_back(1+corner*numSegments+i);
			coordIndex.push_back(marginBase+corner*(numSegments+1)+i);
			normalIndex.push_back(1+corner*numSegments+i);
			coordIndex.push_back(innerBorderBase+corner*(numSegments+1)+i);
			normalIndex.push_back(1+((corner*numSegments+(i+1))%(4*numSegments)));
			coordIndex.push_back(innerBorderBase+corner*(numSegments+1)+(i+1));
			normalIndex.push_back(1+((corner*numSegments+(i+1))%(4*numSegments)));
			coordIndex.push_back(marginBase+corner*(numSegments+1)+(i+1));
			normalIndex.push_back(-1);
			coordIndex.push_back(-1);
			colors.push_back(borderColor);
			}
		
		/* Generate a face for the inner border rectangle: */
		normalIndex.push_back(1+((corner+1)*numSegments)%(4*numSegments));
		coordIndex.push_back(marginBase+corner*(numSegments+1)+numSegments);
		normalIndex.push_back(1+((corner+1)*numSegments)%(4*numSegments));
		coordIndex.push_back(innerBorderBase+corner*(numSegments+1)+numSegments);
		normalIndex.push_back(1+((corner+1)*numSegments)%(4*numSegments));
		coordIndex.push_back(innerBorderBase+((corner+1)%4)*(numSegments+1));
		normalIndex.push_back(1+((corner+1)*numSegments)%(4*numSegments));
		coordIndex.push_back(marginBase+((corner+1)%4)*(numSegments+1));
		normalIndex.push_back(-1);
		coordIndex.push_back(-1);
		colors.push_back(borderColor);
		}
	
	/* Generate the top border: */
	int topBorderBase=innerBorderBase+4*(numSegments+1);
	int bubblePointBase=topBorderBase+2*4*(numSegments+1);
	for(int corner=0;corner<4;++corner)
		{
		/* Generate faces for the rounded corner: */
		for(int i=0;i<numSegments;++i)
			{
			normalIndex.push_back(0);
			coordIndex.push_back(innerBorderBase+corner*(numSegments+1)+(i+1));
			normalIndex.push_back(0);
			coordIndex.push_back(innerBorderBase+corner*(numSegments+1)+i);
			normalIndex.push_back(0);
			coordIndex.push_back(topBorderBase+corner*(numSegments+1)+i);
			normalIndex.push_back(0);
			coordIndex.push_back(topBorderBase+corner*(numSegments+1)+(i+1));
			normalIndex.push_back(-1);
			coordIndex.push_back(-1);
			colors.push_back(borderColor);
			}
		
		/* Generate a face for the top border rectangle: */
		normalIndex.push_back(0);
		coordIndex.push_back(innerBorderBase+corner*(numSegments+1)+numSegments);
		normalIndex.push_back(0);
		coordIndex.push_back(topBorderBase+corner*(numSegments+1)+numSegments);
		if(corner==0)
			{
			/* Insert the attachment points for the bubble point: */
			normalIndex.push_back(0);
			coordIndex.push_back(bubblePointBase+0);
			normalIndex.push_back(0);
			coordIndex.push_back(bubblePointBase+1);
			}
		normalIndex.push_back(0);
		coordIndex.push_back(topBorderBase+((corner+1)%4)*(numSegments+1));
		normalIndex.push_back(0);
		coordIndex.push_back(innerBorderBase+((corner+1)%4)*(numSegments+1));
		normalIndex.push_back(-1);
		coordIndex.push_back(-1);
		colors.push_back(borderColor);
		}
	
	/* Generate the outer border: */
	int bottomBorderBase=topBorderBase+4*(numSegments+1);
	for(int corner=0;corner<4;++corner)
		{
		/* Generate faces for the rounded corner: */
		for(int i=0;i<numSegments;++i)
			{
			normalIndex.push_back(1+((corner+2)*numSegments+(i+1))%(4*numSegments));
			coordIndex.push_back(topBorderBase+corner*(numSegments+1)+(i+1));
			normalIndex.push_back(1+((corner+2)*numSegments+i)%(4*numSegments));
			coordIndex.push_back(topBorderBase+corner*(numSegments+1)+i);
			normalIndex.push_back(1+((corner+2)*numSegments+i)%(4*numSegments));
			coordIndex.push_back(bottomBorderBase+corner*(numSegments+1)+i);
			normalIndex.push_back(1+((corner+2)*numSegments+(i+1))%(4*numSegments));
			coordIndex.push_back(bottomBorderBase+corner*(numSegments+1)+(i+1));
			normalIndex.push_back(-1);
			coordIndex.push_back(-1);
			colors.push_back(borderColor);
			}
		
		/* Generate a face for the outer border rectangle: */
		normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
		coordIndex.push_back(topBorderBase+corner*(numSegments+1)+numSegments);
		normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
		coordIndex.push_back(bottomBorderBase+corner*(numSegments+1)+numSegments);
		if(corner==0)
			{
			/* Split the face into two to leave space for the bubble point: */
			normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
			coordIndex.push_back(bubblePointBase+3);
			normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
			coordIndex.push_back(bubblePointBase+0);
			normalIndex.push_back(-1);
			coordIndex.push_back(-1);
			colors.push_back(borderColor);
			
			normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
			coordIndex.push_back(bubblePointBase+1);
			normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
			coordIndex.push_back(bubblePointBase+4);
			}
		normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
		coordIndex.push_back(bottomBorderBase+((corner+1)%4)*(numSegments+1));
		normalIndex.push_back(1+((corner+3)*numSegments)%(4*numSegments));
		coordIndex.push_back(topBorderBase+((corner+1)%4)*(numSegments+1));
		normalIndex.push_back(-1);
		coordIndex.push_back(-1);
		colors.push_back(borderColor);
		}
	
	/* Generate the backside: */
	for(int corner=3;corner>=0;--corner)
		{
		if(corner==0)
			{
			/* Insert the attachment points for the bubble point: */
			normalIndex.push_back(1+4*numSegments);
			coordIndex.push_back(bubblePointBase+4);
			normalIndex.push_back(1+4*numSegments);
			coordIndex.push_back(bubblePointBase+3);
			}
		for(int i=numSegments;i>=0;--i)
			{
			normalIndex.push_back(1+4*numSegments);
			coordIndex.push_back(bottomBorderBase+corner*(numSegments+1)+i);
			}
		}
	normalIndex.push_back(-1);
	coordIndex.push_back(-1);
	colors.push_back(borderColor);
	
	/* Generate the bubble point: */
	normalIndex.push_back(0);
	coordIndex.push_back(bubblePointBase+1);
	normalIndex.push_back(0);
	coordIndex.push_back(bubblePointBase+0);
	normalIndex.push_back(0);
	coordIndex.push_back(bubblePointBase+2);
	coordIndex.push_back(-1);
	normalIndex.push_back(-1);
	colors.push_back(borderColor);
	
	normalIndex.push_back(1+4*numSegments+1);
	coordIndex.push_back(bubblePointBase+0);
	normalIndex.push_back(1+4*numSegments+1);
	coordIndex.push_back(bubblePointBase+3);
	normalIndex.push_back(1+4*numSegments+1);
	coordIndex.push_back(bubblePointBase+5);
	normalIndex.push_back(1+4*numSegments+1);
	coordIndex.push_back(bubblePointBase+2);
	normalIndex.push_back(-1);
	coordIndex.push_back(-1);
	colors.push_back(borderColor);
	
	normalIndex.push_back(1+4*numSegments+2);
	coordIndex.push_back(bubblePointBase+2);
	normalIndex.push_back(1+4*numSegments+2);
	coordIndex.push_back(bubblePointBase+5);
	normalIndex.push_back(1+4*numSegments+2);
	coordIndex.push_back(bubblePointBase+4);
	normalIndex.push_back(1+4*numSegments+2);
	coordIndex.push_back(bubblePointBase+1);
	normalIndex.push_back(-1);
	coordIndex.push_back(-1);
	colors.push_back(borderColor);
	
	normalIndex.push_back(1+4*numSegments);
	coordIndex.push_back(bubblePointBase+3);
	normalIndex.push_back(1+4*numSegments);
	coordIndex.push_back(bubblePointBase+4);
	normalIndex.push_back(1+4*numSegments);
	coordIndex.push_back(bubblePointBase+5);
	normalIndex.push_back(-1);
	coordIndex.push_back(-1);
	colors.push_back(borderColor);
	
	color->update();
	faceSet->color.setValue(color);
	
	faceSet->colorPerVertex.setValue(false);
	faceSet->normalPerVertex.setValue(true);
	faceSet->ccw.setValue(true);
	faceSet->convex.setValue(true);
	faceSet->solid.setValue(true);
	faceSet->update();
	
	return faceSet;
	}

}

SceneGraph::OGTransformNodePointer createNodeLabel(const JsonMap::Map& nodeProperties,unsigned int numLinks,SceneGraph::FancyFontStyleNode& fontStyle)
	{
	/* Create the label root node: */
	SceneGraph::OGTransformNodePointer root=new SceneGraph::OGTransformNode;
	
	/* Put a billboard node to align the label into the root node: */
	SceneGraph::BillboardNodePointer billboard=new SceneGraph::BillboardNode;
	billboard->axisOfRotation.setValue(SceneGraph::Vector::zero);
	
	/* Put a transform node to center the label bubble's point into the billboard node: */
	SceneGraph::ONTransformNodePointer labelTransform=new SceneGraph::ONTransformNode;
	
	/* Create a shape node containing the given node properties as text: */
	SceneGraph::ShapeNodePointer textShape=new SceneGraph::ShapeNode;
	textShape->appearance.setValue(SceneGraph::createDiffuseAppearance(SceneGraph::Color(0.0f,0.0f,0.0f)));
	
	/* Create the text node: */
	SceneGraph::FancyTextNodePointer text=new SceneGraph::FancyTextNode;
	
	/* Add the node's properties to the text node's string field: */
	for(JsonMap::Map::ConstIterator npIt=nodeProperties.begin();!npIt.isFinished();++npIt)
		{
		/* Start with the property name: */
		std::string line=npIt->getSource();
		line.append(": ");
		
		/* Convert the property value to text: */
		switch(npIt->getDest()->getType())
			{
			case JsonEntity::BOOLEAN:
				if(JsonBooleanPointer(npIt->getDest())->getBoolean())
					line.append("true");
				else
					line.append("false");
				break;
			
			case JsonEntity::NUMBER:
				{
				char number[64];
				snprintf(number,sizeof(number),"%f",JsonNumberPointer(npIt->getDest())->getNumber());
				line.append(number);
				break;
				}
			
			case JsonEntity::STRING:
				line.append(JsonStringPointer(npIt->getDest())->getString());
				break;
			
			case JsonEntity::LIST:
				line.append("<LIST>");
				break;
			
			case JsonEntity::MAP:
				line.append("<MAP>");
				break;
			}
		
		/* Append the property line to the text node: */
		text->string.appendValue(line);
		}
	
	/* Append a line showing the node's number of outgoing links: */
	std::string line("# links: ");
	char number[64];
	snprintf(number,sizeof(number),"%u",numLinks);
	line.append(number);
	text->string.appendValue(line);
	
	text->fontStyle.setValue(&fontStyle);
	text->depth.setValue(fontStyle.size.getValue()*SceneGraph::Scalar(0.025));
	text->front.setValue(true);
	text->outline.setValue(true);
	text->back.setValue(false);
	
	text->update();
	textShape->geometry.setValue(text);
	
	textShape->update();
	labelTransform->addChild(*textShape);
	
	/* Create a shape node containing the label bubble: */
	SceneGraph::ShapeNodePointer bubbleShape=new SceneGraph::ShapeNode;
	bubbleShape->appearance.setValue(SceneGraph::createDiffuseAppearance(SceneGraph::Color(1.0f,1.0f,1.0f)));
	bubbleShape->geometry.setValue(makeBubble(*textShape));
	bubbleShape->update();
	labelTransform->addChild(*bubbleShape);
	
	/* Align the label transform: */
	SceneGraph::Box bbox=bubbleShape->calcBoundingBox();
	SceneGraph::Point bubblePoint(Math::mid(bbox.min[0],bbox.max[0]),bbox.min[1],Math::mid(bbox.min[2],bbox.max[2]));
	labelTransform->setTransform(SceneGraph::ONTransformNode::ONTransform::translateToOriginFrom(bubblePoint));
	
	labelTransform->update();
	billboard->addChild(*labelTransform);
	
	billboard->update();
	root->addChild(*billboard);
	
	return root;
	}
