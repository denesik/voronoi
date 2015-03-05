#include "Lloyd.h"

#include <algorithm>

Lloyd::Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size, unsigned int iteration)
 : mListSite(sites)
{
  std::vector<std::vector<unsigned int> > listVertex;

  for(unsigned int i = 0; i < iteration; ++i)
  {
    // Строим диаграмму
    Voronoi voronoi(mListSite, size);
    listVertex.clear();
    listVertex.resize(mListSite.size());
    mListSite.clear();

    // Строим список вершин для каждой точки.
    auto edges = voronoi.GetEdges();

    for(auto it = edges.begin(); it != edges.end(); ++it)
    {
      const Voronoi::Edge &edge = (*it);

      if(std::find(listVertex[edge.site1].begin(), listVertex[edge.site1].end(), edge.vertex1) ==
         listVertex[edge.site1].end())
      {
        listVertex[edge.site1].push_back(edge.vertex1);
      }
      if(std::find(listVertex[edge.site1].begin(), listVertex[edge.site1].end(), edge.vertex2) ==
         listVertex[edge.site1].end())
      {
        listVertex[edge.site1].push_back(edge.vertex2);
      }
      if(std::find(listVertex[edge.site2].begin(), listVertex[edge.site2].end(), edge.vertex1) ==
         listVertex[edge.site2].end())
      {
        listVertex[edge.site2].push_back(edge.vertex1);
      }
      if(std::find(listVertex[edge.site2].begin(), listVertex[edge.site2].end(), edge.vertex2) ==
         listVertex[edge.site2].end())
      {
        listVertex[edge.site2].push_back(edge.vertex2);
      }
    }


    auto vertex = voronoi.GetVertex();
    // Вычисляем новые значения точек.
    for(unsigned int j = 0; j < listVertex.size(); ++j)
    {
      auto const &poligon = listVertex[j];
      assert(!poligon.empty());
      glm::vec2 point;
      for(auto jt = poligon.begin(); jt != poligon.end(); ++jt)
      {
        point += vertex[*jt];
      }
      mListSite.push_back(point / static_cast<float>(poligon.size()));
    }
  }
}

Lloyd::~Lloyd()
{

}

std::vector<glm::vec2> &Lloyd::GetSites()
{
  return mListSite;
}
