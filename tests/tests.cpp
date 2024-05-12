
#include "tests.h"
#include "await_lifetime_tests.h"
#include "eager_task_tests.h"
#include "result_tests.h"
#include "take_until_tests.h"
#include "task_tests.h"
#include "when_all_tests.h"
#include "sequence_tests.h"
#include "channel_spsc_tests.h"
#include "channel_mpsc_tests.h"
#include "async_scope_tests.h"

#ifdef _MSC_VER
#include <windows.h>
#endif
#include <iomanip>
#include <chrono>
#include <cmath>
#include <array>
#include <algorithm>

namespace macoro
{


	const Color ColorDefault([]() -> Color {
#ifdef _MSC_VER
		CONSOLE_SCREEN_BUFFER_INFO   csbi;
		HANDLE m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(m_hConsole, &csbi);

		return (Color)(csbi.wAttributes & 255);
#else
		return Color::White;
#endif

		}());

#ifdef _MSC_VER
	static const HANDLE __m_hConsole(GetStdHandle(STD_OUTPUT_HANDLE));
#endif
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

	std::array<const char*, 16> colorMap
	{
		"",         //    -- = 0,
		"",         //    -- = 1,
		GREEN,      //    LightGreen = 2,
		BLACK,      //    LightGrey = 3,
		RED,        //    LightRed = 4,
		WHITE,      //    OffWhite1 = 5,
		WHITE,      //    OffWhite2 = 6,
		"",         //         = 7
		BLACK,      //    Grey = 8,
		"",         //    -- = 9,
		BOLDGREEN,  //    Green = 10,
		BOLDBLUE,   //    Blue = 11,
		BOLDRED,    //    Red = 12,
		BOLDCYAN,   //    Pink = 13,
		BOLDYELLOW, //    Yellow = 14,
		RESET       //    White = 15
	};

	std::ostream& operator<<(std::ostream& out, Color tag)
	{
		if (tag == Color::Default)
			tag = ColorDefault;
#ifdef _MSC_VER
		SetConsoleTextAttribute(__m_hConsole, (WORD)tag | (240 & (WORD)ColorDefault));
#else

		out << colorMap[15 & (char)tag];
#endif
		return out;
	}


	void TestCollection::add(std::string name, std::function<void(const CLP&)> fn)
	{
		mTests.push_back({ std::move(name), std::move(fn) });
	}
	void TestCollection::add(std::string name, std::function<void()> fn)
	{
		mTests.push_back({ std::move(name),[fn](const CLP& cmd)
		{
			fn();
		} });
	}

	TestCollection::Result TestCollection::runOne(std::size_t idx, CLP const* cmd)
	{
		if (idx >= mTests.size())
		{
			std::cout << Color::Red << "No test " << idx << std::endl;
			return Result::failed;
		}

		CLP dummy;
		if (cmd == nullptr)
			cmd = &dummy;

		Result res = Result::failed;
		int w = int(std::ceil(std::log10(mTests.size())));
		std::cout << std::setw(w) << idx << " - " << Color::Blue << mTests[idx].mName << ColorDefault << std::flush;

		auto start = std::chrono::high_resolution_clock::now();
		try
		{
			mTests[idx].mTest(*cmd); std::cout << Color::Green << "  Passed" << ColorDefault;
			res = Result::passed;
		}
		catch (const UnitTestSkipped& e)
		{
			std::cout << Color::Yellow << "  Skipped - " << e.what() << ColorDefault;
			res = Result::skipped;
		}
		catch (const std::exception& e)
		{
			std::cout << Color::Red << "Failed - " << e.what() << ColorDefault;
		}
		auto end = std::chrono::high_resolution_clock::now();



		uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		std::cout << "   " << time << "ms" << std::endl;

		return res;
	}

	TestCollection::Result TestCollection::run(std::vector<std::size_t> testIdxs, std::size_t repeatCount, CLP const* cmd)
	{
		std::size_t numPassed(0), total(0), numSkipped(0);

		for (std::size_t r = 0; r < repeatCount; ++r)
		{
			for (auto i : testIdxs)
			{
				if (repeatCount != 1) std::cout << r << " ";
				auto res = runOne(i, cmd);
				numPassed += (res == Result::passed);
				total += (res != Result::skipped);
				numSkipped += (res == Result::skipped);
			}
		}

		if (numPassed == total)
		{
			std::cout << Color::Green << std::endl
				<< "=============================================\n"
				<< "            All Passed (" << numPassed << ")\n";
			if (numSkipped)
				std::cout << Color::Yellow << "            skipped (" << numSkipped << ")\n";

			std::cout << Color::Green
				<< "=============================================" << std::endl << ColorDefault;
			return Result::passed;
		}
		else
		{
			std::cout << Color::Red << std::endl
				<< "#############################################\n"
				<< "           Failed (" << total - numPassed << ")\n" << Color::Green
				<< "           Passed (" << numPassed << ")\n";

			if (numSkipped)
				std::cout << Color::Yellow << "            skipped (" << numSkipped << ")\n";

			std::cout << Color::Red
				<< "#############################################" << std::endl << ColorDefault;
			return Result::failed;
		}
	}

