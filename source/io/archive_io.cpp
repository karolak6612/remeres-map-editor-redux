//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "archive_io.h"

#ifdef OTGZ_SUPPORT
	#include <archive.h>
	#include <archive_entry.h>
#endif

#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>

#ifdef OTGZ_SUPPORT

struct ArchiveDeleter {
	void operator()(struct archive* a) const {
		if (a) {
			archive_read_free(a);
		}
	}
};

struct ArchiveWriterDeleter {
	void operator()(struct archive* a) const {
		if (a) {
			archive_write_close(a);
			archive_write_free(a);
		}
	}
};

struct ArchiveEntryDeleter {
	void operator()(struct archive_entry* e) const {
		if (e) {
			archive_entry_free(e);
		}
	}
};

using ScopedArchive = std::unique_ptr<struct archive, ArchiveDeleter>;
using ScopedArchiveWriter = std::unique_ptr<struct archive, ArchiveWriterDeleter>;
using ScopedArchiveEntry = std::unique_ptr<struct archive_entry, ArchiveEntryDeleter>;

struct ArchiveReader::Impl {
	std::filesystem::path path;
};

ArchiveReader::ArchiveReader() : m_impl(std::make_unique<Impl>()) { }
ArchiveReader::~ArchiveReader() = default;

bool ArchiveReader::open(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path)) {
		return false;
	}
	m_impl->path = path;
	return true;
}

std::optional<std::vector<uint8_t>> ArchiveReader::extractFile(const std::string& internalPath) {
	ScopedArchive a(archive_read_new());
	archive_read_support_filter_all(a.get());
	archive_read_support_format_all(a.get());

	if (archive_read_open_filename(a.get(), m_impl->path.string().c_str(), 10240) != ARCHIVE_OK) {
		spdlog::error("ArchiveIO: Failed to open archive: {}", m_impl->path.string());
		return std::nullopt;
	}

	struct archive_entry* entry;
	while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK) {
		std::string entryName = archive_entry_pathname(entry);
		if (entryName == internalPath) {
			size_t size = archive_entry_size(entry);
			std::vector<uint8_t> buffer(size);

			if (archive_read_data(a.get(), buffer.data(), size) < 0) {
				spdlog::error("ArchiveIO: Failed to read data from entry: {}", internalPath);
				return std::nullopt;
			}

			return buffer;
		}
	}

	return std::nullopt;
}

std::vector<std::string> ArchiveReader::listFiles() {
	std::vector<std::string> files;
	ScopedArchive a(archive_read_new());
	archive_read_support_filter_all(a.get());
	archive_read_support_format_all(a.get());

	if (archive_read_open_filename(a.get(), m_impl->path.string().c_str(), 10240) != ARCHIVE_OK) {
		return files;
	}

	struct archive_entry* entry;
	while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK) {
		files.emplace_back(archive_entry_pathname(entry));
	}

	return files;
}

struct ArchiveWriter::Impl {
	ScopedArchiveWriter archive;
	std::filesystem::path path;
};

ArchiveWriter::ArchiveWriter() : m_impl(std::make_unique<Impl>()) { }
ArchiveWriter::~ArchiveWriter() {
	close();
}

bool ArchiveWriter::open(const std::filesystem::path& path) {
	m_impl->archive.reset(archive_write_new());
	if (!m_impl->archive) {
		return false;
	}

	archive_write_set_compression_gzip(m_impl->archive.get());
	archive_write_set_format_pax_restricted(m_impl->archive.get());

	if (archive_write_open_filename(m_impl->archive.get(), path.string().c_str()) != ARCHIVE_OK) {
		spdlog::error("ArchiveIO: Failed to open archive for writing: {}", path.string());
		return false;
	}

	m_impl->path = path;
	return true;
}

bool ArchiveWriter::addFile(const std::string& internalPath, std::span<const uint8_t> data) {
	if (!m_impl->archive) {
		return false;
	}

	ScopedArchiveEntry entry(archive_entry_new());
	archive_entry_set_pathname(entry.get(), internalPath.c_str());
	archive_entry_set_size(entry.get(), data.size());
	archive_entry_set_filetype(entry.get(), AE_IFREG);
	archive_entry_set_perm(entry.get(), 0644);

	if (archive_write_header(m_impl->archive.get(), entry.get()) != ARCHIVE_OK) {
		spdlog::error("ArchiveIO: Failed to write header for: {}", internalPath);
		return false;
	}

	if (archive_write_data(m_impl->archive.get(), data.data(), data.size()) < 0) {
		spdlog::error("ArchiveIO: Failed to write data for: {}", internalPath);
		return false;
	}

	return true;
}

void ArchiveWriter::close() {
	m_impl->archive.reset();
}

#else // OTGZ_SUPPORT not defined

// Stubs for non-OTGZ builds
struct ArchiveReader::Impl { };
ArchiveReader::ArchiveReader() = default;
ArchiveReader::~ArchiveReader() = default;
bool ArchiveReader::open(const std::filesystem::path&) {
	return false;
}
std::optional<std::vector<uint8_t>> ArchiveReader::extractFile(const std::string&) {
	return std::nullopt;
}
std::vector<std::string> ArchiveReader::listFiles() {
	return {};
}

struct ArchiveWriter::Impl { };
ArchiveWriter::ArchiveWriter() = default;
ArchiveWriter::~ArchiveWriter() = default;
bool ArchiveWriter::open(const std::filesystem::path&) {
	return false;
}
bool ArchiveWriter::addFile(const std::string&, std::span<const uint8_t>) {
	return false;
}
void ArchiveWriter::close() { }

#endif
