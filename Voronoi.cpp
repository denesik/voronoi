#include "Voronoi.h"

#include <assert.h>
#include <algorithm>
#include "geometry.h"

//#define EPS 0.0001

int BreakPoint::BPIdGeneration = 1;
int Parabola::PIdGeneration = 1;


Voronoi::Voronoi(const std::list<glm::vec2> &points, glm::vec2 size)
  : mRect(Point(0.0, 0.0), size), mEvents([](Event *e1, Event *e2)
                         {
                           return e1->height > e2->height;
                         })

{
  mHead = nullptr;

  for(auto it = points.begin(); it != points.end(); ++it)
  {
    mParabols.push_back(new Parabola(*it));
    mEvents.insert(new Event(SiteEvent(mParabols.back()))); 
  }
  /*
  for(auto it = mEvents.begin(); it != mEvents.end(); ++it)
  {
    printf("y:%f, ", (*it)->height);
  }

  InsertArcHead(mParabols[0]);
  GenerateListsBPA();
  InsertArc(mListArc[0], mParabols[1]);
  GenerateListsBPA();
  InsertArc(mListArc[1], mParabols[2]);
  GenerateListsBPA();
  InsertArc(mListArc[2], mParabols[3]);
  GenerateListsBPA();
  RemoveArc(4, Point());
  GenerateListsBPA();
  InsertArc(mListArc[0], mParabols[4]);
  GenerateListsBPA();
  RemoveArc(2, Point());
  GenerateListsBPA();
  RemoveArc(2, Point());
  GenerateListsBPA();
  InsertArc(mListArc[2], mParabols[5]);
  GenerateListsBPA();
  RemoveArc(2, Point());
  GenerateListsBPA();
  RemoveArc(3, Point());
  GenerateListsBPA();
  PrintListsBPA();
  */
  Process();
  //PrintListsBPA();
}

Voronoi::~Voronoi()
{

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


void Voronoi::InsertArcHead(Parabola *parabola)
{
  assert(parabola);
  assert(mHead == nullptr);
  mHead = new BtreeElement(Element(parabola));
}

void Voronoi::InsertArc(BtreeElement *btreeElement, Parabola *parabola)
{
  // параметры должны существовать, элемент дерева должен быть листом,
  // листом дерева должна быть арка.
  assert(parabola);
  assert(btreeElement);
  assert(IsList(btreeElement));
  assert(btreeElement->element.type == Element::ARC);

  // Если существует событие круга для арки, его нужно удалить.
  if(btreeElement->element.arc.event)
  {
    RemoveEvent(btreeElement->element.arc.event);
    btreeElement->element.arc.event = nullptr;
  }

  Element breakArc = btreeElement->element;

  // Создаем два новых брекпоинта
  // лежащих слева и справа от вставляемой параболы.
  Element bpLeft(new BreakPoint(BreakPoint::POINT));
  Element bpRight(new BreakPoint(BreakPoint::POINT));

  // Добавим новую грань
  Edge newEdge;
  newEdge.bp1 = bpLeft.breakPoint;
  newEdge.bp2 = bpRight.breakPoint;
  newEdge.p1 = breakArc.arc.parabola;
  newEdge.p2 = parabola;
  mEdgeList.push_back(newEdge);

  // Создаем две новых арки, лежищих слева и справа
  // от вставляемой параболы
  Element arcLeft(breakArc);
  Element arcRight(breakArc);

  // Создаем поддерево
  // Элементом правого листа будет правый брекпоинт.
  btreeElement->right = new BtreeElement(bpRight);
  btreeElement->right->parent = btreeElement;
  // Элементом  левого листа будет левая арка.
  btreeElement->left = new BtreeElement(arcLeft);
  btreeElement->left->parent = btreeElement;
  // Элементом текущего листа будет левый брекпоинт
  btreeElement->element = bpLeft;

  // левый лист - вставляемая парабола.
  btreeElement->right->left = new BtreeElement(parabola);
  btreeElement->right->left->parent = btreeElement->right;
  // правый лист - правая арка.
  btreeElement->right->right = new BtreeElement(arcRight);
  btreeElement->right->right->parent = btreeElement->right;
}


void Voronoi::GenerateListsBPA()
{
  mListArc.clear();
  mListBreakPoints.clear();
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
    mListBreakPoints.push_back(node);
    GenerateListsBPA(node->right);
  }
}

