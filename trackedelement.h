/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __TRACKEDELEMENT_H__
#define __TRACKEDELEMENT_H__

#include "config.h"

#include <stdio.h>
#include <stdint.h>

#include <string>
#include <stdexcept>

#include <vector>
#include <map>

#include "macaddr.h"
#include "uuid.h"

// Types of fields we can track and automatically resolve
// Statically assigned type numbers which MUST NOT CHANGE as things go forwards for binary/fast
// serialization, new types must be added to the end of the list
enum TrackerType {
    TrackerString = 0,

    TrackerInt8 = 1, 
    TrackerUInt8 = 2,

    TrackerInt16 = 3, 
    TrackerUInt16 = 4,

    TrackerInt32 = 5, 
    TrackerUInt32 = 6,

    TrackerInt64 = 7,
    TrackerUInt64 = 8,

    TrackerFloat = 9,
    TrackerDouble = 10,

    // Less basic types
    TrackerMac = 11, 
    TrackerUuid = 12,

    // Vector and named map
    TrackerVector = 13, 
    TrackerMap = 14,

    // unsigned integer map (int-keyed data not field-keyed)
    TrackerIntMap = 15,

    TrackerCustom = 16,
};

class TrackerElement {
public:
    TrackerElement() {
        this->type = TrackerCustom;
        reference_count = 0;
    }

    TrackerElement(TrackerType type);
    TrackerElement(TrackerType type, int id);

    virtual ~TrackerElement();

    // Factory-style for easily making more of the same if we're subclassed
    virtual TrackerElement *clone() {
        return new TrackerElement(get_type(), get_id());
    }

    virtual TrackerElement *clone(int in_id) {
        TrackerElement *dupl = clone();
        dupl->set_id(in_id);

        return dupl;
    }

    int get_id() {
        return tracked_id;
    }

    void set_id(int id) {
        tracked_id = id;
    }

    void link() {
        reference_count++;
    }

    void unlink() {
        reference_count--;

        // what?
        if (reference_count < 0) {
            throw std::runtime_error("tracker element link count < 0");
        }

        // Time to go
        if (reference_count == 0) {
            delete(this);
#if 1
            fprintf(stderr, "debug - element %p id %u hit reference count 0, deleting\n", this, get_id());
#endif
        }
    }

    int get_links() {
        return reference_count;
    }

    void set_type(TrackerType type) {
        this->type = type;
    }

    TrackerType get_type() { return type; }

    // Getter per type, use templated GetTrackerValue() for easy fetch
    string get_string() {
        except_type_mismatch(TrackerString);
        return string_value;
    }

    uint8_t get_uint8() {
        except_type_mismatch(TrackerUInt8);
        return uint8_value;
    }

    int8_t get_int8() {
        except_type_mismatch(TrackerInt8);
        return int8_value;
    }

    uint16_t get_uint16() {
        except_type_mismatch(TrackerUInt16);
        return uint16_value;
    }

    int16_t get_int16() {
        except_type_mismatch(TrackerInt16);
        return int16_value;
    }

    uint32_t get_uint32() {
        except_type_mismatch(TrackerUInt32);
        return uint32_value;
    }

    int32_t get_int32() {
        except_type_mismatch(TrackerInt32);
        return int32_value;
    }

    uint64_t get_uint64() {
        except_type_mismatch(TrackerUInt64);
        return uint64_value;
    }

    int64_t get_int64() {
        except_type_mismatch(TrackerInt64);
        return int64_value;
    }

    float get_float() {
        except_type_mismatch(TrackerFloat);
        return float_value;
    }

    double get_double() {
        except_type_mismatch(TrackerDouble);
        return double_value;
    }

    mac_addr get_mac() {
        except_type_mismatch(TrackerMac);
        return mac_value;
    }

    vector<TrackerElement *> *get_vector() {
        except_type_mismatch(TrackerVector);
        return &subvector_value;
    }

    map<int, TrackerElement *> *get_map() {
        except_type_mismatch(TrackerMap);
        return &submap_value;
    }

