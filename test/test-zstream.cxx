
#include <cstddef>
#include <cstring>
#include <cstdio>

#include <iostream>
#include <fstream>
#include <string>

#include "../zstream.hxx"

int pack(const char * gzfn, const char * fn);
int unpack(const char * fn, const char * gzfn);

int main(int argc, const char * argv[])
{
  static const char z[] = ".z";
  std::size_t zlen = std::strlen(z);
  std::size_t alen, blen;
  const char * a, * b;
  char buf[1024];

  if (argc == 2) {
    a = argv[1];
    alen = std::strlen(a);
    if (alen > zlen && std::strcmp(a + alen - zlen, z) == 0) {
      blen = alen - zlen;
      std::memcpy(buf, a, blen);
      buf[blen] = 0;
      return unpack(a, buf);
    } else {
      sprintf(buf, "%s.z", a);
      return pack(buf, a);
    }
  } else if (argc == 3) {
    a = argv[1];
    b = argv[2];
    if (std::strcmp(a, "pack") == 0) {
      sprintf(buf, "%s.z", b);
      return pack(buf, b);
    } else if (std::strcmp(a, "unpack") == 0) {
      blen = std::strlen(b);
      if (blen > zlen && std::strcmp(b + blen - zlen, z) == 0) {
	std::memcpy(buf, b, blen -= zlen);
	buf[blen] = 0;
	return unpack(buf, b);
      } else {
	sprintf(buf, "%s.z", b);
	return unpack(b, buf);
      }
    } else {
      alen = std::strlen(a);
      blen = std::strlen(b);
      if (alen > zlen && std::strcmp(a + alen - zlen, z) == 0)
	return unpack(b, a);
      else if (blen > zlen && std::strcmp(b + blen - zlen, z) == 0)
	return pack(b, a);
      std::cerr << "What do you want me to do with " << a << " and "
		<< b << std::endl;
      return 1;
    }
  } else if (argc == 4) {
    a = argv[1];
    if (std::strcmp(a, "pack") == 0) {
      a = argv[2];
      b = argv[3];
      blen = std::strlen(b);
      if (blen <= zlen || std::strcmp(b + blen - zlen, z) != 0) {
	sprintf(buf, "%s.z", b);
	b = buf;
      }
      return pack(b, a);
    } else if (std::strcmp(a, "unpack") == 0) {
      a = argv[2];
      b = argv[3];
      alen = std::strlen(a);
      if (alen <= zlen || std::strcmp(a + alen - zlen, z) != 0) {
	sprintf(buf, "%s.z", a);
	a = buf;
      }
      return unpack(b, a);
    }
    std::cerr << "What do you want me to do with " << a << ", " << argv[2]
	      << " and " << argv[3] << std::endl;
  } else
    std::cerr << "too many arguments." << std::endl;
  return 1;
}

int pack(const char * gzfn, const char * fn)
{
  std::ofstream gzf(gzfn);
  alf::DefaultCompressor<char> C;
  alf::zostream<std::ofstream,alf::DefaultCompressor<char>> z(gzf, C);
  std::ifstream f(fn);
  char c;
  while (f.get(c))
    z << c;
  return 0;
}

int unpack(const char * fn, const char * gzfn)
{
  std::ofstream out(fn);
  std::ifstream inp(gzfn);
  alf::DefaultDecompressor<char> D;
  alf::zistream<std::ifstream,alf::DefaultDecompressor<char>> z(inp, D);
  char c;
  while (z.get(c))
    out << c;
  return 0;
}