void Voronoi::RemoveArc(unsigned int arcPos, Point intersection)
{
  assert(arcPos > 0 && arcPos + 1 < mListArc.size());
  assert(mListBreakPoints.size() + 1 == mListArc.size());

  // Ищем левый и правый брекпоинты от арки.
  BtreeElement *bpLeft = mListBreakPoints[arcPos - 1];
  BtreeElement *bpRight = mListBreakPoints[arcPos];
  BtreeElement *arc = mListArc[arcPos];

  // Удалим события круга для левой и правой арки.
  if(mListArc[arcPos - 1]->element.arc.event)
  {
    RemoveEvent(mListArc[arcPos - 1]->element.arc.event);
    mListArc[arcPos - 1]->element.arc.event = nullptr;
  }
  if(mListArc[arcPos + 1]->element.arc.event)
  {
    RemoveEvent(mListArc[arcPos + 1]->element.arc.event);
    mListArc[arcPos + 1]->element.arc.event = nullptr;
  }


  // Смотрим какой из брекпоинтов является родителем арки, а какой нет.
  BtreeElement *bpArcFirst = arc->parent; // Родитель арки, нужно удалить
  BtreeElement *bpArcSecond = nullptr;    // Не родитель, нужно модифицировать.
  if(bpArcFirst == bpLeft)
  {
    bpArcSecond = bpRight;
  }
  else if (bpArcFirst == bpRight)
  {
    bpArcSecond = bpLeft;
  }
  assert(bpArcSecond);

  // Точка соединения трех граней
  BreakPoint *endPoint = new BreakPoint(BreakPoint::END_POINT);
  endPoint->point = intersection;

  BreakPoint *newBreakPoint = new BreakPoint(BreakPoint::POINT);

  // Обновим список граней. Лучи превращаются в отрезки, прямые в лучи.
  UpdateEdge(bpLeft->element.breakPoint, endPoint);
  UpdateEdge(bpRight->element.breakPoint, endPoint);

  // Добавим точку пересечения граней в список.
  //FindedBP(intersection, arc->element.arc.parabola->focus);
  //FindedBP(intersection, mListArc[arcPos - 1]->element.arc.parabola->focus);  // Левая арка
  //FindedBP(intersection, mListArc[arcPos + 1]->element.arc.parabola->focus);  // Правая арка

  // Добавим новую грань
  Edge newEdge;
  newEdge.bp1 = newBreakPoint;
  newEdge.bp2 = endPoint;
  newEdge.p1 = mListArc[arcPos - 1]->element.arc.parabola;
  newEdge.p2 = mListArc[arcPos + 1]->element.arc.parabola;
  mEdgeList.push_back(newEdge);

  // Удаляем брекпоинты
  delete bpArcFirst->element.breakPoint;
  delete bpArcSecond->element.breakPoint;

  // Изменяем брекпоинты
  // Вместо двух брекпоинтов будет один.
  bpArcFirst->element.breakPoint = nullptr;
  bpArcSecond->element = Element(newBreakPoint);

  // Ищем второго ребенка для первого брекпоинта(который нужно удалить).
  // Первый ребенок - наша арка.
  BtreeElement *bpArcChildSecond = nullptr;
  if(bpArcFirst->left == arc)
  {
    bpArcChildSecond = bpArcFirst->right;
  }
  else if(bpArcFirst->right == arc)
  {
    bpArcChildSecond = bpArcFirst->left;
  }
  assert(bpArcChildSecond);

  // Ищем родителя для первого брекпоинта.
  BtreeElement *bpArcFirstParent = bpArcFirst->parent;
  // Соединяем второго ребенка для первого брекпоинта и отца первого брекпоинта.
  if(bpArcFirstParent->left == bpArcFirst)
  {
    bpArcFirstParent->left = bpArcChildSecond;
    bpArcChildSecond->parent = bpArcFirstParent;
  }
  else if(bpArcFirstParent->right == bpArcFirst)
  {
    bpArcFirstParent->right = bpArcChildSecond;
    bpArcChildSecond->parent = bpArcFirstParent;
  }
  else
  {
    assert(true);
  }

  // Удаляем арку и первый брекпоинт.
  delete arc;
  delete bpArcFirst;
}

