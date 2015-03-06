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

  /// Грань.
  /// Содержит индексы на две точки в списке точек, лежащих слева и справа от грани.
  /// Так же содержит индексы на две вершины в списке вершин, лежащих на концах грани.
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

  /// Конструктор.
  /// @param sites Список точек. Точки не должен содержать одинаковых точек.
  /// @param size Размер рабочей области.
  Voronoi(const std::vector<glm::vec2> &sites, const glm::vec2 &size);

  ~Voronoi();

  /// Построить диаграмму вороного.
  void operator()();

  /// Очистить диаграмму вороного.
  /// Освобождаются списки вершин и граней.
  /// Список точек не освобождается.
  void Clear();

  /// Вернуть список точек.
  std::vector<glm::vec2> &GetSites();

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
  std::vector<glm::vec2> mListSite;

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

  VertexIndex NewVertex(const glm::vec2 &point);

  /// Добавить новую грань.
  void NewEdge(PointIndex el1, PointIndex el2, const SiteIndex site1, const SiteIndex site2);

  /// Обновить список граней.
  void UpdateEdge(PointIndex el1, PointIndex el2, PointIndex ep);

  /// Удалить грань.
  void DeleteEdge(EdgeIndex el);

  /// Обработать оставшиеся грани.
  void PostProcess();

private:
  // Отладочные функции.
  bool IsList(BtreeElement *btreeElement);
  bool IsNode(BtreeElement *btreeElement);

  bool IsListEdgeElementEmpty();
  bool IsListPointsEmpty();

#ifdef VORONOI_DEBUG_INFO
  // Отобразить состояние дерева.
  void PrintListsBPA();

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

#endif // VORONOI_H
















