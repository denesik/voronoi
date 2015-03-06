#include "Lloyd.h"
#include "gif-h/gif.h"
#include "image.h"

#include <algorithm>

std::vector<glm::vec2> Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size)
{
  // Строим диаграмму
  Voronoi voronoi(sites, size);
  voronoi();


  // Подготавливаем массив для заполнения полигонов.
  std::vector<std::vector<unsigned int> > listVertex;
  listVertex.reserve(sites.size());
  for(unsigned int i = 0; i < sites.size(); ++i)
  {
    // Резервируем память под 9 вершин (среднее значение с запасом),
    // однако каждая вершина содержиттся в 2-х гранях, которые относятся к данному
    // полигону, поэтому резервируем в 2 раза больше памяти.
    listVertex.emplace_back();
    listVertex.back().reserve(9 * 2);
  }


  // Проходим по всем граням и добавляем вершины соответствующим точкам.
  // Каждая вершина продублируется 2 раза, но это не страшно.
  auto edges = voronoi.GetEdges();
  for(auto it = edges.begin(); it != edges.end(); ++it)
  {
    const Voronoi::Edge &edge = (*it);

    listVertex[edge.site1].push_back(edge.vertex1);
    listVertex[edge.site1].push_back(edge.vertex2);
    listVertex[edge.site2].push_back(edge.vertex1);
    listVertex[edge.site2].push_back(edge.vertex2);
  }

  // Вычисляем новые значения точек.
  auto const &vertex = voronoi.GetVertex();
  std::vector<glm::vec2> listSites(sites);
  listSites.reserve(sites.size());
  for(unsigned int j = 0; j < listVertex.size(); ++j)
  {
    auto const &poligon = listVertex[j];
    if(poligon.empty())
    {
      continue;
    }
    glm::vec2 point;
    for(auto jt = poligon.begin(); jt != poligon.end(); ++jt)
    {
      point += vertex[*jt];
    }
    point /= static_cast<float>(poligon.size());
    point += glm::vec2(rand() % 7 - 3, rand() % 7 - 3);
    //point = (point * 3.0f + sites[j]) / 4.0f;
    listSites[j] = point;
  }

  return std::move(listSites);
}