void Voronoi::PrintListsBPA()
{
  printf("arcs: ");
  for(auto it = mListArc.begin(); it != mListArc.end(); ++it)
  {
    printf("%i, ", (*it)->element.arc.parabola->id);
  }
  printf("\nbp: ");
  for(auto it = mListBreakPoints.begin(); it != mListBreakPoints.end(); ++it)
  {
    printf("%i, ", (*it)->element.breakPoint->id);
  }
  printf("\n");
  for(auto it = mEdgeList.begin(); it != mEdgeList.end(); ++it)
  {
    printf("[bp:%s%i:%s%i; arc:%i:%i; p1:%f,%f p2:%f,%f]\n",
           (*it).bp1->type == BreakPoint::POINT ? "b" : "p", (*it).bp1->id,
           (*it).bp2->type == BreakPoint::POINT ? "b" : "p", (*it).bp2->id,
           (*it).p1->id, (*it).p2->id,
           //(*it).p1->focus.x, (*it).p1->focus.y,
           //(*it).p2->focus.x, (*it).p2->focus.y,
           (*it).bp1->point.x, (*it).bp1->point.y,
           (*it).bp2->point.x, (*it).bp2->point.y);
  }
}

/// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Короче тут есть утечка, хз когда удалять END_POINT
void Voronoi::UpdateEdge(BreakPoint *removedPoint, BreakPoint *endPoint)
{
  assert(removedPoint && removedPoint->type == BreakPoint::POINT);
  assert(endPoint && endPoint->type == BreakPoint::END_POINT);

  // Если первая точка та, которую нужно удалить, то если вторая точка - конечная,
  // получился отрезок - удалим. в противном случае первая точка заменяется конечной.
  for(auto it = mEdgeList.begin(); it != mEdgeList.end();)
  {
    assert((*it).bp1 != (*it).bp2);
    if((*it).bp1 == removedPoint)
    {
      // Удалим точку пересечения и грань, если грань ограничена двумя EndPoint
      if((*it).bp2->type == BreakPoint::END_POINT)
      {
        Edge &edge = (*it);
        // Отрезок между добавляемым ендпоинтом и найденным ендпоинтом.
        // (*it).bp1 здесь брекпоинт, который нужно заменить на endPoint,
        // но т.к. у нас получится грань с двумя ендпоинтами, удаляем ее.
        auto points = IntersectRectSegment(mRect, Segment(endPoint->point, edge.bp2->point));
        InsertBP(points, edge.p1->focus, edge.p2->focus);
        it = mEdgeList.erase(it);
        continue;
      }
      else
      {
        (*it).bp1 = endPoint;
      }
    }
    else if((*it).bp2 == removedPoint)
    {
      if((*it).bp1->type == BreakPoint::END_POINT)
      {
        Edge &edge = (*it);
        auto points = IntersectRectSegment(mRect, Segment(edge.bp1->point, endPoint->point));
        InsertBP(points, edge.p1->focus, edge.p2->focus);
        it = mEdgeList.erase(it);
        continue;
      }
      else
      {
        (*it).bp2 = endPoint;
      }
    }
    ++it;
  }
}

void Voronoi::CalcBPPos(unsigned int bPPos)
{
  assert(bPPos < mListBreakPoints.size());
  assert(mListBreakPoints.size() + 1 == mListArc.size());

  // Ищем брекпоинт и арку слева и справа.
  BreakPoint *bp = mListBreakPoints[bPPos]->element.breakPoint;
  Parabola *arcLeft = mListArc[bPPos]->element.arc.parabola;
  Parabola *arcRight = mListArc[bPPos + 1]->element.arc.parabola;
  assert(bp->type == BreakPoint::POINT);

  // Ищем отрезок, являющийся пересечением парабол.
  // т.к. параболы не могут находиться на одном уровне по y и они
  // одинаково направлены, у них только 2 точки пересечения.
  bp->point = IntersectParabols(mSweptLine, arcLeft->focus, arcRight->focus);
}

