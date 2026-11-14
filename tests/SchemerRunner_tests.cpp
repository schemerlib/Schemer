#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

#include <Schemer/Schemer.h>
using namespace Schemer;

static Setup make_setup() {
    return Builder()
            .set_alpha(1.5)
            .set_num_intervals(4)
            .set_length(4.0)
            .set_dt(0.01)
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

        runner.run(4, history_stream, 2);

        std::string output = history_stream.str();
        CHECK_FALSE(output.empty());
        CHECK_NE(output.find("#alpha: 1.5"), std::string::npos);
        for (const char *field : {
                 "#beta:", "#sigma:", "#length:", "#dt:", "#num_intervals:", "#theta:",
                 "#dx:", "#grid_size:", "#delimiter:", "#log_interval_percent:", "#log_file_path:",
                 "#force_scheme:", "#reset_input_rate:",
                 "#initial_values:", "#force:", "#x_coordinates:"
             }) {
            CHECK_NE(output.find(field), std::string::npos);
        }
        CHECK_EQ(output.find("#zero_input_type:"), std::string::npos);
        CHECK_EQ(output.find("#values_input_type:"), std::string::npos);
        CHECK_EQ(output.find("#force_input_type:"), std::string::npos);
        CHECK_EQ(output.find("#reset_input_type:"), std::string::npos);
        CHECK_EQ(output.find("#zero_input_index:"), std::string::npos);
        CHECK_EQ(output.find("#values_input_vector:"), std::string::npos);
        CHECK_EQ(output.find("#force_input_vector:"), std::string::npos);
        CHECK_EQ(output.find("#reset_input_index:"), std::string::npos);
        CHECK_EQ(output.find("#force_input_value:"), std::string::npos);
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
"Runner creates to configured log file"
)
 {
    const std::string log_filepath = "test_progress.log";
    Runner runner = Builder()
            .set_alpha(1.5)
            .set_num_intervals(4)
            .set_log_interval_percent(10.0)
            .set_log_file_path(log_filepath)
            .build();

    runner.run(100);

    std::ifstream file(log_filepath);

    CHECK(file.is_open());

    file.close();
    std::remove(log_filepath.c_str());
}

TEST_CASE (
"Runner does not create the log file at 0 log interval"
)
 {
    const std::string log_filepath = "test_progress.log";
    Runner runner = Builder()
            .set_alpha(1.5)
            .set_num_intervals(4)
            .set_log_interval_percent(0.0)
            .set_log_file_path(log_filepath)
            .build();

    runner.run(100);

    std::ifstream file(log_filepath);
    CHECK_FALSE(file.is_open());
    file.close();
}

TEST_CASE (
"Runner copy and move semantics"
)
 {
    Setup setup = make_setup();
    Runner from_lvalue(setup);
    CHECK(from_lvalue.get_values().isApprox(setup.initial_state));

    Runner from_rvalue{Setup(setup)};
    CHECK(from_rvalue.get_values().isApprox(setup.initial_state));

    from_lvalue.run(2);

    SUBCASE("Copy constructor creates independent instance") {
        Runner runner2(from_lvalue);
        CHECK(runner2.get_values().isApprox(from_lvalue.get_values()));

        from_lvalue.run(3);
        CHECK_FALSE(runner2.get_values().isApprox(from_lvalue.get_values()));
    }

    SUBCASE("Move constructor transfers state successfully") {
        const Eigen::VectorXd expected = from_lvalue.get_values();
        Runner runner2(std::move(from_lvalue));

        CHECK(runner2.get_values().isApprox(expected));
    }

    SUBCASE("Copy assignment copies state") {
        Runner assigned(make_setup());
        assigned = from_lvalue;

        CHECK(assigned.get_values().isApprox(from_lvalue.get_values()));
        from_lvalue.run(1);
        CHECK_FALSE(assigned.get_values().isApprox(from_lvalue.get_values()));
    }

    SUBCASE("Move assignment transfers state") {
        const Eigen::VectorXd expected = from_lvalue.get_values();
        Runner assigned(make_setup());
        assigned = std::move(from_lvalue);

        CHECK(assigned.get_values().isApprox(expected));
    }
}

TEST_CASE (
"Getters"
)
{    Setup setup = make_setup();
    Runner runner(setup);
    auto values = setup.initial_state;

    SUBCASE("Get_values works") {
        CHECK_EQ(runner.get_values(), values);
    }

    SUBCASE("Get_values_vector works") {
        const std::vector<double> expected = {values.begin(), values.end()};
        CHECK_EQ(runner.get_values_vector(), expected);
    }
}