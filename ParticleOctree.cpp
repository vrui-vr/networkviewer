/***********************************************************************
ParticleOctree - Class to sort particles from a particle system into an
adaptive octree for fast neighborhood queries and close particle
interaction.
Copyright (c) 2018-2023 Oliver Kreylos

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

#include "ParticleOctree.h"

#include <utility>
#include <stdexcept>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLVertexTemplates.h>

#include "ParticleSystem.h"

#include "ParticleOctree.icpp"

#if PARTICLEOCTREE_DEBUGGING
#include <iostream>
#endif

#define TESTING 0
#if TESTING
#include <iostream>
#include <Realtime/Time.h>
double totalTreeUpdateTime=0.0;
#endif

/*********************************************
Static elements of class ParticleOctree::Node:
*********************************************/

size_t ParticleOctree::Node::maxParticlesPerNode=16;

/*************************************
Methods of class ParticleOctree::Node:
*************************************/

void ParticleOctree::Node::splitLeaf(const ParticleSystem& particles)
	{
	/* Create the node's new children: */
	Node* newChildren=new Node[8];
	for(int childIndex=0;childIndex<8;++childIndex)
		{
		/* Initialize the child node: */
		Node& child=newChildren[childIndex];
		child.parent=this;
		for(int i=0;i<3;++i)
			{
			if(childIndex&(1<<i))
				{
				child.min[i]=center[i];
				child.max[i]=max[i];
				}
			else
				{
				child.min[i]=min[i];
				child.max[i]=center[i];
				}
			}
		child.center=Geometry::mid(child.min,child.max);
		child.numParticles=0;
		child.particleIndices=new Index[maxParticlesPerNode];
		}
	
	/* Distribute the node's particles amongst its children: */
	Index* end=particleIndices+numParticles;
	for(Index* sPtr=particleIndices;sPtr!=end;++sPtr)
		{
		/* Add the particle to the appropriate child's particle array: */
		int childIndex=getChildIndex(particles.getParticlePosition(*sPtr));
		newChildren[childIndex].particleIndices[newChildren[childIndex].numParticles]=*sPtr;
		++newChildren[childIndex].numParticles;
		}
	
	/* Delete this node's particle index array and install the new children array: */
	delete[] particleIndices;
	children=newChildren;
	}

size_t ParticleOctree::Node::collapseSubtree(void)
	{
	/* Gather all particles from the node's children, which are all leaf nodes: */
	Index* newParticleIndices=new Index[maxParticlesPerNode];
	Index* npiPtr=newParticleIndices;
	size_t numChildParticles=0;
	
	for(int childIndex=0;childIndex<8;++childIndex)
		{
		/* Copy the child's particles: */
		Index* cpiEnd=children[childIndex].particleIndices+children[childIndex].numParticles;
		for(Index* cpiPtr=children[childIndex].particleIndices;cpiPtr!=cpiEnd;++cpiPtr,++npiPtr)
			*npiPtr=*cpiPtr;
		
		numChildParticles+=children[childIndex].numParticles;
		}
	
	#if PARTICLEOCTREE_DEBUGGING
	if(numChildParticles>maxParticlesPerNode)
		throw std::runtime_error("ParticleOctree::Node::collapseSubtree: Copied too many particles, shit's on fire, yo");
	#endif
	
	/* Delete the node's children and install the new particle array: */
	delete[] children;
	particleIndices=newParticleIndices;
	
	/* Return the number of particles now in the leaf node: */
	return numChildParticles;
	}

ParticleOctree::Node::~Node(void)
	{
	/* Check if this is an interior node: */
	if(numParticles>maxParticlesPerNode)
		{
		/* Destroy the child nodes: */
		delete[] children;
		}
	else
		{
		/* Destroy the particle index array: */
		delete[] particleIndices;
		}
	}

void ParticleOctree::Node::addParticle(const ParticleSystem& particles,Index particleIndex,const Point& position)
	{
	#if PARTICLEOCTREE_DEBUGGING
	if(!isInside(position))
		throw std::runtime_error("ParticleOctreeNode::addParticle: Particle not inside node");
	#endif
	
	/* Check if this is either an interior node or a leaf node that needs to be split: */
	if(numParticles>=maxParticlesPerNode)
		{
		/* Check if this is a full leaf node: */
		if(numParticles==maxParticlesPerNode)
			{
			/* Split this node to make room for the new particle: */
			splitLeaf(particles);
			}
		
		/* Recurse into the child node whose domain contains the new particle: */
		int childIndex=getChildIndex(position);
		children[childIndex].addParticle(particles,particleIndex,position);
		}
	else
		{
		/* Add the particle to the node's particle array: */
		particleIndices[numParticles]=particleIndex;
		}
	
	/* Increase the number of particles in this node's subtree: */
	++numParticles;
	}

