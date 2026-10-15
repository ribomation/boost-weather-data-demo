#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "csv-reader.hxx"

using ribomation::weatherdata::CSVReader;
using ribomation::weatherdata::CSVHeader;
using ribomation::weatherdata::FieldsMapper;
using ribomation::weatherdata::Fields;

namespace {
    struct Collector : FieldsMapper {
        std::vector<Fields> rows{};
        std::vector<unsigned> dates{};

        void operator()(CSVHeader const& header, Fields const& fields) override {
            rows.push_back(fields);
            dates.push_back(header.getIndex("DATE"));
        }
    };
}

TEST_CASE("csv-reader") {
    SECTION("parse") {
        SECTION("plain fields") {
            CHECK(CSVReader::parse("a,b,c") == Fields{"a", "b", "c"});
        }

        SECTION("single field") {
            CHECK(CSVReader::parse("abc") == Fields{"abc"});
        }

        SECTION("empty line gives no fields") {
            CHECK(CSVReader::parse("").empty());
        }

        SECTION("empty fields are kept") {
            CHECK(CSVReader::parse("a,,b") == Fields{"a", "", "b"});
            CHECK(CSVReader::parse(",a") == Fields{"", "a"});
            CHECK(CSVReader::parse("a,") == Fields{"a", ""});
        }

        SECTION("quotes are removed") {
            CHECK(CSVReader::parse(R"("a","b")") == Fields{"a", "b"});
        }

        SECTION("comma inside quotes is not a separator") {
            CHECK(CSVReader::parse(R"("x, y",z)") == Fields{"x, y", "z"});
        }

        SECTION("fields are trimmed, also inside quotes") {
            CHECK(CSVReader::parse(R"(  a ,  " b " )") == Fields{"a", "b"});
        }

        SECTION("backslash is not an escape character") {
            CHECK(CSVReader::parse(R"(a\b,c)") == Fields{R"(a\b)", "c"});
        }

        SECTION("GSOD header line") {
            auto fields = CSVReader::parse(
                R"("STATION","DATE","LATITUDE","LONGITUDE","ELEVATION","NAME","TEMP","TEMP_ATTRIBUTES",)"
                R"("DEWP","DEWP_ATTRIBUTES","SLP","SLP_ATTRIBUTES","STP","STP_ATTRIBUTES","VISIB",)"
                R"("VISIB_ATTRIBUTES","WDSP","WDSP_ATTRIBUTES","MXSPD","GUST","MAX","MAX_ATTRIBUTES",)"
                R"("MIN","MIN_ATTRIBUTES","PRCP","PRCP_ATTRIBUTES","SNDP","FRSHTT")");
            REQUIRE(fields.size() == 28);
            CHECK(fields.front() == "STATION");
            CHECK(fields[5] == "NAME");
            CHECK(fields.back() == "FRSHTT");
        }

        SECTION("GSOD data line") {
            auto fields = CSVReader::parse(
                R"("99999913762","1945-03-01","36.695","-76.13556","4.9",)"
                R"("FENTRESS NAVAL AUXILIARY FIELD, VA US","  44.3","11","  28.8","11","1031.2","11",)"
                R"("031.2","11"," 31.1","11","  8.7","11"," 12.0","999.9","  48.0","*","  37.9","*",)"
                R"(" 0.00","I","999.9","000000")");
            REQUIRE(fields.size() == 28);
            CHECK(fields[0] == "99999913762");
            CHECK(fields[1] == "1945-03-01");
            CHECK(fields[5] == "FENTRESS NAVAL AUXILIARY FIELD, VA US");
            CHECK(fields[6] == "44.3");     // TEMP, leading blanks trimmed
            CHECK(fields[20] == "48.0");    // MAX
            CHECK(fields[22] == "37.9");    // MIN
            CHECK(fields[24] == "0.00");    // PRCP
            CHECK(fields[27] == "000000");  // FRSHTT
        }
    }

    SECTION("CSVHeader") {
        auto header = CSVHeader{};
        header.init(Fields{"STATION", "DATE", "TEMP"});

        SECTION("first field") {
            CHECK(header.first == "STATION");
        }

        SECTION("getIndex") {
            CHECK(header.getIndex("STATION") == 0);
            CHECK(header.getIndex("DATE") == 1);
            CHECK(header.getIndex("TEMP") == 2);
        }

        SECTION("getIndex of missing field") {
            CHECK_THROWS_WITH(header.getIndex("PRCP"), "missing field: PRCP");
        }

        SECTION("isHeader") {
            CHECK(header.isHeader(Fields{"STATION", "DATE", "TEMP"}));
            CHECK_FALSE(header.isHeader(Fields{"01001099999", "1945-01-01", "22.6"}));
        }
    }

    SECTION("CSVReader") {
        SECTION("header is read by the constructor") {
            auto input = std::istringstream{"\"STATION\",\"DATE\",\"TEMP\"\n"};
            auto reader = CSVReader{input};
            CHECK(reader.header.first == "STATION");
            CHECK(reader.header.getIndex("TEMP") == 2);
        }

        SECTION("empty input has no header") {
            auto input = std::istringstream{""};
            CHECK_THROWS_WITH(CSVReader{input}, "missing header line");
        }

        SECTION("blank first line has no header") {
            auto input = std::istringstream{"\nSTATION,DATE,TEMP\n"};
            CHECK_THROWS_WITH(CSVReader{input}, "missing header line");
        }

        SECTION("whitespace-only first line has no header") {
            auto input = std::istringstream{"  \t \r\nSTATION,DATE,TEMP\n"};
            CHECK_THROWS_WITH(CSVReader{input}, "missing header line");
        }

        SECTION("header only gives no rows") {
            auto input = std::istringstream{"STATION,DATE,TEMP\n"};
            auto reader = CSVReader{input};
            auto mapper = Collector{};
            CHECK_FALSE(reader.next(mapper));
            CHECK(mapper.rows.empty());
        }

        SECTION("next reads one row at a time") {
            auto input = std::istringstream{
                "STATION,DATE,TEMP\n"
                "S1,1945-01-01,22.6\n"
                "S1,1945-01-02,23.1\n"
            };
            auto reader = CSVReader{input};
            auto mapper = Collector{};

            REQUIRE(reader.next(mapper));
            REQUIRE(mapper.rows.size() == 1);
            CHECK(mapper.rows[0] == Fields{"S1", "1945-01-01", "22.6"});

            REQUIRE(reader.next(mapper));
            REQUIRE(mapper.rows.size() == 2);
            CHECK(mapper.rows[1] == Fields{"S1", "1945-01-02", "23.1"});

            CHECK_FALSE(reader.next(mapper));
            CHECK(mapper.rows.size() == 2);
        }

        SECTION("mapper gets the header") {
            auto input = std::istringstream{"STATION,DATE,TEMP\nS1,1945-01-01,22.6\n"};
            auto reader = CSVReader{input};
            auto mapper = Collector{};
            REQUIRE(reader.next(mapper));
            CHECK(mapper.dates == std::vector<unsigned>{1});
        }

        SECTION("blank lines are skipped") {
            auto input = std::istringstream{
                "STATION,DATE,TEMP\n"
                "\n"
                "   \t \n"
                "S1,1945-01-01,22.6\n"
                "\n"
            };
            auto reader = CSVReader{input};
            auto mapper = Collector{};
            CHECK(reader.next(mapper));
            CHECK_FALSE(reader.next(mapper));
            CHECK(mapper.rows == std::vector<Fields>{{"S1", "1945-01-01", "22.6"}});
        }

        SECTION("repeated header lines are skipped") {
            auto input = std::istringstream{
                "STATION,DATE,TEMP\n"
                "S1,1945-01-01,22.6\n"
                "STATION,DATE,TEMP\n"
                "S2,1945-01-01,18.4\n"
            };
            auto reader = CSVReader{input};
            auto mapper = Collector{};
            while (reader.next(mapper)) {}
            REQUIRE(mapper.rows.size() == 2);
            CHECK(mapper.rows[0][0] == "S1");
            CHECK(mapper.rows[1][0] == "S2");
        }

        SECTION("CRLF line endings") {
            auto input = std::istringstream{"STATION,DATE,TEMP\r\nS1,1945-01-01,22.6\r\n"};
            auto reader = CSVReader{input};
            CHECK(reader.header.getIndex("TEMP") == 2);
            auto mapper = Collector{};
            REQUIRE(reader.next(mapper));
            CHECK(mapper.rows[0] == Fields{"S1", "1945-01-01", "22.6"});
        }
    }
}
