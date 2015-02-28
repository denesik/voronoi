#include "Voronoi.h"
#include "image.h"
#include "geometry.h"

#include <iostream>
#include <stdlib.h>
#include <ctime>
#include <algorithm>
#include <iterator>

float get_msec(){
    return clock() / static_cast<float>(CLOCKS_PER_SEC/10);
}

int main()
{
  {

    glm::uvec2 size(10000, 10000);

    std::set<glm::vec2, Voronoi::SiteComparator> points;

    printf("%7gms Start generate\n", get_msec());

    unsigned int seed = static_cast<unsigned int>(time(NULL));
    srand(seed);
    printf("%7gms Seed: %i\n", get_msec(), seed);

    struct Generator
    {
      const glm::uvec2 size;
      Generator(const glm::uvec2 &s)
        : size(s)
      {}
      glm::vec2 operator()() {return glm::vec2(rand() % size.x, rand() % size.y);}
    } generator(size);

    std::generate_n(std::inserter(points, points.end()), 1000000, generator);

    printf("%7gms End generate, Count: %i\n", get_msec(), static_cast<int>(points.size()));

    printf("%7gms Start Voronoi\n", get_msec());
    Voronoi v(points, size);
    printf("%7gms End Voronoi\n", get_msec());
    v.Sort();
    auto diagram = v.GetDiagram();
    printf("%7gms Start drawing\n", get_msec());

    Image image;
    image.Resize(size.x + 1, size.y + 1);
    image.Fill(0xFFFFFFFF);

    for(auto it = diagram.begin(); it != diagram.end(); ++it)
    {
      auto jt = (*it).second.begin();
      ++jt;
      auto prev = jt;
      for(; jt != (*it).second.end(); ++jt)
      {
        prev = jt;
        --prev;
        glm::vec2 p1 = (*prev) + (*it).first;
        glm::vec2 p2 = (*jt) + (*it).first;
        image.DrawLine(p1, p2, 0x00FF00FF);
      }
      prev = (*it).second.end();
      --prev;
      jt = (*it).second.begin();
      glm::vec2 p1 = (*prev) + (*it).first;
      glm::vec2 p2 = (*jt) + (*it).first;
      image.DrawLine(p1, p2, 0x00FF00FF);
    }

    printf("%7gms Start saving\n", get_msec());
    for(auto it = diagram.begin(); it != diagram.end();)
    {
      auto range = diagram.equal_range((*it).first);
      image.DrawPoint((*it).first, 0xFF0000FF);

      it = range.second;
    }


    image.Save("img.png");
  }

  printf("%7gms End\n", get_msec());
  return 0;
}


