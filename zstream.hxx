#ifndef __ALF_ZSTREAM_HXX__
#define __ALF_ZSTREAM_HXX__

#include <cstring>

#include "zlib.h"

#include <string>
#include <iostream>

namespace alf {

/*
** This class is intended to be used as follows:
**
** std::ostream some_ostream;
**
** alf::DefaultCompressor C; // or some other compressor you write yourself.
**
** alf::zostream is(some_ostream, C);
**
** Now, if you write to is, it will come out to some_ostream as
** compressed data.
** Some ostream should use char as char type, std::ostream should work
** fine.
**
** or the other way:
**
** std::istream some_istream; // compressed data.
**
** // decompressor - or your own but it must be able to decompress the
** // data from some_istream.
**
** alf::DefaultDecompressor D;
**
** alf::zistream is(some_istream, D);
**
** The data from is is now decompressed.
**
** The following compressors/decomprssors already exists:
**
** DefaultCompressor/DefaultDecompressor - zlib gzip compression/decompression.
** NoCompressor/NoDecompressor - dummy, no compression/decompression.
** FailCompressor/FailDecompressor - dummy - always fails.
**
** Underlying the istream/ostreams are a zstreambuf and it will use
** FailCompressor as the compressor for zistream and FailDecompressor
** as the decompressor for zostream.
**
** There is also a ziostream but there is never any reason to use it so
** stick to zistream/zostream.
**
**
** The file also defines a class buffer<char_type,traits_type> to be used
** as buffer for the compression/decompression.
** it is simply a char pointer that can grow as more space is needed and
** has a very simple interface:
**
** Say you plan to write 8K bytes to a char buffer:
**
** buffer<char> B;
**
** char * p = B.get(8192);
**
** // but then you ended up only writing 4096 of those data.
** std::memcpy(p, some_data, 4096);
**
** // Note that the buffer's length is still whatever it was in the beginning,
** .. i.e. 0 in this case.
**
** B.inclen(4096);
**
** // The buffer has capacity for at least 4096 more bytes but they are
** //  currently unused.
**
** You can clear the buffer which will set the length to 0 but it still
** keeps the buffer so you can add to it.
** If the buffer is too large and you really want it to shrink
** you can do shrink(len); To shrink it to length len. If the buffer's
** capacity is less than len, it will do nothing.
 */


// data types for compression/decompression algorithms.
// this is not itself necessarily templated, it operates on bytes only
// but variations of it can appear as template argument to zstreambuf
// and friends.

// these classes are supposed to have the following members.
//
// class Compressor {
// public:
//
//      // Note "cin" here does not mean std::cin but "compressor in(put)".
//      typedef ..... cin_char_type; // The char type of your input.
//      typedef ...... cin_traits_type; // the corresponding traits type.
//      typedef .....  cout_char_type; // The char type of compressed data.
//      typedef ...... cout_traits_type; // traits_type for compressed data.
//
//      // cin_char_type is used by user to write to the z(i)ostream.
//      // cout_char_type is associated with the attached stream.
//
//      // Note that cout_char_type must match the char_type of the
//      // ostream type attached to the zstreambuf.
//
//      // compress data in buffer s of length ssz into d.
//      // except that a compress with empty buffer resets context and makes
//      // the compressor forget previous context and start as if from fresh.
//
//      int compress(buffer<cout_char_type,cout_traits_type> & d,
//                   const buffer<cin_char_type,cin_traits_type> & s,
//                   bool flush);
//
//    }; // end of class Compressor
//
//    class Decompressor {
//    public:
//
//      typedef .... din_char_type; // input to the decompressor often char.
//      typedef .... din_traits_type; // associated traits.
//      typedef .... dout_char_type; // output char type of decompressor.
//      typedef  ... dout_traits_type; // associated traits.
//
//      // dout_char_type is the char_type that user read/write to the zstream.
//      // din_char_type is the char_type that we read from the attached
//      // stream.
//
//      // Decompresses the data. Again, context are previous calls which
//      // can be reset by passing on a buffer where s.len() == 0.
//      // That will also reset the decompressor and flush out all
//      data not yet given out.
//
//      int decompress(buffer<dout_char_type,dout_traits_type> & d,
//                     const buffer<din_char_type,din_traits_type> & s,
//                     bool flush);
//
//      Note that context only changes for succesful calls to
//      compress and decompress.
//   };
//
//   It is possible to have one class have all methods and so it can be
//   used as both compressor and decompressor for zstream.
//

// some compressors and decompressors to use.
template <typename IChT,
	  typename ITrT = std::char_traits<IChT>,
	  typename OChT = char,
	  typename OTrT = std::char_traits<OChT> >
class DefaultCompressor;

template <typename OChT,
	  typename OTrT = std::char_traits<OChT>,
	  typename IChT = char,
	  typename ITrT = std::char_traits<IChT> >
class DefaultDecompressor;

template <typename IChT,
	  typename ITrT = std::char_traits<IChT>,
	  typename OChT = IChT,
	  typename OTrT = std::char_traits<OChT> >
class NoCompressor;

template <typename OChT,
	  typename OTrT = std::char_traits<OChT>,
	  typename IChT = OChT,
	  typename ITrT = std::char_traits<IChT> >
class NoDecompressor;

template <typename IChT,
	  typename ITrT = std::char_traits<IChT>,
	  typename OChT = char,
	  typename OTrT = std::char_traits<OChT> >
class FailCompressor;

template <typename OChT,
	  typename OTrT = std::char_traits<OChT>,
	  typename IChT = char,
	  typename ITrT = std::char_traits<IChT> >
class FailDecompressor;

// we also need this nifty utility.
// start with a non-template base class.
class basic_buffer {
public:

  std::size_t len() const { return n_; }
  std::size_t cap() const { return m_; }
  std::size_t avail() const { return m_ - n_; }
  std::size_t char_size() const { return s_; }

protected:

  basic_buffer(std::size_t s) : p_(0), s_(s), n_(0), m_(0) { }

  basic_buffer(void * p, std::size_t s, std::size_t n, std::size_t m)
    : p_(reinterpret_cast<char *>(p)),  s_(s), n_(n), m_(m)
  { }

  basic_buffer(basic_buffer && b)
    : p_(b.p_), s_(b.s_), n_(b.n_), m_(b.m_)
  { b.p_ = 0; b.n_ = b.m_ = 0; }

  ~basic_buffer() { delete [] p_; }

  void ensure_(std::size_t n);
  void shrink_(std::size_t n);

  char * p_;
  std::size_t s_;
  std::size_t n_;
  std::size_t m_;

}; // end of class basic_buffer.

// Now for the utility class.
template <typename ChT, typename TrT = std::char_traits<ChT> >
class buffer : public basic_buffer {
public:

  typedef ChT char_type;
  typedef TrT traits_type;

  buffer() : basic_buffer(sizeof(char_type)) { }

  buffer(std::size_t m) : basic_buffer(sizeof(char_type)) { ensure(m); }

  buffer(const buffer & b)
    : basic_buffer(sizeof(char_type))
  { ensure(b.m_); traits_type::move(p_, b.p_, n_*sizeof(char_type)); }

  buffer(buffer && b) : basic_buffer(std::move(b)) { }

  buffer & ensure(std::size_t n)
  { ensure_(n); return *this; }

  buffer & shrink(std::size_t n)
  { shrink_(n); return *this; }

  char_type * data() { return reinterpret_cast<char_type *>(p_); }

  const char_type * data() const
  { return reinterpret_cast<const char_type *>(p_); }

  const char_type * cdata() const
  { return reinterpret_cast<const char_type *>(p_); }

  buffer & clear() { n_ = 0; return *this; }
  char_type * endp() { return data() + n_; }
  const void * real_endp() const { return cdata() + m_; }
  const char_type * endp() const { return cdata() + n_; }
  const char_type * cendp() const { return cdata() + n_; }

  template <typename OChT, typename OTrT>
  buffer & append(const buffer<OChT,OTrT> & other)
  {
    std::size_t len = other.len();
    ensure_(len);
    const OChT * s = other.data();
    const OChT * e = s + len;
    char_type * p = endp();
    while (s < e)
      *p++ = traits_type::to_char_type(OTrT::to_int_type(*s++));
    n_ += len;
    return *this;
  }
    