void Voronoi::CalcBPPos()
{
  for(unsigned int i = 0; i < mListBreakPoints.size(); ++i)
  {
    CalcBPPos(i);
  }
}

void Voronoi::GenerateCircleEvent()
{
  for(int i = 1; i < static_cast<int>(mListArc.size()) - 1; ++i)
  {
    BtreeElement *arc0 = mListArc[i - 1];
    BtreeElement *arc1 = mListArc[i];
    BtreeElement *arc2 = mListArc[i + 1];

    // Если событие существует для этой арки, пропускаем ее.
    if(arc1->element.arc.event)
    {
      continue;
    }

    // Если точки лежат на одной прямой или против часовой - выходим.
    if(RotationPoint(arc0->element.arc.parabola->focus, arc1->element.arc.parabola->focus, arc2->element.arc.parabola->focus) >= 0)
    {
      continue;
    }

    // Ищем центр окружности по трем точкам.
    Point c = CreateCircle(arc0->element.arc.parabola->focus,
                           arc1->element.arc.parabola->focus,
                           arc2->element.arc.parabola->focus);

    // Ищем радиус окружности.
    double r = sqrt(pow(arc0->element.arc.parabola->focus.x - c.x, 2) +
                    pow(arc0->element.arc.parabola->focus.y - c.y, 2));

    // Создаем событие круга.
    CircleEvent event(arc1);
    event.pos = Point(c.x, c.y - r);
    event.center = c;

    // Добавляем событие в том случае, если оно не выше заметающей прямой.
    if(event.pos.y <= mSweptLine)
    {
      Event *e = new Event(event);
      arc1->element.arc.event = e;
      mEvents.insert(e); 
    }
  }
}

void Voronoi::RemoveEvent(Event *event)
{
  for(auto it = mEvents.begin(); it != mEvents.end(); ++it)
  {
    if(event == (*it))
    {
      mEvents.erase(it);
      break;
    }
  }

  delete event;
}

void Voronoi::Process()
{
  if(mEvents.empty())
    return;

  Event *event = *(mEvents.begin());
  assert(event->type == Event::SITE_EVENT);

  mSweptLine = event->height;

  InsertArcHead(event->se.parabola);
  RemoveEvent(event);
  GenerateListsBPA();

  // Проходим по всем событиям
  while(!mEvents.empty())
  {
    event = *(mEvents.begin());
    mSweptLine = event->height;

    switch(event->type)
    {
    case Event::SITE_EVENT:
      {
        // Вычислять кооринаты брекпоинтов нужно перед вставкой новой арки, т.к.
        // Мы гарантированно спустимся вниз по y и расстояние между параболой и 
        // заметающей прямой будет гарантированно больше 0.
        CalcBPPos();
        assert(event->se.parabola);
        // Ищем параболу в которую нужно вставить.
        InsertArc(FindInsertArc(event->se.parabola), event->se.parabola);
      }

      break;
    case Event::CIRCLE_EVENT:
      {
        assert(event->ce.arcNode);
        // Ищем номер параболы в списке.
        auto it = std::find(mListArc.begin(), mListArc.end(), event->ce.arcNode);
        assert(it != mListArc.end());

        // Удалим параболу из списка.
        RemoveArc(std::distance(mListArc.begin(), it), event->ce.center);
      }
      break;
    default:
      {
        assert(true);
      }
    }
    RemoveEvent(event);
    GenerateListsBPA();
    GenerateCircleEvent();
    //PrintListsBPA();
  }
  // Смещаемся вниз и вычисляем брекпоинты
  mSweptLine -= 1;
  CalcBPPos();
  PostProcess();
}

