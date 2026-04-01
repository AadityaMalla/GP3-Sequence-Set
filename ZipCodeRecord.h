/**
 * @file ZipCodeRecord.h
 * @brief Data structure for US Postal Code records
 * @author CSCI 331 Team 2
 * @date 03/13/2026
 * @version 2.0
 */

#ifndef ZIPCODERECORD_H
#define ZIPCODERECORD_H

#include <string>
#include <iostream>
#include <iomanip>

/**
 * @struct ZipCodeRecord
 * @brief Structure to hold information for a single ZIP code
 * This structure contains all fields from the US Postal Code database:
 * - ZIP code (5-digit postal code)
 * - Place name (city/town name)
 * - State (2-character state abbreviation)
 * - County name
 * - Geographic coordinates (latitude and longitude)
 * Longitude values are negative for locations west of the Prime Meridian, and latitude values are positive for locations north of the Equator.
 * @pre All fields must be properly initialized before use
 * @post Instances of ZipCodeRecord can be used to store and retrieve ZIP code information
 * @note This structure serves as the fundamental data unit for managing and processing ZIP code information in our application.
 */
struct ZipCodeRecord
{
    int zipCode;
    std::string placeName;
    std::string state;
    std::string county;
    double latitude;
    double longitude;

    /**
     * @brief Default constructor
     * @pre None
     * @post All fields are initialized to default values
     */
    ZipCodeRecord() : zipCode(0), latitude(0.0), longitude(0.0) {}

    /**
     * @brief Parameterized constructor
     * @pre None
     * @post All fields are initialized to the provided values
     */
    ZipCodeRecord(int zip, const std::string &place, const std::string &st,
                  const std::string &cnty, double lat, double lon)
        : zipCode(zip), placeName(place), state(st), county(cnty),
          latitude(lat), longitude(lon) {}

    /**
     * @brief Calculate the packed size of this record
     * @pre None
     * @post Returns the size in bytes when this record is packed for length-indicated format
     * @return Size in bytes when packed for length-indicated format
     */
    size_t getPackedSize() const
    {
        size_t size = 0;
        size += sizeof(int);            // zipCode
        size += placeName.length() + 1; // placeName + null
        size += state.length() + 1;     // state + null
        size += county.length() + 1;    // county + null
        size += sizeof(double);         // latitude
        size += sizeof(double);         // longitude
        return size;
    }

    /**
     * @brief Display record information
     * @pre None
     * @post Record information is printed to the console in a readable format
     * @note This method is useful for debugging and verifying the contents of a ZipCodeRecord instance.
     */
    void display() const
    {
        std::cout << std::setfill(' ');
        std::cout << "  ZIP Code:   " << zipCode << std::endl;
        std::cout << "  Place:      " << placeName << std::endl;
        std::cout << "  State:      " << state << std::endl;
        std::cout << "  County:     " << county << std::endl;
        std::cout << "  Latitude:   " << latitude << std::endl;
        std::cout << "  Longitude:  " << longitude << std::endl;
    }

    /**
     * @brief Display record in a single line
     * @pre None
     * @post Record information is printed to the console in a single line format
     * @note This method is useful for displaying multiple records in a compact format, such as in a list or table.
     */
    void displayInline() const
    {
        std::cout << zipCode << "," << placeName << "," << state << ","
                  << county << "," << latitude << "," << longitude;
    }
};

#endif // ZIPCODERECORD_H