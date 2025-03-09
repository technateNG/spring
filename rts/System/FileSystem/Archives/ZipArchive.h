/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ZIP_ARCHIVE_H
#define _ZIP_ARCHIVE_H

#include "IArchiveFactory.h"
#include "BufferedArchive.h"
#include "minizip/unzip.h"

#include <string>
#include <vector>


/**
 * Creates zip compressed, single-file archives.
 * @see CZipArchive
 */
class CZipArchiveFactory : public IArchiveFactory {
public:
	CZipArchiveFactory(): IArchiveFactory("sdz") {}

private:
	IArchive* DoCreateArchive(const std::string& filePath) const;
};


/**
 * A zip compressed, single-file archive.
 */
class CZipArchive : public CBufferedArchive
{
public:
	CZipArchive(const std::string& archiveName);
	virtual ~CZipArchive();

	int GetType() const override { return ARCHIVE_TYPE_SDZ; }

	bool IsOpen() override { return (zipPerThread[0] != nullptr); }

	uint32_t NumFiles() const override { return (fileEntries.size()); }
	SFileInfo FileInfo(uint32_t fid) const override;

	#if 0
	uint32_t GetCrc32(uint32_t fid) {
		assert(IsFileId(fid));
		return fileEntries[fid].crc;
	}
	#endif

	static constexpr int MAX_THREADS = 32;
protected:
	int GetFileImpl(uint32_t fid, std::vector<std::uint8_t>& buffer) override;
private:
	std::array<unzFile, MAX_THREADS> zipPerThread = {nullptr};

	// actual data is in BufferedArchive
	struct FileEntry {
		unz_file_pos fp;
		int size;
		std::string origName;
		uint32_t crc;
		uint32_t modTime;
	};

	std::vector<FileEntry> fileEntries;

	static inline spring::mutex archiveLock;
};

#endif // _ZIP_ARCHIVE_H
