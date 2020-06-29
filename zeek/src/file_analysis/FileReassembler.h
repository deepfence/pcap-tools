#pragma once

#include "Reassem.h"

class BroFile;
class Connection;

namespace file_analysis {

class File;

class FileReassembler final : public Reassembler {
public:

	FileReassembler(File* f, uint64_t starting_offset);
	~FileReassembler() override;

	void Done();

	// Checks if we have delivered all contents that we can possibly
	// deliver for this endpoint.
	void CheckEOF();

	/**
	 * Discards all contents of the reassembly buffer.  This will spin through
	 * the buffer and call File::DeliverStream() and File::Gap() wherever
	 * appropriate.
	 * @return the number of new bytes now detected as gaps in the file.
	 */
	uint64_t Flush();

	/**
	 * Discards all contents of the reassembly buffer up to a given sequence
	 * number.  This will spin through the buffer and call
	 * File::DeliverStream() and File::Gap() wherever appropriate.
	 * @param sequence the sequence number to flush until.
	 * @return the number of new bytes now detected as gaps in the file.
	 */
	uint64_t FlushTo(uint64_t sequence);

	/**
	 * @return whether the reassembler is currently is the process of flushing
	 * out the contents of its buffer.
	 */
	bool IsCurrentlyFlushing() const
		{ return flushing; }

protected:
	FileReassembler();

	void Undelivered(uint64_t up_to_seq) override;
	void BlockInserted(DataBlockMap::const_iterator it) override;
	void Overlap(const u_char* b1, const u_char* b2, uint64_t n) override;

	File* the_file;
	bool flushing;
};

} // namespace analyzer::*
