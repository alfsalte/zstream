// -*- c++ -*-

// included by zstream.hxx

template <typename IST, typename OST, typename D__, typename C__>
void
alf::zstreambuf<IST,OST,D__,C__>::init(istream_type * isp,
				       ostream_type * osp,
				       D__ * d, C__ * c)
{
  zis_ = isp;
  zos_ = osp;
  D_ = d;
  C_ = c;
  if (zis_ != 0) {
    if (d == 0)
      // something is wrong.
      throw "zis_ is non-zero while D_ is 0";
    
    // set up initial get area.
    char_type * g = ibuf.get(BUFSZ);
    this->setg(g, g + BUFSZ, g + BUFSZ);
  }
  if (zos_ != 0) {
    if (c == 0)
      // something is wrong.
      throw "zos_ is non-zero while C_ is 0";
    // set up initial put area.
    char_type * p = obuf.get(BUFSZ);
    this->setp(p, p + BUFSZ);
  }
}

template <typename IST, typename OST, typename D__, typename C__>
// virtual
typename alf::zstreambuf<IST,OST,D__,C__>::int_type
alf::zstreambuf<IST,OST,D__,C__>::underflow()
{
  // get area:
  // eback() -- start of get area.
  // gptr() -- pointer to current char.
  // egptr() -- pointer to end of get area.

  // if data left in buffer, return it.
  if (this->gptr() && this->gptr() < this->egptr())
    return traits_type::to_int_type(*this->gptr());
  // if not reading return eof.
  if (zis_ == 0)
    return traits_type::eof();
  // do we have any data in compressed buffer?
  std::size_t n = 0;
  bool is_eof = false;
  if (zibuf.len() > 0 && zibufpos > 0 && zibufpos < zibuf.len()) {
    // move left over data to beginning of buffer.
    n = zibuf.len() - zibufpos;
    traits_type::move(zibuf.data(), zibuf.data() + zibufpos, n);
    zibuf.force_len(n);
  }
  zibufpos = 0;
  // read in more data from source.
  std::size_t zibufpos_ = zibuf.len();
  int k = read_in_(zibuf);
  if (k < 0)
    return traits_type::eof();
  char_type * ibuf_save = ibuf.data();

  k = D_->decompress(ibuf, zibuf, zibuf.avail() > 0);

  char_type * ibufp = ibuf.data();
  if (ibufp != ibuf_save)
    // decompress expanded the buffer.
    this->setg(ibufp, ibufp, ibufp);

  if (this->gptr() == 0 && ibufp != 0)
    this->setg(ibufp, ibufp, ibufp);

  zibufpos = zibufpos_;

  if (k < 0)
    return traits_type::eof();

  this->setg(ibufp, ibufp, ibufp + ibuf.len());

  return traits_type::to_int_type(*this->gptr());
}

template <typename IST, typename OST, typename D__, typename C__>
// virtual
typename alf::zstreambuf<IST,OST,D__,C__>::int_type
alf::zstreambuf<IST,OST,D__,C__>::overflow(int_type c)
{
  int k = 0;

  bool is_eof = traits_type::eq_int_type(c, traits_type::eof());
  if (this->pbase()) {
    if (this->pptr() > this->epptr() || this->pptr() < this->pbase())
      return traits_type::eof();
    // make sure pbase, pptr and epptr points into obuf.
    // I.e. pbase must equal obuf.data().
    // pptr() must point somewhere in obuf.data()..obuf.real_endp()
    // epptr() must point somewhere in pptr()..obuf.real_endp()
    if (this->pbase() != obuf.data() ||
	(this->pptr() > obuf.real_endp()) ||
	(this->pptr() < obuf.data()) ||
	(this->epptr() > obuf.real_endp()) ||
	(this->epptr() < this->pptr()))
      return traits_type::eof();
    int n = this->pptr() - this->pbase(); // should be the same as obuf.len().
    obuf.force_len(n);

    // add extra char to buffer if not eof.
    if (! is_eof) {
      char_type * obufp = obuf.get(2);
      *obufp = traits_type::to_char_type(c);
      obuf.inclen(1);
      this->pbump(1);
      ++n;
    }
    if (zos_ == 0)
      return traits_type::eof();
    k = C_->compress(zobuf, obuf, is_eof);
    std::size_t olen = obuf.len();
    if (olen < 4096)
      obuf.ensure(olen = 4096);
    char_type * obufp = obuf.data();
    this->setp(obufp, obufp + olen);
  } else {
    if (zos_ == 0)
      return traits_type::eof();
    if (! is_eof) {
      char_type * obufp2 = obuf.get(1);
      *obufp2 = traits_type::to_char_type(c);
      obuf.inclen(1);
    }
    // do we need space for compressed data?
    k = C_->compress(zobuf, obuf, is_eof);
  }
  if (k < 0)
    return traits_type::eof();
  // write output
  std::size_t zlen = zobuf.len();
  if (zlen > 0) {
    int u = write_out_(zobuf);
    if (u != zlen)
      return traits_type::eof();
    zobuf.clear();
  }
  return is_eof ? traits_type::not_eof(c) : c;
}

