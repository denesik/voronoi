#include "Voronoi.h"
#include "image.h"
#include "geometry.h"

#include <stdlib.h>
#include <ctime>
#include <algorithm>
#include <iterator>

float get_msec(){
    return clock() / static_cast<float>(CLOCKS_PER_SEC);
}

struct SiteComparator
{
  bool operator()(const glm::vec2 &a, const glm::vec2 &b) const
  {
    if(a.y == b.y)
      return a.x > b.x;
    return a.y > b.y;
  }
};

void Generate(std::vector<glm::vec2> &points, const glm::uvec2 &size)
{
  std::set<glm::vec2, SiteComparator> p;

  printf("%7gs Start generate\n", get_msec());

  unsigned int seed = static_cast<unsigned int>(time(NULL));
  srand(seed);
  printf("%7gs Seed: %i\n", get_msec(), seed);

  struct Generator
  {
    const glm::uvec2 size;
    Generator(const glm::uvec2 &s)
      : size(s)
    {}
    glm::vec2 operator()() {return glm::vec2(rand() % size.x, rand() % size.y);}
  } generator(size);

  std::generate_n(std::inserter(p, p.end()), 1000000, generator);

  points.reserve(p.size());
  points.insert(points.end(), p.begin(), p.end());
}

int main()
{
    glm::uvec2 size(10000, 10000);

    std::vector<glm::vec2> points;
    Generate(points, size);
/*
    points.push_back(glm::vec2(20, 20));
    points.push_back(glm::vec2(40, 60));
    points.push_back(glm::vec2(60, 40));
    points.push_back(glm::vec2(80, 80));
    points.push_back(glm::vec2(20, 80));
*/
    printf("%7gs End generate, Count: %i\n", get_msec(), static_cast<int>(points.size()));

    printf("%7gs Start Voronoi\n", get_msec());
    Voronoi v(points, size);
    printf("%7gs End Voronoi\n", get_msec());

    const std::vector<glm::vec2> &vertex = v.GetVertex();
    const std::vector<Voronoi::Edge> &edge = v.GetEdges();

    printf("%7gms Start drawing\n", get_msec());

    Image image;
    image.Resize(size.x + 1, size.y + 1);
    image.Fill(0xFFFFFFFF);

    for(auto it = edge.begin(); it != edge.end(); ++it)
    {
      glm::vec2 p1 = vertex[(*it).vertex1];
      glm::vec2 p2 = vertex[(*it).vertex2];
      image.DrawLine(p1, p2, 0x00FF00FF);
    }

    for(auto it = edge.begin(); it != edge.end(); ++it)
    {
      glm::vec2 p1 = points[(*it).site1];
      glm::vec2 p2 = points[(*it).site2];
      image.DrawPoint(p1, 0xFF0000FF);
      image.DrawPoint(p2, 0xFF0000FF);
    }

    printf("%7gms Start saving\n", get_msec());

    image.Save("img.png");

    printf("%7gs End\n", get_msec());

  //system("pause");
  return 0;
}

