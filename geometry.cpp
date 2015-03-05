#include "geometry.h"

#include <assert.h> // Переделать на исключения

#include <functional>
#include <list>

#define EPS 0.0001

double geometry::RotationPoint(const Point &a, const Point &b, const Point &c)
{
  return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
}

bool geometry::RectContainsPoint(const Rect &rect, const Point &point)
{
  return point.x >= (rect.lb.x - EPS) && point.x <= (rect.rt.x + EPS) && 
    point.y >= (rect.lb.y - EPS) && point.y <= (rect.rt.y + EPS);
}


bool geometry::IsIntersectionLine(const Line &a, const Line &b)
{
  return a.a * b.b - b.a * a.b != 0.0;
}

double geometry::FindLineX(const Line &line, double y)
{
  assert(LineContainsX(line));
  return (- line.c - line.b * y) / line.a;
}

double geometry::FindLineY(const Line &line, double x)
{
  assert(LineContainsY(line));
  return (- line.c - line.a * x) / line.b;
}

geometry::Point geometry::IntersectLines(const Line &a, const Line &b)
{
  Point p;
  double k = a.a * b.b - b.a * a.b;
  p.x = (b.c * a.b - a.c * b.b) / k;
  p.y = (a.c * b.a - b.c * a.a) / k;
  return p;
}

bool geometry::LineContainsX(const Line &line)
{
  return line.a != 0.0;
}

bool geometry::LineContainsY(const Line &line)
{
  return line.b != 0.0;
}

std::vector<geometry::Point> geometry::IntersectRectLine(const Rect &rect, const Line &line)
{
  // Примечание. Желательно в конце удалять одинаковые точки.
  // Возникает в случае, если линия проходит через угол.
  std::vector<Point> points;
  points.reserve(4);

  // Создаем прямые, проходащие через стороны ректа.
  Point lb(rect.lb);
  Point lt(rect.lb.x, rect.rt.y);
  Point rt(rect.rt);
  Point rb(rect.rt.x, rect.lb.y);
  Line sides[4] = {Line(lb, lt), Line(lt, rt), Line(rt, rb), Line(rb, lb)};

  // Ищем точки пересечения линий.
  for(int i = 0; i < 4; ++i)
  {
    if(IsIntersectionLine(line, sides[i]))
    {
      points.push_back(IntersectLines(line, sides[i]));
    }
  }

  for(auto it = points.begin(); it != points.end();)
  {
    // Если точка не лежит в области, удалим ее.
    if(!RectContainsPoint(rect, *it))
    {
      it = points.erase(it);
    }
    else
    {
      ++it;
    }
  }

  assert(points.size() == 0 || points.size() == 2);
  return std::move(points);
}

geometry::Line geometry::Perpendicular(const Line &line, const Point &point)
{
  return Line(-line.b, line.a, line.b * point.x - line.a * point.y);
}

geometry::Point geometry::Center(const Point &a, const Point &b)
{
  return Point((b.x + a.x) / 2.0, (b.y + a.y) / 2.0);
}

geometry::Ray geometry::PerpRayLine(const Ray &ray, const Line &line)
{
  // Строим перпендикулярную линию, проходящую через начало луча.
  Line perp(Perpendicular(line, ray.point));
  // Строим еще один перпендикуляр к perp, проходящий через точку направления луча.
  Line par(Perpendicular(perp, ray.dir));
  // Ищем точку пересечения перпендикуляров.
  Point p(IntersectLines(perp, par));
  // Теперь у нас есть луч, перпендикулярный заданной прямой, направленный по заданному лучу.
  return Ray(ray.point, p);
}

