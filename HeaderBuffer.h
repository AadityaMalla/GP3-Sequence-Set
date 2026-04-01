/**
 * @file HeaderBuffer.h
 * @brief Reads and writes the DataFileHeader as the first length-indicated
 *        record of a .li binary data file
 * @author CSCI 331 Team 2
 * @date 03/13/2026
 *
 * @details HeaderBuffer is responsible for ALL file I/O involving the header.
 *          It uses DataFileHeader::pack() and DataFileHeader::unpack() for
 *          serialisation, and writes/reads using a 2-byte big-endian length
 *          prefix — exactly the same format as every other record in the file.
 *
 *          Write layout at byte 0 of the .li file:
 *          [uint8_t hi][uint8_t lo]   <- 2-byte big-endian payload length
 *          [payload bytes ...]        <- DataFileHeader::pack() output
 */

#ifndef HEADERBUFFER_H
#define HEADERBUFFER_H

#include <fstream>
#include <string>
#include "DataFileHeader.h"

/**
 * @class HeaderBuffer
 * @brief Writes and reads a DataFileHeader as the first record of a .li file.
 *
 * @details Responsibilities:
 *          - writeHeader() -- packs the header and writes it with a 2-byte
 *            length prefix to byte 0 of an open output stream
 *          - readHeader()  -- reads the 2-byte length, reads the payload,
 *            and calls DataFileHeader::unpack() to restore all fields
 *
 *          NOT responsible for:
 *          - Opening or closing files  (caller does that)
 *          - Storing the DataFileHeader (caller owns it)
 *
 * @pre  Streams passed to read/write methods must already be open in
 *       binary mode and positioned at byte 0.
 * @post Stream position advances past the header record after each call.
 */
class HeaderBuffer
{
public:
    /**
     * @brief Default constructor.
     * @pre  None.
     * @post A HeaderBuffer instance is ready to use; no file is associated.
     */
    HeaderBuffer() = default;

    /**
     * @brief Default destructor.
     * @pre  None.
     * @post No resources to release.
     */
    ~HeaderBuffer() = default;

    /**
     * @brief Pack a DataFileHeader and write it as the first length-indicated
     *        record of a binary output stream.
     *
     * @param out     Open std::ofstream in binary mode, positioned at byte 0.
     * @param header  The DataFileHeader to serialise and write.
     * @return true on success; false on I/O error.
     *
     * @pre  out is open in std::ios::binary mode at position 0.
     * @post Header record written; stream advances past it.
     */
    bool writeHeader(std::ofstream& out, DataFileHeader& header);

    /**
     * @brief Read the first length-indicated record and unpack it into
     *        a DataFileHeader.
     *
     * @param in      Open std::ifstream in binary mode, positioned at byte 0.
     * @param header  DataFileHeader reference to populate.
     * @return true on success; false on I/O or unpack error.
     *
     * @pre  in is open in std::ios::binary mode at position 0.
     * @post header is fully populated; stream advances past header record.
     */
    bool readHeader(std::ifstream& in, DataFileHeader& header);
};

#endif // HEADERBUFFER_H