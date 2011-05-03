#include <vector>

class Point {
 public:
  int x;
  int y;
  Point(int x, int y) {
    this->x = x;
    this->y = y;
  }
};

class Segment {
 public:
  std::vector<Point> points;
};

class Page {
 public:
  std::vector<Segment>  segments;
};

class Notebook {
 public:
  std::vector<Page> pages;
};
