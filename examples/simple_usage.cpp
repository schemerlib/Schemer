/**
 * @file simple_usage.cpp
 * @brief Minimal example: configure a simulation, run it and save the result.
 *
 * The distribution starts as a Dirac delta at x = 0 (the middle of the domain)
 * and spreads according to a symmetric (beta = 0) alpha-stable diffusion.
 */

#include <iostream>
#include <Eigen/Dense>
#include <Schemer/Schemer.h>

int main() {
    // 1. Configure the simulation with the Builder. Every setter returns a
    //    reference to the builder, so calls can be chained.
    Schemer::Runner runner = Schemer::Builder()
            .set_alpha(1.5)               // stability index, alpha in (0, 2)
            .set_beta(0.0)                // skewness, beta in [-1, 1]
            .set_sigma(1.0)               // scale parameter
            .set_length(20.0)             // domain spans [-10, 10]
            .set_num_intervals(200)       // 201 grid points
            .set_dt(0.01)                 // time step
            .set_log_interval_percent(0)  // disable progress logging
            .build();

    // 2. Advance the simulation by 500 time steps (t = 500 * dt = 5.0).
    runner.run(500);

    // 3. Read the current state.
    const Eigen::VectorXd values = runner.get_values();
    const double dx = runner.get_setup().config.get_dx();
    std::cout << "Grid points: " << values.size() << '\n';
    std::cout << "Total probability: " << values.sum() * dx << '\n';
    std::cout << "Value at x = 0: " << values(runner.get_setup().starting_index) << '\n';

    // 4. Save the configuration and the final state to a file.
    runner.save_result("simple_result.tsv");

    return 0;
}
