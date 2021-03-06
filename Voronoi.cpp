#include "Voronoi.h"


using namespace geometry;

#ifdef VORONOI_DEBUG_INFO
#include <stdio.h>
#endif
#include <algorithm>

#define EPS 0.001

Voronoi::Voronoi()
  : mRect(Point(), Point())
{
  mHead = nullptr;
  mSiteEventsIndex = 0;
}

Voronoi::Voronoi(const std::vector<glm::vec2> &sites, const glm::vec2 &size)
  : mListSite(sites), mRect(Point(), size)
{
  mHead = nullptr;
  mSiteEventsIndex = 0;
}

Voronoi::Voronoi(const Voronoi &voronoi)
  : mListSite(voronoi.mListSite), mRect(voronoi.mRect),
    mListVertex(voronoi.mListVertex), mListEdge(voronoi.mListEdge)
{
  mHead = nullptr;
  mSiteEventsIndex = 0;
}

Voronoi &Voronoi::operator=(const Voronoi &voronoi)
{
  if (this != &voronoi)
  {
    assert(mHead == nullptr);
    mRect = voronoi.mRect;
    mListSite = voronoi.mListSite;
    mListVertex = voronoi.mListVertex;
    mListEdge = voronoi.mListEdge;
  }
  return *this;
}

Voronoi::Voronoi(Voronoi &&voronoi)
  : mListSite(std::move(voronoi.mListSite)), mRect(voronoi.mRect),
    mListVertex(std::move(voronoi.mListVertex)), mListEdge(std::move(voronoi.mListEdge))
{
  mHead = nullptr;
  mSiteEventsIndex = 0;
}

Voronoi &Voronoi::operator=(Voronoi &&voronoi)
{
  if (this != &voronoi)
  {
    assert(mHead == nullptr);
    mRect = voronoi.mRect;
    mListSite = std::move(voronoi.mListSite);
    mListVertex = std::move(voronoi.mListVertex);
    mListEdge = std::move(voronoi.mListEdge);
  }
  return *this;
}

Voronoi::~Voronoi()
{
  assert(mHead == nullptr);
  assert(mSiteEvents.empty());
  assert(mCircleEvents.empty());
  assert(mListPoints.empty());
  assert(mListEdgeElement.empty());

  // Все ресурсы кроме списка граней и списка вершин уже должны быть освобождены.
  // Списки граней и вершин освободятся автоматически.
}

Voronoi &Voronoi::operator()()
{
  assert(mHead == nullptr);
  assert(mSiteEvents.empty());
  assert(mCircleEvents.empty());
  assert(mListPoints.empty());
  assert(mListEdgeElement.empty());
  // Перед построением диаграммы все ресурсы должны быть освобождены.

  // Очищаем списки вершин и граней. Вдруг мы строим диаграмму не в первый раз?
  Clear();

  // Резервируем память.
  mListPoints.reserve(mListSite.size() * 6);
  mListEdgeElement.reserve(mListSite.size() * 3);
  mListVertex.reserve(mListSite.size() * 2);
  mListEdge.reserve(mListSite.size() * 3);

  // Создаем события точек.
  mSiteEventsIndex = 0;
  mSiteEvents.reserve(mListSite.size());
  for(SiteIndex i = 0; i < mListSite.size(); ++i)
  {
    mSiteEvents.push_back(i);
  }

  // Сортируем события точек сверху вниз.
  // Точки лежащие на одной высоте сортируются справа налево.
  std::sort(mSiteEvents.begin(), mSiteEvents.end(), [this](SiteIndex n1, SiteIndex n2) -> bool
  {
    const Point &p1 = mListSite[n1];
    const Point &p2 = mListSite[n2];

    if(p1.y == p2.y)
      return p1.x > p2.x;
    return p1.y > p2.y;
  });

  // Строим диаграмму.
  Process();

  return *this;
}

void Voronoi::Clear()
{
  std::vector<glm::vec2>().swap(mListVertex);
  std::vector<Edge>().swap(mListEdge);
}

void Voronoi::ReleaseProcess()
{
  assert(mSiteEventsIndex == mSiteEvents.size());
  assert(mCircleEvents.empty());
  RemoveTree();

  std::vector<SiteIndex>().swap(mSiteEvents);
  std::multiset<CircleEvent *, CircleEventComparator>().swap(mCircleEvents);
}

void Voronoi::ReleasePostProcess()
{
  assert(IsListEdgeElementEmpty());
  assert(IsListPointsEmpty());

  std::vector<IElement *>().swap(mListPoints);
  std::vector<EdgeElement *>().swap(mListEdgeElement);
}

