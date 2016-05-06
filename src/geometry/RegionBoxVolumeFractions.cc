/*
  Geometry

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Lipnikov Konstantin (lipnikov@lanl.gov)
           Rao Garimella (rao@lanl.gov)

  A box region defined by two corner points and normals to its side.
*/

#include <vector>

#include "dbc.hh"
#include "errors.hh"

#include "Point.hh"
#include "RegionBoxVolumeFractions.hh"

namespace Amanzi {
namespace AmanziGeometry {

// -------------------------------------------------------------------
// Constructor
// -------------------------------------------------------------------
RegionBoxVolumeFractions::RegionBoxVolumeFractions(
    const std::string& name, const Set_ID id,
    const Point& p0, const Point& p1,
    const std::vector<Point>& normals,
    const LifeCycleType lifecycle)
  : Region(name, id, true, BOX_VOF, p0.dim(), p0.dim(), lifecycle),
    p0_(p0),
    p1_(p1),
    normals_(normals),
    degeneracy_(-1)
{
  Errors::Message msg;
  if (p0_.dim() != p1_.dim()) {
    msg << "Mismatch in dimensions of corner points of RegionBoxVolumeFractions \""
        << Region::name() << "\"";
    Exceptions::amanzi_throw(msg);

    for (int n = 0; n < normals.size(); ++n) {
      if (p0_.dim() != normals_[n].dim()) {
        msg << "Mismatch in dimensions of points and normals of RegionBoxVolumeFractions \""
            << Region::name() << "\"";
        Exceptions::amanzi_throw(msg);
      }
    }
  }

  // Calculate the transformation tensor
  int dim = p0.dim();
  N_.set(dim);

  for (int i = 0; i < dim; ++i) {
    for (int j = 0; j < dim; ++j) {
      N_(j, i) = normals_[i][j];
    }
  }
  N_.Inverse();

  // Check if this is a reduced dimensionality box (e.g. even though
  // it is in 3D space it is a 2D box). Normalize the tensor to unit
  // reference cube.
  Point v1(dim);
  v1 = N_ * (p1_ - p0_);

  jacobian_ = 1.0;
  int mdim(dim);
  for (int i = 0; i != dim; ++i) {
    double len = v1[i];
    if (std::abs(len) < 1e-12 * norm(v1)) {
      mdim--;
      degeneracy_ = i;
    } else {
      jacobian_ *= std::abs(len);
      for (int j = 0; j < dim; ++j) N_(i, j) /= len;
    }
  } 
  
  if (mdim < dim) set_manifold_dimension(mdim);

  if (mdim < dim - 1) {
    msg << "The box degenerate in more than one direction is not supported.\n";
    Exceptions::amanzi_throw(msg);
  }
}


// -------------------------------------------------------------------
// Implementation of a virtual member function.
// -------------------------------------------------------------------
bool RegionBoxVolumeFractions::inside(const Point& p) const
{
#ifdef ENABLE_DBC
  if (p.dim() != p0_.dim()) {
    Errors::Message msg;
    msg << "Mismatch in corner dimension of RegionBox \""
        << name() << "\" and query point.";
    Exceptions::amanzi_throw(msg);
  }
#endif

  Point phat(N_ * (p - p0_));
 
  bool result(true);
  for (int i = 0; i != p.dim(); ++i) {
    result &= (phat[i] > -TOL && phat[i] < 1.0 + TOL);
  }
  return result;
}


// -------------------------------------------------------------------
// Implementation of a virtual member function.
// We have to analyze 
// -------------------------------------------------------------------
double RegionBoxVolumeFractions::intersect(
    const std::vector<Point>& polytope,
    const std::vector<std::vector<int> >& faces) const
{
  double volume(0.0);
  int mdim, sdim;

  mdim = manifold_dimension();
  sdim = polytope[0].dim();
   
  if ((sdim == 2 && degeneracy_ < 0) || (sdim == 3 && degeneracy_ >= 0)) {
    std::vector<Point> box, result_xy;

    box.push_back(Point(0.0, 0.0));
    box.push_back(Point(1.0, 0.0)); 
    box.push_back(Point(1.0, 1.0));
    box.push_back(Point(0.0, 1.0));

    // project manifold on a plane
    Point p3d(3), p2d(2);
    std::vector<Point> nodes;

    if (degeneracy_ >= 0) {
      double eps(1.0e-6);

      for (int n = 0; n < polytope.size(); ++n) {
        p3d = N_ * (polytope[n] - p0_);
        if (std::abs(p3d[degeneracy_]) > eps) return volume;

        int i(0);
        for (int k = 0; k < sdim; ++k) p2d[i++] = p3d[k];
        nodes.push_back(p2d);
      }
      IntersectConvexPolygons(nodes, box, result_xy);
    } else {
      for (int n = 0; n < polytope.size(); ++n) {
        p2d = N_ * (polytope[n] - p0_);
        nodes.push_back(p2d);
      }
    }

    IntersectConvexPolygons(nodes, box, result_xy);

    int nnodes = result_xy.size(); 
    if (nnodes > 0) {
      for (int i = 0; i < nnodes; ++i) {
        int j = (i + 1) % nnodes;

        const Point& p1 = result_xy[i];
        const Point& p2 = result_xy[j];
        volume += (p1[0] + p2[0]) * (p2[1] - p1[1]) / 2;
      }
      volume *= jacobian_;
    }
  }

  else if (sdim == 3 && degeneracy_ < 0) {
    std::vector<std::vector<int> > result_faces;
    std::vector<Point> result_xyz;
    std::vector<std::pair<Point, Point> > box;

    box.push_back(std::make_pair(Point(0.0, 0.0, 0.0), Point(-1.0, 0.0, 0.0)));
    box.push_back(std::make_pair(Point(0.0, 0.0, 0.0), Point(0.0, -1.0, 0.0)));
    box.push_back(std::make_pair(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, -1.0)));

    box.push_back(std::make_pair(Point(1.0, 1.0, 1.0), Point(1.0, 0.0, 0.0)));
    box.push_back(std::make_pair(Point(1.0, 1.0, 1.0), Point(0.0, 1.0, 0.0)));
    box.push_back(std::make_pair(Point(1.0, 1.0, 1.0), Point(0.0, 0.0, 1.0)));

    IntersectConvexPolyhedra(polytope, faces, box, result_xyz, result_faces);

    int nfaces = result_faces.size(); 
    if (nfaces > 3) {
      for (int i = 0; i < nfaces; ++i) {
        int nnodes = result_faces[i].size();
        for (int k = 0; k < nnodes - 2; ++k) {
          const Point& p0 = result_xyz[result_faces[i][k]];
          const Point& p1 = result_xyz[result_faces[i][k + 1]];
          const Point& p2 = result_xyz[result_faces[i][k + 2]];
          volume += ((p1 - p0)^(p2 - p0)) * p0;
        }
      }
      volume *= jacobian_ / 6;
    }
  } else {
    ASSERT(0);
  }

