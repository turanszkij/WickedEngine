#pragma once
// (C) Sebastian Aaltonen 2023
// MIT License (see file: LICENSE)

// Modified for Wicked Engine
//  - removed cpp20 features
//	- removed constructors
//	- changed node storage to std::vector
//	- reduced size of Node structure

//#define USE_16_BIT_OFFSETS

#include <vector>

namespace OffsetAllocator
{
    typedef unsigned char uint8;
    typedef unsigned short uint16;
    typedef unsigned int uint32;

    // 16 bit offsets mode will halve the metadata storage cost
    // But it only supports up to 65536 maximum allocation count
#ifdef USE_16_BIT_NODE_INDICES
    typedef uint16 NodeIndex;
#else
    typedef uint32 NodeIndex;
#endif

    static constexpr uint32 NUM_TOP_BINS = 32;
    static constexpr uint32 BINS_PER_LEAF = 8;
    static constexpr uint32 TOP_BINS_INDEX_SHIFT = 3;
    static constexpr uint32 LEAF_BINS_INDEX_MASK = 0x7;
    static constexpr uint32 NUM_LEAF_BINS = NUM_TOP_BINS * BINS_PER_LEAF;

    struct Allocation
    {
        static constexpr uint32 NO_SPACE = 0xffffffff;
        
        uint32 offset = NO_SPACE;
        NodeIndex metadata = NO_SPACE; // internal: node index
    };

    struct StorageReport
    {
		uint32 totalFreeSpace = 0;
		uint32 largestFreeRegion = 0;
    };

    struct StorageReportFull
    {
        struct Region
        {
			uint32 size = 0;
			uint32 count = 0;
        };
        
        Region freeRegions[NUM_LEAF_BINS];
    };

    class Allocator
    {
    public:
		void init(uint32 size, uint32 maxAllocs = 128 * 1024);
		void reset();
        
        Allocation allocate(uint32 size);
        void free(Allocation allocation);

        uint32 allocationSize(Allocation allocation) const;
        StorageReport storageReport() const;
        StorageReportFull storageReportFull() const;
        
    private:
        uint32 insertNodeIntoBin(uint32 size, uint32 dataOffset);
        void removeNodeFromBin(uint32 nodeIndex);

        struct Node
        {
            static constexpr NodeIndex unused = 0xffffffff;
            
			uint32 dataOffset : 32;
			uint32 dataSize : 31;
			uint32 used : 1;
            NodeIndex binListPrev : 32;
            NodeIndex binListNext : 32;
            NodeIndex neighborPrev : 32;
            NodeIndex neighborNext : 32;

			Node()
			{
				dataOffset = 0;
				dataSize = 0;
				binListPrev = unused;
				binListNext = unused;
				neighborPrev = unused;
				neighborNext = unused;
				used = 0;
			}
        };
    
        uint32 m_size = 0;
        uint32 m_maxAllocs = 0;
        uint32 m_freeStorage = 0;

        uint32 m_usedBinsTop = 0;
		uint8 m_usedBins[NUM_TOP_BINS] = {};
		NodeIndex m_binIndices[NUM_LEAF_BINS] = {};
                
		std::vector<Node> m_nodes;
		std::vector<NodeIndex> m_freeNodes;
		uint32 m_freeOffset = 0;
    };
}
