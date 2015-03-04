#ifndef VORONOI_H
#define VORONOI_H


#include "geometry.h"
#include <set>
#include <vector>

using namespace geometry;

//#define VORONOI_DEBUG_INFO

#ifdef VORONOI_DEBUG_INFO
template<int T>
struct Val
{
  static int Get()
  {
    static int id = 0;
    return id++;
  }
};
#endif

class Voronoi
{
public:

  struct Edge
  {
    unsigned int site1;
    unsigned int site2;
    unsigned int vertex1;
    unsigned int vertex2;
    Edge(unsigned int s1, unsigned int s2, unsigned int v1, unsigned int v2)
     : site1(s1), site2(s2), vertex1(v1), vertex2(v2)
    {
    }
  };


  Voronoi(const std::vector<glm::vec2> &sites, const glm::vec2 &size);

  ~Voronoi();

  /// Вернуть список граней.
  std::vector<Edge> &GetEdges();

  /// Вернуть список вершин.
  std::vector<glm::vec2> &GetVertex();


private:
  typedef unsigned int SiteIndex;
  typedef unsigned int PointIndex;
  typedef int EdgeIndex;
  typedef int VertexIndex;

  /// Интерфейс элемента.
  /// Элемент может быть аркой, точкой пересечения арок, либо точкой пересечения граней.
  struct IElement
  {
    enum ElementType
    {
      ARC,
      BREAK_POINT,
      END_POINT,
    };

    IElement(ElementType t)
      : type(t)
    {}

    const ElementType type;
  };

  struct CircleEvent;
  /// Арка. Содержит указатель на входную точку.
  /// Так же содержит указатель на событие круга для этой арки, если такое существует.
  struct ArcElement : public IElement
  {
    const SiteIndex site;
    CircleEvent *event;
    ArcElement(const SiteIndex s)
      : IElement(ARC), site(s)
    {
      event = nullptr;
    }
  };

  struct EdgeElement;
  /// Точка пересечения арок.
  /// Содержит индекс на себя в списке и
  /// указатель на грань, один из концов которой является данные брекпоинт.
  struct BPElement : public IElement
  {
    const PointIndex pos;
#ifdef VORONOI_DEBUG_INFO
    const unsigned int id;
#endif
    EdgeIndex edge;
    BPElement(PointIndex p)
      : IElement(BREAK_POINT), pos(p)
#ifdef VORONOI_DEBUG_INFO
      , id(Val<BREAK_POINT>::Get())
#endif
    {
      edge = -1;
    }
  };

  /// Точка пересечения граней.
  /// Содержит 3 входных точки, между которыми она находится
  /// и позицию пересечения.
  /// Так же содержит счетчик ссылок. Изначально вершина содержится в 3-х гранях.
  /// По мере обработки граней, счетчик ссылок должен уменьшаться.
  /// Если вершина больше не содержится ни в одной грани, она удаляется.
  struct EPElement : public IElement
  {
    const VertexIndex pos;
    const SiteIndex site1;
    const SiteIndex site2;
    const SiteIndex site3;
    unsigned int refCount;
#ifdef VORONOI_DEBUG_INFO
    const int id;
#endif
    EPElement(const VertexIndex p, const SiteIndex s1, const SiteIndex s2, const SiteIndex s3)
      : IElement(END_POINT), pos(p), site1(s1), site2(s2), site3(s3)
#ifdef VORONOI_DEBUG_INFO
      , id(Val<END_POINT>::Get())
#endif
    {
      refCount = 3;
    }
  };

  /// Грань.
  /// Содержит ссылки на 2 места(исходных точки) и 2 точки соединяющие грань.
  /// Точки можут быть точками пересечения парабол либо точками пересечения граней.
  struct EdgeElement
  {
    PointIndex el1;
    PointIndex el2;
    const SiteIndex site1;
    const SiteIndex site2;
    EdgeElement(PointIndex e1, PointIndex e2, const SiteIndex s1, const SiteIndex s2)
      : el1(e1), el2(e2), site1(s1), site2(s2)
    {}
  };

