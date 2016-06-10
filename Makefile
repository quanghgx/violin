OFILES=ppm.o utils.o feature.o weak_classifier.o strong_classifier.o cascade_classifier.o
CXXFLAGS=-pthread -std=c++11 -g -O0
CXX=g++
all : libclassy.a tests learn

libclassy.a : $(OFILES)
	ar rvs libclassy.a $(OFILES)

tests : libclassy.a
	$(CXX) $(CXXFLAGS) test_ppm.cpp -otest_ppm -L. -lclassy
	$(CXX) $(CXXFLAGS) test_feature.cpp -otest_feature -L. -lclassy
	$(CXX) $(CXXFLAGS) test_zip.cpp -otest_zip -L. -lclassy

learn : learn.cpp libclassy.a
	$(CXX) $(CXXFLAGS) learn.cpp -olearn -L. -lclassy
clean :
	rm -f *.o
	rm -f libclassy.a
	rm -f test_ppm
	rm -f test_feature
	rm -f learn
	rm -f *.ppm
