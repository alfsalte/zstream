
CXX := g++
CFLAGS := -g
CXXFLAGS := $(CFLAGS) -std=c++17

ODIR := obj
O := .o
X := .exe

OBJS := $(ODIR)/test-zstream$(O)

all: $(ODIR)/test-zstream$(X)

$(ODIR)/test-zstream$(X): $(ODIR)/test-zstream$(O) ../obj/zstream.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -lz

../obj/zstream$(O): ../zstream.cxx ../zstream.hxx ../zstream.tcc
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(ODIR)/test-zstream$(O): test-zstream.cxx ../zstream.hxx ../zstream.tcc
	$(CXX) -c $(CXXFLAGS) -o $@ $<
