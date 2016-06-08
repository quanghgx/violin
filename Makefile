OFILES=ppm.o utils.o feature.o weak_classifier.o strong_classifier.o cascade_classifier.o
CXXFLAGS=-std=c++11 -g -O0

all : libclassy.a tests learn

libclassy.a : $(OFILES)
	ar rvs libclassy.a $(OFILES)

tests : libclassy.a
	g++ -g -O0 -std=c++11 test_ppm.cpp -otest_ppm -L. -lclassy
	g++ -g -O0 -std=c++11 test_feature.cpp -otest_feature -L. -lclassy

learn : learn.cpp libclassy.a
	g++ -g -O0 -std=c++11 learn.cpp -olearn -L. -lclassy
clean :
	rm -f *.o
	rm -f libclassy.a
	rm -f test_ppm
	rm -f test_feature
	rm -f learn
	rm -f *.ppm
