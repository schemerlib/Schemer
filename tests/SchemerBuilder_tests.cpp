#include <doctest/doctest.h>

#include <stdexcept>
#include <variant>
#include <vector>

#include <Schemer/Schemer.h>
using namespace Schemer;

static Builder get_valid_builder() {
    return Builder().set_alpha(1.5).set_num_intervals(4);
}

TEST_CASE("Builder basic construction") {
    Builder builder;
    builder.set_num_intervals(10)
            .set_alpha(1.0)
            .set_beta(0.0)
            .set_sigma(1.0)
            .set_length(10.0)
            .set_dt(0.001)
            .set_theta(0.5);

    CHECK_NOTHROW(builder.build());
}

TEST_CASE("Builder set_config") {
    Builder builder = get_valid_builder();
    Config test_config;
    test_config.alpha = 0.75;
    test_config.num_intervals = 4;
    test_config.zero_input.type = ZeroInput::Type::Distance;
    test_config.reset_input.rate = 80.0;

    builder.set_config(test_config);

    const Setup setup = builder.build_setup();
    CHECK_EQ(setup.config.alpha, test_config.alpha);
    CHECK_EQ(setup.config.num_intervals, test_config.num_intervals);
    CHECK_EQ(setup.config.zero_input.type, test_config.zero_input.type);
    CHECK_EQ(setup.config.zero_input.distance, test_config.zero_input.distance);
    CHECK_EQ(setup.config.reset_input.rate, test_config.reset_input.rate);
}

TEST_CASE("Builder build_setup places initial state at zero index") {
    Builder builder = get_valid_builder();
    Eigen::Index zero_index = 1;

    builder.set_zero_index(zero_index);

    const Setup setup = builder.build_setup();
    CHECK_EQ(setup.starting_index, zero_index);
    CHECK_EQ(setup.initial_state(0), 0.0);
    CHECK_EQ(setup.initial_state(2), 0.0);
    CHECK_EQ(setup.initial_state(3), 0.0);
    CHECK_EQ(setup.initial_state(4), 0.0);

    CHECK_GE(setup.initial_state(zero_index), 0.0);
}

TEST_CASE("Builder build_setup places initial state at zero distance") {
    Builder builder = get_valid_builder();

    builder.set_length(8.0)
            .set_zero_distance(2.0);

    const Setup setup = builder.build_setup();
    CHECK_EQ(setup.config.zero_input.type, ZeroInput::Type::Distance);
    CHECK_EQ(setup.config.zero_input.distance, 2.0);
    CHECK_EQ(setup.starting_index, 1);
    CHECK_EQ(setup.initial_state(0), 0.0);
    CHECK_EQ(setup.initial_state(2), 0.0);
    CHECK_EQ(setup.initial_state(3), 0.0);
    CHECK_EQ(setup.initial_state(4), 0.0);

    CHECK_GE(setup.initial_state(1), 0.0);
}