    TrackerElement *get_map_value(int fn) {
        except_type_mismatch(TrackerMap);

        map<int, TrackerElement *>::iterator i = submap_value.find(fn);

        if (i == submap_value.end()) {
            return NULL;
        }

        return i->second;
    }

    map<int, TrackerElement *> *get_intmap() {
        except_type_mismatch(TrackerIntMap);
        return &subintmap_value;
    }

    TrackerElement *get_intmap_value(int idx) {
        except_type_mismatch(TrackerIntMap);

        map<int, TrackerElement *>::iterator i = subintmap_value.find(idx);

        if (i == submap_value.end()) {
            return NULL;
        }

        return i->second;
    }

    uuid get_uuid() {
        except_type_mismatch(TrackerUuid);
        return uuid_value;
    }

    // Overloaded set
    void set(string v) {
        except_type_mismatch(TrackerString);
        string_value = v;
    }

    void set(uint8_t v) {
        except_type_mismatch(TrackerUInt8);
        uint8_value = v;
    }

    void set(int8_t v) {
        except_type_mismatch(TrackerInt8);
        int8_value = v;
    }

    void set(uint16_t v) {
        except_type_mismatch(TrackerUInt16);
        uint16_value = v;
    }

    void set(int16_t v) {
        except_type_mismatch(TrackerInt16);
        int16_value = v;
    }

    void set(uint32_t v) {
        except_type_mismatch(TrackerUInt32);
        uint32_value = v;
    }

    void set(int32_t v) {
        except_type_mismatch(TrackerInt32);
        int32_value = v;
    }

    void set(uint64_t v) {
        except_type_mismatch(TrackerUInt64);
        uint64_value = v;
    }

    void set(int64_t v) {
        except_type_mismatch(TrackerInt64);
        int64_value = v;
    }

    void set(float v) {
        except_type_mismatch(TrackerFloat);
        float_value = v;
    }

    void set(double v) {
        except_type_mismatch(TrackerDouble);
        double_value = v;
    }

    void set(mac_addr v) {
        except_type_mismatch(TrackerMac);
        mac_value = v;
    }

    void set(uuid v) {
        except_type_mismatch(TrackerUuid);
        uuid_value = v;
    }

    void add_map(int f, TrackerElement *s);
    void add_map(TrackerElement *s); 
    void del_map(int f);
    void del_map(TrackerElement *s);

    void add_intmap(int i, TrackerElement *s);
    void del_intmap(int i);

    void add_vector(TrackerElement *s);
    void del_vector(unsigned int p);

    size_t size();

    // Do our best to increment a value
    TrackerElement& operator++(int);

    // Do our best to decrement a value
    TrackerElement& operator--(int);

    // Do our best to do compound addition
    TrackerElement& operator+=(const int& v);
    TrackerElement& operator+=(const unsigned int& v);
    TrackerElement& operator+=(const float& v);
    TrackerElement& operator+=(const double& v);

    TrackerElement& operator+=(const int64_t& v);
    TrackerElement& operator+=(const uint64_t& v);

    // We can append to vectors
    TrackerElement& operator+=(TrackerElement* v);

    // Do our best to do compound subtraction
    TrackerElement& operator-=(const int& v);
    TrackerElement& operator-=(const unsigned int& v);
    TrackerElement& operator-=(const float& v);
    TrackerElement& operator-=(const double& v);

    TrackerElement& operator-=(const int64_t& v);
    TrackerElement& operator-=(const uint64_t& v);

    // Do our best for equals comparison
    
    // Comparing tracked elements themselves presents weird problems - how do we deal with 
    // conflicting ids but equal data?  Lets see if we actually need it.  /D
    // friend bool operator==(TrackerElement &te1, TrackerElement &te2);

    friend bool operator==(TrackerElement &te1, int8_t i);
    friend bool operator==(TrackerElement &te1, uint8_t i);
    friend bool operator==(TrackerElement &te1, int16_t i);
    friend bool operator==(TrackerElement &te1, uint16_t i);
    friend bool operator==(TrackerElement &te1, int32_t i);
    friend bool operator==(TrackerElement &te1, uint32_t i);
    friend bool operator==(TrackerElement &te1, int64_t i);
    friend bool operator==(TrackerElement &te1, uint64_t i);
    friend bool operator==(TrackerElement &te1, float f);
    friend bool operator==(TrackerElement &te1, double d);
    friend bool operator==(TrackerElement &te1, mac_addr m);
    friend bool operator==(TrackerElement &te1, uuid u);

