//////////////////////////////////////////////////////////////////////
// Test for FileHandle and NodeFileHandle classes
// Verifies basic read/write operations and binary compatibility
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "io/filehandle.h"

// Helper to clean up temporary files
struct TempFile {
	std::string path;
	TempFile(std::string p) : path(p) {}
	~TempFile() {
		if (std::filesystem::exists(path)) {
			std::filesystem::remove(path);
		}
	}
};

void test_disk_node_file_handle() {
	std::cout << "Test: DiskNodeFileHandle read/write..." << std::endl;

	std::string filename = "test_node_file.dat";
	TempFile temp(filename);
	std::string identifier = "TEST";

	// Write
	{
		DiskNodeFileWriteHandle writeHandle(filename, identifier);
		if (!writeHandle.isOk()) {
			std::cerr << "Failed to open file for writing: " << writeHandle.getErrorMessage() << std::endl;
			assert(false);
		}

		// Node structure:
		// ROOT (implied)
		//   - NODE_START
		//     - U8: 1
		//     - NODE_START
		//       - String: "Hello"
		//     - NODE_END
		//   - NODE_END

		writeHandle.addNode(1);
		writeHandle.addString("Hello");
		writeHandle.endNode();
		writeHandle.close();
	}

	// Read
	{
		std::vector<std::string> acceptable = {identifier};
		DiskNodeFileReadHandle readHandle(filename, acceptable);
		if (!readHandle.isOk()) {
			std::cerr << "Failed to open file for reading: " << readHandle.getErrorMessage() << std::endl;
			assert(false);
		}

		BinaryNode* root = readHandle.getRootNode();
		assert(root != nullptr);

		// We expect the first node to be type 1 (which we wrote)
		// But wait, DiskNodeFileReadHandle::getRootNode reads the first byte to check for NODE_START
		// And then returns a node.
		// The node type is actually the NEXT byte after NODE_START.
		// Let's check how BinaryNode works.
		// BinaryNode::load reads until next node starts.
		// It seems the node type is part of the data of the *parent* node if we look at addNode implementation:
		// cache[local_write_index++] = NODE_START;
		// cache[local_write_index++] = nodetype;

		// So getRootNode() reads NODE_START. Then it creates a node.
		// Then node->load() reads everything until next NODE_START/NODE_END.
		// So the 'nodetype' byte we wrote (1) should be the first byte of data in the root node?

		// Let's verify.
		uint8_t type;
		if (!root->getU8(type)) {
			std::cerr << "Failed to read node type" << std::endl;
			assert(false);
		}
		assert(type == 1);

		std::string str;
		if (!root->getString(str)) {
			std::cerr << "Failed to read string" << std::endl;
			assert(false);
		}
		assert(str == "Hello");
	}

	std::cout << "Test: DiskNodeFileHandle PASSED" << std::endl;
}

void test_file_read_handle() {
	std::cout << "Test: FileReadHandle..." << std::endl;

	std::string filename = "test_raw_file.dat";
	TempFile temp(filename);

	// Create a raw file
	{
		std::ofstream out(filename, std::ios::binary);
		uint8_t u8 = 42;
		uint16_t u16 = 12345;
		uint32_t u32 = 0xDEADBEEF;
		out.write((char*)&u8, sizeof(u8));
		out.write((char*)&u16, sizeof(u16));
		out.write((char*)&u32, sizeof(u32));
	}

	// Read
	{
		FileReadHandle fh(filename);
		assert(fh.isOk());

		uint8_t u8;
		assert(fh.getU8(u8));
		assert(u8 == 42);

		uint16_t u16;
		assert(fh.getU16(u16));
		assert(u16 == 12345);

		uint32_t u32;
		assert(fh.getU32(u32));
		assert(u32 == 0xDEADBEEF);
	}

	std::cout << "Test: FileReadHandle PASSED" << std::endl;
}

int main() {
	try {
		test_disk_node_file_handle();
		test_file_read_handle();
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "Test failed with exception: " << e.what() << std::endl;
		return 1;
	}
}