std::vector<geometry::Point> geometry::IntersectRectRay(const Rect &rect, const Ray &ray)
{
  std::vector<Point> points;
  points.reserve(5);

  Point lb(rect.lb);
  Point lt(rect.lb.x, rect.rt.y);
  Point rt(rect.rt);
  Point rb(rect.rt.x, rect.lb.y);
  Segment sides[4] = {Segment(lb, lt), Segment(lt, rt), Segment(rt, rb), Segment(rb, lb)};

  points.push_back(ray.point);

  for(int i = 0; i < 4; ++i)
  {
    if(IsIntersectionRaySegment(sides[i], ray))
    {
      points.push_back(IntersectLines(Line(ray.dir, ray.point), Line(sides[i].a, sides[i].b)));
    }
  }

  for(auto it = points.begin(); it != points.end();)
  {
    // Если точка не лежит в области, удалим ее.
    if(!RectContainsPoint(rect, *it))
    {
      it = points.erase(it);
    }
    else
    {
      ++it;
    }
  }

  assert(points.size() == 0 || points.size() == 2);
  std::vector<Point> output;
  if(points.size() < 2)
  {
    return std::move(output);
  }

  // У нас есть 2 точки, ближняя к началу луча - начало, дальняя - конец.
  if(glm::distance(ray.point, points[0]) < glm::distance(ray.point, points[1]))
  {
    output.push_back(points[0]);
    output.push_back(points[1]);
  }
  else
  {
    output.push_back(points[1]);
    output.push_back(points[0]);
  }

  return std::move(output);
}

bool geometry::IsIntersectionRaySegment(const Segment &segment, const Ray &ray)
{
  Line line(segment.a, segment.b);
  // Строим перпендикулярную линию, проходящую через начало луча.
  Line perp(Perpendicular(line, ray.point));
  // Ищем точку пересечения перпендикуляра и линии.
  Point p(IntersectLines(line, perp));

  glm::dvec2 a(p.x - ray.point.x, p.y - ray.point.y);
  glm::dvec2 b(ray.dir.x - ray.point.x, ray.dir.y - ray.point.y);

  double cosa = (a.x * b.x + a.y * b.y) / (sqrt(a.x * a.x + a.y * a.y) * sqrt(b.x * b.x + b.y * b.y));

  if(cosa < 0)
  {
    // Угол тупой, вектор не направлен на прямую.
    return false;
  }

  double t1 = RotationPoint(ray.point, ray.dir, segment.a);
  double t2 = RotationPoint(ray.point, ray.dir, segment.b);

  return (t1 >= 0.0 && t2 <= 0.0) || (t2 >= 0.0 && t1 <= 0.0);
}

double geometry::IntersectParabols(double sl, const Point &p1, const Point &p2)
{
  // Если фокус параболы лежит на заметающей прямой, они пересекаются там,
  // где находится эта парабола по x.
  if(sl == p1.y)
  {
    return p1.x;
  }
  if(sl == p2.y)
  {
    return p2.x;
  }
  const double a = p2.y - p1.y;
  if(a != 0)
  {
    const double b = p2.x * p1.y - p1.x * p2.y + sl * (p1.x - p2.x);

    const double x12 = p1.x * p1.x;
    const double y12 = p1.y * p1.y;
    const double x22 = p2.x * p2.x;
    const double y22 = p2.y * p2.y;

    const double c = (sl * sl + p1.y * p2.y) * (p1.y - p2.y) +
               sl * (x22 + y22 - x12 - y12) +
               x12 * p2.y - x22 * p1.y;

    //y = a * (x * x) + 2 * b * x + c;
    const double d4 = b * b - a * c;
    assert(d4 >= 0);
    const double x1 = (-b + sqrt(d4)) / (a);
    const double x2 = (-b - sqrt(d4)) / (a);

    // Нам не нужны координаты по y
    //double y1 = (pow((x1 - p1.x), 2) + p1.y * p1.y - sl * sl) / (2 * (p1.y - sl));
    //double y2 = (pow((x2 - p1.x), 2) + p1.y * p1.y - sl * sl) / (2 * (p1.y - sl));

    const double leftx = x1 < x2 ? x1 : x2;
    const double rightx = x1 < x2 ? x2 : x1;

    // Мы получили отрезок, при чем отрезок расположен слева направо.
    if(p1.y > p2.y)
    {
      return leftx;
    }
    else
    {
      return rightx;
    }
  }


  // Если фокусы парабол на одном уровне, они пересекаются по x в центре между фокусами.
  const double x = (p1.x + p2.x) / 2;

  //double y = (p1.y + p2.y) / 2;
  //double y = (pow(x - p1.x, 2.0) + p1.y * p1.y - sl * sl) / (2 * (p1.y - sl));

  return x;
}