  /// Элемент дерева.
  struct BtreeElement
  {
    BtreeElement *parent;
    BtreeElement *left;
    BtreeElement *right;
    IElement *element;
    BtreeElement(IElement *el)
    {
      parent = nullptr;
      left = nullptr;
      right = nullptr;
      element = el;
    }
  };

  /// Событие круга. Содержит кооринату события по высоте и указатель на арку,
  /// к которой относится данное событие.
  struct CircleEvent
  {
    const float posy;
    BtreeElement *arc;
    CircleEvent(float p, BtreeElement *a)
      : posy(p), arc(a)
    {
    }
  };

private:
  /// Список исходных точек.
  const std::vector<glm::vec2> &mListSite;

  /// Ограничивающая область диаграммы.
  const Rect mRect;

  // Уровень заметающей прямой.
  // Прямая опускается сверху вниз.
  float mSweepLine;

  // Голова дерева.
  BtreeElement *mHead;

  /// Упорядоченный список событий точек.
  /// Хранит номера точек в списке точек.
  std::vector<SiteIndex> mSiteEvents;

  /// Номер текущего события в списке событий точек.
  unsigned int mSiteEventsIndex;

  struct CircleEventComparator
  {
    bool operator()(CircleEvent *e1, CircleEvent *e2) const
    {
      return e1->posy > e2->posy;
    }
  };

  /// Упорядоченный список событий кгура.
  /// Ключ - высота события, значение - номер арки, к которому относится событие.
  std::multiset<CircleEvent *, CircleEventComparator> mCircleEvents;

  /// Список граней.
  std::vector<EdgeElement *> mListEdgeElement;

  /// Список брекпоинтов и точек пересечения граней.
  std::vector<IElement *> mListPoints;

  /// Список вершин полигонов.
  std::vector<glm::vec2> mListVertex;

  /// Список граней.
  std::vector<Edge> mListEdge;


private:

  /// Основной цикл алгоритма.
  void Process();

  /// Добавить самую первую точку.
  /// @param site Индекс точки. Для данной точки будет создана новая арка.
  void InsertSiteFirstHead(const SiteIndex site);

  /// Добавить точку на самом верхнем уровне.
  /// @param site Индекс точки. Для данной точки будет создана новая арка.
  void InsertSiteTop(const SiteIndex site);

  /// Добавить точку в дерево.
  /// @param element Арка в которую будем вставлять новую арку.
  /// @param site Индекс точки. Для данной точки будет создана новая арка.
  void InsertArc(BtreeElement *element, const SiteIndex site);

  /// Удалить арку.
  /// @param element Точка соединения трех граней.
  void RemoveArc(BtreeElement *element);

  /// Проверить событие круга для заданной арки.
  void CheckCircleEvent(BtreeElement *leftArc, BtreeElement *arc, BtreeElement *rightArc);

  /// Добавить новое событие круга.
  void NewCircleEvent(BtreeElement *arc, float posy);

  /// Удалить событие круга.
  void RemoveCircleEvent(CircleEvent *event);

  /// Найти арку и брекпоинт слева от текущей
  std::pair<BtreeElement *, BtreeElement *> LeftArcBP(BtreeElement *element);

  /// Найти арку справа от текущей
  std::pair<BtreeElement *, BtreeElement *> RightArcBP(BtreeElement *element);

  /// Создать брекпоинт.
  PointIndex NewBPElement();

  /// Удалить брекпоинт.
  void DeleteBPElement(PointIndex el);

  /// Освободить ресурсы.
  void ReleaseProcess();

  /// Освободить ресурсы.
  void ReleasePostProcess();

  PointIndex NewEPElement(const SiteIndex s1, const SiteIndex s2, const SiteIndex s3);

  void DeleteEPElement(PointIndex p);

  /// Добавить новую грань.
  void NewEdge(PointIndex el1, PointIndex el2, const SiteIndex site1, const SiteIndex site2);

  /// Обновить список граней.
  void UpdateEdge(PointIndex el1, PointIndex el2, PointIndex ep);