BtreeElement *Voronoi::FindInsertArc(Parabola *parabola)
{
  assert(parabola);
  assert(!mListArc.empty());
  double pos = parabola->focus.x;

  if(mListBreakPoints.empty())
  {
    assert(mListArc.size() == 1);
    return *mListArc.begin();
  }

  for(unsigned int i = 0; i < mListBreakPoints.size(); ++i)
  {
    assert(mListBreakPoints[i]->element.type == Element::BREAK_POINT);
    if(mListBreakPoints[i]->element.breakPoint->point.x > pos)
    {
      return mListArc[i];
    }
  }

  return mListArc[mListArc.size() - 1];
}
/*
void Voronoi::FindedBP(const Point &bp, const Point &arcPos)
{
  mDiagramData.insert(std::make_pair<glm::vec2, glm::vec2>(glm::vec2(arcPos.x, arcPos.y), glm::vec2(bp.x, bp.y)));
}
*/
const std::multimap<glm::vec2, glm::vec2, Voronoi::DDComparator> &Voronoi::GetDiagram()
{
  return mDiagramData;
}

void Voronoi::PostProcess()
{

  // Пробегаемся по оставшимся граням.
  // Могут быть либо лучи, либо прямые.
  for(auto it = mEdgeList.begin(); it != mEdgeList.end(); ++it)
  {
    Edge &edge = (*it);
    BreakPoint::BPType bptype1 = edge.bp1->type;
    BreakPoint::BPType bptype2 = edge.bp2->type;
    // Грань не должна быть отрезком.
    assert(bptype1 != BreakPoint::END_POINT || bptype2 != BreakPoint::END_POINT);

    // Нашли прямую.
    if(bptype1 == BreakPoint::POINT && bptype2 == BreakPoint::POINT)
    {
      auto points = IntersectRectLine(mRect, Perpendicular(Line(edge.p1->focus, edge.p2->focus),
                                                         Center(edge.p1->focus, edge.p2->focus)));
      InsertBP(points, edge.p1->focus, edge.p2->focus);
    }

    if(bptype1 == BreakPoint::POINT && bptype2 == BreakPoint::END_POINT)
    {
      // Нашли луч из точки 2 в точку 1.
      // Делаем луч перпендикулярным линии между фокусами.
      Ray ray = PerpRayLine(Ray(edge.bp2->point, edge.bp1->point), Line(edge.p1->focus, edge.p2->focus));
      
      auto points = IntersectRectRay(mRect, ray);
      InsertBP(points, edge.p1->focus, edge.p2->focus);
    }

    if(bptype1 == BreakPoint::END_POINT && bptype2 == BreakPoint::POINT)
    {
      Ray ray = PerpRayLine(Ray(edge.bp1->point, edge.bp2->point), Line(edge.p1->focus, edge.p2->focus));

      auto points = IntersectRectRay(mRect, ray);
      InsertBP(points, edge.p1->focus, edge.p2->focus);
    }
  }

}

void Voronoi::InsertBP(const std::list<Point> &points, const Point &arc1, const Point &arc2)
{
  for(auto it = points.begin(); it != points.end(); ++it)
  {
    glm::vec2 p((*it).x, (*it).y);
    glm::vec2 a1(arc1.x, arc1.y);
    glm::vec2 a2(arc2.x, arc2.y);

    bool finded;

    finded = false;
    auto ela1 = mDiagramData.equal_range(a1);
    for(auto it = ela1.first; it != ela1.second; ++it)
    {
      if((*it).second == p)
      {
        finded = true;
        break;
      }
    }
    if(!finded)
      mDiagramData.insert(std::make_pair<glm::vec2, glm::vec2>(std::move(a1), std::move(p)));

    finded = false;
    auto ela2 = mDiagramData.equal_range(a2);
    for(auto it = ela2.first; it != ela2.second; ++it)
    {
      if((*it).second == p)
      {
        finded = true;
        break;
      }
    }
    if(!finded)
      mDiagramData.insert(std::make_pair<glm::vec2, glm::vec2>(std::move(a2), std::move(p)));
  }
}



