/**
 * @file advanced_usage.cpp
 * @brief Advanced example: forces, force schemes, stochastic resetting,
 * custom initial conditions, grid origin and reusing configurations.
 */

#include <cmath>
#include <functional>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>
#include <Eigen/Dense>
#include <Schemer/Schemer.h>

// Mean position of the distribution, computed on the simulation grid.
static double mean_position(const Schemer::Runner &runner) {
    const Schemer::Setup setup = runner.get_setup();
    const std::vector<double> x = setup.config.get_coordinates(setup.starting_index);
    const Eigen::VectorXd p = runner.get_values();
    const double dx = setup.config.get_dx();

    double mean = 0.0;
    for (Eigen::Index i = 0; i < p.size(); ++i) {
        mean += x[static_cast<std::size_t>(i)] * p(i) * dx;
    }
    return mean;
}

int main() {
    // A base builder shared by all the runs below.
    Schemer::Builder base;
    base.set_alpha(1.2)
            .set_beta(0.5)
            .set_sigma(1.0)
            .set_length(40.0)
            .set_num_intervals(400)
            .set_dt(0.005)
            .set_theta(0.5)              
            .set_log_interval_percent(10)
            .set_log_file_path("advanced_progress.log");

    // --- 1. Constant drift --------------------------------------------------
    Schemer::Runner drift_runner = Schemer::Builder(base)
            .set_drift(1.0)
            .build();
    drift_runner.run(400);
    std::cout << "Drift:           mean = " << mean_position(drift_runner) << '\n';

    // --- 2. Position-dependent force F(x), choosing the force scheme -------
    // Harmonic potential: F(x) = -2x pulls the distribution towards x = 0.
    const std::function<double(double)> harmonic = [](const double x) { return -2.0 * x; };


    Schemer::Runner runner = Schemer::Builder(base)
            .set_force(harmonic)
            .build();
    runner.run(400);
    std::cout << "mean = " << mean_position(runner) << '\n';

    // --- 3. Force given as one value per grid point ------------------------
    const Schemer::Config config = base.get_config();
    const std::vector<double> x = config.get_coordinates(config.num_intervals / 2);
    std::vector<double> force_values(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        force_values[i] = -4.0 * std::sin(x[i]);
    }
    Schemer::Runner vector_force_runner = Schemer::Builder(base)
            .set_force(force_values)
            .build();
    vector_force_runner.run(400);
    std::cout << "Force vector:    mean = " << mean_position(vector_force_runner) << '\n';

    // --- 4. Stochastic resetting --------------------------------------------
    // Probability is removed everywhere at the given rate and returned to the
    // reset target (the origin by default, or any grid index).
    Schemer::Runner reset_runner = Schemer::Builder(base)
            .set_drift(1.0)
            .set_reset_rate(0.5)
            .set_reset_index_zero()
            .build();
    reset_runner.run(400);
    std::cout << "Drift + reset:   mean = " << mean_position(reset_runner) << '\n';

    // --- 5. Custom initial conditions and origin position -------------------
    // Place x = 0 at 15 length units from the left edge and start from a
    // normalised Gaussian instead of the default Dirac delta.
    Schemer::Builder gaussian_builder(base);
    gaussian_builder.set_zero_distance(15.0);

    const Schemer::Config gaussian_config = gaussian_builder.get_config();
    const Eigen::Index zero_index = static_cast<Eigen::Index>(
        std::round(15.0 / gaussian_config.get_dx()));
    const std::vector<double> gx = gaussian_config.get_coordinates(zero_index);

    Eigen::VectorXd initial(gaussian_config.get_size());
    constexpr double width = 1.0;
    for (Eigen::Index i = 0; i < initial.size(); ++i) {
        const double xi = gx[static_cast<std::size_t>(i)];
        initial(i) = std::exp(-xi * xi / (2.0 * width * width)) / (width * std::sqrt(2.0 * std::numbers::pi));
    }

    Schemer::Runner gaussian_runner = gaussian_builder
            .set_initial_conditions(initial)
            .build();
    gaussian_runner.run(400);
    std::cout << "Gaussian start:  mean = " << mean_position(gaussian_runner) << '\n';

    // --- 6. Editing a Config directly and rerunning -------------------------
    Schemer::Config edited = base.get_config();
    edited.alpha = 0.8;
    edited.beta = -0.5;
    Schemer::Runner config_runner = Schemer::Builder().set_config(edited).build();
    config_runner.run(200);
    const double mean_after_200 = mean_position(config_runner);

    config_runner.reset();          // back to step 0 and the initial state
    config_runner.run(200);         // reproduces the same result
    std::cout << "From Config:     mean = " << mean_after_200
            << " (after reset: " << mean_position(config_runner) << ")\n";

    // --- 7. Building a Setup and constructing the Runner manually -----------
    const Schemer::Setup setup = Schemer::Builder(base).build_setup();
    std::cout << "Step matrix:     " << setup.step_matrix.rows() << " x " << setup.step_matrix.cols() << '\n';
    Schemer::Runner manual_runner(setup);
    manual_runner.run(100);

    return 0;
}