template <typename IST, typename OST, typename D__, typename C__>
// virtual
alf::zstreambuf<IST,OST,D__,C__> *
alf::zstreambuf<IST,OST,D__,C__>::setbuf(char_type * b, std::streamsize n)
{
  return this;
}

template <typename IST, typename OST, typename D__, typename C__>
void
alf::zstreambuf<IST,OST,D__,C__>::cleanup()
{
  // flush any output buffer data.
  if (zos_ != 0 && overflow(traits_type::eof()) == traits_type::eof()) {
    // something went wrong during cleanup.
  }
}

template <typename IST, typename OST, typename D__, typename C__>
int
alf::zstreambuf<IST,OST,D__,C__>::read_in_(zibuf_type & b)
{
  if (zis_ == 0)
    return -1;
  if (! *zis_ )
    return -1;
  // Dang, wish there was a function like readsome but which would
  // underflow() its buffer when reaching the end.
  // Have to use read() here and get the chars read from somewhere
  // else, also, if we read some chars, we should clear the state
  // as we can read the eof next time when we get no chars.
  std::streamsize k;

  std::size_t n = b.avail();
  if (n < 4096) {
    b.ensure(4096);
    n = b.avail();
  }
  char_type * p = b.get(n);
  if (! zis_->read(p, n)) {
    k = zis_->gcount();
    // if eof it will be set again when we call read next time.
    if (k == 0)
      return -1;
  }
  k = zis_->gcount();
  if (k > 0) {
    zis_->clear();
    b.inclen(k);
  }
  return k;
}

template <typename IST, typename OST, typename D__, typename C__>
int
alf::zstreambuf<IST,OST,D__,C__>::write_out_(const zobuf_type & b)
{
  if (zos_ == 0)
    return -1;
  if (! *zos_)
    return -1;
  if (! zos_->write(b.data(), b.len()))
    return -1;
  return b.len();
}

///////////////////////
// buffer

// fill buffer with fill char until length n_ is a multiple of a.
// a == 0 is treated as a == 1 and is unaligned, i.e. do nothing.
// a should be a power of 2, so 2, 4, 8, 16, 32, 64, 128, 256, etc is ok.
template <typename ChT, typename TrT>
alf::buffer<ChT,TrT> &
alf::buffer<ChT,TrT>::align(std::size_t a, char_type fill /* = char_type() */)
{
  if (a < 2) return *this;
  // check that a is a power of two (has only one bit)
  if ( ( a & -a ) != a ) return *this;
  std::size_t k = (n_ + a - 1) & -a;

  if (k > n_) {
    std::size_t i = k - n_;
    ensure(i);
    traits_type::assign(p_ + n_, i, fill);
    n_ = k;
  }
  return *this;
}


/////////////////////////
// DefaultCompressor

template <typename IChT, typename ITrT, typename OChT, typename OTrT>
alf::DefaultCompressor<IChT,ITrT,OChT,OTrT>::DefaultCompressor()
{
  U.ensure(8192);
  C.ensure(8192);
  // initialize s_stream.
  Z.total_in = 0;
  Z.total_out = 0;
  Z.msg = 0;
  Z.state = 0;
  Z.zalloc = 0;
  Z.zfree = 0;
  Z.opaque = 0;
  Z.data_type = Z_TEXT;
  Z.adler = 0;
  Z.reserved = 0;
  ret = deflateInit(&Z, 9);
}

template <typename IChT, typename ITrT, typename OChT, typename OTrT>
int
alf::DefaultCompressor<IChT,ITrT,OChT,OTrT>::
compress(cout_buf_type & d, const cin_buf_type & s, bool flush /* = false */)
{
  // first copy the data to U.
  std::size_t len = s.len();
  std::size_t blen = len * sizeof(cin_char_type);
  char * p = U.get(blen);
  cin_traits_type::move(reinterpret_cast<cin_char_type *>(p), s.data(), len);
  U.inclen(blen);
  std::size_t ulen = U.len();
  int flush_ = flush || len == 0;
  int r = Z_OK;
  if (flush_ || ulen >= 8192) {
    // compress what we have in U.
    unsigned char * pp = reinterpret_cast<unsigned char *>(U.data());
    Z.next_in = pp;
    Z.avail_in = ulen;
    // twice U.len() should suffice - we are supposed to compress
    // so in practice we should need less than ulen normally.
    unsigned char * q = reinterpret_cast<unsigned char *>(C.get(ulen << 1));
    Z.next_out = q;
    Z.avail_out = C.avail();
    if ((r = ret = deflate(&Z, flush_)) == Z_OK) {
      std::size_t k_in = Z.next_in - pp;
      std::size_t k_out = Z.next_out - q;
      // only take out whole chars to output.
      std::size_t n = k_out / sizeof(cout_char_type);
      std::size_t n_c = n * sizeof(cout_char_type);
      std::size_t rest = k_out - n_c;
      // n == size in number of chars
      // copy this to output buffer d.
      C.force_len(n*sizeof(cin_char_type));
      cout_traits_type::move(d.get(n),
			     reinterpret_cast<cout_char_type *>(C.data()), n);
      d.inclen(n);
      // if we want to flush we shouldn't have any data left in rest.
      if (flush_ && rest != 0) {
	q = reinterpret_cast<unsigned char *>(C.get(sizeof(cin_char_type)));
	// rest bytes filled with data, need to fill remaining s_ - rest.
	std::memset(q + rest, 0, sizeof(cin_char_type) - rest);
	cout_traits_type::move(d.get(1),
			       reinterpret_cast<cout_char_type *>
			       (C.data()) + n,
			       1);
	d.inclen(1);
	rest = 0;
      }
      // move remaining stuff to beginning of C.
      if (rest) std::memmove(C.data(), C.data() + n_c, rest);
      C.force_len(rest);
      // ditto for input buffer U.
      n = k_in / sizeof(cin_char_type);
      n_c = n * sizeof(cin_char_type);
      rest = k_in - n_c;
      // if input buffer has any data left while flusing something
      // is very wrong.
      if (flush_ && rest != 0) {
	// panic!, some data were not compressed!
	throw "Some data were not compressed";
      }
      // move rest bytes at end to beginning of U.
      if (rest) std::memmove(U.data(), U.data() + n_c, rest);
      U.force_len(rest);
    }
  }
  return r;
}