bool Voronoi::IsList(BtreeElement *btreeElement)
{
  assert(btreeElement);

  return btreeElement->left == nullptr && btreeElement->right == nullptr;
}

bool Voronoi::IsNode(BtreeElement *btreeElement)
{
  assert(btreeElement);

  return !IsList(btreeElement);
}


#ifdef VORONOI_DEBUG_INFO
void Voronoi::GenerateListsBPA()
{
  mListArc.clear();
  mListBP.clear();
  GenerateListsBPA(mHead);
}

void Voronoi::GenerateListsBPA(BtreeElement *node)
{
  if(node == nullptr)
  {
    return;
  }

  if(IsList(node))
  {
    mListArc.push_back(node);
  }
  else
  {
    GenerateListsBPA(node->left);
    mListBP.push_back(node);
    GenerateListsBPA(node->right);
  }
}

void Voronoi::PrintListsBPA()
{
  printf("arcs: ");
  for(auto it = mListArc.begin(); it != mListArc.end(); ++it)
  {
    printf("%i, ", static_cast<ArcElement *>((*it)->element)->site);
  }
  printf("\nbp:     ");
  for(auto it = mListBP.begin(); it != mListBP.end(); ++it)
  {
    printf("%i, ", static_cast<BPElement *>((*it)->element)->id);
  }
  printf("\n");

}

#endif


void Voronoi::InsertSiteFirstHead(const SiteIndex site)
{
  assert(mHead == nullptr);
  mHead = new BtreeElement(new ArcElement(site));
}


void Voronoi::InsertSiteTop(const SiteIndex site)
{
  assert(site < mListSite.size());
  assert(mHead);
  // Точки, лежащие на одном уровне, отсортированы справа налево.
  // Поэтому вставляем в голову дерева поддерево вида:
  //
  //    arc1          BP
  //          ->     |  |
  //              arc2  arc1
  // Где BP - новый брекпоинт, arc2 - новая арка, arc1 - старый корень дерева.

  // Создаем элементы.
  PointIndex newBpIndex = NewBPElement();
  PointIndex bpTopIndex = NewBPElement();
  BtreeElement *newArc = new BtreeElement(new ArcElement(site));
  BtreeElement *newBp = new BtreeElement(mListPoints[newBpIndex]);

  // Связываем элементы дерева.
  newBp->left = newArc;
  newArc->parent = newBp;

  newBp->right = mHead;
  mHead->parent = newBp;

  mHead = newBp;

  // Новая грань появилась между аркой в которую вставляем и новой аркой.
  NewEdge(newBpIndex, bpTopIndex,
          static_cast<ArcElement *>(newArc->element)->site,
          static_cast<ArcElement *>(RightArc(newBp->right)->element)->site);
}


void Voronoi::InsertArc(BtreeElement *btreeElement, const SiteIndex site)
{
  // параметры должны существовать, элемент дерева должен быть листом,
  // листом дерева должна быть арка.
  assert(site < mListSite.size());
  assert(btreeElement);
  assert(IsList(btreeElement));
  assert(btreeElement->element);
  assert(btreeElement->element->type == IElement::ARC);

  // Если существует событие круга для арки, его нужно удалить.
  if(static_cast<ArcElement *>(btreeElement->element)->event)
  {
    RemoveCircleEvent(static_cast<ArcElement *>(btreeElement->element)->event);
    static_cast<ArcElement *>(btreeElement->element)->event = nullptr;
  }

  // Удаляем элемент текущего листа (arc1) и вставляем новое поддерево вида:
  //      BP1
  //     |   |
  //  arc1   BP2
  //        |   |
  //     arc2   arc1

  const SiteIndex siteArc1 = static_cast<ArcElement *>(btreeElement->element)->site;

  // Удаляем элемент узла arc1 и сам узел.
  delete static_cast<ArcElement *>(btreeElement->element);
  btreeElement->element = nullptr;

  // Создаем 4 новых элемента дерева.
  // И 1 элемент заменяем.
  // 3 арки и 2 брекпоинта.

  BtreeElement *arcLeft = new BtreeElement(new ArcElement(siteArc1));
  BtreeElement *arcMid = new BtreeElement(new ArcElement(site));
  BtreeElement *arcRight = new BtreeElement(new ArcElement(siteArc1));

  IElement *bpLeft = mListPoints[NewBPElement()];
  BtreeElement *bpRight = new BtreeElement(mListPoints[NewBPElement()]);

  // Связываем элементы.
  btreeElement->element = bpLeft;

  btreeElement->left = arcLeft;
  arcLeft->parent = btreeElement;

  btreeElement->right = bpRight;
  bpRight->parent = btreeElement;

  bpRight->left = arcMid;
  arcMid->parent = bpRight;

  bpRight->right = arcRight;
  arcRight->parent = bpRight;

  // Проверяем событие круга для левой и правой арки.
   CheckCircleEvent(LeftArcBP(arcLeft).second, arcLeft, arcMid);
   CheckCircleEvent(arcMid, arcRight, RightArcBP(arcRight).second);

  // Новая грань появилась между аркой в которую вставляем и новой аркой.
   NewEdge(static_cast<BPElement *>(bpLeft)->pos,
           static_cast<BPElement *>(bpRight->element)->pos,
           siteArc1, site);
}


