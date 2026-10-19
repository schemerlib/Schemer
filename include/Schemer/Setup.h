/**
 * @file Setup.h
 * @brief Defines the complete setup structure needed to run a simulation.
 */

#pragma once

#include "Config.h"
#include <Eigen/Dense>
#include <variant>
#include <vector>

namespace Schemer {
    /**
     * @struct Setup
     * @brief Holds all pre-calculated matrices, initial conditions, and configuration settings.
     *
     * Packs together everything required by the simulation runner to advance time steps,
     * including solver settings, initial grid values, step matrices, and external forces.
     */
    struct Setup {
        /**
         * @brief Configuration settings including physical parameters, time steps, and grid details.
         */
        Config config;

        /**
         * @brief Pre-calculated system matrix used to compute each step forward in time.
         */
        Eigen::MatrixXd step_matrix;

        /**
         * @brief Starting values for all physical points across the spatial grid at time t = 0.
         */
        Eigen::VectorXd initial_state;

        /**
         * @brief Grid point index corresponding to position x = 0.0.
         */
        Eigen::Index starting_index;

        /**
         * @brief External force acting on the system.
         *
         * Holds either a single constant value applied everywhere (`double`) or
         * individual force values for each grid point (`std::vector<double>`).
         */
        std::variant<double, std::vector<double> > force_variant;
    };
} // namespace Schemer