    friend bool operator<(TrackerElement &te1, int8_t i);
    friend bool operator<(TrackerElement &te1, uint8_t i);
    friend bool operator<(TrackerElement &te1, int16_t i);
    friend bool operator<(TrackerElement &te1, uint16_t i);
    friend bool operator<(TrackerElement &te1, int32_t i);
    friend bool operator<(TrackerElement &te1, uint32_t i);
    friend bool operator<(TrackerElement &te1, int64_t i);
    friend bool operator<(TrackerElement &te1, uint64_t i);
    friend bool operator<(TrackerElement &te1, float f);
    friend bool operator<(TrackerElement &te1, double d);
    friend bool operator<(TrackerElement &te1, mac_addr m);
    friend bool operator<(TrackerElement &te1, uuid u);

    friend bool operator>(TrackerElement &te1, int8_t i);
    friend bool operator>(TrackerElement &te1, uint8_t i);
    friend bool operator>(TrackerElement &te1, int16_t i);
    friend bool operator>(TrackerElement &te1, uint16_t i);
    friend bool operator>(TrackerElement &te1, int32_t i);
    friend bool operator>(TrackerElement &te1, uint32_t i);
    friend bool operator>(TrackerElement &te1, int64_t i);
    friend bool operator>(TrackerElement &te1, uint64_t i);
    friend bool operator>(TrackerElement &te1, float f);
    friend bool operator>(TrackerElement &te1, double d);
    // We don't have > operators on mac or uuid
   
    // Bitwise
    TrackerElement& operator|=(const int8_t i);
    TrackerElement& operator|=(const uint8_t i);
    TrackerElement& operator|=(const int16_t i);
    TrackerElement& operator|=(const uint16_t i);
    TrackerElement& operator|=(const int32_t i);
    TrackerElement& operator|=(const uint32_t i);
    TrackerElement& operator|=(const int64_t i);
    TrackerElement& operator|=(const uint64_t i);

    TrackerElement& operator&=(const int8_t i);
    TrackerElement& operator&=(const uint8_t i);
    TrackerElement& operator&=(const int16_t i);
    TrackerElement& operator&=(const uint16_t i);
    TrackerElement& operator&=(const int32_t i);
    TrackerElement& operator&=(const uint32_t i);
    TrackerElement& operator&=(const int64_t i);
    TrackerElement& operator&=(const uint64_t i);

    TrackerElement& operator^=(const int8_t i);
    TrackerElement& operator^=(const uint8_t i);
    TrackerElement& operator^=(const int16_t i);
    TrackerElement& operator^=(const uint16_t i);
    TrackerElement& operator^=(const int32_t i);
    TrackerElement& operator^=(const uint32_t i);
    TrackerElement& operator^=(const int64_t i);
    TrackerElement& operator^=(const uint64_t i);

    TrackerElement *operator[](const int i);

    typedef map<int, TrackerElement *>::iterator map_iterator;
    typedef map<int, TrackerElement *>::const_iterator map_const_iterator;

    map_const_iterator begin();
    map_const_iterator end();
    map_iterator find(int k);

    static string type_to_string(TrackerType t);

protected:
    // Generic coercion exception
    void except_type_mismatch(TrackerType t) {
        if (type != t) {
            string w = "element type mismatch, is " + type_to_string(this->type) + 
                " tried to use as " + type_to_string(t);

            throw std::runtime_error(w);
        }
    }

    // Garbage collection?  Say it ain't so...
    int reference_count;

    TrackerType type;
    int tracked_id;

    string string_value;

    // We could make these all one type, but then we'd have odd interactions
    // with incrementing and I'm not positive that's safe in all cases
    uint8_t uint8_value;
    int8_t int8_value;

    uint16_t uint16_value;
    int16_t int16_value;

    uint32_t uint32_value;
    int32_t int32_value;

    uint64_t uint64_value;
    int64_t int64_value;

