/**
 * @file LIBuffer.h
 * @brief Reads and writes length-indicated ZIP code data files
 * @author CSCI 331 Team 2
 * @date 03/13/2026
 *
 * @details A length-indicated (LI) file stores every record — header and data —
 *          as a 2-byte big-endian uint16_t length prefix followed by the
 *          packed payload bytes.
 *
 *          File layout:
 *          [2-byte len][header payload]       <- HeaderBuffer writes/reads this
 *          [2-byte len][record payload]       <- data record 1
 *          [2-byte len][record payload]       <- data record 2
 *          ...
 *
 *          Data record payload (packed binary):
 *          [4-byte int  zipCode]
 *          [c-string    placeName\0]
 *          [c-string    state\0]
 *          [c-string    county\0]
 *          [8-byte double latitude]
 *          [8-byte double longitude]
 *
 * @note LIBuffer owns the file streams.
 *       HeaderBuffer and DataFileHeader are used as helpers.
 */

#ifndef LIBUFFER_H
#define LIBUFFER_H

#include <fstream>
#include <string>
#include <cstdint>
#include "ZipCodeRecord.h"
#include "DataFileHeader.h"
#include "HeaderBuffer.h"

/**
 * @class LIBuffer
 * @brief Buffered reader/writer for length-indicated ZIP code data files.
 *
 * @details Write workflow:
 *          1. Construct with filename
 *          2. openForWrite()
 *          3. writeHeader()
 *          4. writeRecord() for each data record
 *          5. close()
 *
 *          Read workflow:
 *          1. Construct with filename
 *          2. openForRead()
 *          3. readHeader()
 *          4. getNextRecord() until false
 *          5. close()
 *
 *          Indexed read workflow:
 *          1. openForRead()
 *          2. readHeader()
 *          3. readRecordAt(offset) for any record directly
 *
 * @pre  The file directory must be writable for write operations.
 * @post After close() the file is a valid LI ZIP code file.
 */
class LIBuffer
{
public:
    // ── Construction / Destruction ────────────────────────────────────────

    /**
     * @brief Construct an LIBuffer associated with a filename.
     * @param filename  Path to the .li binary data file.
     * @pre  None.
     * @post Object initialised; no file opened yet.
     */
    explicit LIBuffer(const std::string& filename);

    /**
     * @brief Destructor — closes any open streams.
     * @pre  None.
     * @post All open streams are flushed and closed.
     */
    ~LIBuffer();

    // ── File lifecycle ────────────────────────────────────────────────────

    /**
     * @brief Open the file for sequential reading.
     * @return true on success; false if file cannot be opened.
     * @pre  File must exist and be a valid LI ZIP code file.
     * @post Stream positioned at byte 0; ready for readHeader().
     */
    bool openForRead();

    /**
     * @brief Open (or create) the file for sequential writing.
     *        Truncates any existing content.
     * @return true on success; false if file cannot be opened.
     * @pre  Directory must be writable.
     * @post Stream positioned at byte 0; ready for writeHeader().
     */
    bool openForWrite();

    /**
     * @brief Close any open read or write streams.
     * @pre  None.
     * @post Both streams closed; internal flags reset.
     */
    void close();

    // ── Read interface ────────────────────────────────────────────────────

    /**
     * @brief Read and unpack the header record from the file.
     * @param header  Output: populated with file metadata on success.
     * @return true on success; false on I/O or unpack error.
     * @pre  openForRead() returned true; stream at byte 0.
     * @post header populated; stream advances past header record.
     */
    bool readHeader(DataFileHeader& header);

    /**
     * @brief Read and unpack the next data record.
     * @param record  Output: populated on success.
     * @return true on success; false at EOF or on error.
     * @pre  openForRead() and readHeader() called.
     * @post record populated; recordCount incremented; stream advances.
     */
    bool getNextRecord(ZipCodeRecord& record);

