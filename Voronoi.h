#ifndef VORONOI_H
#define VORONOI_H

#include "geometry.h"

#include <vector>
#include <list>
#include <set>
#include <map>
#include <functional>

using namespace geometry;

struct Parabola
{
  const int id;
  static int PIdGeneration;
  Point focus;
  Parabola(Point f)
    : id(PIdGeneration), focus(f)
  {
    PIdGeneration++;
  }
};

struct Event;

struct Arc
{
    Parabola *parabola;
    Event *event;
    Arc()
    {
        parabola = nullptr;
        event = nullptr;
    }

    Arc(const Arc &arc)
    {
        parabola = arc.parabola;
        event = arc.event;
    }
    Arc(Parabola *p)
    {
        parabola = p;
        event = nullptr;
    }
    Arc& operator=(const Arc& arc)
    {
      if (this == &arc)
      {
        return *this;
      }
      parabola = arc.parabola;
      event = arc.event;
      return *this;
    }
};

struct BreakPoint
{
  const int id;
  static int BPIdGeneration;
  enum BPType
  {
    POINT = 1,
    END_POINT = 2,
  };

  BPType type;
  Point point;

  BreakPoint(BPType t)
    : id(BPIdGeneration), type(t)
  {
    BPIdGeneration++;
  }
};

struct Element
{
  enum
  {
    EMPTY = 0,
    ARC = 1,
    BREAK_POINT = 2,
  }type;
  
  Arc arc;
  BreakPoint *breakPoint;
  
  Element()
    : type(EMPTY)
  {
    breakPoint = nullptr;
  }
  Element(const Element& el)
    : type(el.type), arc(el.arc)
  {
    breakPoint = el.breakPoint;
  }
  Element& operator=(const Element& el) 
  {
    if (this == &el) 
    {
      return *this;
    }
    type = el.type;
    arc = el.arc;
    breakPoint = el.breakPoint;
    return *this;
  }

  Element(Parabola *p)
    : type(ARC), arc(p)
  {
    breakPoint = nullptr;
  }
  Element(BreakPoint *bp)
    : type(BREAK_POINT)
  {
    breakPoint = bp;
  }
};

struct Edge
{
  BreakPoint *bp1;
  BreakPoint *bp2;
  Parabola *p1;
  Parabola *p2;
  Edge()
  {
    bp1 = nullptr;
    bp2 = nullptr;
    p1 = nullptr;
    p2 = nullptr;
  }
};

struct BtreeElement
{
  BtreeElement *parent;
  BtreeElement *left;
  BtreeElement *right;
  Element element;
  BtreeElement()
  {
    parent = nullptr;
    left = nullptr;
    right = nullptr;
  }
  BtreeElement(const BtreeElement &el)
    : element(el.element)
  {
    parent = el.parent;
    left = el.left;
    right = el.right;
  }
  BtreeElement(const Element &el)
    : element(el)
  {
    parent = nullptr;
    left = nullptr;
    right = nullptr;
  }
};

struct SiteEvent
{
  Parabola *parabola;
  SiteEvent()
  {
    parabola = nullptr;
  }
  SiteEvent(Parabola *p)
  {
    parabola = p;
  }
};

struct CircleEvent
{
  Point center;
  Point pos;
  BtreeElement *arcNode;
  CircleEvent()
  {
    arcNode = nullptr;
  }
  CircleEvent(BtreeElement *arc)
  {
    arcNode = arc;
  }
};

struct Event
{
  enum EventType
  {
    SITE_EVENT,
    CIRCLE_EVENT,
  } type;
  SiteEvent se;
  CircleEvent ce;
  double height;
  Event(const SiteEvent &s)
    : type(SITE_EVENT), se(s)
  {
    height = s.parabola->focus.y;
  }
  Event(const CircleEvent &c)
    : type(CIRCLE_EVENT), ce(c)
  {
    height = c.pos.y;
  }
};

class Voronoi
{
public:
  Voronoi(const std::list<glm::vec2> &points, glm::vec2 size);
  ~Voronoi();

  bool IsList(BtreeElement *btreeElement);
  bool IsNode(BtreeElement *btreeElement);

  // Вставить новую параболу в корень дерева.
  void InsertArcHead(Parabola *parabola);

  // Вставить новую параболу в существующую.
  void InsertArc(BtreeElement *btreeElement, Parabola *parabola);

  // Удалить существующую арку.
  // Арка должна находиться между двумя другими арками.
  // arcPos - номер арки в списке арок.
  void RemoveArc(unsigned int arcPos, Point intersection);

  // Обновить грань, прамая становится лучем, или луч становится отрезком.
  void UpdateEdge(BreakPoint *removedPoint, BreakPoint *endPoint);

  // Вычислить позицию брекпоинта.
  // BPPos - позиция в списке брекпоинтов.
  void CalcBPPos(unsigned int BPPos);

  void GenerateCircleEvent();

  void RemoveEvent(Event *event);

  void Process();

  BtreeElement *FindInsertArc(Parabola *parabola);

  void CalcBPPos();

  bool IsLine(const Point &p1, const Point &p2, const Point &p3);

  // Обрезаем оставшиеся лучи и прямые ограничивающей областью.
  void PostProcess();

private:
  struct DDComparator
  {
     bool operator()(const glm::vec2& a, const glm::vec2& b) const
     {
       if(a.x == b.x)
        return a.y < b.y;
       return a.x < b.x;
     }
  };

public:
  const std::multimap<glm::vec2, glm::vec2, DDComparator> &GetDiagram();

  // Брекпоинт превратился в точку пересечения граней.
  //void FindedBP(const Point &bp, const Point &arcpos);

  // Вставить брекпоинты.
  void InsertBP(const std::list<Point> &points, const Point &arc1, const Point &arc2);

private:

  Rect mRect;

  BtreeElement *mHead;

  std::vector<Parabola *> mParabols;
  std::vector<BtreeElement *> mListArc;
  std::vector<BtreeElement *> mListBreakPoints;
  std::list<Edge> mEdgeList;
  std::multiset<Event *, std::function<bool (Event *, Event *)> > mEvents;
  std::multimap<glm::vec2, glm::vec2, DDComparator> mDiagramData;

  double mSweptLine;

public:
  // Создать список арок и список брекпоинтов.
  void GenerateListsBPA();

  // Получить арку для текущей ноды.
  void GenerateListsBPA(BtreeElement *node);

  void PrintListsBPA();

};

#endif // VORONOI_H