    float float_value;
    double double_value;

    mac_addr mac_value;

    // Field ID,Element keyed map
    map<int, TrackerElement *> submap_value;
    // Index int,Element keyed map
    map<int, TrackerElement *> subintmap_value;
    vector<TrackerElement *> subvector_value;

    uuid uuid_value;

    void *custom_value;
};

// Templated access functions

template<typename T> T GetTrackerValue(TrackerElement *);

template<> string GetTrackerValue(TrackerElement *e);
template<> int8_t GetTrackerValue(TrackerElement *e);
template<> uint8_t GetTrackerValue(TrackerElement *e);
template<> int16_t GetTrackerValue(TrackerElement *e);
template<> uint16_t GetTrackerValue(TrackerElement *e);
template<> int32_t GetTrackerValue(TrackerElement *e);
template<> uint32_t GetTrackerValue(TrackerElement *e);
template<> int64_t GetTrackerValue(TrackerElement *e);
template<> uint64_t GetTrackerValue(TrackerElement *e);
template<> float GetTrackerValue(TrackerElement *e);
template<> double GetTrackerValue(TrackerElement *e);
template<> mac_addr GetTrackerValue(TrackerElement *e);
template<> map<int, TrackerElement *> *GetTrackerValue(TrackerElement *e);
template<> vector<TrackerElement *> *GetTrackerValue(TrackerElement *e);

class TrackerElementFormatter {
public:
    virtual void get_as_stream(TrackerElement *e, ostream& stream) = 0;
    virtual void vector_to_stream(TrackerElement *e, ostream& stream) = 0;
    virtual void map_to_stream(TrackerElement *e, ostream& stream) = 0;
};

class TrackerElementFormatterBasic : public TrackerElementFormatter {
public:
    virtual void get_as_stream(TrackerElement *e, ostream& stream) {
        switch (e->get_type()) {
            case TrackerString:
                stream << GetTrackerValue<string>(e);
            case TrackerInt8:
                stream << GetTrackerValue<int8_t>(e);
                break;
            case TrackerUInt8:
                stream << GetTrackerValue<uint8_t>(e);
                break;
            case TrackerInt16:
                stream << GetTrackerValue<int16_t>(e);
                break;
            case TrackerUInt16:
                stream << GetTrackerValue<uint16_t>(e);
                break;
            case TrackerInt32:
                stream << GetTrackerValue<int32_t>(e);
                break;
            case TrackerUInt32:
                stream << GetTrackerValue<uint32_t>(e);
                break;
            case TrackerInt64:
                stream << GetTrackerValue<int64_t>(e);
                break;
            case TrackerUInt64:
                stream << GetTrackerValue<uint16_t>(e);
                break;
            case TrackerFloat:
                stream << GetTrackerValue<float>(e);
                break;
            case TrackerDouble:
                stream << GetTrackerValue<double>(e);
                break;
            case TrackerMac:
                stream << GetTrackerValue<mac_addr>(e).Mac2String();
                break;
            case TrackerVector:
                vector_to_stream(e, stream);
                break;
            case TrackerMap:
                map_to_stream(e, stream);
                break;
            case TrackerCustom:
                throw std::runtime_error("can't stream a custom");
            default:
                throw std::runtime_error("can't stream unknown");
        }

    }

    virtual void vector_to_stream(TrackerElement *e, ostream& stream) {
        unsigned int x;

        stream << "vector[";

        vector<TrackerElement *> *vec = GetTrackerValue<vector<TrackerElement *>*>(e);

        for (x = 0; x < vec->size(); x++) {
            get_as_stream((*vec)[x], stream);
            stream << ",";
        }

        stream << "]";
    }

    virtual void map_to_stream(TrackerElement *e, ostream& stream) {
        map<int, TrackerElement *>::iterator i;

        stream << "map{";

        map<int, TrackerElement *> *smap = GetTrackerValue<map<int, TrackerElement *>*>(e);

        for (i = smap->begin(); i != smap->end(); ++i) {
            stream << "[" << i->first << ",";
            get_as_stream(i->second, stream);
            stream << "],";
        }

        stream << "}";
    }


};

#endif