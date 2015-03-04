#include "Voronoi.h"


#ifdef VORONOI_DEBUG_INFO
#include <stdio.h>
#endif
#include <algorithm>

#define EPS 0.001

Voronoi::Voronoi(const std::vector<glm::vec2> &sites, const glm::vec2 &size)
  : mListSite(sites), mRect(Point(), size)
{
  mHead = nullptr;

  mListPoints.reserve(mListSite.size() * 7);
  mListEdge.reserve(mListSite.size() * 3);
  mListVertex.reserve(mListSite.size() * 3);

  // Создаем события точек.
  mSiteEventsIndex = 0;
  mSiteEvents.reserve(mListSite.size());
  for(SiteIndex i = 0; i < mListSite.size(); ++i)
  {
    //mFinderAnglePoints.Check(i);
    mSiteEvents.push_back(i);
  }

  std::sort(mSiteEvents.begin(), mSiteEvents.end(), [this](SiteIndex n1, SiteIndex n2) -> bool
  {
    const Point &p1 = mListSite[n1];
    const Point &p2 = mListSite[n2];

    if(p1.y == p2.y)
      return p1.x > p2.x;
    return p1.y > p2.y;
  });

  printf("Start Process\n");

  Process();

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

// void Voronoi::PrintEdgeList()
// {
//
//   for(auto it = mListEdge.begin(); it != mListEdge.end(); ++it)
//   {
//     printf("[e: {%s:%s}]\n",
//       (*it)->el1->type == IElement::BREAK_POINT ? "b" : "p",
//       (*it)->el2->type == IElement::BREAK_POINT ? "b" : "p"
//       );
//   }
//
// }
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

  printf("point count: %i\n", mListPoints.size());
  printf("edge count: %i\n", mListEdge.size());
  printf("vertex count: %i\n", mListVertex.size());

  ReleaseProcess();
  ReleasePostProcess();
  //CalcEdge();
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


void Voronoi::ReleaseProcess()
{
  assert(mSiteEventsIndex == mSiteEvents.size());
  assert(mCircleEvents.empty());
  RemoveTree();

  {
    std::vector<SiteIndex> tmp;
    std::swap(mSiteEvents, tmp);
  }
  {
    std::multiset<CircleEvent *, CircleEventComparator> tmp;
    std::swap(mCircleEvents, tmp);
  }
}

void Voronoi::ReleasePostProcess()
{
  //assert(mListEdge.empty());
  
  for(PointIndex i = 0; i < mListPoints.size(); ++i)
  {
    if(mListPoints[i])
    {
      if(mListPoints[i]->type == IElement::BREAK_POINT)
      {
        DeleteBPElement(i);
      }
      else
      if(mListPoints[i]->type == IElement::END_POINT)
      {
        unsigned int refCount = static_cast<EPElement *>(mListPoints[i])->refCount;
        for(unsigned int j = 0; j < refCount; ++j)
        {
          DeleteEPElement(i);
        }
      }
    }
  }
  for(EdgeIndex i = 0; i < static_cast<int>(mListEdge.size()); ++i)
  {
    if(mListEdge[i])
    {
      DeleteEdge(i);
    }
  }
  {
    std::vector<IElement *> tmp;
    std::swap(mListPoints, tmp);
  }
  {
    std::vector<Edge *> tmp;
    std::swap(mListEdge, tmp);
  }
  {
    std::vector<glm::vec2> tmp;
    std::swap(mListVertex, tmp);
  }
}

void Voronoi::NewEdge(PointIndex el1, PointIndex el2, const SiteIndex site1, const SiteIndex site2)
{
  assert(site1 < mListSite.size() && site2 < mListSite.size());
  assert(mListPoints[el1] && mListPoints[el2]);
  assert(mListPoints[el1]->type == IElement::BREAK_POINT || mListPoints[el1]->type == IElement::END_POINT);
  assert(mListPoints[el2]->type == IElement::BREAK_POINT || mListPoints[el2]->type == IElement::END_POINT);

  Edge *edge = new Edge(el1, el2, site1, site2);

  mListEdge.push_back(edge);

  if(mListPoints[el1]->type == IElement::BREAK_POINT)
  {
    static_cast<BPElement *>(mListPoints[el1])->edge = mListEdge.size() - 1;
  }
  if(mListPoints[el2]->type == IElement::BREAK_POINT)
  {
    static_cast<BPElement *>(mListPoints[el2])->edge = mListEdge.size() - 1;
  }
}

Voronoi::PointIndex Voronoi::NewEPElement(const SiteIndex s1, const SiteIndex s2, const SiteIndex s3)
{
  assert(s1 < mListSite.size() && s2 < mListSite.size() && s3 < mListSite.size());
  Point point = CreateCircle(mListSite[s1], mListSite[s2], mListSite[s3]);

  VertexIndex pointIndex = -1;
  if(RectContainsPoint(mRect, point))
  {
    mListVertex.push_back(point);
    pointIndex = mListVertex.size() - 1;
  }

  EPElement *element = new EPElement(pointIndex, s1, s2, s3);
  mListPoints.push_back(element);
  return mListPoints.size() - 1;
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
  assert(mListEdge[e1]->el1 == el1 ||
         mListEdge[e1]->el2 == el1);
  assert(mListEdge[e2]->el1 == el2 ||
         mListEdge[e2]->el2 == el2);

  mListEdge[e1]->el1 == el1 ? mListEdge[e1]->el1 = ep : mListEdge[e1]->el2 = ep;
  static_cast<BPElement *>(mListPoints[el1])->edge = -1;

  mListEdge[e2]->el1 == el2 ? mListEdge[e2]->el1 = ep : mListEdge[e2]->el2 = ep;
  static_cast<BPElement *>(mListPoints[el2])->edge = -1;

  // Если мы нашли грань и она полностью лежит в рабочей области, обработаем ее и удалим.
  if(mListPoints[mListEdge[e1]->el1]->type == IElement::END_POINT &&
     mListPoints[mListEdge[e1]->el2]->type == IElement::END_POINT)
  {
    EPElement *ep1 = static_cast<EPElement *>(mListPoints[mListEdge[e1]->el1]);
    EPElement *ep2 = static_cast<EPElement *>(mListPoints[mListEdge[e1]->el2]);
    if(ep1->pos >= 0 && ep2->pos >= 0)
    {
      DeleteEPElement(mListEdge[e1]->el1);
      DeleteEPElement(mListEdge[e1]->el2);
      DeleteEdge(e1);
    }
  }
  if(mListPoints[mListEdge[e2]->el1]->type == IElement::END_POINT &&
     mListPoints[mListEdge[e2]->el2]->type == IElement::END_POINT)
  {
    EPElement *ep1 = static_cast<EPElement *>(mListPoints[mListEdge[e2]->el1]);
    EPElement *ep2 = static_cast<EPElement *>(mListPoints[mListEdge[e2]->el2]);
    if(ep1->pos >= 0 && ep2->pos >= 0)
    {
      DeleteEPElement(mListEdge[e2]->el1);
      DeleteEPElement(mListEdge[e2]->el2);
      DeleteEdge(e2);
    }
  }
}

void Voronoi::DeleteEdge(EdgeIndex el)
{
  assert(static_cast<int>(mListEdge.size()) > el);
  assert(mListEdge[el]);

  delete mListEdge[el];
  mListEdge[el] = nullptr;
}

/*
#include <stdio.h>
#include <algorithm>

#define EPS 0.001

Voronoi::Voronoi(const std::set<glm::vec2, SiteComparator> &points, const glm::vec2 &size)
  : mHead(nullptr), mRect(Point(0, 0), Point(size)), mFinderAnglePoints(mRect), mInPoints(points)
{
  // Добавляем точки и создаем события для них.
//   for(auto it = points.begin(); it != points.end(); ++it)
//   {
//     Site *site = new Site(*it);
//     mSiteList.push_back(site);
//     mFinderAnglePoints.Check(site->pos);
//     NewSiteEvent(site);
//   }

  Process();

//   if(!mEvents.empty())
//   {
//     Process();
//   }
  
  //PrintEdgeList();
  Release();
}

Voronoi::~Voronoi()
{
  Release();
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

void Voronoi::InsertSiteFirstHead(const Site *site)
{
  assert(site);
  assert(mHead == nullptr);
  mHead = new BtreeElement(new ArcElement(site));
}

void Voronoi::InsertArc(BtreeElement *btreeElement, const Site *site)
{
  // параметры должны существовать, элемент дерева должен быть листом,
  // листом дерева должна быть арка.
  assert(site);
  assert(btreeElement);
  assert(IsList(btreeElement));
  assert(btreeElement->element);
  assert(btreeElement->element->type == IElement::ARC);
  assert(static_cast<ArcElement *>(btreeElement->element)->site);

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

  const Site *siteArc1 = static_cast<ArcElement *>(btreeElement->element)->site;

  // Удаляем элемент узла arc1 и сам узел.
  delete static_cast<ArcElement *>(btreeElement->element);
  btreeElement->element = nullptr;

  // Создаем 4 новых элемента дерева.
  // И 1 элемент заменяем.
  // 3 арки и 2 брекпоинта.

  BtreeElement *arcLeft = new BtreeElement(new ArcElement(siteArc1));
  BtreeElement *arcMid = new BtreeElement(new ArcElement(site));
  BtreeElement *arcRight = new BtreeElement(new ArcElement(siteArc1));

  IElement *bpLeft = NewBPElement();
  BtreeElement *bpRight = new BtreeElement(NewBPElement());

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
  NewEdge(bpLeft, bpRight->element, siteArc1, site);
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
    printf("%i, ", static_cast<ArcElement *>((*it)->element)->site->id);
  }
  printf("\nbp: ");
  for(auto it = mListBP.begin(); it != mListBP.end(); ++it)
  {
    printf("%i, ", static_cast<BPElement *>((*it)->element)->id);
  }
  printf("\n");

}

void Voronoi::PrintEdgeList()
{
  
  for(auto it = mListEdge.begin(); it != mListEdge.end(); ++it)
  {
    printf("[e: {%s:%s}]\n",
           (*it)->el1->type == IElement::BREAK_POINT ? "b" : "p",
           (*it)->el2->type == IElement::BREAK_POINT ? "b" : "p"
          );
  }
  
}
#endif

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

Voronoi::BPElement *Voronoi::NewBPElement()
{
  BPElement *element = new BPElement();
  mListPoints.push_back(element);
  return element;
}

Voronoi::EPElement *Voronoi::NewEPElement(const Point &p, const Site *s1, const Site *s2, const Site *s3)
{
  EPElement *element = new EPElement(p, s1, s2, s3);
  mListPoints.push_back(element);
  return element;
}

void Voronoi::RemoveArc(BtreeElement *arc, const Point &ep)
{
  assert(arc);
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
  EPElement *endPoint = NewEPElement(ep, static_cast<ArcElement *>(left.second->element)->site,
                                         static_cast<ArcElement *>(arc->element)->site,
                                         static_cast<ArcElement *>(right.second->element)->site);

  // Новый брекпоинт.
  BPElement *newBreakPoint = NewBPElement();

  // Обновляем текущие грани
  UpdateEdge(static_cast<BPElement *>(bpLeft->element), 
             static_cast<BPElement *>(bpRight->element), 
             endPoint);
  // И добавляем новую грань.
  NewEdge(newBreakPoint, endPoint, 
          static_cast<ArcElement *>(left.second->element)->site, 
          static_cast<ArcElement *>(right.second->element)->site);


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
     static_cast<ArcElement *>(arc->element)->site      == static_cast<ArcElement *>(rightArc->element)->site ||
     static_cast<ArcElement *>(rightArc->element)->site == static_cast<ArcElement *>(leftArc->element)->site)
  {
    return;
  }

  // Если точки лежат на одной прямой - выходим.
  double rotation = RotationPoint(static_cast<ArcElement *>(leftArc->element)->site->pos,
                                  static_cast<ArcElement *>(arc->element)->site->pos,
                                  static_cast<ArcElement *>(rightArc->element)->site->pos);

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
  Point c = CreateCircle(static_cast<ArcElement *>(leftArc->element)->site->pos,
                         static_cast<ArcElement *>(arc->element)->site->pos,
                         static_cast<ArcElement *>(rightArc->element)->site->pos);

  // Ищем радиус окружности.
  double r = sqrt(pow(static_cast<ArcElement *>(arc->element)->site->pos.x - c.x, 2) +
                  pow(static_cast<ArcElement *>(arc->element)->site->pos.y - c.y, 2));

  Point pos(c.x, c.y - r);

  // Добавляем событие в том случае, если оно не выше заметающей прямой.
  if(pos.y <= mSweepLine + EPS)
  {
    NewCircleEvent(arc, pos, c);
  }
}

void Voronoi::NewCircleEvent(BtreeElement *arc, const Point &point, const Point &center)
{
  assert(arc);
  assert(arc->element);
  assert(arc->element->type == IElement::ARC);
  assert(!static_cast<ArcElement *>(arc->element)->event);

  // Создаем событие круга для данной арки.
  CircleEvent *event = new CircleEvent(arc, point, center);

  // Добавляем событие в арку
  static_cast<ArcElement *>(arc->element)->event = event;

  // Добавляем событие в список.
  mEvents.insert(event);
}

void Voronoi::RemoveCircleEvent(CircleEvent *event)
{
  assert(event);
  assert(event->type == IEvent::CIRCLE);
  assert(event->arc);
  assert(event->arc->element);
  assert(event->arc->element->type == IElement::ARC);
  assert(static_cast<ArcElement *>(event->arc->element)->event == event);
  
  // Для данной арки больше нет события круга.
  static_cast<ArcElement *>(event->arc->element)->event = nullptr;
  
  // Ищем все события с такой высотой.
  auto range = mEvents.equal_range(event);

  // Удаляем только пришедшее событие.
  for(auto it = range.first; it != range.second; ++it)
  {
    if(*it == event)
    {
      mEvents.erase(it);
      break;
    }
  }
  delete static_cast<CircleEvent *>(event);
}


void Voronoi::NewSiteEvent(const Site *site)
{
  assert(site);
  // Создаем событие точки.
  SiteEvent *event = new SiteEvent(site);

  // Добавляем событие в список.
  mEvents.insert(event);
}

void Voronoi::NewEdge(IElement *el1, IElement *el2, const Site *site1, const Site *site2)
{
  assert(el1 && el2 && site1 && site2);
  assert(el1->type == IElement::BREAK_POINT || el1->type == IElement::END_POINT);
  assert(el2->type == IElement::BREAK_POINT || el2->type == IElement::END_POINT);

  Edge *edge = new Edge;
  
  edge->el1 = el1;
  edge->el2 = el2;
  edge->site1 = site1;
  edge->site2 = site2;

  if(el1->type == IElement::BREAK_POINT)
  {
    static_cast<BPElement *>(el1)->edge = edge;
  }
  if(el2->type == IElement::BREAK_POINT)
  {
    static_cast<BPElement *>(el2)->edge = edge;
  }

  mListEdge.push_back(edge);
}

void Voronoi::UpdateEdge(BPElement *el1, BPElement *el2, EPElement *ep)
{
  assert(el1 && el2 && ep);
  assert(el1->edge && el2->edge);
  assert(el1->edge->el1 == el1 || el1->edge->el2 == el1);
  assert(el2->edge->el1 == el2 || el2->edge->el2 == el2);

  el1->edge->el1 == el1 ? el1->edge->el1 = ep : el1->edge->el2 = ep;
  el1->edge = nullptr;

  el2->edge->el1 == el2 ? el2->edge->el1 = ep : el2->edge->el2 = ep;
  el2->edge = nullptr;
}

void Voronoi::Release()
{
  for(auto it = mSiteList.begin(); it != mSiteList.end(); ++it)
  {
    delete (*it);
  }

  for(auto it = mListPoints.begin(); it != mListPoints.end(); ++it)
  {
    assert((*it)->type == IElement::BREAK_POINT || (*it)->type == IElement::END_POINT);
    
    if((*it)->type == IElement::BREAK_POINT)
    {
      delete static_cast<BPElement *>(*it);
      continue;
    }
    if((*it)->type == IElement::END_POINT)
    {
      delete static_cast<EPElement *>(*it);
      continue;
    }  
  }

  for(auto it = mListEdge.begin(); it != mListEdge.end(); ++it)
  {
    delete (*it);
  }

  mSiteList.clear();
  mListPoints.clear();
  mEvents.clear();
  mListEdge.clear();
}

Voronoi::BtreeElement *Voronoi::FindArc(double x)
{
  return FindArc(mHead, x);
}

Voronoi::BtreeElement *Voronoi::FindArc(BtreeElement *bp, double x)
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
  double bpx = IntersectParabols(mSweepLine,
                                 static_cast<ArcElement *>(left->element)->site->pos, 
                                 static_cast<ArcElement *>(right->element)->site->pos);

  if(x > bpx)
  {
    return FindArc(bp->right, x);
  }

  return FindArc(bp->left, x);
}

void Voronoi::Process()
{
  printf("Process\n");

  // Оптимизация.
  // Раньше все точки и события добавлялись в списки до построения диаграммы.
  // Сейчас точки будут добавляться во время процесса построения.
  // Так расходуется меньше памяти и скорость удаления события круга существенно увеличивается,
  // т.к. количество событий в списке событий много меньше.
  auto itSite = mInPoints.begin();
  AddSite(itSite);

  // Вставляем первую арку.
  std::multiset<IEvent *, EventsComparator>::const_iterator event = mEvents.begin();
  assert((*event)->type == IEvent::SITE);
  InsertSiteFirstHead(static_cast<SiteEvent *>(*event)->site);
  mSweepLine = static_cast<SiteEvent *>(*event)->site->pos.y;

  // Удаляем событие точки.
  delete static_cast<SiteEvent *>(*event);
  mEvents.erase(event);

  AddSite(itSite);

  // Вставляем все самые верхние точки, лежащие на одной высоте.
  while(!mEvents.empty())
  {
    event = mEvents.begin();
    assert((*event)->type == IEvent::SITE);

    if(mSweepLine > static_cast<SiteEvent *>(*event)->site->pos.y)
    {
      break;
    }
    InsertSiteTop(static_cast<SiteEvent *>(*event)->site);
    delete static_cast<SiteEvent *>(*event);
    mEvents.erase(event);

    AddSite(itSite);
  }

  while(!mEvents.empty())
  {
    event = mEvents.begin();
    // Заметающая прямая нужна для вычислений координат брекпоинтов и
    // при возникновении нового события круга.
    //mSweepLine = (*event)->pos.y;

    switch((*event)->type)
    {
    case IEvent::SITE:
      {
        mSweepLine = static_cast<SiteEvent *>(*event)->site->pos.y;
        // Обрабатываем событие точки.
        InsertArc(FindArc(static_cast<SiteEvent *>(*event)->site->pos.x), static_cast<SiteEvent *>(*event)->site);
        delete static_cast<SiteEvent *>(*event);
        mEvents.erase(event);

        AddSite(itSite);
      }
      break;
    case IEvent::CIRCLE:
      {
        mSweepLine = static_cast<CircleEvent *>(*event)->pos.y;
        RemoveArc(static_cast<CircleEvent *>(*event)->arc, static_cast<CircleEvent *>(*event)->center);
        delete static_cast<CircleEvent *>(*event);
        mEvents.erase(event);
      }
      break;
    default:
      assert(true);
    }

    //GenerateListsBPA();
    //PrintListsBPA();
  }

  RemoveTree();
  CalcEdge();
}

void Voronoi::CalcEdge()
{
  printf("PostProcess\n");
  for(auto it = mListEdge.begin(); it != mListEdge.end(); ++it)
  {
    Edge *edge = (*it);
    assert(edge->el1->type == IElement::BREAK_POINT || 
           edge->el1->type == IElement::END_POINT);
    assert(edge->el2->type == IElement::BREAK_POINT || 
           edge->el2->type == IElement::END_POINT);
    assert(edge->site1 && edge->site2);

    if(edge->el1->type == IElement::END_POINT &&
       edge->el2->type == IElement::END_POINT)
    {
      // Обрабатываем отрезок.
      auto points = IntersectRectSegment(mRect, Segment(static_cast<EPElement *>(edge->el1)->pos,
                                                        static_cast<EPElement *>(edge->el2)->pos));

      InsertPoligonPoint(points, edge->site1->pos, edge->site2->pos);
      continue;
    }
    if(edge->el1->type == IElement::BREAK_POINT &&
       edge->el2->type == IElement::BREAK_POINT)
    {
      // Обрабатываем прямую.
      auto points = IntersectRectLine(mRect, 
                                      Perpendicular(Line(edge->site1->pos, edge->site2->pos),
                                      Center(edge->site1->pos, edge->site2->pos)));
      
      InsertPoligonPoint(points, edge->site1->pos, edge->site2->pos);

      continue;
    }
    // Обрабатываем луч.
    EPElement *ep = edge->el1->type == IElement::END_POINT ? 
      static_cast<EPElement *>(edge->el1) :
      static_cast<EPElement *>(edge->el2);

    // Ищем точку C в треугольнике.
    const Site *dirPoint = ep->site1;
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
    Point center = Center(edge->site1->pos, edge->site2->pos);
    Line rayLine = Perpendicular(Line(edge->site1->pos, edge->site2->pos), center);
    
    // Ищем еще один перпендикуляр к данной линии в точку C.
    Line perpRay = Perpendicular(rayLine, dirPoint->pos);
    Point dir = ep->pos - IntersectLines(rayLine, perpRay);

    auto points = IntersectRectRay(mRect, Ray(ep->pos, center + dir));

    InsertPoligonPoint(points, edge->site1->pos, edge->site2->pos);

    continue;
  }

  mDiagramData[mFinderAnglePoints.mSitelb].push_back(mRect.lb - mFinderAnglePoints.mSitelb);
  mDiagramData[mFinderAnglePoints.mSitelt].push_back(Point(0, mRect.rt.y) - mFinderAnglePoints.mSitelt);
  mDiagramData[mFinderAnglePoints.mSitert].push_back(mRect.rt - mFinderAnglePoints.mSitert);
  mDiagramData[mFinderAnglePoints.mSiterb].push_back(Point(mRect.rt.x, 0) - mFinderAnglePoints.mSiterb);
}

void Voronoi::InsertPoligonPoint(const std::list<Point> &points, const Point &site1, const Point &site2)
{
  for(auto it = points.begin(); it != points.end(); ++it)
  {
    const glm::vec2 p1((*it) - site1);
    const glm::vec2 p2((*it) - site2);

    auto &list1 = mDiagramData[site1];
    auto &list2 = mDiagramData[site2];

    // Элементы в списке граней расположены геометрически примерно сверху вниз.
    // Соответственно массив точек points подается практически также.
    // В этом же порядке мы добавляем точки в список полигонов.
    // Это означает, что необходимо проверять точки на дубликаты, 
    // проходясь с конца списка вершин в полигоне. Велика вероятность, 
    // что надо сделать всего пару проверок.

    if(list1.empty())
    {
      list1.reserve(9);
      list1.push_back(p1);
    }
    else
    if(std::find(list1.rbegin(), list1.rend(), p1) == list1.rend())
    {
      list1.push_back(p1);
    }

    if(list2.empty())
    {
      list2.reserve(9);
      list2.push_back(p2);
    }
    else
    if(std::find(list2.rbegin(), list2.rend(), p2) == list2.rend())
    {
      list2.push_back(p2);
    }
    //mDiagramData[site1].push_back((*it) - site1);
    //mDiagramData[site2].push_back((*it) - site2);
  }


  //for(auto it = mDiagramData.begin(); it != mDiagramData.end(); ++it)
  //{
  //  for(auto jt = (*it).second.begin(); jt != (*it).second.end(); ++jt)
  //  {
  //     printf("k: [%f, %f], v: [%f, %f]\n", (*it).first.x, (*it).first.y, (*jt).x, (*jt).y);
  //  }
  //}

}

Voronoi::DiagramData Voronoi::GetDiagram()
{
  return mDiagramData;
}

void Voronoi::Sort()
{
//   struct
//   {
//     bool operator()(const glm::vec2 &a, const glm::vec2 &b) const
//     {
//       return atan2(a.y, a.x) > atan2(b.y, b.x);
//     }
//   } rotationComparator;

  // Проходим по всем полигонам и сортируем вершины полигонов против часовой.
  for(auto it = mDiagramData.begin(); it != mDiagramData.end(); ++it)
  {
    auto &list = (*it).second;
    //std::sort(list.begin(), list.end(), rotationComparator);

    // Ищем самую левую точку.
    // Если таких точек несколько - выбираем верхную
    auto itPos = list.begin();
    for(auto it = list.begin(); it != list.end(); ++it)
    {
      if((*itPos).x > (*it).x)
      {
        itPos = it;
      }
      else if((*itPos).x == (*it).x)
      {
        if((*itPos).y <= (*it).y)
        {
          itPos = it;
        }
      }
    }

    std::swap(*list.begin(), *itPos);
    itPos = list.begin();
    auto const &point = *itPos;
    ++itPos;

    std::sort(itPos, list.end(), [point](const glm::vec2 &a,const glm::vec2 &b)
    {
      return RotationPoint(point, a, b) > 0;
    });

  }
}

void Voronoi::AddSite(std::set<glm::vec2, SiteComparator>::const_iterator &itSite)
{
  if(itSite != mInPoints.end())
  {
    Site *site = new Site(*itSite);
    mSiteList.push_back(site);
    mFinderAnglePoints.Check(site->pos);
    NewSiteEvent(site);
    ++itSite;
  }
}

void Voronoi::InsertSiteTop(const Site *site)
{
  assert(site);
  assert(mHead);
  // Точки, лежащие на одном уровне, отсортированы справа налево.
  // Поэтому вставляем в голову дерева поддерево вида:
  //
  //    arc1          BP
  //          ->     |  |
  //              arc2  arc1
  // Где BP - новый брекпоинт, arc2 - новая арка, arc1 - старый корень дерева.

  // Создаем элементы.
  BtreeElement *newArc = new BtreeElement(new ArcElement(site));
  BtreeElement *newBp = new BtreeElement(NewBPElement());
  IElement *bpTop = NewBPElement();

  // Связываем элементы дерева.
  newBp->left = newArc;
  newArc->parent = newBp;

  newBp->right = mHead;
  mHead->parent = newBp;

  mHead = newBp;

  // Новая грань появилась между аркой в которую вставляем и новой аркой.
  NewEdge(newBp->element, bpTop, 
          static_cast<ArcElement *>(newArc->element)->site, 
          static_cast<ArcElement *>(RightArc(newBp->right)->element)->site);
}


Voronoi::FinderAnglePoints::FinderAnglePoints(const Rect &rect)
  : mRect(rect)
{
  Clear();
}

void Voronoi::FinderAnglePoints::Check(const Point &point)
{
  // Измеряем квадрат расстояния от этой точки до каждого из углов.

  double distlb = (point.x - 0) * (point.x - 0) + (point.y - 0) * (point.y - 0);
  double distlt = (point.x - 0) * (point.x - 0) + (point.y - mRect.rt.y) * (point.y - mRect.rt.y);
  double distrt = (point.x - mRect.rt.x) * (point.x - mRect.rt.x) + (point.y - mRect.rt.y) * (point.y - mRect.rt.y);
  double distrb = (point.x - mRect.rt.x) * (point.x - mRect.rt.x) + (point.y - 0) * (point.y - 0);

  if(distlb < mDistance.lb) 
  {
    mDistance.lb = distlb;
    mSitelb = point;
  }
  if(distlt < mDistance.lt) 
  {
    mDistance.lt = distlt;
    mSitelt = point;
  }
  if(distrt < mDistance.rt) 
  {
    mDistance.rt = distrt;
    mSitert = point;
  }
  if(distrb < mDistance.rb) 
  {
    mDistance.rb = distrb;
    mSiterb = point;
  }
}

void Voronoi::FinderAnglePoints::Clear()
{
  mDistance.rb = mRect.rt.x * mRect.rt.x + mRect.rt.y * mRect.rt.y;
  mDistance.rt = mDistance.rb;
  mDistance.lt = mDistance.rb;
  mDistance.lb = mDistance.rb;
}

*/


