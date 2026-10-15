#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include "csv-reader.hxx"

namespace ribomation::weatherdata {
    namespace alg = boost::algorithm;

    void CSVHeader::init(Fields const& header_fields) {
        first = header_fields[0];
        for (auto idx = 0U; idx < header_fields.size(); ++idx) {
            fields_index[ header_fields[idx] ] = idx;
        }
    }

    unsigned CSVHeader::getIndex(string const& field_name) const {
        auto iter = fields_index.find(field_name);
        if (iter == fields_index.end()) {
            throw std::runtime_error{"missing field: "s + field_name};
        }
        return iter->second;
    }

    CSVReader::CSVReader(istream& input_): input{input_} {
        auto first_line = string{};
        if (not std::getline(input, first_line)) {
            throw std::runtime_error("missing header line");
        }
        alg::trim(first_line);
        auto header_fields = parse(first_line);
        if (header_fields.empty()) {
            throw std::runtime_error("missing header line");
        }
        header.init(header_fields);
    }

    auto CSVReader::parse(string const& csv_line) -> Fields {
        using Separator = boost::escaped_list_separator<char>;
        auto tokens = boost::tokenizer<Separator>(csv_line, Separator("", ",", "\""));

        auto fields = Fields{};
        for (auto const& token : tokens) {
            fields.push_back(alg::trim_copy(token));
        }
        return fields;
    }

    bool CSVReader::next(FieldsMapper& mapper) const {
        for (auto csv = string{}; std::getline(input, csv); ) {
            alg::trim(csv);
            if (csv.empty()) continue;

            auto fields = parse(csv);
            if (header.isHeader(fields)) continue;

            mapper(header, fields);
            return true;
        }
        return false;
    }
}

