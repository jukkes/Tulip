/**
 *
 * This file is part of Tulip (www.tulip-software.org)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
#include <cmath>
#include <algorithm>
#include <tulip/GraphTools.h>
#include "TreeTools.h"
#include "Orientation.h"
#include "DatasetTools.h"
#include "MyPlugin.h"
using namespace std;
using namespace tlp;


PLUGIN(MyPlugin)

//====================================================================
MyPlugin::MyPlugin(const tlp::PluginContext* context)
:LayoutAlgorithm(context) {
  addNodeSizePropertyParameter(this);
  addOrientationParameters(this);
  addOrthogonalParameters(this);
  addSpacingParameters(this);
}

//====================================================================
MyPlugin::~MyPlugin() {
}
//====================================================================
void MyPlugin::computeLevelHeights(tlp::Graph *tree, tlp::node n, unsigned int depth,
                                     OrientableSizeProxy *oriSize) {
  if (levelHeights.size() == depth)
    levelHeights.push_back(0);

  float nodeHeight = oriSize->getNodeValue(n).getH();

  if (nodeHeight > levelHeights[depth])
    levelHeights[depth] = nodeHeight;

  node on;
  forEach(on, tree->getOutNodes(n))
    computeLevelHeights(tree, on, depth + 1, oriSize);
}
//====================================================================
bool MyPlugin::run() {
  orientationType mask = getMask(dataSet);
  OrientableLayout *oriLayout = new OrientableLayout(result, mask);
  SizeProperty* size;

  if (!getNodeSizePropertyParameter(dataSet, size))
    size = graph->getProperty<SizeProperty>("viewSize");


  OrientableSizeProxy oriSize(size, mask);
  getSpacingParameters(dataSet, nodeSpacing, spacing);

  if (pluginProgress)
    pluginProgress->showPreview(false);

  // push a temporary graph state (not redoable)
  // preserving layout updates
  std::vector<PropertyInterface*> propsToPreserve;

  if (result->getName() != "")
    propsToPreserve.push_back(result);

  graph->push(false, &propsToPreserve);

  tree = TreeTest::computeTree(graph, pluginProgress);

  if (pluginProgress && pluginProgress->state() != TLP_CONTINUE) {
    graph->pop();
    return false;
  }

  root = tree->getSource();
  computeLevelHeights(tree, root, 0, &oriSize);

  // check if the specified layer spacing is greater
  // than the max of the minimum layer spacing of the tree
  for (unsigned int i = 0; i < levelHeights.size() - 1;  ++i) {
    float minLayerSpacing = (levelHeights[i] + levelHeights[i + 1]) / 2;

    if (minLayerSpacing + nodeSpacing > spacing)
      spacing = minLayerSpacing + nodeSpacing;
  }

  IntegerProperty *positionNode = tree->getProperty<IntegerProperty>("position");
  Iterator<node> *itNode =  tree->getNodes();





   while (itNode->hasNext()) {
        node currentNode   = itNode->next();
        OrientableCoord coord   =  oriLayout->getNodeValue(currentNode);
//        if (isLeaf(tree, currentNode)) {

            coord.setX(positionNode->getNodeValue(currentNode) * nodeSpacing / 10000.);
            oriLayout->setNodeValue(currentNode, coord);
//        } else {
//            float posX = computeFatherXPosition(currentNode, oriLayout);
//            coord.setX(posX);
//            oriLayout->setNodeValue(currentNode, coord);
//        }
   }

//  setAllNodesCoordXBis(root, 0.f, oriLayout, &oriSize);
//  shiftAllNodes(root, 0.f, oriLayout);
  setAllNodesCoordYBis(oriLayout, &oriSize);

  if (hasOrthogonalEdge(dataSet))
    oriLayout->setOrthogonalEdge(graph, spacing);


  // forget last temporary graph state
  graph->pop();

  delete oriLayout;
  return true;
}

//====================================================================
float MyPlugin::setAllNodesCoordX(tlp::node n, float rightMargin,
                                    OrientableLayout *oriLayout, OrientableSizeProxy *oriSize) {
  float leftMargin       = rightMargin;

  Iterator<node>* itNode = tree->getOutNodes(n);

  while (itNode->hasNext()) {
    node currentNode   = itNode->next();
    leftMargin         = setAllNodesCoordX(currentNode, leftMargin, oriLayout, oriSize);
  }

  delete itNode;

  const float nodeWidth  =  oriSize->getNodeValue(n).getW()
    + nodeSpacing;

  if (isLeaf(tree, n))
    leftMargin = rightMargin + nodeWidth;

  const float freeRange  = leftMargin - rightMargin;

  float posX;

  if (isLeaf(tree, n))
    posX = freeRange / 2.f + rightMargin;
  else
    posX = computeFatherXPosition(n, oriLayout);

  const float rightOverflow = max(rightMargin-(posX-nodeWidth/2.f), 0.f);
  const float leftOverflow  = max((posX+nodeWidth/2.f)-leftMargin, 0.f);
  leftshift[n]              = rightOverflow;

  setNodePosition(n, posX, 0.f, 0.f, oriLayout);
  return  leftMargin + leftOverflow + rightOverflow;
}

//void MyPlugin::setAllNodesCoordXBis(tlp::node n,
//                                    OrientableLayout *oriLayout, OrientableSizeProxy *oriSize) {

//  Iterator<node>* itNode = tree->getOutNodes(n);

//  while (itNode->hasNext()) {
//    node currentNode   = itNode->next();
//    setAllNodesCoordX(currentNode, oriLayout, oriSize);
//  }

//  delete itNode;



//  if (!isLeaf(tree, n)) {
//    float posX = computeFatherXPosition(n, oriLayout);
//    setNodePosition(n, posX, 0.f, 0.f, oriLayout);
//  }
//}

//====================================================================
void MyPlugin::setAllNodesCoordY(OrientableLayout *oriLayout,
                                   OrientableSizeProxy *oriSize) {
  float maxYLeaf         = -FLT_MAX;
  setCoordY(root, maxYLeaf, oriLayout, oriSize);

  Iterator<node>* itNode = tree->getNodes();

  while (itNode->hasNext()) {
    node currentNode   = itNode->next();

    if (isLeaf(tree,currentNode)) {
      OrientableCoord coord = oriLayout->getNodeValue(currentNode);
      float newY            = maxYLeaf;
      float coordX          = coord.getX();
      float coordZ          = coord.getZ();
      setNodePosition(currentNode, coordX, newY, coordZ, oriLayout);
    }
  }

  delete itNode;
}

void MyPlugin::setAllNodesCoordYBis(OrientableLayout *oriLayout,
				      OrientableSizeProxy *oriSize) {

  IntegerProperty *level = tree->getProperty<IntegerProperty>("level");

  float maxYLeaf         = 0;
  //  setCoordY(root, maxYLeaf, oriLayout, oriSize);

  Iterator<node>* itNode = tree->getNodes();

  while (itNode->hasNext()) {
    node currentNode   = itNode->next();

    //    if (isLeaf(tree,currentNode)) {
    OrientableCoord coord = oriLayout->getNodeValue(currentNode);
    float newY            = maxYLeaf - level->getNodeValue(currentNode) * spacing;
    float coordX          = coord.getX();
    float coordZ          = coord.getZ();
    setNodePosition(currentNode, coordX, newY, coordZ, oriLayout);
    //    }
  }

  delete itNode;
}


//====================================================================
float MyPlugin::computeFatherXPosition(tlp::node father, OrientableLayout *oriLayout) {
  float minX             =  FLT_MAX;
  float maxX             = -FLT_MAX;

  Iterator<node> *itNode =  tree->getOutNodes(father);

  while (itNode->hasNext()) {
    node currentNode   = itNode->next();
    const float x      =  oriLayout->getNodeValue(currentNode).getX()
      + leftshift[currentNode];
    minX               = min(minX, x);
    maxX               = max(maxX, x);
  }

  delete itNode;
  return (maxX + minX) / 2.f;
}

//====================================================================
void MyPlugin::shiftAllNodes(tlp::node n, float shift, OrientableLayout *oriLayout) {
  OrientableCoord coord   =  oriLayout->getNodeValue(n);
  shift                  +=  leftshift[n];
  float coordX            =  coord.getX();

  coord.setX(coordX + shift);


  oriLayout->setNodeValue(n, coord);

  Iterator<node>* itNode  =   tree->getOutNodes(n);

  while (itNode->hasNext())
    shiftAllNodes(itNode->next(), shift, oriLayout);

  delete itNode;
}

//====================================================================
inline void MyPlugin::setNodePosition(tlp::node n, float x, float y,
                                        float z, OrientableLayout *oriLayout) {
  OrientableCoord coord = oriLayout->createCoord(x, y, z);
  oriLayout->setNodeValue(n, coord);
}

//====================================================================
void MyPlugin::setCoordY(tlp::node n, float& maxYLeaf,
                           OrientableLayout *oriLayout, OrientableSizeProxy *oriSize) {
  if (tree->indeg(n) != 0) {
    node fatherNode             = tree->getInNode(n, 1);
    OrientableCoord coord       = oriLayout->getNodeValue(n);
    OrientableCoord coordFather = oriLayout->getNodeValue(fatherNode);
    float nodeY                 = coordFather.getY() + spacing;

    coord.setY(nodeY);
    oriLayout->setNodeValue(n, coord);

    if (isLeaf(tree, n)) {
      maxYLeaf = max(maxYLeaf, nodeY);
    }
  }

  Iterator<node> *itNode = tree->getOutNodes(n);

  while (itNode->hasNext())
    setCoordY(itNode->next(), maxYLeaf, oriLayout, oriSize);

  delete itNode;
}
