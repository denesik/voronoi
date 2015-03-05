#ifndef LLOYD_H
#define LLOYD_H


#include "Voronoi.h"

/// Релаксация методом Ллойда.
class Lloyd
{
public:
  Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size, unsigned int iteration);
  ~Lloyd();

  std::vector<glm::vec2> &GetSites();

private:

  /// Список точек.
  std::vector<glm::vec2> mListSite;

};

#endif // LLOYD_H