void ParticleOctree::Node::removeParticle(const ParticleSystem& particles,Index particleIndex,const Point& position)
	{
	/* Check if this is an interior node: */
	if(numParticles>maxParticlesPerNode)
		{
		/* Recurse into the child node whose domain contains the particle: */
		int childIndex=getChildIndex(position);
		children[childIndex].removeParticle(particles,particleIndex,position);
		
		/* Check if this node's subtree can be collapsed: */
		if(numParticles==maxParticlesPerNode+1)
			collapseSubtree();
		}
	else
		{
		/* Find the particle in this node's particle array: */
		Index* end=particleIndices+numParticles;
		Index* piPtr;
		for(piPtr=particleIndices;piPtr!=end&&*piPtr!=particleIndex;++piPtr)
			;
		if(piPtr==end)
			throw std::runtime_error("ParticleOctreeNode::removeParticle: Particle not found in node's particle list");
		
		/* Remove the particle by copying the last particle in the array to the found particle's spot: */
		*piPtr=end[-1];
		}
	
	/* Decrease the number of particles in this node's subtree: */
	--numParticles;
	}

void ParticleOctree::Node::updateParticles(const ParticleSystem& particles,std::vector<Index>& outOfDomainParticles)
	{
	/* Check if this is an interior node: */
	if(numParticles>maxParticlesPerNode)
		{
		/* Recurse into this node's children: */
		for(int childIndex=0;childIndex<8;++childIndex)
			children[childIndex].updateParticles(particles,outOfDomainParticles);
		
		/* Count the total number of particles remaining in this node's sub-tree: */
		numParticles=0;
		for(int childIndex=0;childIndex<8;++childIndex)
			numParticles+=children[childIndex].numParticles;
		
		/* Collapse this node's subtree if it has too few particles: */
		if(numParticles<=maxParticlesPerNode)
			collapseSubtree();
		}
	else
		{
		/* Test all particles against the boundaries of this leaf node and re-insert them up the tree if they are outside: */
		for(size_t i=0;i<numParticles;++i)
			{
			Index index=particleIndices[i];
			const Point& pos=particles.getParticlePosition(index);
			if(!isInside(pos))
				{
				/* Remove the particle from this leaf node: */
				--numParticles;
				particleIndices[i]=particleIndices[numParticles];
				
				/* Find the parent node whose domain includes the new particle position: */
				Node* particleParent=parent;
				while(particleParent!=0&&!particleParent->isInside(pos))
					particleParent=particleParent->parent;
				
				if(particleParent!=0)
					{
					/* Insert the particle into the found parent's sub-tree: */
					particleParent->addParticle(particles,index,pos);
					}
				else
					{
					/* The particle is now outside the octree's domain; punt it to the caller: */
					outOfDomainParticles.push_back(index);
					}
				
				--i;
				}
			}
		}
	}

#if PARTICLEOCTREE_DEBUGGING

void ParticleOctree::Node::checkTree(const ParticleSystem& particles) const
	{
	/* Check if this is an interior node: */
	if(numParticles>maxParticlesPerNode)
		{
		/* Check if all children point back to this node, are properly embedded into this node's domain, and contain the correct number of particles: */
		size_t childrenNumParticles=0;
		for(int childIndex=0;childIndex<8;++childIndex)
			{
			/* Check if the child node points back to this node: */
			if(children[childIndex].parent!=this)
				throw std::runtime_error("ParticleOctreeNode::checkTree: Child node does not point back to parent node");
			
			/* Calculate the domain of the child node: */
			Point cmin,cmax;
			for(int i=0;i<3;++i)
				{
				if(childIndex&(1<<i))
					{
					cmin[i]=center[i];
					cmax[i]=max[i];
					}
				else
					{
					cmin[i]=min[i];
					cmax[i]=center[i];
					}
				}
			
			if(children[childIndex].min!=cmin||children[childIndex].max!=cmax)
				throw std::runtime_error("ParticleOctreeNode::checkTree: Child node is not embedded into parent node");
			
			childrenNumParticles+=children[childIndex].numParticles;
			
			/* Check the child node's subtree for correctness: */
			children[childIndex].checkTree(particles);
			}
		if(numParticles!=childrenNumParticles)
			throw std::runtime_error("ParticleOctreeNode::checkTree: Number of particles in child nodes does not match number in parent node");
		}
	else
		{
		/* Check if all particles are contained inside the node's domain: */
		for(size_t particleIndex=0;particleIndex<numParticles;++particleIndex)
			{
			if(!isInside(particles.getParticlePosition(particleIndices[particleIndex])))
				throw std::runtime_error("ParticleOctreeNode::checkTree: Particle outside leaf node domain");
			}
		}
	}

