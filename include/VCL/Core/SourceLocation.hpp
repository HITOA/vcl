#pragma once

#include <cstdint>


namespace VCL {

    /**
     * Represent a location in a Source.
     * This is used by every class that needs to deal with a Source.
     * It work as an iterator, and simply store a ptr to the Source location.
     * Because it is an iterator, the current char in the source it is locating
     * can also be fetched directly from the source location.
     */
    struct SourceLocation {
    public:
        SourceLocation() = default;
        SourceLocation(uintptr_t id) : id{ id } {}
        SourceLocation(const SourceLocation& location) = default;
        SourceLocation(SourceLocation&& location) = default;
        ~SourceLocation() = default;

        SourceLocation& operator=(const SourceLocation& location) = default;
        SourceLocation& operator=(SourceLocation&& location) = default;

        /** Simply check if the stored ptr is zero or not */
        inline bool IsValid() const { return id != 0; }
        inline bool IsInvalid() const { return id == 0; }

        inline uintptr_t GetID() const { return id; }
        inline const char* GetPtr() const { return (const char*)id; }
        inline char GetChar() const { return *(const char*)id; }

        inline bool operator==(const SourceLocation& location) { 
            return id == location.id;
        }
        inline bool operator!=(const SourceLocation& location) { return !(*this == location); }
        inline bool operator<(const SourceLocation& location) { return id < location.id; }
        inline bool operator>(const SourceLocation& location) { return id > location.id; }
        inline bool operator<=(const SourceLocation& location) { return id <= location.id; }
        inline bool operator>=(const SourceLocation& location) { return id >= location.id; }
        
        inline SourceLocation& operator++() {
            ++id;
            return *this;
        }
        inline SourceLocation& operator--() {
            --id;
            return *this;
        }
        inline SourceLocation operator++(int) {
            SourceLocation loc{ *this };
            ++id;
            return loc;
        }
        inline SourceLocation operator--(int) {
            SourceLocation loc{ *this };
            --id;
            return loc;
        }


    private:
        uintptr_t id;
    };

    /**
     * Represent a range inside a Source with a start and an end SourceLocation.
     */
    struct SourceRange {
        SourceLocation start;
        SourceLocation end;
    };

}