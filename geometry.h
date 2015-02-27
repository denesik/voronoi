#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glm/glm.hpp>
#include <set>
#include <vector>
#include <list>

namespace geometry
{

  typedef glm::dvec2 Point;

  struct Rect
  {
    Point lb;
    Point rt;
    Rect()
      : lb(), rt()
    {}
    Rect(const Point &_lb, const Point &_rt)
      : lb(_lb), rt(_rt)
    {}
    Rect(const Rect &rect)
      : lb(rect.lb), rt(rect.rt)
    {}
  };

  // Описание прямой
  struct Line
  {
    double a;
    double b;
    double c;
    Line()
      : a(0.0), b(0.0), c(0.0)
    {}
    Line(double _a,double _b,double _c)
      : a(_a), b(_b), c(_c)
    {}
    Line(const Line &line)
      : a(line.a), b(line.b), c(line.c)
    {}
    Line(const Point &_a, const Point &_b)
      : a(_a.y - _b.y), b(_b.x - _a.x), c(_a.x * _b.y - _b.x * _a.y)
    {}
  };

  // Отрезок.
  struct Segment
  {
    Point a;
    Point b;
    Segment()
      : a(), b()
    {}
    Segment(const Point &_a, const Point &_b)
      : a(_a), b(_b)
    {}
    Segment(const Segment &segment)
      : a(segment.a), b(segment.b)
    {}
  };

  // Луч. Луч задается двумя точками, первая точка - начало луча, вторая точка - направление луча.
  struct Ray
  {
    Point point;
    Point dir;
    Ray()
      : point(), dir()
    {}
    Ray(const Point &p, const Point &d)
      : point(p), dir(d)
    {}
    Ray(const Ray &ray)
      : point(ray.point), dir(ray.dir)
    {}
  };

  // Содержит ли линия координату по x?
  bool LineContainsX(const Line &line);

  // Содержит ли линия координату по y?
  bool LineContainsY(const Line &line);

  // Найти x на прямой по заданному y
  double FindLineX(const Line &line, double y);
  // Найти y на прямой по заданному x
  double FindLineY(const Line &line, double x);

  // Пересекаются ли прямые?
  bool IsIntersectionLine(const Line &a, const Line &b);

  // Найти точку пересечения прямых.
  Point IntersectLines(const Line &a, const Line &b);

  // Содержит ли область точку?
  bool RectContainsPoint(const Rect &rect, const Point &point);

  // Как расположены точки, по часовой стрелке или против?
  // Если результат больше 0, то против часовой.
  double RotationPoint(const Point &a, const Point &b, const Point &c);

  // Найти перпендикулярную прямую к данной проходящий через заданную точку.
  Line Perpendicular(const Line &line, const Point &point);

  // Найти центральную точку между двумя точками.
  Point Center(const Point &a, const Point &b);

  // Даны прямая и луч. развернуть луч таким образом, что бы он стал перпендикулярен заданной прямой.
  Ray PerpRayLine(const Ray &ray, const Line &line);

  // Найти пересечение парабол.
  // sl - координата по y заметающей прямой
  // p1 и p2 координыта центров парабол.
  double IntersectParabols(double sl, const Point &p1, const Point &p2);

  // Пересекается ли луч и отрезок.
  bool IsIntersectionRaySegment(const Segment &segment, const Ray &ray);

  // Построить окружность по трем точкам.
  // Вернуть центр окружности.
  Point CreateCircle(const Point &a, const Point &b, const Point &c);

  // Найти пересечение области линией.
  std::list<Point> IntersectRectLine(const Rect &rect, const Line &line);

  // Найти пересечение области лучем.
  std::list<Point> IntersectRectRay(const Rect &rect, const Ray &ray);

  std::list<Point> IntersectRectSegment(const Rect &rect, const Segment &segment);

  // Создать рект по заданной диагонали.
  Rect CreateRect(const Segment &segment);
}



#endif // GEOMETRY_H