void Voronoi::RemoveArc(BtreeElement *arc)
{
  assert(arc);
  assert(arc->element);
  assert(arc->element->type == IElement::ARC);
  assert(static_cast<ArcElement *>(arc->element)->event);

  std::pair<BtreeElement *, BtreeElement *> left = LeftArcBP(arc);
  std::pair<BtreeElement *, BtreeElement *> right = RightArcBP(arc);

  // Арки слева и справа должны существовать.
  assert(left.second && right.second);

  // Ищем левый и правый брекпоинты от арки.
  BtreeElement *bpLeft = left.first;
  BtreeElement *bpRight = right.first;

  // Удаляем событие круга для левой и правой арки.
  if(static_cast<ArcElement *>(left.second->element)->event)
  {
    RemoveCircleEvent(static_cast<ArcElement *>(left.second->element)->event);
  }
  if(static_cast<ArcElement *>(right.second->element)->event)
  {
    RemoveCircleEvent(static_cast<ArcElement *>(right.second->element)->event);
  }

  // Смотрим какой из брекпоинтов является родителем арки, а какой нет.
  BtreeElement *bpArcRemove = arc->parent;  // Родитель арки, нужно удалить
  BtreeElement *bpArcModify = nullptr;      // Не родитель, нужно модифицировать.

  if(bpArcRemove == bpLeft)
  {
    bpArcModify = bpRight;
  }
  else if (bpArcRemove == bpRight)
  {
    bpArcModify = bpLeft;
  }
  assert(bpArcModify);

  // Точка соединения трех граней
  unsigned int endPointPos =
    NewEPElement(static_cast<ArcElement *>(left.second->element)->site,
                 static_cast<ArcElement *>(arc->element)->site,
                 static_cast<ArcElement *>(right.second->element)->site);

  // Новый брекпоинт.
  PointIndex newBreakPointPos = NewBPElement();
  IElement *newBreakPoint = mListPoints[newBreakPointPos];

  // Обновляем текущие грани
  UpdateEdge(static_cast<BPElement *>(bpLeft->element)->pos,
             static_cast<BPElement *>(bpRight->element)->pos,
             endPointPos);
  // И добавляем новую грань.
  NewEdge(newBreakPointPos, endPointPos,
          static_cast<ArcElement *>(left.second->element)->site,
          static_cast<ArcElement *>(right.second->element)->site);

  // Брекпоинты которые мы заменили нам больше не нужны, удалим.
  DeleteBPElement(static_cast<BPElement *>(bpLeft->element)->pos);
  DeleteBPElement(static_cast<BPElement *>(bpRight->element)->pos);

  //      BPM                 BPM
  //     |   |               |   |
  //  arc1   BPR     ->   arc1   arc2
  //        |   |
  //      arc   arc2

  // Изменяем брекпоинты
  // Вместо двух брекпоинтов будет один.
  bpArcRemove->element = nullptr;
  bpArcModify->element = newBreakPoint;

  // Ищем второго ребенка для первого брекпоинта(который нужно удалить).
  // Первый ребенок - наша арка.
  BtreeElement *bpArcChildSecond = nullptr;
  if(bpArcRemove->left == arc)
  {
    bpArcChildSecond = bpArcRemove->right;
  }
  else if(bpArcRemove->right == arc)
  {
    bpArcChildSecond = bpArcRemove->left;
  }
  assert(bpArcChildSecond);

  // Ищем родителя для первого брекпоинта.
  BtreeElement *bpArcFirstParent = bpArcRemove->parent;
  // Соединяем второго ребенка для первого брекпоинта и отца первого брекпоинта.
  if(bpArcFirstParent->left == bpArcRemove)
  {
    bpArcFirstParent->left = bpArcChildSecond;
    bpArcChildSecond->parent = bpArcFirstParent;
  }
  else if(bpArcFirstParent->right == bpArcRemove)
  {
    bpArcFirstParent->right = bpArcChildSecond;
    bpArcChildSecond->parent = bpArcFirstParent;
  }
  else
  {
    assert(true);
  }

  // Удаляем арку и первый брекпоинт.
  delete static_cast<ArcElement *>(arc->element);
  delete arc;
  delete bpArcRemove;

  // Проверяем событие круга для левой и правой арки от удаленной.
   CheckCircleEvent(LeftArcBP(left.second).second, left.second, right.second);
   CheckCircleEvent(left.second, right.second, RightArcBP(right.second).second);
}