  template <typename OChT, typename OTrT>
  buffer & copy(const buffer<OChT,OTrT> & other)
  {
    n_ = 0;
    return append(other);
  }

  // these two are used together like this:
  // step 1. char_type * buf = mybuf.get(15);
  // step 2. write data to buf - no more than 15 chars.
  // step 3. increase the length by how much you actually
  //         wrote - a value between 1 and 15 - say 13 chars.
  //         mybuf.inclen(13);
  // get an area of n bytes where we can write stuff.
  // Note that two calls to get will get the same buffer.
  // I.e. get does not reserve the space, just return a pointer to
  // end of current data, if you want three different sections you
  // have to arrange that yourself. Do one get to get all sections,
  // then split it up and then arrange and possibly pack the data
  // before doing an inclen  to add all of them to the buffer.
  char_type * get(std::size_t n)
  { ensure(n); return data() + n_; }

  // increase the length by n.
  buffer & inclen(std::size_t n)
  {
    std::size_t k = n_ + n;
    if (k > m_) k = m_;
    n_ = k;
    return *this;
  }

  // force length to n, 0 <= n <= m_
  buffer & force_len(std::size_t n)
  {
    if (n <= m_) n_ = n;
    return *this;
  }

  // fill buffer with fill char until length n_ is a multiple of a.
  // a == 0 is treated as a == 1 and is unaligned, i.e. do nothing.
  // a should be a power of 2, so 2, 4, 8, 16, 32, 64, 128, 256, etc is ok.
  buffer & align(std::size_t a, char_type fill = char_type());

}; // end of class buffer
      
// Note that input stream and output stream must both have same
// char_type and traits_type.
template <typename IST = std::ifstream,
	  typename OST = std::ofstream,
	  // NB! D__ is associated with IST and must have the same
	  // char type out as C__ has char type in and
	  // D__::in_char_type must be the same as IST::char_type etc.
	  // D__::out_char_type must be the same as C__::in_char_type.
	  // C__::out_char_type must be the same as OST::char_type etc
	  // and should be the same as IST::char_type.
	  typename D__ = DefaultDecompressor<typename IST::char_type,
					     typename IST::traits_type,
					     char,std::char_traits<char> >,