  return volume;
}


// -------------------------------------------------------------------
// Non-member function.
// Intersection of two counter clockwise oriented polygons given
// by vertices xy1 and xy2.
// -------------------------------------------------------------------
void IntersectConvexPolygons(const std::vector<Point>& xy1,
                             const std::vector<Point>& xy2,
                             std::vector<Point>& xy3)
{
  std::list<std::pair<double, Point> > result;
  std::list<std::pair<double, Point> >::iterator it, it_next, it2;

  // populate list with the second polygon
  int n2 = xy2.size();
  for (int i = 0; i < xy2.size(); ++i) {
    result.push_back(std::make_pair(0.0, xy2[i]));
  }

  // intersect each edge of the first polygon with the result
  int n1 = xy1.size();
  double eps(1e-6);
  Point v1(xy1[0]);

  for (int i = 0; i < n1; ++i) {
    if (result.size() <= 2) break;

    int j = (i + 1) % n1;
    Point edge(xy1[j] - xy1[i]);
    Point normal(edge[1], -edge[0]);
    normal /= norm(normal);

    // Calculate distance of polygon vertices to the plane defined by 
    // the point xy1[i] and exterior normal normal.
    for (it = result.begin(); it != result.end(); ++it) {
      double tmp = normal * (it->second - xy1[i]);
      if (std::fabs(tmp) < eps) tmp = 0.0;
      it->first = tmp;
    }

    double d1, d2, tmp;
    for (it = result.begin(); it != result.end(); ++it) {
      it_next = it;
      if (++it_next == result.end()) it_next = result.begin();

      d1 = it->first;
      d2 = it_next->first;
      // add vertex if intersection was found; otherwise, remove vertex.
      if (d1 * d2 < 0.0) {  
        tmp = d2 / (d2 - d1);
        v1 = tmp * it->second + (1.0 - tmp) * it_next->second; 
        result.insert(it_next, std::make_pair(0.0, v1));
      }
    } 

    // removing cut-out edges
    it = result.begin();
    while (it != result.end()) {
      if (it->first > 0.0) {
        it = result.erase(it);
        if (result.size() == 2) break;
      } else {
        it++;
      }
    }
  }

  xy3.clear();
  if (result.size() > 2) { 
    for (it = result.begin(); it != result.end(); ++it) {
      xy3.push_back(it->second);
    }
  }
}


// -------------------------------------------------------------------
// Non-member function.
// Intersection of two convex polyhedra P1 and P2. Polyhedron P1 is 
// defined by vertices xyz1 and faces ordered counter clockwise with
// respect to their exterior normals. Polyhedron P2 is defined by a
// set of half-spaces (point and exterior normal). The result is
// polyhedron P3 ordered similar to P1.
// -------------------------------------------------------------------
void IntersectConvexPolyhedra(const std::vector<Point>& xyz1,
                              const std::vector<std::vector<int> >& faces1,
                              const std::vector<std::pair<Point, Point> >& xyz2,
                              std::vector<Point>& xyz3,
                              std::vector<std::vector<int> >& faces3)
{
  // initialize result with the first polyhedron
  int nfaces1 = faces1.size();
  std::vector<std::pair<double, Point> > result_xyz;
  std::vector<std::list<int> > result_faces(nfaces1);
  std::vector<std::pair<int, int> > new_edges;

  int nxyz = xyz1.size();
  for (int i = 0; i < nxyz; ++i)
    result_xyz.push_back(std::make_pair(0.0, xyz1[i]));

  for (int i = 0; i < nfaces1; ++i) {
    for (int n = 0; n < faces1[i].size(); ++n) {
      result_faces[i].push_back(faces1[i][n]);
    }
  }

  // clip polyhedron using the second polyhedron.
  int nfaces2 = xyz2.size();
  double d1, d2, tmp, eps(1e-6);
  Point v1(xyz1[0]);
  std::list<int>::iterator it, it_prev, it_next, it2;

  for (int n = 0; n < nfaces2; ++n) {
    const Point& p = xyz2[n].first;
    const Point& normal = xyz2[n].second;
std::cout << "plane=" << p << " normal=" << normal << std::endl;

    // location of nodes relative to the n-th plane
    for (int i = 0; i < nxyz; ++i) {
      if (result_xyz[i].first <= 0.0) {
        tmp = normal * (result_xyz[i].second - p);
        if (std::fabs(tmp) < eps) tmp = 0.0;
        result_xyz[i].first = tmp / norm(normal);
      }
    }

    // clip each face of the resulting polyhedron
    int nfaces3 = result_faces.size();
    new_edges.clear();
    new_edges.resize(nfaces3, std::make_pair(-1, -1));

    for (int m = 0; m < nfaces3; ++m) {
      if (result_faces[m].size() <= 2) continue;

      for (it = result_faces[m].begin(); it != result_faces[m].end(); ++it) {
        it_next = it;
        if (++it_next == result_faces[m].end()) it_next = result_faces[m].begin();

        std::pair<double, Point>& p1 = result_xyz[*it];
        std::pair<double, Point>& p2 = result_xyz[*it_next];

        d1 = p1.first;
        d2 = p2.first;
        // add vertex if intersection was found; otherwise, remove vertex.
        if (d1 * d2 < 0.0) {  
          tmp = d2 / (d2 - d1);
          v1 = tmp * p1.second + (1.0 - tmp) * p2.second; 

          int idx(-1);
          for (int i = 0; i < nxyz; ++i) {
            if (norm(v1 - result_xyz[i].second) < 1e-8) {
              idx = i;
              break;
            }
          }

          if (idx >= 0) {
            result_faces[m].insert(it_next, idx);
          } else {
            result_xyz.push_back(std::make_pair(0.0, v1));
            result_faces[m].insert(it_next, nxyz);
std::cout << "  add " << v1 << std::endl;
            nxyz++;
          }
        }
      }

      // removing cut-out edges
      it = result_faces[m].begin();
      while (it != result_faces[m].end()) {
        if (result_xyz[*it].first > 0.0) {
          it = result_faces[m].erase(it);
if (result_faces[m].size() == 2) std::cout << "  removing face, m=" << m << std::endl;
          if (result_faces[m].size() == 2) break;

          it_next = it;
          if (it_next == result_faces[m].end()) it_next = result_faces[m].begin();

          it_prev = it;
          if (it_prev == result_faces[m].begin()) it_prev = result_faces[m].end();
          else it_prev--;

          new_edges[m] = std::make_pair(*it_prev, *it_next);
std::cout << "  new edge:" << *it_prev << " " << *it_next << "   p1=" 
          << result_xyz[*it_prev].second << "  p2=" << result_xyz[*it_next].second << std::endl;
        } else {
          it++;
        }
      }
    } 

    // forming a new face
    int n1, n2, n3, n4(-1);
    std::list<int> new_face;

    for (int m = 0; m < nfaces3; ++m) {
      n1 = new_edges[m].second;
      n2 = new_edges[m].first;
      if (n1 >= 0) {
        new_face.push_back(n1);
        break;
      }
    }

    while(n4 != n1) {
      for (int m = 0; m < nfaces3; ++m) {
        n3 = new_edges[m].second;
        n4 = new_edges[m].first;
        if (n2 == n3) {
          new_face.push_back(n3);
          n2 = n4;
          break;
        }
      }
    }

    if (new_face.size() > 2) {
      std::cout << "  adding new face:" << std::endl;
      result_faces.push_back(new_face);
    }
  }

  // output of the result
  int n(0), nfaces3(result_faces.size());
  int nxyz3(result_xyz.size());
  std::vector<int> map(nxyz3, -1);

  // -- count true faces
  for (int i = 0; i < nfaces3; ++i) {
    if (result_faces[i].size() > 2) n++;
  }
  faces3.resize(n);

  n = 0;
  for (int i = 0; i < nfaces3; ++i) {
    if (result_faces[i].size() > 2) {
      faces3[n].clear();
      for (it = result_faces[i].begin(); it != result_faces[i].end(); ++it) {
        faces3[n].push_back(*it);
        map[*it] = 0;
      }
      n++;
    }
  }

  int m(0);
  for (int i = 0; i < nxyz3; ++i) {
    if (map[i] == 0) map[i] = m++;
  }

  // -- count true vertices
  xyz3.clear();
  if (n > 3) { 
    for (int i = 0; i < nxyz3; ++i) {
      if (map[i] >= 0) xyz3.push_back(result_xyz[i].second);
    }
  }

std::cout << "updating map" << std::endl;
  // -- update face-to-nodes maps
  for (int i = 0; i < n; ++i) {
    int nnodes = faces3[i].size();
    for (int k = 0; k < nnodes; ++k) {
      faces3[i][k] = map[faces3[i][k]];
    }
  }
}

}  // namespace AmanziGeometry
}  // namespace Amanzi
