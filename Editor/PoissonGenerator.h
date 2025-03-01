/**
 * \file PoissonGenerator.h
 * \brief
 *
 * Poisson Disk Points Generator
 *
 * \version 1.6.1
 * \date 16/02/2024
 * \author Sergey Kosarevsky, 2014-2024
 * \author support@linderdaum.com   http://www.linderdaum.com   http://blog.linderdaum.com
 */

/*
   Usage example:

      #define POISSON_PROGRESS_INDICATOR 1
      #include "PoissonGenerator.h"
      ...
      PoissonGenerator::DefaultPRNG PRNG;
      const auto Points = PoissonGenerator::generatePoissonPoints( NumPoints, PRNG );
      ...
      const auto Points = PoissonGenerator::generateVogelPoints( NumPoints );
*/

// Fast Poisson Disk Sampling in Arbitrary Dimensions
// http://people.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf

// Implementation based on http://devmag.org.za/2009/05/03/poisson-disk-sampling/

/* Versions history:
 *		1.6.1   Feb 16, 2024    Reformatted using .clang-format
 *		1.6     May 29, 2023    Added generateHammersleyPoints() to generate Hammersley points
 *		1.5     Mar 26, 2022    Added generateJitteredGridPoints() to generate jittered grid points
 *		1.4.1   Dec 12, 2021		Replaced default Mersenne Twister and <random> with fast and lightweight LCG
 *		1.4     Dec  5, 2021		Added generateVogelPoints() to generate Vogel disk points
 *		1.3     Mar 14, 2021		Bugfixes: number of points in the !isCircle mode, incorrect loop boundaries
 *		1.2     Dec 28, 2019		Bugfixes; more consistent progress indicator; new command line options in demo app
 *		1.1.6   Dec  7, 2019		Removed duplicate seed initialization; fixed warnings
 *		1.1.5   Jun 16, 2019		In-class initializers; default ctors; naming, shorter code
 *		1.1.4   Oct 19, 2016		POISSON_PROGRESS_INDICATOR can be defined outside of the header file, disabled by default
 *		1.1.3a  Jun  9, 2016		Update constructor for DefaultPRNG
 *		1.1.3   Mar 10, 2016		Header-only library, no global mutable state
 *		1.1.2   Apr  9, 2015		Output a text file with XY coordinates
 *		1.1.1   May 23, 2014		Initialize PRNG seed, fixed uninitialized fields
 *		1.1     May  7, 2014		Support of density maps
 *		1.0     May  6, 2014
 */

#include <stdint.h>
#include <vector>

namespace PoissonGenerator {

const char* Version = "1.6.1 (16/02/2024)";

class DefaultPRNG {
 public:
  DefaultPRNG() = default;
  explicit DefaultPRNG(unsigned int seed) : seed_(seed) {}
  inline float randomFloat() {
    seed_ *= 521167;
    uint32_t a = (seed_ & 0x007fffff) | 0x40000000;
    // remap to 0..1
    return 0.5f * (*((float*)&a) - 2.0f);
  }
  inline uint32_t randomInt(uint32_t maxInt) {
    return uint32_t(randomFloat() * maxInt);
  }
  inline uint32_t getSeed() const {
    return seed_;
  }