    /**
     * @brief Seek to a byte offset and read exactly one data record.
     * @param offset  Byte offset of the record's 2-byte length prefix.
     * @param record  Output: populated on success.
     * @return true on success; false on seek/read/unpack error.
     * @pre  openForRead() returned true.
     * @post record populated; stream positioned after the record.
     * @note Used by IndexLoader for O(1) direct access.
     */
    bool readRecordAt(std::streamoff offset, ZipCodeRecord& record);

    // ── Write interface ───────────────────────────────────────────────────

    /**
     * @brief Write the header record as the first LI record in the file.
     * @param header  The metadata to write.
     * @return true on success; false on I/O error.
     * @pre  openForWrite() returned true; must be the first write call.
     * @post Header occupies first bytes of file; stream advances.
     */
    bool writeHeader(DataFileHeader& header);

    /**
     * @brief Serialise and append one ZipCodeRecord to the file.
     * @param record  The ZIP code data to write.
     * @return true on success; false on I/O error.
     * @pre  openForWrite() and writeHeader() called.
     * @post One LI data record appended; recordCount incremented.
     */
    bool writeRecord(const ZipCodeRecord& record);

    // ── Accessors ─────────────────────────────────────────────────────────

    /**
     * @brief Return the byte offset of the current read position.
     * @pre  openForRead() called.
     * @post No state change.
     * @return Current byte offset; -1 if not open.
     */
    std::streamoff getCurrentOffset() const;

    /**
     * @brief Return the number of data records read or written so far.
     * @pre  None.
     * @post No state change.
     * @return Cumulative data record count.
     */
    long getRecordCount() const;

    /**
     * @brief Return the filename associated with this buffer.
     * @pre  None.
     * @post No state change.
     * @return The filename string.
     */
    std::string getFilename() const;

private:
    // ── Private helpers ───────────────────────────────────────────────────

    /**
     * @brief Pack a ZipCodeRecord into a raw byte buffer.
     * @param record  The record to serialise.
     * @param buffer  Caller-allocated buffer of at least getRecordPackedSize() bytes.
     * @pre  buffer is valid and large enough.
     * @post All record fields written to buffer.
     * @return Number of bytes written.
     */
    size_t packRecord(const ZipCodeRecord& record, char* buffer) const;

    /**
     * @brief Unpack a ZipCodeRecord from a raw byte buffer.
     * @param buffer  Buffer produced by packRecord().
     * @param record  Output: populated on success.
     * @pre  buffer contains a valid packed record.
     * @post record fully populated.
     * @return Number of bytes consumed.
     */
    size_t unpackRecord(const char* buffer, ZipCodeRecord& record, size_t len) const;

    /**
     * @brief Calculate the packed byte size of a ZipCodeRecord.
     * @param record  The record to measure.
     * @pre  None.
     * @post No state change.
     * @return Required buffer size in bytes.
     */
    size_t getRecordPackedSize(const ZipCodeRecord& record) const;

    /**
     * @brief Read a 2-byte big-endian length prefix from the read stream.
     * @param length  Output: decoded length.
     * @return true on success; false on EOF or I/O error.
     * @pre  inStream open and positioned at a length prefix.
     * @post Stream advances 2 bytes.
     */
    bool readLength(uint16_t& length);

    /**
     * @brief Write a 2-byte big-endian length prefix to the write stream.
     * @param length  The value to encode.
     * @return true on success; false on I/O error.
     * @pre  outStream open in binary write mode.
     * @post Stream advances 2 bytes.
     */
    bool writeLength(uint16_t length);

    // ── Member variables ──────────────────────────────────────────────────

    std::string   filename;      /**< Path to the .li data file              */
    std::ifstream inStream;      /**< Stream used for reading                */
    std::ofstream outStream;     /**< Stream used for writing                */
    bool          readOpen;      /**< True when inStream is open             */
    bool          writeOpen;     /**< True when outStream is open            */
    long          recordCount;   /**< Cumulative data record count           */
    HeaderBuffer  headerBuf;     /**< Delegate for header record I/O        */
};

#endif // LIBUFFER_H