void Voronoi::CheckCircleEvent(BtreeElement *leftArc, BtreeElement *arc, BtreeElement *rightArc)
{
  assert(arc);
  if(!leftArc || !rightArc)
  {
    return;
  }
  assert(leftArc->element->type == IElement::ARC);
  assert(arc->element->type == IElement::ARC);
  assert(rightArc->element->type == IElement::ARC);

  // Если событие существует для этой арки, ничего не делаем.
  if(static_cast<ArcElement *>(arc->element)->event)
  {
    return;
  }

  // Проверяем на совпадение точек.
  if(static_cast<ArcElement *>(leftArc->element)->site  == static_cast<ArcElement *>(arc->element)->site ||
    static_cast<ArcElement *>(arc->element)->site       == static_cast<ArcElement *>(rightArc->element)->site ||
    static_cast<ArcElement *>(rightArc->element)->site  == static_cast<ArcElement *>(leftArc->element)->site)
  {
    return;
  }

  // Если точки лежат на одной прямой - выходим.
  double rotation = RotationPoint(mListSite[static_cast<ArcElement *>(leftArc->element)->site],
                                  mListSite[static_cast<ArcElement *>(arc->element)->site],
                                  mListSite[static_cast<ArcElement *>(rightArc->element)->site]);

  if(rotation == 0)
  {
    return;
  }

  // Создаем событие круга, если точки лежат по часовой.
  if(rotation > 0)
  {
    return;
  }

  // Ищем центр окружности по трем точкам.
  Point c = CreateCircle(mListSite[static_cast<ArcElement *>(leftArc->element)->site],
                         mListSite[static_cast<ArcElement *>(arc->element)->site],
                         mListSite[static_cast<ArcElement *>(rightArc->element)->site]);

  // Ищем радиус окружности.
  double r = sqrt(pow(mListSite[static_cast<ArcElement *>(arc->element)->site].x - c.x, 2) +
                  pow(mListSite[static_cast<ArcElement *>(arc->element)->site].y - c.y, 2));

  float posy = static_cast<float>(c.y - r);

  // Добавляем событие в том случае, если оно не выше заметающей прямой.
  if(posy <= mSweepLine + EPS)
  {
    NewCircleEvent(arc, posy);
  }
}

void Voronoi::NewCircleEvent(BtreeElement *arc, float posy)
{
  assert(arc);
  assert(arc->element);
  assert(arc->element->type == IElement::ARC);
  assert(!static_cast<ArcElement *>(arc->element)->event);

  // Создаем событие круга для данной арки.
  CircleEvent *event = new CircleEvent(posy, arc);

  // Добавляем событие в арку
  static_cast<ArcElement *>(arc->element)->event = event;

  // Добавляем событие в список.
  mCircleEvents.insert(event);
}

void Voronoi::RemoveCircleEvent(CircleEvent *event)
{
  assert(event);
  assert(event->arc);
  assert(event->arc->element);
  assert(event->arc->element->type == IElement::ARC);
  assert(static_cast<ArcElement *>(event->arc->element)->event == event);

  // Для данной арки больше нет события круга.
  static_cast<ArcElement *>(event->arc->element)->event = nullptr;

  // Ищем все события с такой высотой.
  auto range = mCircleEvents.equal_range(event);

  // Удаляем только пришедшее событие.
  for(auto it = range.first; it != range.second; ++it)
  {
    if(*it == event)
    {
      mCircleEvents.erase(it);
      break;
    }
  }
  delete event;
}

std::pair<Voronoi::BtreeElement *, Voronoi::BtreeElement *> Voronoi::LeftArcBP(BtreeElement *element)
{
  assert(element);

  BtreeElement *bp = LeftBP(element);
  BtreeElement *arc = nullptr;
  if(bp)
  {
    arc = LeftArc(bp->left);
  }

  return std::pair<BtreeElement *, BtreeElement *>(bp, arc);
}

