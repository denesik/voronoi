#ifndef LLOYD_H
#define LLOYD_H
#include <vector>
#include <glm/glm.hpp>

/// Релаксация методом Ллойда.
/// @param sites Список точек.
/// @param size Размер органичивающей области.
/// @return Список точек после одной итерации релаксации Ллойда.
std::vector<glm::vec2> Lloyd(const std::vector<glm::vec2> &sites, const glm::vec2 &size);


#endif // LLOYD_H
