#ifndef LLOYD_H
#define LLOYD_H

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include "Voronoi.h"

struct LloydPredicateDefault
{
  glm::vec2 operator()(const glm::vec2 &, const std::vector<unsigned int> &poligon, const std::vector<glm::vec2> &vertex)
  {
    glm::vec2 point;
    for(auto jt = poligon.begin(); jt != poligon.end(); ++jt)
    {
      point += vertex[*jt];
    }
    point /= static_cast<float>(poligon.size());
    return point;
  }
};

/// Релаксация методом Ллойда.
/// @param sites Список точек.
/// @param size Размер органичивающей области.
/// @return Список точек после одной итерации релаксации Ллойда.
template<class Predicate>
std::vector<glm::vec2> Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size, Predicate predicate = Predicate())
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
    // однако каждая вершина содержится в 2-х гранях, которые относятся к данному
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

    listSites[j] = predicate(sites[j], poligon, vertex);
  }

  return std::move(listSites);
}

/// Релаксация методом Ллойда.
/// @param sites Список точек.
/// @param size Размер органичивающей области.
/// @return Список точек после одной итерации релаксации Ллойда.
std::vector<glm::vec2> Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size)
{
  return Lloyd(sites, size, LloydPredicateDefault());
}


#endif // LLOYD_H



