set (SOURCE_FILES
	allocator.cpp
	clusterizer.cpp
	indexcodec.cpp
	indexgenerator.cpp
	overdrawanalyzer.cpp
	overdrawoptimizer.cpp
	simplifier.cpp
	spatialorder.cpp
	stripifier.cpp
	vcacheanalyzer.cpp
	vcacheoptimizer.cpp
	vertexcodec.cpp
	vertexfilter.cpp
	vfetchanalyzer.cpp
	vfetchoptimizer.cpp
)

add_library(meshoptimizer STATIC ${SOURCE_FILES})