/////////////////////////
// DefaultDecompressor

template <typename OChT, typename OTrT, typename IChT, typename ITrT>
alf::DefaultDecompressor<OChT,OTrT,IChT,ITrT>::DefaultDecompressor()
{
  U.ensure(8192);
  C.ensure(8192);
  // initialize s_stream.
  Z.total_in = 0;
  Z.total_out = 0;
  Z.msg = 0;
  Z.state = 0;
  Z.zalloc = 0;
  Z.zfree = 0;
  Z.opaque = 0;
  Z.data_type = Z_TEXT;
  Z.adler = 0;
  Z.reserved = 0;
  ret = inflateInit(&Z);
}

template <typename OChT, typename OTrT, typename IChT, typename ITrT>
int
alf::DefaultDecompressor<OChT,OTrT,IChT,ITrT>::
decompress(dout_buf_type & d, const din_buf_type & s, bool flush /* = false */)
{
  // first copy the data to C.
  std::size_t len = s.len();
  char * p = C.get(len);
  std::memcpy(p, reinterpret_cast<const char *>(s.data()), len);
  C.inclen(len);
  std::size_t clen = C.len();
  int flush_ = flush || len < s.cap();
  int r = Z_OK;
  if (flush_ || clen >= 8192) {
    // decompress what we have in C.
    unsigned char * pp = reinterpret_cast<unsigned char *>(C.data());
    Z.next_in = pp;
    Z.avail_in = clen;
    // Eight times C.len() should hopefully suffice.
    unsigned char * q = reinterpret_cast<unsigned char *>(U.get(clen << 3));
    Z.next_out = q;
    Z.avail_out = U.avail();
    if ((r = ret = inflate(&Z, flush)) == Z_OK) {
      std::size_t k_in = Z.next_in - pp;
      std::size_t k_out = Z.next_out - q;
      // only take out whole chars to output.
      std::size_t n = k_out / sizeof(dout_char_type);
      std::size_t n_c = n * sizeof(dout_char_type);
      std::size_t rest = k_out - n_c;
      // n == size in number of chars
      // copy this to output buffer d.
      U.force_len(n*sizeof(dout_char_type));
      dout_traits_type::move(d.get(n),
			     reinterpret_cast<dout_char_type *>(U.data()), n);
      d.inclen(n);
      // if we want to flush, we should have no rest bytes
      // if we do have any, add enough bytes to make a char and move it
      // as well.
      if (flush_ && rest != 0) {
	q = reinterpret_cast<unsigned char *>(U.get(sizeof(dout_char_type)));
	// rest bytes filled with data, need to fill remaining
	// sizeof(dout_char_type) - rest.
	std::memset(q + rest, 0, sizeof(dout_char_type) - rest);
	dout_traits_type::move(d.get(1),
			       reinterpret_cast<dout_char_type *>
			       (U.data()) + n,
			       1);
	d.inclen(1);
	rest = 0;
      }

      // move remaining stuff to beginning of U.
      if (rest) std::memmove(U.data(), U.data() + n_c, rest);
      U.force_len(rest);
      // ditto for input buffer C.
      n = k_in / sizeof(din_char_type);
      n_c = n * sizeof(din_char_type);
      rest = k_in - n_c;
      // if we are flusing and we have data remaining in rest
      // we panic.
      if (flush_ && rest != 0) {
	throw "Some data were not decompressed.";
      }
      // move rest bytes at end to beginning of U.
      if (rest) std::memmove(C.data(), C.data() + n_c, rest);
      C.force_len(rest);
    }
  }
  return r;
}
