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

#ifndef CREATENODELABEL_INCLUDED
#define CREATENODELABEL_INCLUDED

#include <SceneGraph/OGTransformNode.h>

#include "JsonMap.h"

/* Forward declarations: */
namespace SceneGraph {
class FancyFontStyleNode;
}

SceneGraph::OGTransformNodePointer createNodeLabel(const JsonMap::Map& nodeProperties,unsigned int numLinks,SceneGraph::FancyFontStyleNode& fontStyle);

#endif
