#ifndef LLOYD_H
#define LLOYD_H


#include "Voronoi.h"

/// Релаксация методом Ллойда.
std::vector<glm::vec2> Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size);


#endif // LLOYD_H
