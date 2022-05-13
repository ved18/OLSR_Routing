default:
	g++-11 -o node node.cpp
	g++-11 -o controller controller.cpp
	touch topology.txt

clean:
	rm *.txt node controller
