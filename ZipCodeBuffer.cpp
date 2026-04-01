/**
 * @file ZipCodeBuffer.cpp
 * @brief Implementation of the ZipCodeBuffer class
 * @author CSCI 331 Team 2
 * @date 03/13/2026
 *
 * @details Supports column re-ordering by reading the header row on first
 *          open and mapping column names to field indices. Data rows are
 *          then parsed using those indices regardless of column order.
 *
 *          Supported header column names (case-insensitive):
 *            zip, zip code, zipcode     -> zipCode
 *            place, place name          -> placeName
 *            state                      -> state
 *            county                     -> county
 *            lat, latitude              -> latitude
 *            long, longitude            -> longitude
 */

#include "ZipCodeBuffer.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// ── Constructor / Destructor ──────────────────────────────────────────────────

/**
 * @brief Constructor — initialises all state; does not open file.
 * @param csvFilename  Path to the CSV file.
 * @pre  None.
 * @post Object ready; file not open; column map empty.
 */
ZipCodeBuffer::ZipCodeBuffer(const std::string& csvFilename)
    : filename(csvFilename)
    , fileIsOpen(false)
    , headerSkipped(false)
    , recordCount(0)
    , colZip(-1), colPlace(-1), colState(-1)
    , colCounty(-1), colLat(-1), colLon(-1)
{
}

/**
 * @brief Destructor — closes file if open.
 * @pre  None.
 * @post File stream closed.
 */
ZipCodeBuffer::~ZipCodeBuffer()
{
    close();
}

// ── File lifecycle ────────────────────────────────────────────────────────────

/**
 * @brief Open the CSV file and parse the header row to build the column map.
 * @return true on success; false if file cannot be opened or header is missing
 *         required columns.
 * @pre  None.
 * @post File open; column indices set; ready for getNextRecord().
 */
bool ZipCodeBuffer::open()
{
    close();
    inputFile.open(filename);
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        fileIsOpen = false;
        return false;
    }

    fileIsOpen    = true;
    headerSkipped = false;
    recordCount   = 0;

    // Reset column indices
    colZip = colPlace = colState = colCounty = colLat = colLon = -1;

    // Strip UTF-8 BOM (EF BB BF) if present — Excel-generated CSVs often have it
    // Without this the first field becomes "\xEF\xBB\xBFZip Code" and never matches
    char bom[3] = {0};
    inputFile.read(bom, 3);
    if ((unsigned char)bom[0] != 0xEF ||
        (unsigned char)bom[1] != 0xBB ||
        (unsigned char)bom[2] != 0xBF)
    {
        // Not a BOM — rewind to the beginning
        inputFile.seekg(0, std::ios::beg);
    }
    // If it IS a BOM we just skip those 3 bytes and continue normally

    // Read the header row — may span multiple physical lines if quoted
    // fields contain embedded newlines (e.g. "Zip\nCode", "Place\nName")
    std::string headerLine;
    while (std::getline(inputFile, headerLine))
    {
        // Skip blank lines
        if (headerLine.empty() ||
            headerLine.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        // Keep appending lines until quotes are balanced
        // Count unescaped quote characters
        while (true)
        {
            int quoteCount = 0;
            bool inEscape  = false;
            for (size_t i = 0; i < headerLine.size(); ++i)
            {
                if (headerLine[i] == '"')
                {
                    if (!inEscape) quoteCount++;
                    inEscape = false;
                }
                else inEscape = false;
            }
            // Odd number of quotes means we are still inside a quoted field
            if (quoteCount % 2 == 0) break;

            std::string nextLine;
            if (!std::getline(inputFile, nextLine)) break;
            // Replace the embedded newline with a space for matching purposes
            headerLine += " " + nextLine;
        }

        if (parseHeaderRow(headerLine))
        {
            headerSkipped = true;
            break;
        }
    }

    if (!headerSkipped)
    {
        std::cerr << "ZipCodeBuffer: could not parse header row in '"
                  << filename << "'\n";
        close();
        return false;
    }

    // Warn about any missing required columns
    if (colZip   < 0) std::cerr << "Warning: no ZIP column found\n";
    if (colPlace < 0) std::cerr << "Warning: no Place Name column found\n";
    if (colState < 0) std::cerr << "Warning: no State column found\n";
    if (colLat   < 0) std::cerr << "Warning: no Latitude column found\n";
    if (colLon   < 0) std::cerr << "Warning: no Longitude column found\n";

    return true;
}

/**
 * @brief Close the CSV file.
 * @pre  None.
 * @post File stream closed; fileIsOpen == false.
 */
void ZipCodeBuffer::close()
{
    if (fileIsOpen && inputFile.is_open())
        inputFile.close();
    fileIsOpen = false;
}

// ── Private helpers ───────────────────────────────────────────────────────────

/**
 * @brief Normalise a string to lowercase with no leading/trailing whitespace.
 * @param s  Input string.
 * @return   Normalised string.
 * @pre  None.
 * @post No state change.
 */
