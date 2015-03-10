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


struct HarmonicMean
{
  glm::vec2 operator()(const glm::vec2 &, const std::vector<unsigned int> &poligon, const std::vector<glm::vec2> &vertex)
  {
    glm::vec2 point;
    for(auto jt = poligon.begin(); jt != poligon.end(); ++jt)
    {
      point += 1.0f / vertex[*jt];
    }
    point = static_cast<float>(poligon.size()) / point;
    return point;
  }
};

struct GeometricMean
{
  glm::vec2 operator()(const glm::vec2 &, const std::vector<unsigned int> &poligon, const std::vector<glm::vec2> &vertex)
  {
    glm::dvec2 point(1.0f, 1.0f);
    for(auto jt = poligon.begin(); jt != poligon.end(); ++jt)
    {
      point *= vertex[*jt];
    }
    point = glm::abs(point);
    point.x = glm::pow(point.x, 1.0f / static_cast<float>(poligon.size()));
    point.y = glm::pow(point.y, 1.0f / static_cast<float>(poligon.size()));
    return point;
  }
};

struct SquareMean
{
  glm::vec2 operator()(const glm::vec2 &, const std::vector<unsigned int> &poligon, const std::vector<glm::vec2> &vertex)
  {
    glm::vec2 point;
    for(auto jt = poligon.begin(); jt != poligon.end(); ++jt)
    {
      point += vertex[*jt] * vertex[*jt];
    }
    point = glm::sqrt(point / static_cast<float>(poligon.size()));
    return point;
  }
};

struct AverageDegree
{
  glm::vec2 operator()(const glm::vec2 &, const std::vector<unsigned int> &poligon, const std::vector<glm::vec2> &vertex)
  {
    glm::vec2 p(10);
    glm::dvec2 point(1.0f, 1.0f);
    for(auto jt = poligon.begin(); jt != poligon.end(); ++jt)
    {
      point.x += glm::pow(vertex[*jt].x, p.x);
      point.y += glm::pow(vertex[*jt].y, p.y);
    }
    point /= static_cast<float>(poligon.size());
    point.x = glm::pow(point.x, 1.0f / p.x);
    point.y = glm::pow(point.y, 1.0f / p.y);
    return point;
  }
};


int main()
{
  glm::uvec2 size(400, 400);

  std::vector<glm::vec2> points;
  //points = Generate(100000, size);
  points = LloidGenerate(300, glm::uvec2(180, 180), glm::uvec2(40, 40));

  printf("%7gs End generate, Count: %i\n", get_msec(), static_cast<int>(points.size()));

  GifWriter gw;
  GifBegin(&gw, "voron.gif", size.x + 1, size.y + 1, 50);

  for(unsigned int i = 0; i < 300; ++i)
  {
    points = Lloyd(points, size, AverageDegree());

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
    /*
    for(auto it = edge.begin(); it != edge.end(); ++it)
    {
      const glm::vec2 &p1 = points[(*it).site1];
      const glm::vec2 &p2 = points[(*it).site2];
      image.DrawLine(p1, p2, 0xFF0000FF);
    }
    */
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

