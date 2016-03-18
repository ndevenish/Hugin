// -*- c-basic-offset: 4 -*-

/** @file ImageGraph.cpp
 *
 *  @brief implementation of ImageGraph Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>, T. Modes
 *
 *  $Id: ImageGraph.cpp 1763 2006-12-17 21:11:57Z dangelo $
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ImageGraph.h"
#include <queue>

namespace HuginGraph
{
/* build adjacency list for all images in pano */
ImageGraph::ImageGraph(const HuginBase::PanoramaData& pano, bool ignoreLinkedPosition)
{
    if (pano.getNrOfImages() > 0)
    {
        m_graph.resize(pano.getNrOfImages());
        if (!ignoreLinkedPosition)
        {
            // handle all linked positions
            for (size_t i = 0; i < pano.getNrOfImages(); ++i)
            {
                const HuginBase::SrcPanoImage& image = pano.getImage(i);
                if (image.YawisLinked())
                {
                    for (size_t j = i + 1; j < pano.getNrOfImages(); ++j)
                    {
                        if (image.YawisLinkedWith(pano.getImage(j)))
                        {
                            m_graph[i].insert(j);
                            m_graph[j].insert(i);
                        };
                    };
                };
            };
        };
        // and now all control points
        const HuginBase::CPVector& cps = pano.getCtrlPoints();
        for (size_t i = 0; i < cps.size(); ++i)
        {
            if (cps[i].mode == HuginBase::ControlPoint::X_Y && cps[i].image1Nr != cps[i].image2Nr)
            {
                m_graph[cps[i].image1Nr].insert(cps[i].image2Nr);
                m_graph[cps[i].image2Nr].insert(cps[i].image1Nr);
            }
        }
    };
};

ImageGraph::ImageGraph(const HuginBase::CalculateImageOverlap& overlap)
{
    const unsigned int nrImages = overlap.getNrOfImages();
    m_graph.resize(nrImages);
    // and now all control points
    for (size_t i = 0; i < nrImages-1; ++i)
    {
        for (size_t j = i + 1; j < nrImages; ++j)
        {
            if (overlap.getOverlap(i, j)>0.001)
            {
                m_graph[i].insert(j);
                m_graph[j].insert(i);
            }
        };
    };
};

template<typename VALUETYPE>
void DepthFirstSearch(const ImageGraph::GraphList& graph, std::vector<VALUETYPE>& marks, const size_t vertex, const VALUETYPE setType, const VALUETYPE unvisitedType)
{
    marks[vertex] = setType;
    for (HuginBase::UIntSet::const_iterator it = graph[vertex].begin(); it != graph[vertex].end(); ++it)
    {
        if (marks[*it] == unvisitedType)
        {
            DepthFirstSearch(graph, marks, *it, setType, unvisitedType);
        };
    };
};

ImageGraph::Components ImageGraph::GetComponents()
{
    ImageGraph::Components comp;
    if (m_graph.empty())
    {
        return comp;
    };
    // and now the depth first search algorithm
    std::vector<size_t> marks(m_graph.size(), 0);
    size_t counter = 0;
    for (size_t i = 0; i < m_graph.size(); ++i)
    {
        if (marks[i] == 0)
        {
            counter++;
            DepthFirstSearch<size_t>(m_graph, marks, i, counter, 0);
        };
    };
    // now create the connected components as vector<UIntSet>
    comp.resize(counter);
    for (size_t imgNr = 0; imgNr < marks.size(); ++imgNr)
    {
        comp[marks[imgNr] - 1].insert(imgNr);
    }
    return comp;
};

bool ImageGraph::IsConnected()
{
    if (m_graph.empty())
    {
        return false;
    };
    // and now the depth first search algorithm
    std::vector<bool> visited(m_graph.size(), false);
    DepthFirstSearch(m_graph, visited, 0, true, false);
    for (std::vector<bool>::const_iterator it = visited.begin(); it != visited.end(); ++it)
    {
        if (!(*it))
        {
            return false;
        }
    }
    return true;
};

void BreadthFirstSearchVisit(const ImageGraph::GraphList& graph,
    std::queue<size_t>& queue, std::vector<bool>& visited, BreadthFirstSearchVisitor* visitor)
{
    while (!queue.empty())
    {
        const size_t vertex = queue.front();
        queue.pop();
        if (!visited[vertex])
        {
            visited[vertex] = true;
            HuginBase::UIntSet visitedNeighbors;
            HuginBase::UIntSet unvisitedNeighbors;
            for (HuginBase::UIntSet::const_iterator it = graph[vertex].begin(); it != graph[vertex].end(); ++it)
            {
                if (visited[*it])
                {
                    visitedNeighbors.insert(*it);
                }
                else
                {
                    unvisitedNeighbors.insert(*it);
                    queue.push(*it);
                };
            };
            visitor->Visit(vertex, visitedNeighbors, unvisitedNeighbors);
        };
    };
}

void ImageGraph::VisitAllImages(const size_t startImg, bool forceAllComponents, BreadthFirstSearchVisitor* visitor)
{
    if (m_graph.empty())
    {
        return;
    }
    // range checking, just in case
    const size_t realStartImg = (startImg >= m_graph.size()) ? 0 : startImg;
    std::vector<bool> visited(m_graph.size(), false);
    std::queue<size_t> queue;
    // go down the graph starting from the startImg
    queue.push(realStartImg);
    BreadthFirstSearchVisit(m_graph, queue, visited, visitor);
    if (forceAllComponents)
    {
        // if the graph contains several components
        // we have not yet visited all images, so
        // restart the breadth first algorithm from the new component start
        for (size_t i = 0; i < m_graph.size(); ++i)
        {
            if (!visited[i])
            {
                queue.push(i);
                BreadthFirstSearchVisit(m_graph, queue, visited, visitor);
            };
        };
    };
};

}  // namespace HuginGraph