	  typename C__ = DefaultCompressor<typename OST::char_type,
					   typename OST::traits_type,
					   char, std::char_traits<char> > >

class zstreambuf :
    public std::basic_streambuf<typename D__::dout_char_type,
				typename D__::dout_traits_type> {
public:

  // restrictions:
  // The "==" here means "is the same type as"
  //
  // D__::dout_char_type == C__::cin_char_type
  // D__::dout_char_traits == C__::cin_char_traits
  // D__::din_char_type == IST::char_type
  // D__::din_char_traits == IST::char_traits
  // C__::cout_char_type == OST::char_type
  // C__::cout_char_traits == OST::char_traits

  typedef typename D__::dout_char_type char_type;
  typedef typename D__::dout_traits_type traits_type;
  typedef IST istream_type;
  typedef OST ostream_type;
  typedef D__ Decompressor;
  typedef C__ Compressor;
  typedef typename IST::char_type istream_char_type;
  typedef typename IST::traits_type istream_traits_type;
  typedef typename OST::char_type ostream_char_type;
  typedef typename OST::traits_type ostream_traits_type;
  typedef buffer<istream_char_type,istream_traits_type> zibuf_type;
  typedef buffer<ostream_char_type,ostream_traits_type> zobuf_type;
  typedef buffer<char_type,traits_type> ibuf_type;
  typedef ibuf_type obuf_type;

  typedef std::basic_streambuf<typename D__::dout_char_type,
			       typename D__::dout_traits_type> base_type;
  typedef typename base_type::int_type int_type;

  
  // for decompressing
  zstreambuf() : zis_(0), zos_(0), D_(0), C_(0) { }

  ~zstreambuf()
  { cleanup(); }

  void init(istream_type * isp, ostream_type * osp, D__ * d, C__ * c);

  virtual int_type underflow();
  virtual int_type overflow(int_type c);

  virtual
  zstreambuf *
  setbuf(char_type *, std::streamsize);

  Compressor * compressor() { return C_; }
  Decompressor * decompressor() { return D_; }

private:

  enum { BUFSZ = 4096 };

  void cleanup();

  int read_in_(zibuf_type & b);
  int write_out_(const zobuf_type & b);

  istream_type * zis_;
  ostream_type * zos_;
  Decompressor * D_;
  Compressor * C_;
  ibuf_type ibuf;
  obuf_type obuf;
  zibuf_type zibuf;
  zobuf_type zobuf;
  std::size_t zibufpos;

}; // end of class zstreambuf

// zistream
template <typename IST = std::istream,
	  typename D__ = DefaultDecompressor<typename IST::char_type,
					     typename IST::traits_type,
					     char,std::char_traits<char> > >
class zistream : public std::basic_istream<typename D__::dout_char_type,
					   typename D__::dout_traits_type> {
public:

  // restrictions: "==" means "is the same type as".
  // IST::char_type == D__::in_char_type
  // IST::char_traits == D__::in_char_traits
  // D__::out_char_type and D__::out_char_traits must be properly defined.
  typedef typename D__::dout_char_type char_type;
  typedef typename D__::dout_traits_type traits_type;
  typedef IST istream_type;
  typedef D__ Decompressor;
  typedef typename IST::char_type istream_char_type;
  typedef typename IST::traits_type istream_traits_type;
  typedef FailCompressor<istream_char_type, istream_traits_type,
			 char_type,traits_type> C__;
  typedef std::basic_ostream<istream_char_type,istream_traits_type> OST;
  typedef OST ostream_type;
  typedef C__ Compressor;
  typedef zstreambuf<IST, OST, D__, C__> streambuf;

  zistream(istream_type & is, Decompressor & D)
  { this->init(& zbuf_); zbuf_.init(& is, 0, & D, 0); }

private:

  streambuf zbuf_;

}; // end of class zistream

// zostream
template <typename OST = std::ofstream,
	  typename C__ = DefaultCompressor<typename OST::char_type,
					   typename OST::traits_type,
					   char, std::char_traits<char> > >
class zostream : public std::basic_ostream<typename C__::cin_char_type,
					   typename C__::cin_traits_type> {
public:

  // restrictions: "==" means "is the same type as".
  // IST::char_type == D__::in_char_type
  // IST::char_traits == D__::in_char_traits
  // D__::out_char_type and D__::out_char_traits must be properly defined.
  typedef typename C__::cin_char_type char_type;
  typedef typename C__::cin_traits_type traits_type;
  typedef OST ostream_type;
  typedef C__ Compressor;
  typedef typename OST::char_type ostream_char_type;
  typedef typename OST::traits_type ostream_traits_type;
  typedef FailDecompressor<char_type,traits_type,
			   ostream_char_type, ostream_traits_type> D__;
  typedef std::basic_istream<ostream_char_type,ostream_traits_type> IST;
  typedef IST istream_type;
  typedef D__ Decompressor;
  typedef zstreambuf<IST, OST, D__, C__> streambuf;

  zostream(OST & os, Compressor & C)
  { this->init(& zbuf_); zbuf_.init(0, & os, 0, & C); }

private:

  streambuf zbuf_;

}; // end of class zostream

// ziostream
template <typename IST = std::ifstream,
	  typename OST = std::ofstream,

	  typename D__ = DefaultDecompressor<typename IST::char_type,
					     typename IST::traits_type,
					     char,std::char_traits<char> >,

	  typename C__ = DefaultCompressor<typename OST::char_type,
					   typename OST::traits_type,
					   char, std::char_traits<char> > >

class ziostream : public std::basic_iostream<typename D__::out_char_type,
					     typename D__::out_traits_type> {
public:

  // restrictions: "==" means "is the same type as".
  // IST::char_type == D__::in_char_type
  // IST::char_traits == D__::in_char_traits
  // D__::out_char_type and D__::out_char_traits must be properly defined.
  typedef typename D__::out_char_type char_type;
  typedef typename D__::out_traits_type traits_type;
  typedef IST istream_type;
  typedef OST ostream_type;
  typedef D__ Decompressor;
  typedef C__ Compressor;
  typedef typename IST::char_type istream_char_type;
  typedef typename IST::traits_type istream_traits_type;
  typedef typename OST::char_type ostream_char_type;
  typedef typename OST::traits_type ostream_traits_type;
  typedef zstreambuf<IST, OST, D__, C__> streambuf;

  ziostream(IST & is, Decompressor & D,
	    OST & os, Compressor & C)
  { this->init(& zbuf_); zbuf_.init(& is, & os, & D, & C); }

private:

  streambuf zbuf_;

}; // end of class ziostream


// some compressors and decompressors
template <typename IChT,
	  typename ITrT /* = std::char_traits<IChT> */,
	  typename OChT /* = char */,
	  typename OTrT /* = std::char_traits<OChT> */ >
class DefaultCompressor {
public:

  typedef IChT cin_char_type;
  typedef ITrT cin_traits_type;
  typedef OChT cout_char_type;
  typedef OTrT cout_traits_type;
  typedef buffer<cout_char_type,cout_traits_type> cout_buf_type;
  typedef buffer<cin_char_type,cin_traits_type> cin_buf_type;

  DefaultCompressor();
  int compress(cout_buf_type & d, const cin_buf_type & s, bool flush=false);
  DefaultCompressor & reset();
  int zlibret() const { return ret; }
  const char * msg() const { return Z.msg; }

private:

  typedef buffer<char,std::char_traits<char> > char_buf_type;

  z_stream Z;
  char_buf_type U; // our private compress src buffer - uncompressed data.
  char_buf_type C; // our private compress dst buffer - compressed data.
  int ret; // return code from last zlib call.

}; // end of class DefaultCompressor

template <typename OChT,
	  typename OTrT /* = std::char_traits<OChT> */,
	  typename IChT /* = char */,
	  typename ITrT /* = std::char_traits<IChT> */ >
class DefaultDecompressor {
public:

  typedef OChT dout_char_type;
  typedef OTrT dout_traits_type;
  typedef IChT din_char_type;
  typedef ITrT din_traits_type;
  typedef buffer<dout_char_type,dout_traits_type> dout_buf_type;
  typedef buffer<din_char_type,din_traits_type> din_buf_type;

  DefaultDecompressor();
  int decompress(dout_buf_type & d, const din_buf_type & s, bool flush=false);
  int zlibret() const { return ret; }
  const char * msg() const { return Z.msg; }

private:

  typedef buffer<char,std::char_traits<char> > char_buf_type;

  z_stream Z;
  char_buf_type C; // our private decompress src buffer - compressed data.
  char_buf_type U; // our private decompress dst buffer - uncompressed data.
  int ret; // return code from last zlib call.

}; // end of class DefaultDecompressor

template <typename IChT,
	  typename ITrT /* = std::char_traits<IChT> */,
	  typename OChT /* = IChT */,
	  typename OTrT /* = std::char_traits<OChT> */ >
class NoCompressor {
public:

  typedef IChT cin_char_type;
  typedef ITrT cin_traits_type;
  typedef OChT cout_char_type;
  typedef OTrT cout_traits_type;
  typedef buffer<cout_char_type,cout_traits_type> cout_buf_type;
  typedef buffer<cin_char_type,cin_traits_type> cin_buf_type;

  int compress(cout_buf_type & d, const cin_buf_type & s, bool = false)
  {
    // Just append the data to d - no compression.
    d.append(s);
    return 0;
  }

}; // end of class NoCompressor

template <typename OChT,
	  typename OTrT /* = std::char_traits<OChT> */,
	  typename IChT /* = OChT */,
	  typename ITrT /* = std::char_traits<IChT> */ >
class NoDecompressor {
public:

  typedef OChT dout_char_type;
  typedef OTrT dout_traits_type;
  typedef IChT din_char_type;
  typedef ITrT din_traits_type;
  typedef buffer<dout_char_type,dout_traits_type> dout_buf_type;
  typedef buffer<din_char_type,din_traits_type> din_buf_type;

  int decompress(dout_buf_type & d, const din_buf_type & s, bool = false)
  {
    // Just append the data to d - no decompression.
    d.append(s);
    return 0;
  }

}; // end of class NoDecompressor

template <typename IChT,
	  typename ITrT /* = std::char_traits<IChT> */,
	  typename OChT /* = char */,
	  typename OTrT /* = std::char_traits<OChT> */ >
class FailCompressor {
public:

  typedef IChT cin_char_type;
  typedef ITrT cin_traits_type;
  typedef OChT cout_char_type;
  typedef OTrT cout_traits_type;
  typedef buffer<cout_char_type,cout_traits_type> cout_buf_type;
  typedef buffer<cin_char_type,cin_traits_type> cin_buf_type;

  int compress(cout_buf_type &, const cin_buf_type &, bool = false)
  { return -1; /* always fail */ }

}; // end of class FailCompressor

template <typename OChT,
	  typename OTrT /* = std::char_traits<OChT> */,
	  typename IChT /* = char */,
	  typename ITrT /* = std::char_traits<IChT> */ >
class FailDecompressor {
public:

  typedef OChT dout_char_type;
  typedef OTrT dout_traits_type;
  typedef IChT din_char_type;
  typedef ITrT din_traits_type;
  typedef buffer<dout_char_type,dout_traits_type> dout_buf_type;
  typedef buffer<din_char_type,din_traits_type> din_buf_type;

  int decompress(dout_buf_type &, const din_buf_type &, bool = false)
  { return -1; /* always fail */ }

}; // end of class FailDecompressor

// ziomanip stuff

// some predeclarations
template <typename IST = std::ifstream,
	  typename OST = std::ofstream,
	  typename D__ = DefaultDecompressor<typename IST::char_type,
					     typename IST::traits_type,
					     char,std::char_traits<char> >,
	  typename C__ = DefaultCompressor<typename OST::char_type,
					   typename OST::traits_type,
					   char, std::char_traits<char> >,
	  typename... T>
class ziomanip;

// one arg
template <typename IST, typename OST, typename D__, typename C__,
	  typename T >
class ziomanip<IST,OST,D__,C__,T> {
protected:

  typedef zistream<IST,D__> zistream_;
  typedef zostream<OST,C__> zostream_;
  typedef ziostream<IST,OST,D__,C__> ziostream_;

  typedef zistream_ & (* ifunp1_type)(zistream_ &, T);
  typedef zostream_ & (* ofunp1_type)(zistream_ &, T);
  typedef ziostream_ & (* iofunp1_type)(ziostream_ &, T);

  friend zistream_  & operator >> (zistream_  & s, const ziomanip & m)
  { return reinterpret_cast<ifunp1_type *>(m.f_)(s, m.v_); }

  friend ziostream_ & operator >> (ziostream_ & s, const ziomanip & m)
  { return reinterpret_cast<iofunp1_type *>(m.f_)(s, m.v_); }

  friend zostream_  & operator << (zostream_  & s, const ziomanip & m)
  { return reinterpret_cast<ofunp1_type *>(m.f_)(s, m.v_); }

  friend ziostream_ & operator << (ziostream_ & s, const ziomanip & m)
  { return reinterpret_cast<iofunp1_type *>(m.f_)(s, m.v_); }

  void * f_; // used for function pointer.
  T v_;

  ziomanip(void * f, T v) : f_(f), v_(v) { }

public:

  ziomanip(zistream_ & (*f)(zistream_ &, T), T v)
    : f_(f), v_(v)
  { }

  ziomanip(zostream_ & (*f)(zostream_ &, T), T v)
    : f_(f), v_(v)
  { }

  ziomanip(ziostream_ & (*f)(ziostream_ &, T), T v)
    : f_(f), v_(v)
  { }

}; // end of template class specialization ziomanip<T>

// two arg
template <typename IST, typename OST, typename D__, typename C__,
	  typename T1, typename T2 >
class ziomanip<IST,OST,D__,C__,T1,T2> : public ziomanip<IST,OST,D__,C__,T1> {
protected:

  typedef ziomanip<IST,OST,D__,C__,T1> base_type;

  typedef typename base_type::zistream_ zistream_;
  typedef typename base_type::zostream_ zostream_;
  typedef typename base_type::ziostream_ ziostream_;
  //using base_type::zistream_;
  //using base_type::zostream_;
  //using base_type::ziostream_;

  typedef zistream_ & (* ifunp2_type)(zistream_ &, T1, T2);
  typedef zostream_ & (* ofunp2_type)(zistream_ &, T1, T2);
  typedef ziostream_ & (* iofunp2_type)(ziostream_ &, T1, T2);