 private:
  uint32_t seed_ = 7133167;
};

struct Point {
  Point() = default;
  Point(float X, float Y) : x(X), y(Y), valid_(true) {}
  float x = 0.0f;
  float y = 0.0f;
  bool valid_ = false;
  //
  bool isInRectangle() const {
    return x >= 0 && y >= 0 && x <= 1 && y <= 1;
  }
  //
  bool isInCircle() const {
    const float fx = x - 0.5f;
    const float fy = y - 0.5f;
    return (fx * fx + fy * fy) <= 0.25f;
  }
  Point& operator+(const Point& p) {
    x += p.x;
    y += p.y;
    return *this;
  }
  Point& operator-(const Point& p) {
    x -= p.x;
    y -= p.y;
    return *this;
  }
};

struct GridPoint {
  GridPoint() = delete;
  GridPoint(int X, int Y) : x(X), y(Y) {}
  int x;
  int y;
};

float getDistance(const Point& P1, const Point& P2) {
  return sqrt((P1.x - P2.x) * (P1.x - P2.x) + (P1.y - P2.y) * (P1.y - P2.y));
}

GridPoint imageToGrid(const Point& P, float cellSize) {
  return GridPoint((int)(P.x / cellSize), (int)(P.y / cellSize));
}

struct Grid {
  Grid(int w, int h, float cellSize) : w_(w), h_(h), cellSize_(cellSize) {
    grid_.resize(h_);
    for (auto i = grid_.begin(); i != grid_.end(); i++) {
      i->resize(w);
    }
  }
  void insert(const Point& p) {
    const GridPoint g = imageToGrid(p, cellSize_);
    grid_[g.x][g.y] = p;
  }
  bool isInNeighbourhood(const Point& point, float minDist, float cellSize) {
    const GridPoint g = imageToGrid(point, cellSize);

    // number of adjucent cells to look for neighbour points
    const int D = 5;

    // scan the neighbourhood of the point in the grid
    for (int i = g.x - D; i <= g.x + D; i++) {
      for (int j = g.y - D; j <= g.y + D; j++) {
        if (i >= 0 && i < w_ && j >= 0 && j < h_) {
          const Point P = grid_[i][j];

          if (P.valid_ && getDistance(P, point) < minDist)
            return true;
        }
      }
    }

    return false;
  }

