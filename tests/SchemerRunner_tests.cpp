#include <doctest/doctest.h>
#include <sstream>
#include <fstream>
#include <cstdio>

#include <Schemer/Schemer.h>
using namespace Schemer;

static Setup make_setup() {
    return Builder()
            .set_alpha(1.5)
            .set_num_intervals(4)
            .set_length(4.0)
            .set_dt(0.01)
            .set_log_interval_percent(0.0)
            .build_setup();
}

TEST_CASE (
"Runner execution and state updates"
)
 {
    Setup setup = make_setup();
    Runner runner(setup);

    SUBCASE("Initial state after construction") {
        CHECK_EQ(runner.get_values().size(), setup.initial_state.size());
        CHECK(runner.get_values().isApprox(setup.initial_state));
    }

    SUBCASE("Advancing simulation steps") {
        Eigen::VectorXd initial_values = runner.get_values();

        CHECK_NOTHROW(runner.run(5));

        CHECK_FALSE(runner.get_values().isApprox(initial_values));
    }

    SUBCASE("Resetting simulation state") {
        runner.run(10);
        CHECK_FALSE(runner.get_values().isApprox(setup.initial_state));

        runner.reset();
        CHECK(runner.get_values().isApprox(setup.initial_state));
    }

    SUBCASE("Negative step handling throws exception") {
        CHECK_THROWS_AS(runner.run(-1), std::invalid_argument);
    }
}

TEST_CASE (
"Runner history and file output"
)
 {
    Setup setup = make_setup();
    Runner runner(setup);

    SUBCASE("History streaming output") {
        std::stringstream history_stream;

        runner.run(4, &history_stream, 2);

        std::string output = history_stream.str();
        CHECK_FALSE(output.empty());
        CHECK_NE(output.find("#alpha: 1.5"), std::string::npos);
    }

    SUBCASE("Saving result to disk") {
        std::string test_filepath = "test_output.dat";

        runner.run(3);
        CHECK_NOTHROW(runner.save_result(test_filepath));

        std::ifstream file(test_filepath);
        CHECK(file.is_open());

        file.close();
        std::remove(test_filepath.c_str());
    }
}

TEST_CASE (
"Runner writes progress to configured log file"
)
 {
    const std::string log_filepath = "test_progress.log";
    Runner runner(Builder()
            .set_alpha(1.5)
            .set_num_intervals(4)
            .set_length(4.0)
            .set_dt(0.01)
            .set_log_interval_percent(100.0)
            .set_log_file_path(log_filepath)
            .build_setup());

    runner.run(1);

    std::ifstream file(log_filepath);
    CHECK(file.is_open());
    file.close();
    std::remove(log_filepath.c_str());
}

TEST_CASE (
"Runner copy and move semantics"
)
 {
    Setup setup = make_setup();
    Runner runner1(setup);
    runner1.run(2);

    SUBCASE("Copy constructor creates independent instance") {
        Runner runner2(runner1);
        CHECK(runner2.get_values().isApprox(runner1.get_values()));

        runner1.run(3);
        CHECK_FALSE(runner2.get_values().isApprox(runner1.get_values()));
    }

    SUBCASE("Move constructor transfers state successfully") {
        Eigen::VectorXd expected = runner1.get_values();
        Runner runner2(std::move(runner1));

        CHECK(runner2.get_values().isApprox(expected));
    }
}