  friend zistream_  & operator >> (zistream_  & s, const ziomanip & m)
  { return reinterpret_cast<ifunp2_type *>(m.f_)(s, m.v_, m.v2_); }

  friend ziostream_ & operator >> (ziostream_ & s, const ziomanip & m)
  { return reinterpret_cast<iofunp2_type *>(m.f_)(s, m.v_, m.v2_); }

  friend zostream_  & operator << (zostream_  & s, const ziomanip & m)
  { return reinterpret_cast<ofunp2_type *>(m.f_)(s, m.v_, m.v2_); }

  friend ziostream_ & operator << (ziostream_ & s, const ziomanip & m)
  { return reinterpret_cast<iofunp2_type *>(m.f_)(s, m.v_, m.v2_); }

  T2 v2_;

  ziomanip(void * f, T1 v1, T2 v2) : base_type(f, v1), v2_(v2) { }

public:

  ziomanip(zistream_ & (*f)(zistream_ &, T1, T2), T1 v1, T2 v2)
    : base_type(f, v1), v2_(v2)
  { }

  ziomanip(zostream_ & (*f)(zostream_ &, T1, T2), T1 v1, T2 v2)
    : base_type(f, v1), v2_(v2)
  { }

  ziomanip(ziostream_ & (*f)(ziostream_ &, T1, T2), T1 v1, T2 v2)
    : base_type(f, v1), v2_(v2)
  { }

}; // end of template class specialization ziomanip<T1,T2>

// three args
template <typename IST, typename OST, typename D__, typename C__,
	  typename T1, typename T2, typename T3 >
class ziomanip<IST,OST,D__,C__,T1,T2,T3> :
    public ziomanip<IST,OST,D__,C__,T1,T2> {
protected:

  typedef ziomanip<IST,OST,D__,C__,T1,T2> base_type;

  typedef typename base_type::zistream_ zistream_;
  typedef typename base_type::zostream_ zostream_;
  typedef typename base_type::ziostream_ ziostream_;
  //using base_type::zistream_;
  //using base_type::zostream_;
  //using base_type::ziostream_;

  typedef zistream_ & (* ifunp3_type)(zistream_ &, T1, T2, T3);
  typedef zostream_ & (* ofunp3_type)(zistream_ &, T1, T2, T3);
  typedef ziostream_ & (* iofunp3_type)(ziostream_ &, T1, T2, T3);

  friend zistream_  & operator >> (zistream_  & s, const ziomanip & m)
  { return reinterpret_cast<ifunp3_type *>(m.f_)(s, m.v_, m.v2_, m.v3_); }

  friend ziostream_ & operator >> (ziostream_ & s, const ziomanip & m)
  { return reinterpret_cast<iofunp3_type *>(m.f_)(s, m.v_, m.v2_, m.v3_); }

  friend zostream_  & operator << (zostream_  & s, const ziomanip & m)
  { return reinterpret_cast<ofunp3_type *>(m.f_)(s, m.v_, m.v2_, m.v3_); }

  friend ziostream_ & operator << (ziostream_ & s, const ziomanip & m)
  { return reinterpret_cast<iofunp3_type *>(m.f_)(s, m.v_, m.v2_, m.v3_); }

  T3 v3_;

  ziomanip(void * f, T1 v1, T2 v2, T3 v3) : base_type(f, v1, v2), v3_(v3) { }

public:

  ziomanip(zistream_ & (*f)(zistream_ &, T1, T2, T3), T1 v1, T2 v2, T3 v3)
    : base_type(f, v1, v2), v3_(v3)
  { }

  ziomanip(zostream_ & (*f)(zostream_ &, T1, T2, T3), T1 v1, T2 v2, T3 v3)
    : base_type(f, v1, v2), v3_(v3)
  { }

  ziomanip(ziostream_ & (*f)(ziostream_ &, T1, T2, T3), T1 v1, T2 v2, T3 v3)
    : base_type(f, v1, v2), v3_(v3)
  { }

}; // end of template class specialization ziomanip<T1,T2,T3>

}; // end of namespace alf

#include "zstream.tcc"

#endif
