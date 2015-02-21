#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <string>
#include <glm/glm.hpp>

class Image
{
public:
  Image();
  ~Image();

  void Resize(unsigned int width, unsigned int height);

  void Set(const glm::uvec2 &point, unsigned int color);

  unsigned int Get(const glm::uvec2 &point);

  void Save(const std::string &fileName);

  void DrawPoint(const glm::uvec2 &point, unsigned int color);

  void DrawLine(const glm::vec2 &point1, const glm::vec2 &point2, unsigned int color);

  void Fill(unsigned int color);

private:

  unsigned int mWidth;
  unsigned int mHeight;
  std::vector<unsigned char> mData;

};

#endif // IMAGE_H
