
#include <cstddef>
#include <cstring>

#include <iostream>
#include <string>

#include "zstream.hxx"

///////////////////////
// basic_buffer

void
alf::basic_buffer::ensure_(std::size_t n)
{
  std::size_t k = n_ + n;
  if (k <= m_)
    return;
  std::size_t u = m_;
  if (m_ < 128)
    u = 128;
  while (k > u && u < 4096) u += u;
  // adding 4096 ensures that u > k, i.e. if k happen to be
  // a multiple of 4096 we get u == k + 4096 rather than u == k.
  if (k > u) u = (k + 4096) & -4096;
  std::size_t N = n_*s_, U = u*s_;
  char * v = new char[U];
  if (n_)
    std::memcpy(v, p_, N);
  std::memset(v + N, 0, U - N);
  delete [] p_;
  p_ = v;
  m_ = u;
}

void
alf::basic_buffer::shrink_(std::size_t n)
{
  if (n < m_) {
    if (n == 0) {
      delete [] p_;
      p_ = 0;
      n_ = m_ = 0;
      return;
    }
    std::size_t N = n*s_;
    std::size_t u = n < n_ ? n : n_;
    std::size_t us = u * s_;
    char * p = new char[N];
    if (n_)
      std::memcpy(p, p_, us);
    if (n_ < n)
      std::memset(p + us, 0, N - us);
    delete [] p_;
    p_ = p;
  }
}