std::pair<Voronoi::BtreeElement *, Voronoi::BtreeElement *> Voronoi::RightArcBP(BtreeElement *element)
{
  assert(element);

  BtreeElement *bp = RightBP(element);
  BtreeElement *arc = nullptr;
  if(bp)
  {
    arc = RightArc(bp->right);
  }

  return std::pair<BtreeElement *, BtreeElement *>(bp, arc);
}

Voronoi::BtreeElement *Voronoi::LeftBP(BtreeElement *element)
{
  assert(element);

  if(!element->parent)
  {
    return nullptr;
  }

  if(element->parent->left == element)
  {
    return LeftBP(element->parent);
  }

  return element->parent;
}

Voronoi::BtreeElement *Voronoi::LeftArc(BtreeElement *element)
{
  assert(element);

  if(element->right)
  {
    return LeftArc(element->right);
  }

  assert(IsList(element));
  return element;
}

Voronoi::BtreeElement *Voronoi::RightBP(BtreeElement *element)
{
  assert(element);

  if(!element->parent)
  {
    return nullptr;
  }

  if(element->parent->right == element)
  {
    return RightBP(element->parent);
  }

  return element->parent;
}

Voronoi::BtreeElement *Voronoi::RightArc(BtreeElement *element)
{
  assert(element);

  if(element->left)
  {
    return RightArc(element->left);
  }

  assert(IsList(element));
  return element;
}

Voronoi::PointIndex Voronoi::NewBPElement()
{
  BPElement *element = new BPElement(mListPoints.size());
  mListPoints.push_back(element);
  return mListPoints.size() - 1;
}

void Voronoi::DeleteBPElement(PointIndex el)
{
  assert(mListPoints.size() > el);
  assert(mListPoints[el]);
  assert(mListPoints[el]->type == IElement::BREAK_POINT);

  delete static_cast<BPElement *>(mListPoints[el]);
  mListPoints[el] = nullptr;
}

void Voronoi::Process()
{
  if(mSiteEvents.empty())
  {
    return;
  }

  // Вставляем первую арку.
  assert(mSiteEvents[mSiteEventsIndex] < mListSite.size());
  InsertSiteFirstHead(mSiteEvents[mSiteEventsIndex]);
  mSweepLine = mListSite[mSiteEvents[mSiteEventsIndex]].y;
  ++mSiteEventsIndex;

  // Вставляем все самые верхние точки, лежащие на одной высоте.
  while(mSiteEventsIndex < mSiteEvents.size())
  {
    assert(mSiteEvents[mSiteEventsIndex] < mListSite.size());

    if(mSweepLine > mListSite[mSiteEvents[mSiteEventsIndex]].y)
    {
      break;
    }

    InsertSiteTop(mSiteEvents[mSiteEventsIndex]);
    ++mSiteEventsIndex;
  }

  while(mSiteEventsIndex < mSiteEvents.size() || !mCircleEvents.empty())
  {
    auto cEvent = mCircleEvents.begin();

    bool isCircleEvent;

    if(mSiteEventsIndex < mSiteEvents.size() && cEvent != mCircleEvents.end())
    {
      assert(mSiteEvents[mSiteEventsIndex] < mListSite.size());
      // Существуют оба события, выбираем то, которое выше.
      isCircleEvent = (*cEvent)->posy >= mListSite[mSiteEvents[mSiteEventsIndex]].y ? true : false;
    }
    else
    {
      isCircleEvent = cEvent != mCircleEvents.end() ? true : false;
    }

    // Заметающая прямая нужна для вычислений координат брекпоинтов и
    // при возникновении нового события круга.

    if(isCircleEvent)
    {
      mSweepLine = (*cEvent)->posy;
      RemoveArc((*cEvent)->arc);
      delete *cEvent;
      mCircleEvents.erase(cEvent);
    }
    else
    {
      assert(mSiteEvents[mSiteEventsIndex] < mListSite.size());
      mSweepLine = mListSite[mSiteEvents[mSiteEventsIndex]].y;
      // Обрабатываем событие точки.
      InsertArc(FindArc(mListSite[mSiteEvents[mSiteEventsIndex]].x), mSiteEvents[mSiteEventsIndex]);

      // Удаляем событие точки.
      ++mSiteEventsIndex;
    }

    //GenerateListsBPA();
    //PrintListsBPA();
  }
  ReleaseProcess();

  PostProcess();
  ReleasePostProcess();
}