TEST_CASE("Builder build_setup uses configured index and vectors") {
    Builder builder = get_valid_builder();
    const std::vector<double> initial_values{1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> force_values{-1.0, -0.5, 0.0, 0.5, 1.0};
    builder
            .set_length(4.0)
            .set_zero_index(1)
            .set_initial_conditions(initial_values)
            .set_force(force_values)
            .set_reset_index(3)
            .set_reset_rate(0.2);

    const Setup setup = builder.build_setup();
    CHECK_EQ(setup.starting_index, 1);
    CHECK_EQ(setup.config.zero_input.type, ZeroInput::Type::Index);
    CHECK_EQ(setup.config.values_input.type, ValuesInput::Type::Vector);
    CHECK_EQ(setup.config.force_input.type, ForceInput::Type::Vector);
    CHECK_EQ(setup.config.reset_input.type, ResetInput::Type::Index);
    CHECK_EQ(setup.config.reset_input.index, 3);
    CHECK_EQ(setup.config.reset_input.rate, 0.2);
    REQUIRE_EQ(setup.initial_state.size(), 5);
    for (Eigen::Index i = 0; i < setup.initial_state.size(); ++i) {
        CHECK_EQ(setup.initial_state(i), initial_values[static_cast<std::size_t>(i)]);
    }
    REQUIRE(std::holds_alternative<std::vector<double>>(setup.force_variant));
    CHECK_EQ(std::get<std::vector<double>>(setup.force_variant), force_values);
    CHECK_EQ(setup.step_matrix.rows(), 5);
    CHECK_EQ(setup.step_matrix.cols(), 5);
    CHECK_UNARY(setup.step_matrix.allFinite());
}

TEST_CASE("Builder build_setup evaluates force at configured coordinates") {
    Builder builder = get_valid_builder();
    builder
            .set_length(8.0)
            .set_zero_distance(2.0)
            .set_force([](const double x) { return 2.0 * x; });

    const Setup setup = builder.build_setup();

    CHECK_EQ(setup.starting_index, 1);
    CHECK_EQ(setup.config.zero_input.type, ZeroInput::Type::Distance);
    CHECK_EQ(setup.config.zero_input.distance, 2.0);
    CHECK_EQ(setup.config.force_input.type, ForceInput::Type::Function);
    CHECK_UNARY(static_cast<bool>(setup.config.force_input.force_function));
    REQUIRE(std::holds_alternative<std::vector<double>>(setup.force_variant));
    CHECK_EQ(std::get<std::vector<double>>(setup.force_variant),
             (std::vector<double>{-4.0, 0.0, 4.0, 8.0, 12.0}));
}

TEST_CASE("Builder applies the configured force scheme") {
    const std::vector<double> force_values{-1.0, 0.0, 1.0, 2.0, 3.0};
    Builder centered_builder = get_valid_builder();
    centered_builder
            .set_force(force_values)
            .set_force_scheme(ForceSchemeType::SecondOrderCentered);

    Builder upwind_builder = get_valid_builder();
    upwind_builder
            .set_force(force_values)
            .set_force_scheme(ForceSchemeType::FirstOrderUpwind);

    const Setup centered_setup = centered_builder.build_setup();
    const Setup upwind_setup = upwind_builder.build_setup();

    CHECK_EQ(centered_setup.config.force_scheme, ForceSchemeType::SecondOrderCentered);
    CHECK_EQ(upwind_setup.config.force_scheme, ForceSchemeType::FirstOrderUpwind);
    CHECK_GT((centered_setup.step_matrix - upwind_setup.step_matrix).norm(), 0.0);
}

TEST_CASE("Builder configures progress log file path") {
    const auto setup = get_valid_builder()
            .set_log_file_path("custom_progress.log")
            .build_setup();

    CHECK_EQ(setup.config.log_file_path, "custom_progress.log");
}

TEST_CASE("Builder validate_parameters") {
    SUBCASE("empty builder build fails") {
        Builder builder;
        CHECK_THROWS(builder.build());
    }

    SUBCASE("alpha too small") {
        auto builder = get_valid_builder();
        builder.set_alpha(0.0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("alpha too large") {
        auto builder = get_valid_builder();
        builder.set_alpha(2.5);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("beta out of range") {
        auto builder = get_valid_builder();
        builder.set_beta(1.5);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("sigma negative") {
        auto builder = get_valid_builder();
        builder.set_sigma(-0.1);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("length nonpositive") {
        auto builder = get_valid_builder();
        builder.set_length(0.0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("dt nonpositive") {
        auto builder = get_valid_builder();
        builder.set_dt(0.0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("num_intervals nonpositive") {
        auto builder = get_valid_builder();
        builder.set_num_intervals(0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("theta outside [0,1]") {
        auto builder = get_valid_builder();
        builder.set_theta(1.5);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("beta nonzero when alpha==1") {
        auto builder = get_valid_builder();
        builder.set_alpha(1.0).set_beta(0.5);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }
}
