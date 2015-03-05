#include "Lloyd.h"
#include "gif-h/gif.h"
#include "image.h"

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

    // Строим список вершин для каждой точки.
    listVertex.resize(mListSite.size());
    auto edges = voronoi.GetEdges();

    for(auto it = edges.begin(); it != edges.end(); ++it)
    {
      const Voronoi::Edge &edge = (*it);

      unsigned int edgeSites[2] = {edge.site1, edge.site2};

      for(unsigned int i = 0; i < 2; ++i)
      {
        bool v1 = true;
        bool v2 = true;
        for(auto jt = listVertex[edgeSites[i]].begin(); jt != listVertex[edgeSites[i]].end(); ++jt)
        {
          if((*jt) == edge.vertex1)
          {
            v1 = false;
          }
          if((*jt) == edge.vertex2)
          {
            v2 = false;
          }
        }
        if(v1)
        {
          listVertex[edgeSites[i]].push_back(edge.vertex1);
        }
        if(v2)
        {
          listVertex[edgeSites[i]].push_back(edge.vertex2);
        }
      }
    }

    auto vertex = voronoi.GetVertex();
    // Вычисляем новые значения точек.
    for(unsigned int i = 0; i < listVertex.size(); ++i)
    {
      assert(!listVertex[i].empty());
      glm::vec2 point;
      for(auto jt = listVertex[i].begin(); jt != listVertex[i].end(); ++jt)
      {
        point += vertex[*jt];
      }
      mListSite[i] = point / static_cast<float>(listVertex[i].size());
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