 private:
  int w_;
  int h_;
  float cellSize_;
  std::vector<std::vector<Point>> grid_;
};

template<typename PRNG>
Point popRandom(std::vector<Point>& points, PRNG& generator) {
  const int idx = generator.randomInt(static_cast<int>(points.size()) - 1);
  const Point p = points[idx];
  points.erase(points.begin() + idx);
  return p;
}

template<typename PRNG>
Point generateRandomPointAround(const Point& p, float minDist, PRNG& generator) {
  // start with non-uniform distribution
  const float R1 = generator.randomFloat();
  const float R2 = generator.randomFloat();

  // radius should be between MinDist and 2 * MinDist
  const float radius = minDist * (R1 + 1.0f);

  // random angle
  const float angle = 2 * 3.141592653589f * R2;

  // the new point is generated around the point (x, y)
  const float x = p.x + radius * cos(angle);
  const float y = p.y + radius * sin(angle);

  return Point(x, y);
}

/**
   Return a vector of generated points

   NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
   Circle  - 'true' to fill a circle, 'false' to fill a rectangle
   MinDist - minimal distance estimator, use negative value for default
**/
template<typename PRNG = DefaultPRNG>
std::vector<Point> generatePoissonPoints(uint32_t numPoints,
                                         PRNG& generator,
                                         bool isCircle = true,
                                         uint32_t newPointsCount = 30,
                                         float minDist = -1.0f) {
  numPoints *= 2;

  // if we want to generate a Poisson square shape, multiply the estimate number of points by PI/4 due to reduced shape area
  if (!isCircle) {
    const double Pi_4 = 0.785398163397448309616; // PI/4
    numPoints = static_cast<int>(Pi_4 * numPoints);
  }

  if (minDist < 0.0f) {
    minDist = sqrt(float(numPoints)) / float(numPoints);
  }

  std::vector<Point> samplePoints;
  std::vector<Point> processList;

  if (!numPoints)
    return samplePoints;

  // create the grid
  const float cellSize = minDist / sqrt(2.0f);

  const int gridW = (int)ceil(1.0f / cellSize);
  const int gridH = (int)ceil(1.0f / cellSize);

  Grid grid(gridW, gridH, cellSize);

  Point firstPoint;
  do {
    firstPoint = Point(generator.randomFloat(), generator.randomFloat());
  } while (!(isCircle ? firstPoint.isInCircle() : firstPoint.isInRectangle()));

  // update containers
  processList.push_back(firstPoint);
  samplePoints.push_back(firstPoint);
  grid.insert(firstPoint);

#if POISSON_PROGRESS_INDICATOR
  size_t progress = 0;
#endif

  // generate new points for each point in the queue
  while (!processList.empty() && samplePoints.size() <= numPoints) {
#if POISSON_PROGRESS_INDICATOR
    // a progress indicator, kind of
    if ((samplePoints.size()) % 1000 == 0) {
      const size_t newProgress = 200 * (samplePoints.size() + processList.size()) / numPoints;
      if (newProgress != progress) {
        progress = newProgress;
        std::cout << ".";
      }
    }
#endif // POISSON_PROGRESS_INDICATOR

    const Point point = popRandom<PRNG>(processList, generator);

    for (uint32_t i = 0; i < newPointsCount; i++) {
      const Point newPoint = generateRandomPointAround(point, minDist, generator);
      const bool canFitPoint = isCircle ? newPoint.isInCircle() : newPoint.isInRectangle();

      if (canFitPoint && !grid.isInNeighbourhood(newPoint, minDist, cellSize)) {
        processList.push_back(newPoint);
        samplePoints.push_back(newPoint);
        grid.insert(newPoint);
        continue;
      }
    }
  }

#if POISSON_PROGRESS_INDICATOR
  std::cout << std::endl << std::endl;
#endif // POISSON_PROGRESS_INDICATOR

  return samplePoints;
}

Point sampleVogelDisk(uint32_t idx, uint32_t numPoints, float phi) {
  const float kGoldenAngle = 2.4f;

  const float r = sqrtf(float(idx) + 0.5f) / sqrtf(float(numPoints));
  const float theta = idx * kGoldenAngle + phi;

  return Point(r * cosf(theta), r * sinf(theta));
}

/**
   Return a vector of generated points
**/
std::vector<Point> generateVogelPoints(uint32_t numPoints, bool isCircle = true, float phi = 0.0f, Point center = Point(0.5f, 0.5f)) {
  std::vector<Point> samplePoints;

  samplePoints.reserve(numPoints);

  const uint32_t numSamples = isCircle ? 4 * numPoints : numPoints;

  for (uint32_t i = 0; i != numPoints; i++) {
    const Point p = sampleVogelDisk(i, numSamples, phi) + center;
    samplePoints.push_back(p);
  }

  return samplePoints;
}

/**
   Return a vector of generated points
**/
template<typename PRNG = DefaultPRNG>
std::vector<Point> generateJitteredGridPoints(uint32_t numPoints,
                                              PRNG& generator,
                                              bool isCircle = false,
                                              float jitterRadius = 0.004f,
                                              Point center = Point(0.5f, 0.5f)) {
  std::vector<Point> samplePoints;

  samplePoints.reserve(numPoints);

  const uint32_t gridSize = uint32_t(sqrt(numPoints));

  for (uint32_t x = 0; x != gridSize; x++) {
    for (uint32_t y = 0; y != gridSize; y++) {
      Point p;
      do {
        const Point offs = generateRandomPointAround(Point(0, 0), jitterRadius, generator) - center + Point(0.5f, 0.5f);
        p = Point(float(x) / gridSize, float(y) / gridSize) + offs;
        // generate a new point until it is within the boundaries
      } while (!p.isInRectangle());

      if (isCircle)
        if (!p.isInCircle())
          continue;

      samplePoints.push_back(p);
    }
  }

  return samplePoints;
}

namespace {

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverse_VdC(uint32_t bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(float(bits) * 2.3283064365386963e-10); // / 0x100000000
}

Point hammersley2d(uint32_t i, uint32_t N) {
  return Point(float(i) / float(N), radicalInverse_VdC(i));
}

} // namespace

/**
   Return a vector of generated points
**/
std::vector<Point> generateHammersleyPoints(uint32_t numPoints) {
  std::vector<Point> samplePoints;

  samplePoints.reserve(numPoints);

  const uint32_t gridSize = uint32_t(sqrt(numPoints));

  for (uint32_t i = 0; i != numPoints; i++) {
    Point p = hammersley2d(i, numPoints);

    samplePoints.push_back(p);
  }
  return samplePoints;
}

} // namespace PoissonGenerator
