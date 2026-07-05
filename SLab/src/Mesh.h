#pragma once

#include <vector>

template <typename V, typename I> class Mesh
{
  public:
    Mesh<V, I>();

  public:
    std::vector<V> m_vertices;
    std::vector<I> m_indices;
};

template <typename V, typename I> inline Mesh<V, I>::Mesh()
{
}
