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

struct ArchiveReadDeleter {
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

using ScopedArchive = std::unique_ptr<struct archive, ArchiveReadDeleter>;
using ScopedArchiveWriter = std::unique_ptr<struct archive, ArchiveWriterDeleter>;
using ScopedArchiveEntry = std::unique_ptr<struct archive_entry, ArchiveEntryDeleter>;

struct ArchiveReader::Impl {
	std::filesystem::path path;
	ScopedArchive archive;

	bool ensureOpen() {
		if (archive) {
			return true;
		}

		archive.reset(archive_read_new());
		archive_read_support_filter_all(archive.get());
		archive_read_support_format_all(archive.get());

		if (archive_read_open_filename(archive.get(), path.string().c_str(), 10240) != ARCHIVE_OK) {
			spdlog::error("ArchiveIO: Failed to open archive: {}", path.string());
			archive.reset();
			return false;
		}
		return true;
	}

	void close() {
		archive.reset();
	}
};

ArchiveReader::ArchiveReader() : m_impl(std::make_unique<Impl>()) { }
ArchiveReader::~ArchiveReader() = default;

ArchiveReader::ArchiveReader(ArchiveReader&&) noexcept = default;
ArchiveReader& ArchiveReader::operator=(ArchiveReader&&) noexcept = default;

bool ArchiveReader::open(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path)) {
		return false;
	}
	m_impl->path = path;
	m_impl->close(); // Reset cached handle if path changes
	return true;
}

std::optional<std::vector<uint8_t>> ArchiveReader::extractFile(const std::string& internalPath) {
	if (!m_impl->ensureOpen()) {
		return std::nullopt;
	}

	// Since libarchive read handle is sequential, we might need to reopen or seek if we already passed it.
	// For simplicity in this iteration, we reopen if we don't find it to support multiple non-sequential calls,
	// but we'll try one pass first.

	auto attemptExtract = [&]() -> std::optional<std::vector<uint8_t>> {
		struct archive_entry* entry;
		while (archive_read_next_header(m_impl->archive.get(), &entry) == ARCHIVE_OK) {
			std::string entryName = archive_entry_pathname(entry);
			if (entryName == internalPath) {
				la_int64_t entry_size = archive_entry_size(entry);

				// Handle unknown size or negative size
				if (entry_size < 0) {
					// Stream read
					std::vector<uint8_t> buffer;
					uint8_t temp[4096];
					la_ssize_t bytes_read;
					while ((bytes_read = archive_read_data(m_impl->archive.get(), temp, sizeof(temp))) > 0) {
						buffer.insert(buffer.end(), temp, temp + bytes_read);
					}
					if (bytes_read < 0) {
						spdlog::error("ArchiveIO: Error reading data from entry: {}", internalPath);
						return std::nullopt;
					}
					return buffer;
				} else if (entry_size > 1024 * 1024 * 512) { // 512MB limit for sanity
					spdlog::error("ArchiveIO: Entry size too large: {} bytes", entry_size);
					return std::nullopt;
				}

				size_t size = static_cast<size_t>(entry_size);
				std::vector<uint8_t> buffer;
				buffer.reserve(size);

				uint8_t temp[4096];
				la_ssize_t bytes_read;
				size_t total_read = 0;
				while (total_read < size && (bytes_read = archive_read_data(m_impl->archive.get(), temp, std::min<size_t>(sizeof(temp), size - total_read))) > 0) {
					buffer.insert(buffer.end(), temp, temp + bytes_read);
					total_read += bytes_read;
				}

				if (bytes_read < 0) {
					spdlog::error("ArchiveIO: Failed to read data from entry: {}", internalPath);
					return std::nullopt;
				}

				return buffer;
			}
		}
		return std::nullopt;
	};

	auto result = attemptExtract();
	if (!result) {
		// If not found, it might be behind us in the stream. Reset and try once more.
		m_impl->close();
		if (m_impl->ensureOpen()) {
			result = attemptExtract();
		}
	}
	return result;
}

std::vector<std::string> ArchiveReader::listFiles() {
	std::vector<std::string> files;
	m_impl->close(); // Always scan from start
	if (!m_impl->ensureOpen()) {
		return files;
	}

	struct archive_entry* entry;
	while (archive_read_next_header(m_impl->archive.get(), &entry) == ARCHIVE_OK) {
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

ArchiveWriter::ArchiveWriter(ArchiveWriter&&) noexcept = default;
ArchiveWriter& ArchiveWriter::operator=(ArchiveWriter&&) noexcept = default;

bool ArchiveWriter::open(const std::filesystem::path& path) {
	m_impl->archive.reset(archive_write_new());
	if (!m_impl->archive) {
		return false;
	}

	archive_write_add_filter_gzip(m_impl->archive.get());
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
