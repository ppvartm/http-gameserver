#pragma once

#include <algorithm>
#include <vector>

namespace collision_detector {

    struct Point {
         double x;
         double y;
    };

struct CollectionResult {
    bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }
    double sq_distance;
    double proj_ratio;
};

CollectionResult TryCollectPoint(Point a, Point b, Point c);

}  // namespace collision_detector