#endif

void ParticleOctree::Node::glRenderAction(void) const
	{
	/* Check if this node is an interior node: */
	if(numParticles>maxParticlesPerNode)
		{
		/* Recurse into the children: */
		for(int childIndex=0;childIndex<8;++childIndex)
			children[childIndex].glRenderAction();
		}
	else
		{
		/* Draw this node's domain: */
		glBegin(GL_LINE_STRIP);
		glVertex(min[0],min[1],min[2]);
		glVertex(max[0],min[1],min[2]);
		glVertex(max[0],max[1],min[2]);
		glVertex(min[0],max[1],min[2]);
		glVertex(min[0],min[1],min[2]);
		glVertex(min[0],min[1],max[2]);
		glVertex(max[0],min[1],max[2]);
		glVertex(max[0],max[1],max[2]);
		glVertex(min[0],max[1],max[2]);
		glVertex(min[0],min[1],max[2]);
		glEnd();
		glBegin(GL_LINES);
		glVertex(max[0],min[1],min[2]);
		glVertex(max[0],min[1],max[2]);
		glVertex(max[0],max[1],min[2]);
		glVertex(max[0],max[1],max[2]);
		glVertex(min[0],max[1],min[2]);
		glVertex(min[0],max[1],max[2]);
		glEnd();
		}
	}

#if PARTICLEOCTREE_BARNES_HUT

void ParticleOctree::Node::updateCentersOfGravity(const ParticleSystem& particles)
	{
	/* Accumulate the node's center of gravity: */
	centerOfGravity=Point::origin;
	
	if(numParticles>maxParticlesPerNode)
		{
		/* Recurse into the node's children: */
		for(int childIndex=0;childIndex<8;++childIndex)
			{
			/* Calculate the child node's center of gravity: */
			children[childIndex].updateCentersOfGravity(particles);
			
			/* Accumulate into this node's center of gravity: */
			for(int i=0;i<3;++i)
				centerOfGravity[i]+=children[childIndex].centerOfGravity[i]*Scalar(children[childIndex].numParticles);
			}
		
		/* Normalize the accumulated center of gravity: */
		for(int i=0;i<3;++i)
			centerOfGravity[i]/=Scalar(numParticles);
		}
	else if(numParticles>0)
		{
		/* Calculate the center of gravity of this node's particles: */
		for(size_t index=0;index<numParticles;++index)
			{
			const Point& pos=particles.getParticlePosition(particleIndices[index]);
			for(int i=0;i<3;++i)
				centerOfGravity[i]+=pos[i];
			}
		
		/* Normalize the accumulated center of gravity: */
		for(int i=0;i<3;++i)
			centerOfGravity[i]/=Scalar(numParticles);
		}
	}

#endif

/*******************************
Methods of class ParticleOctree:
*******************************/

void ParticleOctree::tryShrink(void)
	{
	/* Check as long as the root node is an interior node: */
	while(root->numParticles>Node::maxParticlesPerNode)
		{
		/* Count how many of the root's children contain particles: */
		int numUsedChildren=0;
		Node* usedChild;
		for(int childIndex=0;childIndex<8;++childIndex)
			if(root->children[childIndex].numParticles>0)
				{
				++numUsedChildren;
				usedChild=&root->children[childIndex];
				}
		
		/* Bail out if the root has more than one child with particles: */
		if(numUsedChildren!=1)
			break;
		
		/* Make the root's only used child the root of the octree: */
		root->min=usedChild->min;
		root->max=usedChild->max;
		root->center=usedChild->center;
		
		/* The root's only used child must have as many particles as the root node; ergo, it is also an interior node: */
		Node* oldChildren=root->children;
		root->children=usedChild->children;
		usedChild->children=0;
		for(int childIndex=0;childIndex<8;++childIndex)
			root->children[childIndex].parent=root;
		delete[] oldChildren;
		}
	}

void ParticleOctree::setMaxParticlesPerNode(size_t newMaxParticlesPerNode)
	{
	/* Set the Node class's static element: */
	Node::maxParticlesPerNode=newMaxParticlesPerNode;
	}

ParticleOctree::ParticleOctree(const ParticleSystem& sParticles)
	:particles(sParticles),
	 root(0)
	{
	}

ParticleOctree::~ParticleOctree(void)
	{
	/* Delete the entire octree: */
	delete root;
	
	#if TESTING
	std::cout<<"Total tree update time: "<<totalTreeUpdateTime*1000.0<<" ms"<<std::endl;
	#endif
	}

