#pragma once

#include "Geometries/CubicSpline.h"
#include "Lanes.h"
#include "Math.hpp"
#include "Utils.hpp"

#include <map>
#include <memory>
#include <set>

namespace odr
{
struct RefLine;

class Road : public std::enable_shared_from_this<Road>
{
public:
    Road(double length, int id, int junction);
    std::shared_ptr<Lane>        get_lane(double s, double t);
    std::shared_ptr<LaneSection> get_lanesection(double s);
    LaneSectionSet               get_lanesections();

    Vec3D get_xyz(double s, double t, double z) const;
    Mat3D get_transformation_matrix(double s) const;

    int    id;
    int    junction;
    double length;

    CubicSpline              lane_offset;
    CubicSpline              superelevation;
    std::shared_ptr<RefLine> ref_line;

    std::map<double, std::shared_ptr<LaneSection>> s0_to_lanesection;
};

using RoadSet = std::set<std::shared_ptr<Road>, SharedPtrCmp<Road, int, &Road::id>>;

} // namespace odr