geometry::Point geometry::CreateCircle(const Point &a, const Point &b, const Point &c)
{
  const double x1 = a.x;
  const double y1 = a.y;
  const double x2 = b.x;
  const double y2 = b.y;
  const double x3 = c.x;
  const double y3 = c.y;

  const double xy3 = x3 * x3 + y3 * y3;
  const double xy1 = x1 * x1 + y1 * y1;
  const double xy2 = x2 * x2 + y2 * y2;

  const double zx = (y1 - y2) * (xy3) + (y2 - y3) * (xy1) + (y3 - y1) * (xy2);
  const double zy = (x1 - x2) * (xy3) + (x2 - x3) * (xy1) + (x3 - x1) * (xy2);
  const double z  = (x1 - x2) * (y3 - y1) - (y1 - y2) * (x3 - x1);

  assert(z != 0);

  return Point(-(zx) / (2 * z), (zy) / (2 * z));
}

std::vector<geometry::Point> geometry::IntersectRectSegment(const Rect &rect, const Segment &segment)
{
  std::vector<Point> points;
  points.reserve(6);

  points.push_back(segment.a);
  points.push_back(segment.b);

  // Создаем прямые, проходящие через стороны ректа.
  Point lb(rect.lb);
  Point lt(rect.lb.x, rect.rt.y);
  Point rt(rect.rt);
  Point rb(rect.rt.x, rect.lb.y);
  Line sides[4] = {Line(lb, lt), Line(lt, rt), Line(rt, rb), Line(rb, lb)};
  Line line(segment.a, segment.b);

  // Ищем точки пересечения линий.
  for(int i = 0; i < 4; ++i)
  {
    if(IsIntersectionLine(line, sides[i]))
    {
      points.push_back(IntersectLines(line, sides[i]));
    }
  }

  Rect sr = CreateRect(segment);

  // Оставляем только те точки, которые входят в оба ректа.
  for(auto it = points.begin(); it != points.end();)
  {
    if(!RectContainsPoint(rect, *it) || !RectContainsPoint(sr, *it))
    {
      it = points.erase(it);
    }
    else
    {
      ++it;
    }
  }

  assert(points.size() == 0 || points.size() == 2);
  std::vector<Point> output;
  if(points.size() < 2)
  {
    return std::move(output);
  }

  // Если точка находится внутри рабочей области, оставляем ее, 
  // иначе ищем ближайшую к ней точку.
  if(RectContainsPoint(rect, segment.a))
  {
    output.push_back(segment.a);
  }
  else
  {
    glm::distance(segment.a, points[0]) < glm::distance(segment.a, points[1]) ?
      output.push_back(points[0]) : output.push_back(points[1]);
  }
  if(RectContainsPoint(rect, segment.b))
  {
    output.push_back(segment.b);
  }
  else
  {
    glm::distance(segment.b, points[0]) < glm::distance(segment.b, points[1]) ?
      output.push_back(points[0]) : output.push_back(points[1]);
  }

  return std::move(output);
}

geometry::Rect geometry::CreateRect(const Segment &segment)
{
  double leftx = segment.a.x < segment.b.x ? segment.a.x : segment.b.x;
  double rightx = segment.a.x >= segment.b.x ? segment.a.x : segment.b.x;
  double bottomy = segment.a.y < segment.b.y ? segment.a.y : segment.b.y;
  double topy = segment.a.y >= segment.b.y ? segment.a.y : segment.b.y;

  return Rect(Point(leftx, bottomy), Point(rightx, topy));
}





