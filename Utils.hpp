#pragma once

#include "Math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <type_traits>
#include <vector>

namespace odr
{
struct Mesh3D
{
    std::vector<Vec3D>  vertices;
    std::vector<size_t> indices;
};

struct Box2D
{
    Box2D();
    Box2D(Vec2D min, Vec2D max);
    double get_distance(const Vec2D& pt);

    Vec2D  min, max;
    Vec2D  center;
    double width, height;
};

template<class C, class T, T C::*member>
struct SharedPtrCmp
{
    bool operator()(const std::shared_ptr<C>& lhs, const std::shared_ptr<C>& rhs) const { return (*lhs).*member < (*rhs).*member; }
};

template<class K, class V>
std::set<K> extract_keys(std::map<K, V> const& input_map)
{
    std::set<K> retval;
    for (auto const& element : input_map)
        retval.insert(element.first);

    return retval;
}

inline Mesh3D generate_mesh_from_borders(const Line3D& inner_border, const Line3D& outer_border)
{
    Mesh3D out_mesh;

    if (inner_border.size() != outer_border.size())
        throw std::runtime_error("outer and inner border line should have equal number of points");

    out_mesh.vertices = outer_border;
    out_mesh.vertices.insert(out_mesh.vertices.end(), inner_border.rbegin(), inner_border.rend());

    const size_t num_pts = out_mesh.vertices.size();
    for (size_t l_idx = 1, r_idx = num_pts - 2; l_idx < (num_pts >> 1); l_idx++, r_idx--)
    {
        std::vector<size_t> indicies_patch = {l_idx, l_idx - 1, r_idx + 1, r_idx, l_idx, r_idx + 1};
        out_mesh.indices.insert(out_mesh.indices.end(), indicies_patch.begin(), indicies_patch.end());
    }

    return out_mesh;
}

template<typename T, typename std::enable_if_t<std::is_arithmetic<T>::value>* = nullptr>
Box2D get_bbox_for_s_values(const std::vector<T>& s_values, const std::function<Vec2D(T)>& get_xyz)
{
    std::vector<Vec2D> points;
    points.reserve(s_values.size());
    for (const T& s_val : s_values)
        points.push_back(get_xyz(s_val));

    auto iter_min_max_x = std::minmax_element(points.begin(), points.end(), [](const Vec2D& lhs, const Vec2D& rhs) { return lhs[0] < rhs[0]; });
    auto iter_min_max_y = std::minmax_element(points.begin(), points.end(), [](const Vec2D& lhs, const Vec2D& rhs) { return lhs[1] < rhs[1]; });

    Vec2D min = {iter_min_max_x.first->at(0), iter_min_max_y.first->at(1)};
    Vec2D max = {iter_min_max_x.second->at(0), iter_min_max_y.second->at(1)};

    return Box2D(min, max);
};

template<typename T, typename std::enable_if_t<std::is_arithmetic<T>::value>* = nullptr>
T golden_section_search(const std::function<T(T)>& f, T a, T b, const T tol)
{
    const T invphi = (std::sqrt(5) - 1) / 2;
    const T invphi2 = (3 - std::sqrt(5)) / 2;

    T h = b - a;
    if (h <= tol)
        return 0.5 * (a + b);

    // Required steps to achieve tolerance
    int n = static_cast<int>(std::ceil(std::log(tol / h) / std::log(invphi)));

    T c = a + invphi2 * h;
    T d = a + invphi * h;
    T yc = f(c);
    T yd = f(d);

    for (int k = 0; k < (n - 1); k++)
    {
        if (yc < yd)
        {
            b = d;
            d = c;
            yd = yc;
            h = invphi * h;
            c = a + invphi2 * h;
            yc = f(c);
        }
        else
        {
            a = c;
            c = d;
            yc = yd;
            h = invphi * h;
            d = a + invphi * h;
            yd = f(d);
        }
    }

    if (yc < yd)
        return 0.5 * (a + d);

    return 0.5 * (c + b);
}

template<typename T, size_t Dim, typename std::enable_if_t<std::is_arithmetic<T>::value>* = nullptr>
void rdp(
    const std::vector<Vec<T, Dim>>& points, const T epsilon, std::vector<Vec<T, Dim>>& out, size_t start_idx = 0, size_t step = 1, int _end_idx = -1)
{
    size_t end_idx = (_end_idx > 0) ? static_cast<size_t>(_end_idx) : points.size();
    size_t last_idx = static_cast<size_t>((end_idx - start_idx - 1) / step) * step + start_idx;

    if ((last_idx + 1 - start_idx) < 2)
        return;

    /* find the point with the maximum distance from line BETWEEN start and end */
    T      d_max(0);
    size_t d_max_idx = 0;
    for (size_t idx = start_idx + step; idx < last_idx; idx += step)
    {
        std::array<T, Dim> delta;
        for (size_t dim = 0; dim < Dim; dim++)
            delta[dim] = points.at(last_idx)[dim] - points.at(start_idx)[dim];

        // Normalise
        T mag(0);
        for (size_t dim = 0; dim < Dim; dim++)
            mag += std::pow(delta.at(dim), 2.0);
        mag = std::sqrt(mag);
        if (mag > 0.0)
        {
            for (size_t dim = 0; dim < Dim; dim++)
                delta.at(dim) = delta.at(dim) / mag;
        }

        std::array<T, Dim> pv;
        for (size_t dim = 0; dim < Dim; dim++)
            pv[dim] = points.at(idx)[dim] - points.at(start_idx)[dim];

        // Get dot product (project pv onto normalized direction)
        T pvdot(0);
        for (size_t dim = 0; dim < Dim; dim++)
            pvdot += delta.at(dim) * pv.at(dim);

        // Scale line direction vector
        std::array<T, Dim> ds;
        for (size_t dim = 0; dim < Dim; dim++)
            ds[dim] = pvdot * delta.at(dim);

        // Subtract this from pv
        std::array<T, Dim> a;
        for (size_t dim = 0; dim < Dim; dim++)
            a[dim] = pv.at(dim) - ds.at(dim);

        T d(0);
        for (size_t dim = 0; dim < Dim; dim++)
            d += std::pow(a.at(dim), 2.0);
        d = std::sqrt(d);

        if (d > d_max)
        {
            d_max = d;
            d_max_idx = idx;
        }
    }

    if (d_max > epsilon)
    {
        std::vector<Vec<T, Dim>> rec_results_1;
        rdp<T, Dim>(points, epsilon, rec_results_1, start_idx, step, d_max_idx + 1);
        std::vector<Vec<T, Dim>> rec_results_2;
        rdp<T, Dim>(points, epsilon, rec_results_2, d_max_idx, step, end_idx);

        out.assign(rec_results_1.begin(), rec_results_1.end() - 1);
        out.insert(out.end(), rec_results_2.begin(), rec_results_2.end());
    }
    else
    {
        out.clear();
        out.push_back(points.at(start_idx));
        out.push_back(points.at(last_idx));
    }
}

} // namespace odr