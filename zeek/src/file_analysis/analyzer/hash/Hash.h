// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <string>

#include "Val.h"
#include "OpaqueVal.h"
#include "File.h"
#include "Analyzer.h"

#include "events.bif.h"

namespace file_analysis {

/**
 * An analyzer to produce a hash of file contents.
 */
class Hash : public file_analysis::Analyzer {
public:

	/**
	 * Destructor.
	 */
	~Hash() override;

	/**
	 * Incrementally hash next chunk of file contents.
	 * @param data pointer to start of a chunk of a file data.
	 * @param len number of bytes in the data chunk.
	 * @return false if the digest is in an invalid state, else true.
	 */
	bool DeliverStream(const u_char* data, uint64_t len) override;

	/**
	 * Finalizes the hash and raises a "file_hash" event.
	 * @return always false so analyze will be deteched from file.
	 */
	bool EndOfFile() override;

	/**
	 * Missing data can't be handled, so just indicate the this analyzer should
	 * be removed from receiving further data.  The hash will not be finalized.
	 * @param offset byte offset in file at which missing chunk starts.
	 * @param len number of missing bytes.
	 * @return always false so analyzer will detach from file.
	 */
	bool Undelivered(uint64_t offset, uint64_t len) override;

protected:

	/**
	 * Constructor.
	 * @param args the \c AnalyzerArgs value which represents the analyzer.
	 * @param file the file to which the analyzer will be attached.
	 * @param hv specific hash calculator object.
	 * @param kind human readable name of the hash algorithm to use.
	 */
	Hash(IntrusivePtr<RecordVal> args, File* file, HashVal* hv, const char* kind);

	/**
	 * If some file contents have been seen, finalizes the hash of them and
	 * raises the "file_hash" event with the results.
	 */
	void Finalize();

private:
	HashVal* hash;
	bool fed;
	const char* kind;
};

/**
 * An analyzer to produce an MD5 hash of file contents.
 */
class MD5 : public Hash {
public:

	/**
	 * Create a new instance of the MD5 hashing file analyzer.
	 * @param args the \c AnalyzerArgs value which represents the analyzer.
	 * @param file the file to which the analyzer will be attached.
	 * @return the new MD5 analyzer instance or a null pointer if there's no
	 *         handler for the "file_hash" event.
	 */
	static file_analysis::Analyzer* Instantiate(IntrusivePtr<RecordVal> args,
	                                            File* file)
		{ return file_hash ? new MD5(std::move(args), file) : nullptr; }

protected:

	/**
	 * Constructor.
	 * @param args the \c AnalyzerArgs value which represents the analyzer.
	 * @param file the file to which the analyzer will be attached.
	 */
	MD5(IntrusivePtr<RecordVal> args, File* file)
		: Hash(std::move(args), file, new MD5Val(), "md5")
		{}
};

/**
 * An analyzer to produce a SHA1 hash of file contents.
 */
class SHA1 : public Hash {
public:

	/**
	 * Create a new instance of the SHA1 hashing file analyzer.
	 * @param args the \c AnalyzerArgs value which represents the analyzer.
	 * @param file the file to which the analyzer will be attached.
	 * @return the new MD5 analyzer instance or a null pointer if there's no
	 *         handler for the "file_hash" event.
	 */
	static file_analysis::Analyzer* Instantiate(IntrusivePtr<RecordVal> args,
	                                            File* file)
		{ return file_hash ? new SHA1(std::move(args), file) : nullptr; }

protected:

	/**
	 * Constructor.
	 * @param args the \c AnalyzerArgs value which represents the analyzer.
	 * @param file the file to which the analyzer will be attached.
	 */
	SHA1(IntrusivePtr<RecordVal> args, File* file)
		: Hash(std::move(args), file, new SHA1Val(), "sha1")
		{}
};

/**
 * An analyzer to produce a SHA256 hash of file contents.
 */
class SHA256 : public Hash {
public:

	/**
	 * Create a new instance of the SHA256 hashing file analyzer.
	 * @param args the \c AnalyzerArgs value which represents the analyzer.
	 * @param file the file to which the analyzer will be attached.
	 * @return the new MD5 analyzer instance or a null pointer if there's no
	 *         handler for the "file_hash" event.
	 */
	static file_analysis::Analyzer* Instantiate(IntrusivePtr<RecordVal> args,
	                                            File* file)
		{ return file_hash ? new SHA256(std::move(args), file) : nullptr; }

protected:

	/**
	 * Constructor.
	 * @param args the \c AnalyzerArgs value which represents the analyzer.
	 * @param file the file to which the analyzer will be attached.
	 */
	SHA256(IntrusivePtr<RecordVal> args, File* file)
		: Hash(std::move(args), file, new SHA256Val(), "sha256")
		{}
};

} // namespace file_analysis
