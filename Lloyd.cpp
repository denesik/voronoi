#include "Lloyd.h"
#include "gif-h/gif.h"
#include "image.h"

#include <algorithm>

Lloyd::Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size, unsigned int iteration)
 : mListSite(sites)
{
  std::vector<std::vector<unsigned int> > listVertex;

  GifWriter gw;
  GifBegin(&gw, "voron.gif", size.x + 1, size.y + 1, 50);

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

    //рисуем гиф
    Image image;
    image.Resize(size.x + 1, size.y + 1);
    image.Fill(0xFFFFFFFF);

    const std::vector<Voronoi::Edge> &edge = voronoi.GetEdges();
    for(auto it = edge.begin(); it != edge.end(); ++it)
    {
      const glm::vec2 &p1 = vertex[(*it).vertex1];
      const glm::vec2 &p2 = vertex[(*it).vertex2];
      image.DrawLine(p1, p2, 0x00FF00FF);
    }

    GifWriteFrame(&gw, &image.Raw()[0], size.x + 1, size.y + 1, 50);
  }
  GifEnd(&gw);
}

Lloyd::~Lloyd()
{

}

std::vector<glm::vec2> &Lloyd::GetSites()
{
  return mListSite;
}