Voronoi::BtreeElement *Voronoi::FindArc(float x)
{
  return FindArc(mHead, x);
}

Voronoi::BtreeElement *Voronoi::FindArc(BtreeElement *bp, float x)
{
  assert(bp);
  assert(bp->element);
  assert(bp->element->type == IElement::BREAK_POINT ||
         bp->element->type == IElement::ARC);

  if(bp->element->type == IElement::ARC)
  {
    assert(IsList(bp));
    return bp;
  }

  // Ищем арки слева и справа от текущего брекпоинта.
  BtreeElement *left = LeftArc(bp->left);
  BtreeElement *right = RightArc(bp->right);

  // Вычисляем x координату брекпоинта.
  float bpx = static_cast<float>(IntersectParabols(mSweepLine,
                                 mListSite[static_cast<ArcElement *>(left->element)->site],
                                 mListSite[static_cast<ArcElement *>(right->element)->site]));

  if(x > bpx)
  {
    return FindArc(bp->right, x);
  }

  return FindArc(bp->left, x);
}


void Voronoi::RemoveTree()
{
  RemoveTree(mHead);
  mHead = nullptr;
}

void Voronoi::RemoveTree(BtreeElement *node)
{
  if(node == nullptr)
  {
    return;
  }

  if(node->left)
  {
    RemoveTree(node->left);
  }

  if(node->right)
  {
    RemoveTree(node->right);
  }

  if(node->element->type == IElement::ARC)
  {
    delete static_cast<ArcElement *>(node->element);
  }
  delete node;
}

bool Voronoi::IsListEdgeElementEmpty()
{
  for(EdgeIndex i = 0; i < static_cast<int>(mListEdgeElement.size()); ++i)
  {
    if(mListEdgeElement[i])
    {
      return false;
    }
  }
  return true;
}

bool Voronoi::IsListPointsEmpty()
{
  for(PointIndex i = 0; i < mListPoints.size(); ++i)
  {
    if(mListPoints[i])
    {
      return false;
    }
  }
  return true;
}

void Voronoi::NewEdge(PointIndex el1, PointIndex el2, const SiteIndex site1, const SiteIndex site2)
{
  assert(site1 < mListSite.size() && site2 < mListSite.size());
  assert(mListPoints[el1] && mListPoints[el2]);
  assert(mListPoints[el1]->type == IElement::BREAK_POINT || mListPoints[el1]->type == IElement::END_POINT);
  assert(mListPoints[el2]->type == IElement::BREAK_POINT || mListPoints[el2]->type == IElement::END_POINT);

  EdgeElement *edge = new EdgeElement(el1, el2, site1, site2);

  mListEdgeElement.push_back(edge);

  if(mListPoints[el1]->type == IElement::BREAK_POINT)
  {
    static_cast<BPElement *>(mListPoints[el1])->edge = mListEdgeElement.size() - 1;
  }
  if(mListPoints[el2]->type == IElement::BREAK_POINT)
  {
    static_cast<BPElement *>(mListPoints[el2])->edge = mListEdgeElement.size() - 1;
  }
}

Voronoi::PointIndex Voronoi::NewEPElement(const SiteIndex s1, const SiteIndex s2, const SiteIndex s3)
{
  assert(s1 < mListSite.size() && s2 < mListSite.size() && s3 < mListSite.size());
  Point point = CreateCircle(mListSite[s1], mListSite[s2], mListSite[s3]);

  VertexIndex pointIndex = -1;
  if(RectContainsPoint(mRect, point))
  {
    pointIndex = NewVertex(point);
  }

  EPElement *element = new EPElement(pointIndex, s1, s2, s3);
  mListPoints.push_back(element);
  return mListPoints.size() - 1;
}

Voronoi::VertexIndex Voronoi::NewVertex(const glm::vec2 &point)
{
  mListVertex.push_back(point);
  return mListVertex.size() - 1;
}

void Voronoi::DeleteEPElement(PointIndex el)
{
  assert(mListPoints.size() > el);
  assert(mListPoints[el]);
  assert(mListPoints[el]->type == IElement::END_POINT);
  assert(static_cast<EPElement *>(mListPoints[el])->refCount > 0);

  --static_cast<EPElement *>(mListPoints[el])->refCount;
  if(static_cast<EPElement *>(mListPoints[el])->refCount == 0)
  {
    delete static_cast<EPElement *>(mListPoints[el]);
    mListPoints[el] = nullptr;
  }
}