  /// Удалить грань.
  void DeleteEdge(EdgeIndex el);

private:
  // Отладочные функции.
  bool IsList(BtreeElement *btreeElement);
  bool IsNode(BtreeElement *btreeElement);

  bool IsListEdgeElementEmpty();
  bool IsListPointsEmpty();

#ifdef VORONOI_DEBUG_INFO
  // Отобразить состояние дерева.
  void PrintListsBPA();

  // Отобразить список граней.
//   void PrintEdgeList();

  // Списки арок и брекпоинтов, расположение элементов слева на право.
  std::vector<BtreeElement *> mListArc;
  std::vector<BtreeElement *> mListBP;

  // Создать список арок и список брекпоинтов.
  void GenerateListsBPA();
  void GenerateListsBPA(BtreeElement *node);
#endif

private:
  // Рекурсивные функции

  /// Найти брекпоинт слева от арки.
  BtreeElement *LeftBP(BtreeElement *element);

  /// Найти арку слева от брекпоинта.
  BtreeElement *LeftArc(BtreeElement *element);

  /// Найти брекпоинт справа от арки.
  BtreeElement *RightBP(BtreeElement *element);

  /// Найти арку справа от брекпоинта.
  BtreeElement *RightArc(BtreeElement *element);

  /// Найти арку, в которую попадает текущая координата по x.
  BtreeElement *FindArc(float x);
  BtreeElement *FindArc(BtreeElement *bp, float x);

  /// Удалить дерево.
  void RemoveTree();
  void RemoveTree(BtreeElement *node);
};



