	std::vector<std::size_t> TestCollection::search(const std::list<std::string>& s)
	{
		std::set<std::size_t> ss;
		std::vector<std::size_t> ret;
		std::vector<std::string> names;

		auto toLower = [](std::string data) {
			std::transform(data.begin(), data.end(), data.begin(),
				[](unsigned char c) { return std::tolower(c); });
			return data;
		};

		for (auto& t : mTests)
			names.push_back(toLower(t.mName));

		for (auto str : s)
		{
			auto lStr = toLower(str);
			for (auto& t : names)
			{
				if (t.find(lStr) != std::string::npos)
				{
					auto i = &t - names.data();
					if (ss.insert(i).second)
						ret.push_back(i);
				}
			}
		}

		return ret;
	}

	TestCollection::Result TestCollection::run(CLP& cmd)
	{
		cmd.set("u");
		return runIf(cmd);
	}
	TestCollection::Result TestCollection::runIf(CLP& cmd)
	{
		if (cmd.isSet("list"))
		{
			list();
			return Result::passed;
		}
		auto unitTestTag = std::vector<std::string>{ "u","unitTests" };
		{
			cmd.setDefault("loop", 1);
			auto loop = cmd.get<std::size_t>("loop");

			if (cmd.hasValue(unitTestTag))
			{
				auto& str = cmd.getList(unitTestTag);
				if (str.front().size() && std::isalpha(str.front()[0]))
					return run(search(str), loop, &cmd);
				else
					return run(cmd.getMany<std::size_t>(unitTestTag), loop, &cmd);
			}
			else
				return runAll(loop, &cmd);
		}
		return Result::skipped;
	}

	TestCollection::Result TestCollection::runAll(uint64_t rp, CLP const* cmd)
	{
		std::vector<std::size_t> v;
		for (std::size_t i = 0; i < mTests.size(); ++i)
			v.push_back(i);

		return run(v, rp, cmd);
	}

	void TestCollection::list()
	{
		int w = int(std::ceil(std::log10(mTests.size())));
		for (uint64_t i = 0; i < mTests.size(); ++i)
		{
			std::cout << std::setw(w) << i << " - " << Color::Blue << mTests[i].mName << std::endl << ColorDefault;
		}
	}


	void TestCollection::operator+=(const TestCollection& t)
	{
		mTests.insert(mTests.end(), t.mTests.begin(), t.mTests.end());
	}



	TestCollection testCollection([](TestCollection& t) {
		using namespace tests;

		t.add("task_int_test                      ", task_int_test);
		t.add("task_void_test                     ", task_void_test);
		t.add("task_ref_test                      ", task_ref_test);
		t.add("task_move_test                     ", task_move_test);
		t.add("task_ex_test                       ", task_ex_test);
		t.add("task_blocking_int_test             ", task_blocking_int_test);
		t.add("task_blocking_void_test            ", task_blocking_void_test);
		t.add("task_blocking_ref_test             ", task_blocking_ref_test);
		t.add("task_blocking_move_test            ", task_blocking_move_test);
		t.add("task_blocking_ex_test              ", task_blocking_ex_test);
		t.add("task_blocking_cancel_test          ", task_blocking_cancel_test);

		t.add("scoped_task_test                   ", scoped_task_test);
		t.add("async_scope_test                   ", async_scope_test);

		t.add("when_all_basic_tests               ", when_all_basic_tests);
		t.add("when_all_scope_test                ", when_all_scope_test);
		
		t.add("schedule_after_test                ", schedule_after);
		t.add("take_until_tests                   ", take_until_tests);
		t.add("schedule_after_cancaled            ", schedule_after_cancaled);
		t.add("result_basic_store_test            ", result_basic_store_test);
		t.add("result_co_await_test               ", result_co_await_test);
		t.add("result_task_wrap_test              ", result_task_wrap_test);
		t.add("result_task_wrap_pipe_test         ", result_task_wrap_pipe_test);
		t.add("result_void_test                   ", result_void_test);

		t.add("make_eager_test                    ", make_eager_test);
		t.add("thread_pool_post_test              ", thread_pool_post_test);
		t.add("thread_pool_dispatch_test          ", thread_pool_dispatch_test);
		t.add("thread_pool_start_on_test          ", thread_pool_start_on_test);

		t.add("eager_task_int_test                ", eager_task_int_test);
		t.add("eager_task_void_test               ", eager_task_void_test);
		t.add("eager_task_ref_test                ", eager_task_ref_test);
		t.add("eager_task_move_test               ", eager_task_move_test);
		t.add("eager_task_ex_test                 ", eager_task_ex_test);
		t.add("eager_task_slow_test               ", eager_task_slow_test);
		t.add("eager_task_fast_test               ", eager_task_fast_test);
		t.add("eager_task_stress_test             ", eager_task_stress_test);

		t.add("store_as_t_lifetime_test           ", store_as_t_lifetime_test);
		t.add("await_lifetime_refOnly_test        ", await_lifetime_refOnly_test);
		t.add("await_lifetime_NoCopyMove_test     ", await_lifetime_NoCopyMove_test);
		t.add("await_lifetime_MovOnly_test        ", await_lifetime_MovOnly_test);
		t.add("await_lifetime_fn_test             ", await_lifetime_fn_test);
		t.add("yield_await_lifetime_test20        ", yield_await_lifetime_test20);
		t.add("yield_await_lifetime_test14        ", yield_await_lifetime_test14);
		t.add("sequence_spsc_test                 ", sequence_spsc_test);
		t.add("spsc_channel_test                  ", spsc_channel_test);
		t.add("spsc_channel_ex_test               ", spsc_channel_ex_test);
		t.add("mpsc_channel_test                  ", mpsc_channel_test);
		t.add("mpsc_channel_ex_test               ", mpsc_channel_ex_test);
		
		});
}