#pragma once
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
	
#include "CLP.h"

namespace macoro
{
    class TestCollection
    {
    public:
        struct Test
        {
            std::string mName;
            std::function<void(const CLP&)> mTest;
        };
        TestCollection() = default;
        TestCollection(std::function<void(TestCollection&)> init)
        {
            init(*this);
        }

        std::vector<Test> mTests;

        enum class Result
        {
            passed,
            skipped,
            failed
        };


        Result runOne(std::size_t idx, CLP const* cmd = nullptr);
        Result run(std::vector<std::size_t> testIdxs, std::size_t repeatCount = 1, CLP const* cmd = nullptr);
        Result runAll(uint64_t repeatCount = 1, CLP const* cmd = nullptr);
        Result runIf(CLP& cmd);
        Result run(CLP& cmd);
        void list();

        std::vector<std::size_t> search(const std::list<std::string>& s);


        void add(std::string name, std::function<void()> test);
        void add(std::string name, std::function<void(const CLP&)> test);

        void operator+=(const TestCollection& add);
    };


    class UnitTestFail : public std::exception
    {
        std::string mWhat;
    public:
        explicit UnitTestFail(std::string reason)
            :std::exception(),
            mWhat(reason)
        {}

        explicit UnitTestFail()
            :std::exception(),
            mWhat("UnitTestFailed exception")
        {
        }

        virtual  const char* what() const throw()
        {
            return mWhat.c_str();
        }
    };

    class UnitTestSkipped : public std::runtime_error
    {
    public:
        UnitTestSkipped()
            : std::runtime_error("skipping test")
        {}

        UnitTestSkipped(std::string r)
            : std::runtime_error(r)
        {}
    };

    enum class Color {
        LightGreen = 2,
        LightGrey = 3,
        LightRed = 4,
        OffWhite1 = 5,
        OffWhite2 = 6,
        Grey = 8,
        Green = 10,
        Blue = 11,
        Red = 12,
        Pink = 13,
        Yellow = 14,
        White = 15,
        Default
    };

    extern const Color ColorDefault;


    std::ostream& operator<<(std::ostream& out, Color color);

	extern TestCollection testCollection;
}
