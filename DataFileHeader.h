/**
 * @file DataFileHeader.h
 * @brief Data structure for the header record of a length-indicated ZIP code file
 * @author CSCI 331 Team 2
 * @date 03/13/2026
 *
 * @details This struct holds all metadata about a .li data file.
 *          It knows how to pack/unpack itself into/from a raw byte buffer,
 *          but has NO file I/O — that responsibility belongs to HeaderBuffer.
 *
 *          The header is always stored as the FIRST length-indicated record
 *          inside the .li file (byte 0), using the same 2-byte length prefix
 *          format as every other record.
 */

#ifndef DATAFILEHEADER_H
#define DATAFILEHEADER_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstring>

/**
 * @struct FieldDescriptor
 * @brief Describes a single field in each data record.
 * @pre  name and typeSchema must be non-empty strings.
 * @post Instances can be stored in a DataFileHeader's fields vector.
 */
struct FieldDescriptor
{
    std::string name;        /**< Field name (e.g. "zipCode", "placeName")   */
    std::string typeSchema;  /**< Field type (e.g. "int", "string", "double") */
    bool        isPrimaryKey;/**< True if this field is the primary key       */

    /**
     * @brief Default constructor — isPrimaryKey defaults to false.
     * @pre  None.
     * @post All fields initialised to empty/false.
     */
    FieldDescriptor() : isPrimaryKey(false) {}

    /**
     * @brief Parameterised constructor.
     * @param n   Field name.
     * @param t   Type schema string.
     * @param pk  True if this is the primary key field.
     * @pre  None.
     * @post All members set to supplied values.
     */
    FieldDescriptor(const std::string& n, const std::string& t, bool pk)
        : name(n), typeSchema(t), isPrimaryKey(pk) {}
};


/**
 * @class DataFileHeader
 * @brief Holds metadata about a length-indicated ZIP code data file.
 *
 * @details Responsibilities:
 *          - Store file-level metadata (type, version, record count, etc.)
 *          - Describe each field via a FieldDescriptor vector
 *          - Pack itself into a raw byte buffer   (used by HeaderBuffer to write)
 *          - Unpack itself from a raw byte buffer (used by HeaderBuffer to read)
 *          - Display its contents for debugging
 *
 *          NOT responsible for:
 *          - Opening or closing files  (HeaderBuffer does that)
 *          - Writing or reading from streams (HeaderBuffer does that)
 *
 * @pre  All string fields must not require binary-safe escaping (no embedded nulls).
 * @post An initialised DataFileHeader can be packed into a buffer and written
 *       by HeaderBuffer as the first record of a .li file.
 */
class DataFileHeader
{
public:
    // ── Metadata fields ──────────────────────────────────────────────────────

    std::string fileStructureType;    /**< e.g. "ZIPCODE_V2"                   */
    int         version;              /**< Schema version number               */
    int         headerSize;           /**< Size of the packed header in bytes  */
    int         recordSizeIntegerBytes; /**< Bytes used for the record-length prefix (2) */
    std::string sizeFormatType;       /**< e.g. "binary"                       */
    int         sizeOfSizes;          /**< Bytes per size field                */
    bool        sizeInclusionFlag;    /**< True if size prefix includes itself */
    std::string primaryKeyIndexFileName; /**< Name of the associated .idx file */
    int         recordCount;          /**< Number of data records in the file  */
    int         fieldCount;           /**< Number of fields per data record    */
    int         primaryKeyFieldIndex; /**< Index into fields[] of the PK (-1 if none) */

    std::vector<FieldDescriptor> fields; /**< One entry per data-record field  */

    // ── Constructors ─────────────────────────────────────────────────────────

    /**
     * @brief Default constructor — initialises all fields to safe defaults.
     * @pre  None.
     * @post fileStructureType == "ZIPCODE_V2", version == 2,
     *       recordCount == 0, fieldCount == 0, primaryKeyFieldIndex == -1.
     */
    DataFileHeader();

    /**
     * @brief Parameterised constructor.
     * @param structType  File structure type string.
     * @param ver         Version number.
     * @pre  structType is non-empty; ver > 0.
     * @post Supplied values set; other fields take safe defaults.
     */
    DataFileHeader(const std::string& structType, int ver);

    // ── Field management ─────────────────────────────────────────────────────

    /**
     * @brief Append a field descriptor and update fieldCount.
     * @param name        Field name.
     * @param typeSchema  Type string.
     * @param isPrimaryKey True if this field is the primary key.
     * @pre  name and typeSchema are non-empty.
     * @post Field appended to fields; fieldCount incremented;
     *       primaryKeyFieldIndex updated if isPrimaryKey is true.
     */
    void addField(const std::string& name,
                  const std::string& typeSchema,
                  bool isPrimaryKey = false);

    // ── Serialisation ─────────────────────────────────────────────────────────

    /**
     * @brief Calculate the number of bytes needed to pack this header.
     * @pre  None.
     * @post No state change.
     * @return Required buffer size in bytes.
     */
    size_t getPackedSize() const;

    /**
     * @brief Serialise this header into a raw byte buffer.
     * @param buffer  Caller-allocated buffer of at least getPackedSize() bytes.
     * @pre  buffer points to valid memory of at least getPackedSize() bytes.
     * @post Header data written to buffer in binary format.
     * @return Number of bytes written.
     */
    size_t pack(char* buffer) const;

    /**
     * @brief Deserialise this header from a raw byte buffer.
     * @param buffer  Buffer containing a previously packed header.
     * @pre  buffer contains valid packed header data (produced by pack()).
     * @post All fields populated from buffer; fields vector rebuilt.
     * @return Number of bytes consumed.
     */
    size_t unpack(const char* buffer);

    // ── Display ───────────────────────────────────────────────────────────────

    /**
     * @brief Print all header fields to std::cout in a formatted layout.
     * @pre  None.
     * @post Contents printed to standard output; no state change.
     */
    void display() const;
};

#endif // DATAFILEHEADER_H
