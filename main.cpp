#include "Voronoi.h"
#include "image.h"
#include "gif-h/gif.h"
#include "geometry.h"
#include "Lloyd.h"

#include <stdlib.h>
#include <ctime>
#include <algorithm>
#include <iterator>

float get_msec(){
    return clock() / static_cast<float>(CLOCKS_PER_SEC);
}

std::vector<glm::vec2> Generate(const unsigned int count, const glm::uvec2 &size)
{
  printf("%7gs Start generate\n", get_msec());

  std::vector<glm::vec2> points;
  points.reserve(count);

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

  std::generate_n(std::back_inserter(points), count, generator);


  std::sort(points.begin(), points.end(), 
    [](const glm::vec2 &p1, const glm::vec2 &p2) -> bool
  {
    if(p1.y == p2.y)
      return p1.x > p2.x;
    return p1.y > p2.y;
  });

  auto it = std::unique(points.begin(), points.end(), 
    [](const glm::vec2 &p1, const glm::vec2 &p2)
    {
      return p1.x == p2.x && p1.y == p2.y;
    });   

  points.resize(std::distance(points.begin(), it));

  return std::move(points);
}


std::vector<glm::vec2> LloidGenerate(const unsigned int count, const glm::uvec2 &pos, const glm::uvec2 &size)
{
  printf("%7gs Start generate\n", get_msec());

  std::vector<glm::vec2> points;
  points.reserve(count);

  unsigned int seed = static_cast<unsigned int>(time(NULL));
  srand(seed);
  printf("%7gs Seed: %i\n", get_msec(), seed);

  struct Generator
  {
    const glm::uvec2 pos;
    const glm::uvec2 size;
    Generator(const glm::uvec2 &p, const glm::uvec2 &s)
      : pos(p), size(s)
    {}
    glm::vec2 operator()()
    {
      return glm::vec2(pos.x + rand() % size.x + (((rand() % 100) - 50) / 100.0f),
                       pos.y + rand() % size.y + (((rand() % 100) - 50) / 100.0f));
    }
  } generator(pos, size);
  std::generate_n(std::back_inserter(points), count, generator);


  std::sort(points.begin(), points.end(),
    [](const glm::vec2 &p1, const glm::vec2 &p2) -> bool
  {
    if(p1.y == p2.y)
      return p1.x > p2.x;
    return p1.y > p2.y;
  });

  auto it = std::unique(points.begin(), points.end(),
    [](const glm::vec2 &p1, const glm::vec2 &p2)
    {
      return p1.x == p2.x && p1.y == p2.y;
    });

  points.resize(std::distance(points.begin(), it));

  return std::move(points);
}

int main()
{
  glm::uvec2 size(400, 400);

  std::vector<glm::vec2> points;
  //points = Generate(100000, size);
  points = LloidGenerate(200, glm::uvec2(200, 200), glm::uvec2(5, 5));

  printf("%7gs End generate, Count: %i\n", get_msec(), static_cast<int>(points.size()));

  GifWriter gw;
  GifBegin(&gw, "voron.gif", size.x + 1, size.y + 1, 50);
  for(unsigned int i = 0; i < 800; ++i)
  {
    points = Lloyd(points, size);

    // Рисуем анимацию.
    Voronoi diagram(points, size);
    diagram();
    const std::vector<glm::vec2> &vertex = diagram.GetVertex();
    const std::vector<Voronoi::Edge> &edge = diagram.GetEdges();

    Image image;
    image.Resize(size.x + 1, size.y + 1);
    image.Fill(0xFFFFFFFF);

    for(auto it = edge.begin(); it != edge.end(); ++it)
    {
      const glm::vec2 &p1 = vertex[(*it).vertex1];
      const glm::vec2 &p2 = vertex[(*it).vertex2];
      image.DrawLine(p1, p2, 0x00FF00FF);
    }
    GifWriteFrame(&gw, &image.Raw()[0], size.x + 1, size.y + 1, 2);
  }
  GifEnd(&gw);


  printf("%7gs Start Voronoi\n", get_msec());
  Voronoi v(points, size);
  v();
  printf("%7gs End Voronoi\n", get_msec());

  const std::vector<glm::vec2> &vertex = v.GetVertex();
  const std::vector<Voronoi::Edge> &edge = v.GetEdges();

  printf("%7gs Start drawing\n", get_msec());

  Image image;
  image.Resize(size.x + 1, size.y + 1);
  image.Fill(0xFFFFFFFF);

  for(auto it = edge.begin(); it != edge.end(); ++it)
  {
    const glm::vec2 &p1 = vertex[(*it).vertex1];
    const glm::vec2 &p2 = vertex[(*it).vertex2];
    image.DrawLine(p1, p2, 0x00FF00FF);
  }

  for(auto it = edge.begin(); it != edge.end(); ++it)
  {
    const glm::vec2 &p1 = points[(*it).site1];
    const glm::vec2 &p2 = points[(*it).site2];
    //image.DrawPoint(p1, 0xFF0000FF);
    //image.DrawPoint(p2, 0xFF0000FF);
    image.DrawLine(p1, p2, 0xFF0000FF);
  }

  printf("%7gs Start saving\n", get_msec());

  image.Save("img.png");

  printf("%7gs End\n", get_msec());

  //system("pause");
  return 0;
}