void Voronoi::UpdateEdge(PointIndex el1, PointIndex el2, PointIndex ep)
{
  assert(mListPoints[el1] && mListPoints[el2] && mListPoints[ep]);
  assert(mListPoints[el1]->type == IElement::BREAK_POINT && 
         mListPoints[el2]->type == IElement::BREAK_POINT);
  EdgeIndex e1 = static_cast<BPElement *>(mListPoints[el1])->edge;
  EdgeIndex e2 = static_cast<BPElement *>(mListPoints[el2])->edge;
  assert(e1 >= 0 && e2 >= 0);
  assert(mListEdgeElement[e1]->el1 == el1 ||
         mListEdgeElement[e1]->el2 == el1);
  assert(mListEdgeElement[e2]->el1 == el2 ||
         mListEdgeElement[e2]->el2 == el2);

  mListEdgeElement[e1]->el1 == el1 ? mListEdgeElement[e1]->el1 = ep : mListEdgeElement[e1]->el2 = ep;
  static_cast<BPElement *>(mListPoints[el1])->edge = -1;

  mListEdgeElement[e2]->el1 == el2 ? mListEdgeElement[e2]->el1 = ep : mListEdgeElement[e2]->el2 = ep;
  static_cast<BPElement *>(mListPoints[el2])->edge = -1;

  // Если мы нашли грань и она полностью лежит в рабочей области, обработаем ее и удалим.
  if(mListPoints[mListEdgeElement[e1]->el1]->type == IElement::END_POINT &&
     mListPoints[mListEdgeElement[e1]->el2]->type == IElement::END_POINT)
  {
    EPElement *ep1 = static_cast<EPElement *>(mListPoints[mListEdgeElement[e1]->el1]);
    EPElement *ep2 = static_cast<EPElement *>(mListPoints[mListEdgeElement[e1]->el2]);
    if(ep1->pos >= 0 && ep2->pos >= 0)
    {
      mListEdge.push_back(Edge(mListEdgeElement[e1]->site1, mListEdgeElement[e1]->site2,
                                  static_cast<EPElement *>(mListPoints[mListEdgeElement[e1]->el1])->pos,
                                  static_cast<EPElement *>(mListPoints[mListEdgeElement[e1]->el2])->pos));
      DeleteEPElement(mListEdgeElement[e1]->el1);
      DeleteEPElement(mListEdgeElement[e1]->el2);
      DeleteEdge(e1);
    }
  }
  if(mListPoints[mListEdgeElement[e2]->el1]->type == IElement::END_POINT &&
     mListPoints[mListEdgeElement[e2]->el2]->type == IElement::END_POINT)
  {
    EPElement *ep1 = static_cast<EPElement *>(mListPoints[mListEdgeElement[e2]->el1]);
    EPElement *ep2 = static_cast<EPElement *>(mListPoints[mListEdgeElement[e2]->el2]);
    if(ep1->pos >= 0 && ep2->pos >= 0)
    {
      mListEdge.push_back(Edge(mListEdgeElement[e2]->site1, mListEdgeElement[e2]->site2,
                               static_cast<EPElement *>(mListPoints[mListEdgeElement[e2]->el1])->pos,
                               static_cast<EPElement *>(mListPoints[mListEdgeElement[e2]->el2])->pos));
      DeleteEPElement(mListEdgeElement[e2]->el1);
      DeleteEPElement(mListEdgeElement[e2]->el2);
      DeleteEdge(e2);
    }
  }
}

void Voronoi::DeleteEdge(EdgeIndex el)
{
  assert(static_cast<int>(mListEdgeElement.size()) > el);
  assert(mListEdgeElement[el]);

  delete mListEdgeElement[el];
  mListEdgeElement[el] = nullptr;
}

const std::vector<Voronoi::Edge> &Voronoi::GetEdges() const
{
  return mListEdge;
}

const std::vector<glm::vec2> &Voronoi::GetVertex() const
{
  return mListVertex;
}

