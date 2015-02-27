#include "Voronoi.h"
#include "image.h"
#include "geometry.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>  
#include <algorithm>
#include <iterator>


int main()
{
  {

    glm::uvec2 size(10000, 10000);

    std::set<glm::vec2, Voronoi::SiteComparator> points;

    printf("Start generate.\n");

    unsigned int seed = static_cast<unsigned int>(time(NULL));
    srand(seed);
    printf("Seed: %i.\n", seed);

    struct Generator 
    {
      const glm::uvec2 size;
      Generator(const glm::uvec2 &s) 
        : size(s)
      {}
      glm::vec2 operator()() {return glm::vec2(rand() % size.x, rand() % size.y);}
    } generator(size);

    std::generate_n(std::inserter(points, points.end()), 1000000, generator);

    printf("End generate. Count: %i.\n", static_cast<int>(points.size()));

    
//     points.insert(Point(20, 20));
//     points.insert(Point(40, 20));
//     points.insert(Point(60, 20));
//     points.insert(Point(80, 20));
//     points.insert(Point(20, 40));
//     points.insert(Point(40, 40));
//     points.insert(Point(60, 40));
//     points.insert(Point(80, 40));
//     points.insert(Point(20, 60));
//     points.insert(Point(40, 60));
//     points.insert(Point(60, 60));
//     points.insert(Point(80, 60));
//     points.insert(Point(50, 80));
    
    
//     points.insert(Point(20, 20));
//     points.insert(Point(40, 20));
//     points.insert(Point(60, 20));
//     points.insert(Point(80, 20));
//     points.insert(Point(10, 40));
//     points.insert(Point(30, 40));
//     points.insert(Point(50, 40));
//     points.insert(Point(70, 40));
//     points.insert(Point(90, 40));
//     points.insert(Point(20, 60));
//     points.insert(Point(40, 60));
//     points.insert(Point(60, 60));
//     points.insert(Point(80, 60));
//     points.insert(Point(50, 80));
    
    
//     points.insert(Point(50, 70));
//     points.insert(Point(45, 50));
//     points.insert(Point(55, 50));
//     points.insert(Point(50, 30));
    
    
//     points.insert(Point(40, 60));
//     points.insert(Point(60, 40));
//     points.insert(Point(20, 40));
    
    
//     points.insert(Point(50, 95));
//     points.insert(Point(10, 10));
//     points.insert(Point(30, 30));
//     points.insert(Point(50, 50));
//     points.insert(Point(70, 70));
//     points.insert(Point(90, 90));
//     points.insert(Point(10, 90));
//     points.insert(Point(30, 70));
//     points.insert(Point(70, 30));
//     points.insert(Point(90, 10));
    
    
//     points.insert(Point(40, 60));
//     points.insert(Point(20, 60));
//     points.insert(Point(70, 70));
//     points.insert(Point(30, 90));
    
    
//     points.insert(Point(50.000000, 70.000000));
//     points.insert(Point(50.000000, 25.000000));
//     points.insert(Point(49.000000, 30.000000));
//     points.insert(Point(51.000000, 30.000000));
    

    /*
    for(auto it = points.begin(); it != points.end(); ++it)
    {
    printf("points.push_back(Point(%f, %f));\n", (*it).x, (*it).y);
    }
    */

    printf("Start Voronoi.\n");
    Voronoi v(points, size);
    printf("End Voronoi.\n");
    v.Sort();
    auto diagram = v.GetDiagram();
    printf("Start drawing.\n");

    //system("pause");

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

      /*
      for(auto jt = (*it).second.begin(); jt != (*it).second.end(); ++jt)
      {
      glm::vec2 point = (*jt) + (*it).first;
      image.DrawPoint(point, 0x00FF00FF);
      }
      */
      //image.DrawPoint((*it).first, 0xFF0000FF);
    }

    printf("Start saving.\n");
    for(auto it = diagram.begin(); it != diagram.end();)
    {
      auto range = diagram.equal_range((*it).first);
      image.DrawPoint((*it).first, 0xFF0000FF);

      it = range.second;
    }


    image.Save("img.png");
  }

  //system("pause");

  return 0;
}