/*
#include "geometry.h"
#include <set>
#include <list>
#include <map>

using namespace geometry;

//#define VORONOI_DEBUG_INFO

#ifdef VORONOI_DEBUG_INFO
template<int T>
struct Val
{
  static int Get()
  {
    static int id = 1;
    return id++;
  }
};
#endif

class Voronoi
{
public:
  struct SiteComparator
  {
    bool operator()(const glm::vec2 &a, const glm::vec2 &b) const
    {
//       if(a.x == b.x)
//         return a.y < b.y;
//       return a.x < b.x;
      if(a.y == b.y)
        return a.x > b.x;
      return a.y > b.y;
    }
  };

  Voronoi(const std::set<glm::vec2, SiteComparator> &points, const glm::vec2 &size);
  ~Voronoi();

  typedef std::map<glm::vec2, std::vector<glm::vec2>, SiteComparator> DiagramData;

  /// Вернуть диаграмму Вороного.
  DiagramData GetDiagram();

  /// Отсортировать вершины полигонов в диаграмме против часовой.
  void Sort();

private:

  /// Интерфейс элемента.
  /// Элемент может быть аркой, точкой пересечения арок, либо точкой пересечения граней.
  struct IElement
  {
    enum ElementType
    {
      ARC,
      BREAK_POINT,
      END_POINT,
    };

    IElement(ElementType t)
     : type(t)
    {}

    const ElementType type;
  };

  /// Входная точка.
  /// Так же является фокусом параболы.
  struct Site
  {
    const Point pos;
#ifdef VORONOI_DEBUG_INFO
    const int id;
#endif
    Site(const Point &p)
     : pos(p)
#ifdef VORONOI_DEBUG_INFO     
     , id(Val<IElement::ARC>::Get())
#endif
    {
    }
  };

  struct CircleEvent;
  /// Арка. Содержит указатель на входную точку.
  /// Так же содержит указатель на событие круга для этой арки, если такое существует.
  struct ArcElement : public IElement
  {
    const Site *site;
    CircleEvent *event;
    ArcElement(const Site *s)
     : IElement(ARC), site(s)
    {
      event = nullptr;
    }
  };

  struct Edge;
  /// Точка пересечения арок.
  /// Содержит позицию пересечения соседних арок и
  /// указатель на грань, один из концов которой является данные брекпоинт.
  struct BPElement : public IElement
  {
#ifdef VORONOI_DEBUG_INFO
    const int id;
#endif
    Edge *edge;
    BPElement()
     : IElement(BREAK_POINT)
#ifdef VORONOI_DEBUG_INFO     
     , id(Val<BREAK_POINT>::Get())
#endif
    {
      edge = nullptr;
    }
  };
  
  /// Точка пересечения граней.
  /// Содержит 3 входных точки, между которыми она находится
  /// и позицию пересечения.
  struct EPElement : public IElement
  {
    const Point pos;
    const Site *site1;
    const Site *site2;
    const Site *site3;
#ifdef VORONOI_DEBUG_INFO    
    const int id;
#endif
    EPElement(const Point &p, const Site *s1, const Site *s2, const Site *s3)
     : IElement(END_POINT), pos(p), site1(s1), site2(s2), site3(s3)
#ifdef VORONOI_DEBUG_INFO
     , id(Val<END_POINT>::Get())
#endif
    {
    }
  };
  
  /// Интерфейс события.
  /// Возможно событие точки или событие круга.
  struct IEvent
  {
    enum EventType
    {
      SITE,
      CIRCLE,
    };

    IEvent(EventType t)
     : type(t)
    {}

    const EventType type;
  };
  
  struct BtreeElement;
  /// Событие круга.
  /// Возникает, если для трех входных точек, лежащих по часовой стрелке
  /// можно построить окружность и самая низкая точка окружности лежит
  /// ниже заметающей прямой.
  /// Создается для точки, расположенной посередине.
  struct CircleEvent : public IEvent // 40 bytes
  {
    BtreeElement *arc;
    const Point center;
    const Point pos;
    CircleEvent(BtreeElement *a, const Point &p, const Point &c)
     : IEvent(CIRCLE), arc(a), center(c), pos(p)
    {
    }
  };
  
  /// Событие точки.
  /// Возникает для каждой входящей точки.
  struct SiteEvent : public IEvent // 8 bytes
  {
    const Site *site;
    SiteEvent(const Site *s)
     : IEvent(SITE), site(s)
    {
    }
  };
  
  /// Элемент дерева.
  struct BtreeElement
  {
    BtreeElement *parent;
    BtreeElement *left;
    BtreeElement *right;
    IElement *element;
    BtreeElement(IElement *el)
    {
      parent = nullptr;
      left = nullptr;
      right = nullptr;
      element = el;
    }
  };
  
  /// Грань.
  /// Содержит указатели на 2 арки и 2 точки соединяющие грань.
  /// Точки может быть указателем на точку пересечения парабол,
  /// либо указателем на точку пересечения граней.
  struct Edge
  {
    IElement *el1;
    IElement *el2;
    const Site *site1;
    const Site *site2;
  };

private:
  // Уровень заметающей прямой.
  // Прямая опускается сверху вниз.
  double mSweepLine;

  // Голова дерева.
  BtreeElement *mHead;

  // Область в которой строится диаграмма вороного.
  // (0;0) - левый нижний угол. Правый верхний угол задается.
  const Rect mRect;

  // Список входных точек.
  // Нужен для корректного освобождения ресурсов.
  std::list<Site *> mSiteList;

  // Список брекпоинтов и точек пересечения граней.
  // Нужен для корректного освобождения ресурсов.
  std::list<IElement *> mListPoints;

  struct EventsComparator
  {
    bool operator()(IEvent *e1, IEvent *e2) const
    {
      const Point &p1 = e1->type == IEvent::SITE ? 
        static_cast<SiteEvent *>(e1)->site->pos :
        static_cast<CircleEvent *>(e1)->pos;
      const Point &p2 = e2->type == IEvent::SITE ? 
        static_cast<SiteEvent *>(e2)->site->pos :
        static_cast<CircleEvent *>(e2)->pos;

      if(p1.y == p2.y)
        return p1.x > p2.x;
      return p1.y > p2.y;
    }
  };

  // Упорядоченный по высоте список событий.
  std::multiset<IEvent *, EventsComparator> mEvents;

  // Список граней.
  std::list<Edge *> mListEdge;

  DiagramData mDiagramData;

  class FinderAnglePoints
  {
  public:
    FinderAnglePoints(const Rect &rect);

    const Rect mRect;

    struct
    {
      double lb;
      double lt;
      double rt;
      double rb;
    } mDistance;

    Point mSitelb;
    Point mSitelt;
    Point mSitert;
    Point mSiterb;

    void Check(const Point &point);

    void Clear();
  } mFinderAnglePoints;

  // Ссылка на список входных данных.
  const std::set<glm::vec2, SiteComparator> &mInPoints;

private:
  // Отладочные функции.
  bool IsList(BtreeElement *btreeElement);
  bool IsNode(BtreeElement *btreeElement);

#ifdef VORONOI_DEBUG_INFO
  // Отобразить состояние дерева.
  void PrintListsBPA();

  // Отобразить список граней.
  void PrintEdgeList();

  // Списки арок и брекпоинтов, расположение элементов слева на право.
  std::vector<BtreeElement *> mListArc;
  std::vector<BtreeElement *> mListBP;

  // Создать список арок и список брекпоинтов.
  void GenerateListsBPA();
  void GenerateListsBPA(BtreeElement *node);
#endif
private:

  /// Добавить самую первую точку.
  void InsertSiteFirstHead(const Site *site);

  /// Добавить точку на самом верхнем уровне.
  void InsertSiteTop(const Site *site);

  /// Добавить точку в дерево.
  void InsertArc(BtreeElement *element, const Site *site);

  /// Удалить арку.
  /// ep - Точка соединения трех граней.
  void RemoveArc(BtreeElement *element, const Point &ep);

  /// Найти арку и брекпоинт слева от текущей
  std::pair<BtreeElement *, BtreeElement *> LeftArcBP(BtreeElement *element);

  /// Найти арку справа от текущей
  std::pair<BtreeElement *, BtreeElement *> RightArcBP(BtreeElement *element);

  /// Проверить событие круга для заданной арки.
  void CheckCircleEvent(BtreeElement *leftArc, BtreeElement *arc, BtreeElement *rightArc);

  /// Удалить событие круга.
  void RemoveCircleEvent(CircleEvent *event);

  /// Добавить новое событие точки.
  void NewSiteEvent(const Site *site);

  /// Добавить новое событие круга.
  void NewCircleEvent(BtreeElement *arc, const Point &point, const Point &center);

  /// Добавить новую грань.
  void NewEdge(IElement *el1, IElement *el2, const Site *site1, const Site *site2);

  /// Обновить список граней.
  void UpdateEdge(BPElement *el1, BPElement *el2, EPElement *ep);

  /// Освободить ресурсы.
  void Release();

  /// Основной цикл алгоритма.
  void Process();

  /// Вычислить грани.
  void CalcEdge();

  /// Вставляем вершины полигона для двух точек.
  void InsertPoligonPoint(const std::list<Point> &points, const Point &site1, const Point &site2);

  /// Добавить новую точку и событие для нее.
  void AddSite(std::set<glm::vec2, SiteComparator>::const_iterator &itSite);

private:
  // Рекурсивные функции

  /// Найти брекпоинт слева от арки.
  BtreeElement *LeftBP(BtreeElement *element);

  /// Найти арку слева от брекпоинта.
  BtreeElement *LeftArc(BtreeElement *element);

  /// Найти брекпоинт справа от арки.
  BtreeElement *RightBP(BtreeElement *element);

  /// Найти арку справа от брекпоинта.
  BtreeElement *RightArc(BtreeElement *element);

  /// Удалить дерево.
  void RemoveTree();
  void RemoveTree(BtreeElement *node);

  /// Функции создания брекпоинтов и точек пересечения граней.
  inline BPElement *NewBPElement();
  inline EPElement *NewEPElement(const Point &p, const Site *s1, const Site *s2, const Site *s3);

  /// Найти арку, в которую попадает текущая координата по x.
  BtreeElement *FindArc(double x);
  BtreeElement *FindArc(BtreeElement *bp, double x);

};
*/
#endif // VORONOI_H
