void Voronoi::PostProcess()
{
  for(EdgeIndex i = 0; i < static_cast<int>(mListEdgeElement.size()); ++i)
  {
    if(mListEdgeElement[i])
    {
      EdgeElement *edge = mListEdgeElement[i];
      assert(edge->site1 < mListSite.size() && edge->site2 < mListSite.size());
      assert(edge->el1 < mListPoints.size() && edge->el2 < mListPoints.size());
      assert(mListPoints[edge->el1]->type == IElement::BREAK_POINT || 
             mListPoints[edge->el1]->type == IElement::END_POINT);
      assert(mListPoints[edge->el2]->type == IElement::BREAK_POINT || 
             mListPoints[edge->el2]->type == IElement::END_POINT);

      if(mListPoints[edge->el1]->type == IElement::END_POINT &&
         mListPoints[edge->el2]->type == IElement::END_POINT)
      {
        // Обрабатываем отрезок.
        // Отрезок не полностью лежит в рабочей области, поэтому его надо обрезать.

        EPElement *ep1 = static_cast<EPElement *>(mListPoints[edge->el1]);
        EPElement *ep2 = static_cast<EPElement *>(mListPoints[edge->el2]);
        assert(ep1 && ep2);

        Point p1 = ep1->pos >= 0 ? Point(mListVertex[ep1->pos]) : 
          CreateCircle(mListSite[ep1->site1], mListSite[ep1->site2], mListSite[ep1->site3]);

        Point p2 = ep2->pos >= 0 ? Point(mListVertex[ep2->pos]) : 
          CreateCircle(mListSite[ep2->site1], mListSite[ep2->site2], mListSite[ep2->site3]);

        auto points = IntersectRectSegment(mRect, Segment(p1, p2));

        if(points.size() == 2)
        {
          VertexIndex v1 = ep1->pos >= 0 ? ep1->pos : NewVertex(points[0]);
          VertexIndex v2 = ep2->pos >= 0 ? ep2->pos : NewVertex(points[1]);

          mListEdge.push_back(Edge(edge->site1, edge->site2, v1, v2));
        }

        DeleteEPElement(edge->el1);
        DeleteEPElement(edge->el2);
        DeleteEdge(i);
        
        continue;
      }
      if(mListPoints[edge->el1]->type == IElement::BREAK_POINT &&
         mListPoints[edge->el2]->type == IElement::BREAK_POINT)
      {
        // Обрабатываем прямую
        auto points = IntersectRectLine(
          mRect, Perpendicular(Line(mListSite[edge->site1], mListSite[edge->site2]),
                               Center(mListSite[edge->site1], mListSite[edge->site2])));
       
        if(points.size() == 2)
        {
          VertexIndex v1 = NewVertex(points[0]);
          VertexIndex v2 = NewVertex(points[1]);

          mListEdge.push_back(Edge(edge->site1, edge->site2, v1, v2));
        }

        DeleteBPElement(edge->el1);
        DeleteBPElement(edge->el2);
        DeleteEdge(i);

        continue;
      }

      EPElement *ep = mListPoints[edge->el1]->type == IElement::END_POINT ? 
        static_cast<EPElement *>(mListPoints[edge->el1]) :
        static_cast<EPElement *>(mListPoints[edge->el2]);

      // Ищем точку C в треугольнике.
      SiteIndex dirPoint = ep->site1;
      if(dirPoint == edge->site1 || dirPoint == edge->site2)
      {
        dirPoint = ep->site2;
        if(dirPoint == edge->site1 || dirPoint == edge->site2)
        {
          dirPoint = ep->site3;
          assert(dirPoint != edge->site1 && dirPoint != edge->site2);
        }
      }
    
      // Ищем срединный перпендикуляр к AB.
      // Эта линия должна проходить через E (центр окружности).
      Point center = Center(mListSite[edge->site1], mListSite[edge->site2]);
      Line rayLine = Perpendicular(Line(mListSite[edge->site1], mListSite[edge->site2]), center);

      // Ищем еще один перпендикуляр к данной линии в точку C.
      Line perpRay = Perpendicular(rayLine, mListSite[dirPoint]);

      Point point = ep->pos >= 0 ? Point(mListVertex[ep->pos]) : 
        CreateCircle(mListSite[ep->site1], mListSite[ep->site2], mListSite[ep->site3]);

      Point dir = point - IntersectLines(rayLine, perpRay);

      auto points = IntersectRectRay(mRect, Ray(point, center + dir));

      if(points.size() == 2)
      {
        VertexIndex v1 = ep->pos >= 0 ? ep->pos : NewVertex(points[0]);
        VertexIndex v2 = NewVertex(points[1]);

        mListEdge.push_back(Edge(edge->site1, edge->site2, v1, v2));
      }

      mListPoints[edge->el1]->type == IElement::END_POINT ? 
        DeleteEPElement(edge->el1) : DeleteBPElement(edge->el1);
      mListPoints[edge->el2]->type == IElement::END_POINT ? 
        DeleteEPElement(edge->el2) : DeleteBPElement(edge->el2);
      DeleteEdge(i);
    }
  }
}

const std::vector<glm::vec2> &Voronoi::GetSites() const
{
  return mListSite;
}

