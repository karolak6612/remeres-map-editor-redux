//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_ARCHIVE_IO_H_
#define RME_ARCHIVE_IO_H_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <filesystem>

/**
 * @brief Helper class to read from compressed map archives (OTGZ)
 */
class ArchiveReader {
public:
	ArchiveReader();
	~ArchiveReader();

	bool open(const std::filesystem::path& path);

	/**
	 * @brief Extracts a specific file from the archive into a memory buffer
	 * @param internalPath Path within the archive (e.g., "world/map.otbm")
	 * @return Buffer with file contents, or nullopt if not found or error
	 */
	std::optional<std::vector<uint8_t>> extractFile(const std::string& internalPath);

	/**
	 * @brief Lists all files in the archive
	 */
	std::vector<std::string> listFiles();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Helper class to create compressed map archives (OTGZ)
 */
class ArchiveWriter {
public:
	ArchiveWriter();
	~ArchiveWriter();

	bool open(const std::filesystem::path& path);

	/**
	 * @brief Adds a file to the archive from a memory buffer
	 * @param internalPath Path within the archive
	 * @param data Data buffer
	 */
	bool addFile(const std::string& internalPath, std::span<const uint8_t> data);

	void close();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // RME_ARCHIVE_IO_H_