void ParticleOctree::addParticle(Index particleIndex)
	{
	/* Get the new particle's position: */
	const Point& newPosition=particles.getParticlePosition(particleIndex);
	
	if(root==0)
		{
		/* Create a new octree around the particle: */
		Point min,max;
		for(int i=0;i<3;++i)
			{
			min[i]=Math::floor(newPosition[i]);
			max[i]=min[i]+Scalar(1);
			}
		root=new Node(0,min,max);
		}
	
	/* Enlarge the octree until the new particle is contained in its domain: */
	while(!root->isInside(newPosition))
		{
		/* Calculate the new root node domain: */
		Point min;
		Point max;
		Point center;
		int rootChildIndex=0;
		for(int i=0;i<3;++i)
			{
			/* Determine in which direction to grow the octree: */
			if(newPosition[i]>=root->center[i])
				{
				min[i]=root->min[i];
				max[i]=root->max[i]+(root->max[i]-root->min[i]);
				center[i]=root->max[i];
				}
			else
				{
				min[i]=root->min[i]-(root->max[i]-root->min[i]);
				max[i]=root->max[i];
				center[i]=root->min[i];
				rootChildIndex|=1<<i;
				}
			}
		
		/* Check if the root node is an interior node: */
		if(root->numParticles>Node::maxParticlesPerNode)
			{
			/* Create a new root node and its children: */
			Node* newRoot=new Node(0,min,max);
			newRoot->center=center;
			newRoot->splitLeaf(particles);
			
			/* Add the old root node as a child to the new root node: */
			newRoot->numParticles=root->numParticles;
			Node& rootChild=newRoot->children[rootChildIndex];
			rootChild.numParticles=root->numParticles;
			rootChild.children=root->children;
			for(int childIndex=0;childIndex<8;++childIndex)
				rootChild.children[childIndex].parent=&rootChild;
			
			/* Check if the old root node and the new root's child node have the exact same domain: */
			if(root->min!=rootChild.min||root->max!=rootChild.max)
				throw std::runtime_error("ParticleOctree::addParticle: Re-rooted octree domain does not match");
			
			/* Delete the old root node and install the new one: */
			root->children=0;
			delete root;
			root=newRoot;
			}
		else
			{
			/* Simply update the root node's domain: */
			root->min=min;
			root->max=max;
			root->center=center;
			}
		}
	
	/* Add the particle to the octree: */
	root->addParticle(particles,particleIndex,newPosition);
	
	#if PARTICLEOCTREE_DEBUGGING
	/* Check the tree for consistency: */
	root->checkTree(particles);
	#endif
	}

void ParticleOctree::removeParticle(Index particleIndex)
	{
	if(root==0)
		throw std::runtime_error("ParticleOctree::removeParticle: Octree is empty");
	
	/* Remove the particle from the root node: */
	root->removeParticle(particles,particleIndex,particles.getParticlePosition(particleIndex));
	
	/* Try shrinking the octree: */
	tryShrink();
	
	#if PARTICLEOCTREE_DEBUGGING
	/* Check the tree for consistency: */
	root->checkTree(particles);
	#endif
	}

void ParticleOctree::finishUpdate(void)
	{
	if(root==0)
		return;
	
	#if PARTICLEOCTREE_BARNES_HUT
	/* Recalculate the centers of gravity of all octree nodes: */
	root->updateCentersOfGravity(particles);
	#endif
	}

void ParticleOctree::updateParticles(void)
	{
	if(root==0)
		return;
	
	#if TESTING
	Realtime::TimePointMonotonic now;
	#endif
	
	/* Update the entire octree: */
	std::vector<Index> outOfDomainParticles;
	root->updateParticles(particles,outOfDomainParticles);
	
	/* Re-insert the out-of-domain particles back into the octree, enlarging the root node: */
	for(std::vector<Index>::iterator oodpIt=outOfDomainParticles.begin();oodpIt!=outOfDomainParticles.end();++oodpIt)
		addParticle(*oodpIt);
	
	/* Try shrinking the octree: */
	tryShrink();
	
	#if PARTICLEOCTREE_BARNES_HUT
	/* Recalculate the centers of gravity of all octree nodes: */
	root->updateCentersOfGravity(particles);
	#endif
	
	#if TESTING
	totalTreeUpdateTime+=now.setAndDiff();
	#endif
	
	#if PARTICLEOCTREE_DEBUGGING
	/* Check the tree for consistency: */
	root->checkTree(particles);
	#endif
	}

void ParticleOctree::glRenderAction(void) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	/* Render the entire octree: */
	if(root!=0)
		root->glRenderAction();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

#if PARTICLEOCTREE_BARNES_HUT

const Point& ParticleOctree::getCenterOfGravity(void) const
	{
	if(root==0)
		throw std::runtime_error("ParticleOctree::getCenterOfGravity: Octree is empty");
	
	return root->centerOfGravity;
	}

#endif
