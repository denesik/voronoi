#include "Voronoi.h"
#include "image.h"
#include "geometry.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>  

// —местить точки лежащие на одинаковой высоте так, что бы все точки лежали на разных высотах.
// ¬се точки должны лежать в диапазоне (0;0) : (size.y;size.y).
std::vector<glm::vec2> CorrectPoints(const std::vector<glm::vec2> &points, const glm::vec2 &size)
{
  // «аполн€ем сортированный по высоте список.
  // —ложность O(log(n)).
  std::multiset<glm::vec2, std::function<bool (const glm::vec2 &, const glm::vec2 &)> > 
    sortPoints(points.begin(), points.end(), 
    [](const glm::vec2 &p1, const glm::vec2 &p2)
    {
      return p1.y < p2.y;
    });

  // Ќаходим минимальное смещение между точками.
  const float offset = size.y / (static_cast<float>(points.size()) * 100);

  std::vector<glm::vec2> editPoints;
  editPoints.reserve(points.size());

  bool process = true;
  while(process)
  {
    process = false;
    // ѕроходим по сортированному списку точек и ищем набор точек одинаковых по высоте.
    for(auto it = sortPoints.begin(); it != sortPoints.end();)
    {
      auto range = sortPoints.equal_range(*it);
      //  оличество элементов с текущим значением по y.
      int count = std::distance(range.first, range.second);

      if(count > 1)
      {
        int rd = count / 2;
        if(size.y - (*it).y < rd * offset) // ≈сли между точкой и верхней границей маленькое рассто€ние.
        {
          rd = count - 1;
        }
        else if((*it).y < rd * offset) // ≈сли между точкой и нижней границей маленькое рассто€ние.
        {
          rd = 0;
        }

        // »зменим высоту.
        int i = 0;
        for(auto jt = range.first; jt != range.second; ++jt)
        {
          editPoints.push_back(glm::vec2((*jt).x, (*jt).y + offset * (static_cast<int>(i) - rd)));
          ++i;
        }
        it = sortPoints.erase(range.first, range.second);

        process = true;
        continue;
      }

      it = range.second;
    }
    
    if(process)
    {
      sortPoints.insert(editPoints.begin(), editPoints.end());
      editPoints.clear();
    }

  }

  return std::vector<glm::vec2>(sortPoints.begin(), sortPoints.end());
}

int main()
{
  /*
  auto l = IntersectRectLine(Rect(Point(0, 0), Point(10, 10)), Line(Point(0, 0), Point(10, 10)));

  auto p = IntersectRectRay(Rect(Point(0, 0), Point(10, 10)), Ray(Point(7, 13), Point(7, -5)));

  auto s = IntersectRectSegment(Rect(Point(0, 0), Point(10, 10)), Segment(Point(7, -2), Point(11, 2)));
*/

  glm::uvec2 size(4000, 4000);

//  std::list<glm::vec2> points;
//   points.push_back(Point(40, 20));
//   points.push_back(Point(40, 50));
//   points.push_back(Point(40, 70));

  
//   points.push_back(Point(54, 86));
//   points.push_back(Point(67, 88));
//   points.push_back(Point(23, 92));
//   
//   points.push_back(Point(20, 30));
//   points.push_back(Point(40, 20));
//   points.push_back(Point(60, 25));
//   points.push_back(Point(80, 35));
//   points.push_back(Point(90, 50));
//   points.push_back(Point(70, 70));
//   points.push_back(Point(10, 63));
//   points.push_back(Point(41, 74));
//   points.push_back(Point(50, 75));
//   points.push_back(Point(39, 73));
//   points.push_back(Point(45, 65));
//   points.push_back(Point(30, 60));
//   points.push_back(Point(25, 55));
//   points.push_back(Point(15, 40));
//   points.push_back(Point(68, 23));
//   points.push_back(Point(17, 57)); 
//   points.push_back(Point(93, 12));
//   points.push_back(Point(57, 38));
//   points.push_back(Point(54, 27));
//   points.push_back(Point(75, 47));
//   points.push_back(Point(86, 43));
//   points.push_back(Point(23, 39));
//   points.push_back(Point(65, 26));
//   points.push_back(Point(13, 41));
//   points.push_back(Point(52, 31));
//   points.push_back(Point(23, 49));
//   points.push_back(Point(56, 76));
//   points.push_back(Point(65, 17));
//   points.push_back(Point(34, 64));
//   points.push_back(Point(27, 34));
//   points.push_back(Point(73, 37));
//   points.push_back(Point(65, 45));
//   points.push_back(Point(74, 52));
//   points.push_back(Point(86, 56));
//   points.push_back(Point(36, 53));
//   points.push_back(Point(76, 78));
//   points.push_back(Point(36, 59));
//   points.push_back(Point(50, 59));
//   points.push_back(Point(43, 59));
//   points.push_back(Point(75, 59));
//   points.push_back(Point(25, 59));
//   points.push_back(Point(43, 59));
//   points.push_back(Point(56, 59));  
  
  /*
  points.push_back(Point(2.0f, 5.0f));
  points.push_back(Point(5.0f, 6.0f));
  points.push_back(Point(3.0f, 5.1f));
  */

  std::vector<glm::vec2> points;

  printf("start generate\n");

  srand(static_cast<unsigned int>(time(NULL)));
  for(unsigned int i = 0; i < (size.x + size.y) / 1; ++i)
  {
    points.push_back(glm::vec2(rand() % size.x, rand() % size.y));
  }


//  points.push_back(Point(30, 30));  // 1
//  points.push_back(Point(20, 60));  // 2
//  points.push_back(Point(50, 30));  // 3
//  points.push_back(Point(70, 30));  // 4
//  points.push_back(Point(10, 20));  // 5
//  points.push_back(Point(60, 10));  // 6
//  points.push_back(Point(82, 30));

  //points = CorrectPoints(points, size);
  printf("start voronoy\n");
  Voronoi v(std::list<glm::vec2>(points.begin(), points.end()), size);

  Image image;
  image.Resize(size.x + 1, size.y + 1);
  image.Fill(0xFFFFFFFF);

  auto diagram = v.GetDiagram();

  
//   for(auto it = diagram.begin(); it != diagram.end(); ++it)
//   {
//     printf("A: (%f:%f); P: (%f:%f)\n", (*it).first.x, (*it).first.y, (*it).second.x, (*it).second.y);
//   }
  

  printf("start driwing\n");
  for(auto it = diagram.begin(); it != diagram.end();)
  {
    auto range = diagram.equal_range((*it).first);
    
    std::vector<glm::vec2> vec;
    for(auto jt = range.first; jt != range.second; ++jt)
    {
      vec.push_back((*jt).second);
    }

    auto p = geometry::SortPointCcw(vec);

    for(auto jt = p.begin() + 1; jt != p.end(); ++jt)
    {
      //image.DrawPoint((*jt), 0x00FF00FF);
      image.DrawLine(*(jt - 1), (*jt), 0x00FF00FF);
    }
    /*
    for(auto jt = range.first; jt != range.second; ++jt)
    {
      image.DrawPoint((*jt).second, 0x00FF00FF);
    }
    */
    //image.DrawPoint((*it).first, 0xFF0000FF);

    it = range.second;
  }

  printf("start saving\n");
  for(auto it = diagram.begin(); it != diagram.end();)
  {
    auto range = diagram.equal_range((*it).first);
    image.DrawPoint((*it).first, 0xFF0000FF);

    it = range.second;
  }


  image.Save("img.png");

  //system("pause");

  return 0;
}


