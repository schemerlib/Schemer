#include <utility>

#include <doctest/doctest.h>

#include <Schemer/Schemer.h>
using namespace Schemer;

TEST_CASE("Config default values") {
    const Config config;

    CHECK_EQ(config.alpha, 0.0);
    CHECK_EQ(config.beta, 0.0);
    CHECK_EQ(config.sigma, 1.0);
    CHECK_EQ(config.length, 40.0);
    CHECK_EQ(config.num_intervals, 1000);
    CHECK_EQ(config.dt, 0.001);
    CHECK_EQ(config.theta, 0.5);
    CHECK_EQ(config.log_interval_percent, 0.0);
    CHECK_EQ(config.log_file_path, "schemer_progress.log");
    CHECK_EQ(config.delimiter, "\t");
    CHECK_EQ(config.force_scheme, ForceSchemeType::SecondOrderCentered);

    CHECK_EQ(config.zero_input.type, ZeroInput::Type::Middle);
    CHECK_EQ(config.zero_input.index, -1);
    CHECK_EQ(config.zero_input.distance, 0.0);
    CHECK_EQ(config.values_input.type, ValuesInput::Type::Dirac);
    CHECK_EQ(config.values_input.vector.size(), 0);
    CHECK_EQ(config.force_input.type, ForceInput::Type::Const);
    CHECK_EQ(config.force_input.force_value, 0.0);
    CHECK_FALSE(static_cast<bool>(config.force_input.force_function));
    CHECK_UNARY(config.force_input.force_vector.empty());
    CHECK_EQ(config.reset_input.type, ResetInput::Type::Zero);
    CHECK_EQ(config.reset_input.index, 0);
    CHECK_EQ(config.reset_input.rate, 0.0);
}

TEST_CASE("Config grid helpers") {
    Config config;
    config.length = 12.0;
    config.num_intervals = 4;

    CHECK_EQ(config.get_dx(), doctest::Approx(3.0));
    CHECK_EQ(config.get_size(), 5);
    CHECK_EQ(config.get_coordinates(0), (std::vector<double>{0.0, 3.0, 6.0, 9.0, 12.0}));
    CHECK_EQ(config.get_coordinates(2), (std::vector<double>{-6.0, -3.0, 0.0, 3.0, 6.0}));
}

TEST_CASE("FractionalScheme config and copy/move construction and assignment") {
    Config config;
    config.alpha = 1.5;
    config.num_intervals = 4;
    const Eigen::MatrixXd expected = FractionalScheme(config).build_diffusion_matrix();

    FractionalScheme copied_config(config);
    CHECK(copied_config.build_diffusion_matrix().isApprox(expected));

    Config config_to_move = config;
    FractionalScheme moved_config(std::move(config_to_move));
    CHECK(moved_config.build_diffusion_matrix().isApprox(expected));

    FractionalScheme copy_constructed(copied_config);
    CHECK(copy_constructed.build_diffusion_matrix().isApprox(expected));

    FractionalScheme move_constructed(std::move(copy_constructed));
    CHECK(move_constructed.build_diffusion_matrix().isApprox(expected));

    FractionalScheme copy_assigned(config);
    copy_assigned = moved_config;
    CHECK(copy_assigned.build_diffusion_matrix().isApprox(expected));

    FractionalScheme move_assigned(config);
    move_assigned = std::move(copy_assigned);
    CHECK(move_assigned.build_diffusion_matrix().isApprox(expected));
}
