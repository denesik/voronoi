#include "image.h"

#include <iostream>
#include "lodepng/lodepng.h"
#include <assert.h>
#include <math.h>

Image::Image()
{
  mWidth = 0;
  mHeight = 0;
}

Image::~Image()
{

}

void Image::Resize(unsigned int width, unsigned int height)
{
  mWidth = width;
  mHeight = height;
  mData.resize(mWidth * mHeight * 4);
}

void Image::Set(const glm::uvec2 &p, unsigned int color)
{
  assert(p.x < mWidth);
  assert(p.y < mHeight);
  unsigned int ty = mHeight - p.y - 1;

  mData[4 * mWidth * ty + 4 * p.x + 0] = (color >> 24) & 0x000000FF;
  mData[4 * mWidth * ty + 4 * p.x + 1] = (color >> 16) & 0x000000FF;
  mData[4 * mWidth * ty + 4 * p.x + 2] = (color >> 8) & 0x000000FF;
  mData[4 * mWidth * ty + 4 * p.x + 3] = (color >> 0) & 0x000000FF;
}

unsigned int Image::Get(const glm::uvec2 &p)
{
  assert(p.x < mWidth);
  assert(p.y < mHeight);
  unsigned int ty = mHeight - p.y - 1;

  unsigned int color = 0;
  color |= (0x000000FF & mData[4 * mWidth * ty + 4 * p.x + 0]) >> 0;
  color |= (0x000000FF & mData[4 * mWidth * ty + 4 * p.x + 1]) >> 8;
  color |= (0x000000FF & mData[4 * mWidth * ty + 4 * p.x + 2]) >> 16;
  color |= (0x000000FF & mData[4 * mWidth * ty + 4 * p.x + 3]) >> 24;
  return color;
}

void Image::Save(const std::string &fileName)
{
  //Encode the image
  unsigned error = lodepng::encode(fileName, mData, mWidth, mHeight);

  //if there's an error, display it
  if(error) std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
}

void Image::DrawPoint(const glm::uvec2 &p, unsigned int color)
{
  Set(p, color);
}

void Image::DrawLine(const glm::vec2 &p1, const glm::vec2 &p2, unsigned int color)
{
//   float a = p1.x - p2.x;
//   float b = p1.y - p2.y;
//   float l = sqrt(a * a + b * b);
// 
//   float dx = p1.x - p2.x;
//   if(dx < 0) dx = -dx;
//   if(dx == 0)
//   {
//     return;
//   }
//   assert(dx);
//   assert(l);
// 
//   float f = dx / l;
// 
//   float k = (p2.y - p1.y) / (p2.x - p1.x);
//   float c = p1.y - k * p1.x;
// 
//   float xMin = p1.x;
//   float xMax = p2.x;
//   if(xMin > xMax)
//   {
//     xMin = p2.x;
//     xMax = p1.x;
//   }
//   for(float i = xMin; i < xMax; i += f)
//   {
//     Set(glm::round(glm::vec2(i, k * i + c)), color);
//   }

  float a = p1.x - p2.x;
  float b = p1.y - p2.y;

  if(a < 0) a = -a;
  if(b < 0) b = -b;

  float A = p1.y - p2.y;
  float B = p2.x - p1.x;
  float C = p1.x * p2.y - p2.x * p1.y;

  if(a > b)
  {
    int xMin = p1.x < p2.x ? int(p1.x) : int(p2.x);
    int xMax = p1.x > p2.x ? int(p1.x) : int(p2.x);

    for(int i = xMin; i <= xMax; ++i)
    {
      int y = int((- C - A * float(i)) / B);
      Set(glm::uvec2(i, y), color);
    }
  }
  else
  {
    int yMin = p1.y < p2.y ? int(p1.y) : int(p2.y);
    int yMax = p1.y > p2.y ? int(p1.y) : int(p2.y);

    for(int i = yMin; i <= yMax; ++i)
    {
      int x = int((- C - B * float(i)) / A);
      Set(glm::uvec2(x, i), color);
    }
  }

}

void Image::Fill(unsigned int color)
{
  for(unsigned int y = 0; y < mHeight; ++y)
    for(unsigned int x = 0; x < mWidth; ++x)
    {
      Set(glm::vec2(x, y), color);
    }
}