std::string ZipCodeBuffer::normalise(const std::string& s) const
{
    std::string out = trim(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return out;
}

/**
 * @brief Parse the header row and set column index members.
 * @param line  The raw header line from the CSV.
 * @return true if at least the ZIP column was identified; false otherwise.
 * @pre  None.
 * @post colZip, colPlace, colState, colCounty, colLat, colLon set where found.
 *
 * @details Handles multi-line column names (e.g. "Zip\nCode") by treating
 *          newlines within quoted fields as spaces before matching.
 */
bool ZipCodeBuffer::parseHeaderRow(const std::string& line)
{
    size_t pos = 0;
    int col = 0;

    while (pos <= line.length())
    {
        std::string field = extractField(line, pos);

        // Replace embedded newlines with space (handles "Zip\nCode" style)
        for (char& c : field)
            if (c == '\n' || c == '\r') c = ' ';

        // Collapse multiple spaces
        std::string norm = normalise(field);
        // Remove all internal spaces for matching
        std::string compact;
        for (char c : norm)
            if (c != ' ') compact += c;



        // Match zip column — handles "zip", "zipcode", "zip code", "zip code" (joined lines)
        if (compact == "zip"      ||
            compact == "zipcode"  ||
            compact == "zipcode"  ||
            norm    == "zip"      ||
            norm    == "zip code")
            colZip = col;
        // Match place column — handles "place", "placename", "place name"
        else if (compact == "place"     ||
                 compact == "placename" ||
                 norm    == "place name")
            colPlace = col;
        else if (norm == "state")
            colState = col;
        else if (norm == "county")
            colCounty = col;
        else if (norm == "lat" || norm == "latitude")
            colLat = col;
        else if (norm == "long" || norm == "longitude")
            colLon = col;

        col++;

        // Stop when we've consumed the whole line
        if (pos >= line.length()) break;
    }

    return (colZip >= 0);
}

/**
 * @brief Parse a data row into a ZipCodeRecord using the column map.
 * @param line    A CSV data line.
 * @param record  Output: populated on success.
 * @return true on success; false on parse error.
 * @pre  parseHeaderRow() must have been called.
 * @post record populated with values from the correct columns.
 */
bool ZipCodeBuffer::parseCsvLine(const std::string& line, ZipCodeRecord& record)
{
    // Extract all fields from this line
    std::vector<std::string> fields;
    size_t pos = 0;
    while (pos <= line.length())
    {
        fields.push_back(extractField(line, pos));
        if (pos >= line.length()) break;
    }

    try
    {
        // Use column indices set by parseHeaderRow()
        if (colZip   >= 0 && colZip   < (int)fields.size())
            record.zipCode = std::stoi(fields[colZip]);

        if (colPlace >= 0 && colPlace < (int)fields.size())
            record.placeName = fields[colPlace];

        if (colState >= 0 && colState < (int)fields.size())
            record.state = fields[colState];

        if (colCounty >= 0 && colCounty < (int)fields.size())
            record.county = fields[colCounty];

        if (colLat  >= 0 && colLat  < (int)fields.size())
            record.latitude = std::stod(fields[colLat]);

        if (colLon  >= 0 && colLon  < (int)fields.size())
            record.longitude = std::stod(fields[colLon]);

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Parse error: " << e.what() << "\n";
        return false;
    }
}

/**
 * @brief Extract a single field from a CSV line, handling quoted fields.
 * @param line  The CSV line.
 * @param pos   Current position; updated to point past the field separator.
 * @return The extracted and trimmed field string.
 * @pre  pos is within bounds of line.
 * @post pos advances past the comma (or to end of line).
 */
std::string ZipCodeBuffer::extractField(const std::string& line, size_t& pos)
{
    std::string field;
    bool inQuotes = false;

    // Skip leading whitespace (but not inside quotes)
    while (pos < line.length() &&
           (line[pos] == ' ' || line[pos] == '\t'))
        pos++;

    // Check for opening quote
    if (pos < line.length() && line[pos] == '"')
    {
        inQuotes = true;
        pos++;
    }

    while (pos < line.length())
    {
        char c = line[pos];

        if (inQuotes)
        {
            if (c == '"')
            {
                // Escaped quote ""
                if (pos + 1 < line.length() && line[pos + 1] == '"')
                {
                    field += '"';
                    pos += 2;
                }
                else
                {
                    pos++;          // closing quote
                    inQuotes = false;
                    // consume the comma separator that follows
                    if (pos < line.length() && line[pos] == ',')
                        pos++;
                    break;
                }
            }
            else
            {
                field += c;
                pos++;
            }
        }
        else
        {
            if (c == ',')
            {
                pos++;  // consume separator
                break;
            }
            field += c;
            pos++;
        }
    }

    return trim(field);
}

/**
 * @brief Trim leading and trailing whitespace from a string.
 * @param str  Input string.
 * @return Trimmed string.
 * @pre  None.
 * @post No state change.
 */
std::string ZipCodeBuffer::trim(const std::string& str) const
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last  = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

// ── Read interface ────────────────────────────────────────────────────────────

/**
 * @brief Read the next data record from the CSV file.
 * @param record  Output: populated on success.
 * @return true on success; false at EOF or on error.
 * @pre  open() returned true.
 * @post record populated; recordCount incremented; stream advances.
 */
bool ZipCodeBuffer::getNextRecord(ZipCodeRecord& record)
{
    if (!fileIsOpen || !inputFile.is_open())
        return false;

    std::string line;
    while (std::getline(inputFile, line))
    {
        // Skip empty lines
        if (line.empty() ||
            line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        if (parseCsvLine(line, record))
        {
            recordCount++;
            return true;
        }
        else
        {
            std::cerr << "Warning: Could not parse line "
                      << (recordCount + 1) << ": "
                      << line.substr(0, 50) << "...\n";
        }
    }
    return false;
}

// ── Accessors ─────────────────────────────────────────────────────────────────

/** @return true if file is open. */
bool ZipCodeBuffer::isOpen()         const { return fileIsOpen; }

/** @return Number of records successfully read. */
long ZipCodeBuffer::getRecordCount() const { return recordCount; }

/** @return The filename. */
std::string ZipCodeBuffer::getFilename() const { return filename; }

/**
 * @brief Close and reopen the file.
 * @return true if reopened successfully.
 */
bool ZipCodeBuffer::reset()
{
    close();
    return open();
}