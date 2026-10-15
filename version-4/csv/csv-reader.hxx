#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <istream>
#include <functional>
#include <boost/algorithm/string/trim.hpp>

namespace ribomation::weatherdata {
    using namespace std::string_literals;
    namespace alg = boost::algorithm;
    using std::string;
    using std::vector;
    using std::unordered_map;
    using std::istream;
    using std::function;

    using Fields = vector<string>;
    using FieldsIndex = unordered_map<string, unsigned>;

    struct CSVHeader {
        string  first{};
        FieldsIndex fields_index{};

        void init(Fields const& header_fields);
        unsigned getIndex(string const& field_name) const;
        bool isHeader(Fields const& fields) const { return fields[0] == first; }
    };

    struct FieldsMapper {
        virtual ~FieldsMapper() = default;
        virtual void operator()(CSVHeader const&, Fields const&) = 0;
    };

    struct CSVReader {
        istream& input;
        CSVHeader header{};

        explicit CSVReader(istream& input_);
        bool next(FieldsMapper& mapper) const;
        static auto parse(string const& csv_line) -> Fields;
    };

}

