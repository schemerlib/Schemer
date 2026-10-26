/**
 * @file Builder.h
 * @brief Builder class for setting up and creating simulation runners.
 */

#pragma once

#include "Runner.h"
#include "Config.h"
#include "FractionalScheme.h"
#include <Eigen/Dense>
#include <functional>
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <stdexcept>


namespace Schemer {
    /**
     * @class Builder
     * @brief Configures simulation settings step-by-step and creates a runner.
     *
     * Provides method chaining to set physical parameters, initial grid values,
     * origin position, and external forces before building the final runner object.
     */
    class Builder {
    private:
        /// Internal configuration object storing parameters and grid settings.
        Config inner_config;

        /**
         * @brief Checks if all settings and parameters are valid before building.
         */
        void validate_parameters() const;

        /**
         * @brief Calculates grid point index corresponding to origin x = 0.0.
         * @return Eigen::Index Grid index for the origin.
         */
        Eigen::Index compute_starting_index() const;

        /**
         * @brief Creates starting value vector for all grid points at t = 0.
         * @param starting_index Grid point index for origin x = 0.0.
         * @return Eigen::VectorXd Initial grid values vector.
         */
        Eigen::VectorXd compute_initial_state(Eigen::Index starting_index) const;

        /**
         * @brief Converts configured external force into vector format.
         * @param starting_index Grid point index for origin x = 0.0.
         * @return td::vector<double> Force representation.
         */
        std::vector<double> compute_force(Eigen::Index starting_index) const;

        /**
         * @brief Calculates grid point index for the probability reset target location.
         * @param starting_index Grid point index corresponding to origin x = 0.0.
         * @return Eigen::Index Grid index for the reset target.
         */
        Eigen::Index compute_reset_index(Eigen::Index starting_index) const;

    public:
        /**
        * @brief getter for config
        * @return Config Inner config struct with current parameters.
        */
        Config get_config() const {
            return inner_config;
        }

        /**
        - @brief Sets the full configuration object.
        - @param config Config object containing all simulation parameters.
        - @return Builder& Reference to this builder for chaining.
        */
        Builder &set_config(Config config) {
            inner_config = std::move(config);
            return *this;
        }

        /**
         * @brief Sets stability parameter \alpha \in (0, 2].
         * @param value Parameter value (\alpha = 2.0 gives standard Gaussian diffusion).
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_alpha(double value) {
            inner_config.alpha = value;
            return *this;
        }

        /**
         * @brief Sets skewness parameter \beta \in [-1, 1].
         * @param value Parameter value (0 gives symmetric spread, negative leans left, positive leans right).
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_beta(double value) {
            inner_config.beta = value;
            return *this;
        }

        /**
         * @brief Sets scale parameter \sigma > 0.
         * @param value Spread/dispersion factor of the diffusion profile.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_sigma(double value) {
            inner_config.sigma = value;
            return *this;
        }

        /**
         * @brief Sets physical length of 1D spatial domain (L).
         * @param value Total domain length in simulation units.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_length(double value) {
            inner_config.length = value;
            return *this;
        }

        /**
         * @brief Applies a constant, uniform external force across the domain.
         * @param value Constant force value.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_force(double value) {
            inner_config.force_input.force_value = value;
            inner_config.force_input.type = ForceInput::Type::Const;
            inner_config.force_input.force_function = nullptr;
            inner_config.force_input.force_vector.clear();
            return *this;
        }

        /**
         * @brief Applies custom force values provided at each grid point.
         * @param value List containing force values for all grid points.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_force(std::vector<double> value) {
            inner_config.force_input.force_vector = std::move(value);
            inner_config.force_input.type = ForceInput::Type::Vector;
            inner_config.force_input.force_function = nullptr;
            inner_config.force_input.force_value = 0.0;
            return *this;
        }

        /**
         * @brief Applies position-dependent force using a mathematical function f(x).
         * @param value Function taking spatial position x and returning local force.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_force(std::function<double(double)> value) {
            inner_config.force_input.force_function = std::move(value);
            inner_config.force_input.type = ForceInput::Type::Function;
            inner_config.force_input.force_vector.clear();
            inner_config.force_input.force_value = 0.0;
            return *this;
        }

        /**
         * @brief Sets the force scheme to be used for building the force matrix.
         * @param value Force scheme type.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_force_scheme(const ForceSchemeType value) {
            inner_config.force_scheme = value;
            return *this;
        }

        /**
         * @brief Sets time step size (\Delta t).
         * @param value Duration of a single step forward in time.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_dt(double value) {
            inner_config.dt = value;
            return *this;
        }

        /**
         * @brief Sets the total number of grid intervals (N).
         * @param value Number of segments (creates N + 1 total grid points).
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_num_intervals(Eigen::Index value) {
            inner_config.num_intervals = value;
            return *this;
        }

        /**
         * @brief Sets time integration scheme parameter \theta.
         * @param value Value weighting scheme (0.0 explicit, 0.5 semi-implicit, 1.0 implicit).
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_theta(double value) {
            inner_config.theta = value;
            return *this;
        }

        /**
         * @brief Sets detail level for terminal logging output.
         * @param value Verbosity level for built objects.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_verbosity(int value) {
            inner_config.verbose = value;
            return *this;
        }

        /**
         * @brief Sets column delimiter string used when saving output data.
         * @param value Delimiter sequence (e.g., tab or comma).
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_delimiter(std::string value) {
            inner_config.delimiter = std::move(value);
            return *this;
        }

        /**
         * @brief Sets completion progress percentage required between log updates.
         * @param value Progress update threshold percentage (%).
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_log_interval_percent(double value) {
            inner_config.log_interval_percent = value;
            return *this;
        }

        /**
         * @brief Sets the file path used for progress log messages.
         * @param value Path to the progress log file.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_log_file_path(std::string value) {
            inner_config.log_file_path = std::move(value);
            return *this;
        }

        /**
        * @brief Sets initial state values using an Eigen vector.
        * @param value Vector holding initial values for all grid points.
        * @return Builder& Reference to this builder for chaining.
        */
        Builder &set_initial_conditions(Eigen::VectorXd value) {
            inner_config.values_input.vector = std::move(value);
            inner_config.values_input.type = ValuesInput::Type::Vector;
            return *this;
        }

        /**
         * @brief Sets initial state values using a standard vector.
         * @param value List holding initial values for all grid points.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_initial_conditions(const std::vector<double> &value) {
            inner_config.values_input.vector = Eigen::VectorXd::Map(value.data(),
                                                                    static_cast<Eigen::Index>(value.size()));
            inner_config.values_input.type = ValuesInput::Type::Vector;
            return *this;
        }

        /**
         * @brief Places origin x = 0.0 at a specific grid point index.
         * @param value Grid index representing position x = 0.0.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_zero_index(Eigen::Index value) {
            inner_config.zero_input.index = value;
            inner_config.zero_input.type = ZeroInput::Type::Index;
            return *this;
        }

        /**
         * @brief Places origin x = 0.0 at a physical distance from the left boundary.
         * @param value Distance value in length units from the left edge.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_zero_distance(double value) {
            inner_config.zero_input.distance = value;
            inner_config.zero_input.type = ZeroInput::Type::Distance;
            return *this;
        }

        /**
         * @brief Places origin x = 0.0 at an exact center of domain.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_zero_middle() {
            inner_config.zero_input.type = ZeroInput::Type::Middle;
            return *this;
        }

        /**
         * @brief Sets the probability reset target to the spatial origin (x = 0.0).
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_reset_index_zero() {
            inner_config.reset_input.type = ResetInput::Type::Zero;
            return *this;
        }

        /**
         * @brief Sets the probability reset target to a specific grid point index.
         * @param value Grid index for the reset target.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_reset_index(Eigen::Index value) {
            inner_config.reset_input.index = value;
            inner_config.reset_input.type = ResetInput::Type::Index;
            return *this;
        }

        /**
         * @brief Sets the rate of probability siphoning for stochastic resetting.
         * @param value Reset rate value.
         * @return Builder& Reference to this builder for chaining.
         */
        Builder &set_reset_rate(double value) {
            inner_config.reset_input.rate = value;
            return *this;
        }

        /**
         * @brief Builds and returns a fully configured simulation runner ready to execute.
         * @return Runner Ready-to-run simulation solver object.
         */
        Runner build() const {
            return Runner(build_setup());
        }

        /**
         * @brief Prepares and returns a complete simulation setup structure.
         * @return Setup Prepared setup with configuration, matrices, and initial states.
         */
        Setup build_setup() const;
    };

    inline void Builder::validate_parameters() const {
        if (inner_config.alpha <= 0.0 || inner_config.alpha >= 2.0) throw std::invalid_argument(
            "alpha must be in (0, 2]");
        if (std::abs(inner_config.beta) > 1.0) throw std::invalid_argument("beta must be in [-1, 1]");
        if (inner_config.sigma < 0.0) throw std::invalid_argument("sigma must be non-negative");
        if (inner_config.length <= 0.0) throw std::invalid_argument("length must be positive");

        if (inner_config.dt <= 0.0) throw std::invalid_argument("dt must be positive");
        if (inner_config.num_intervals <= 0) throw std::invalid_argument("grid_points must be positive");
        if (inner_config.theta < 0.0 || inner_config.theta > 1.0) throw
                std::invalid_argument("theta must be in [0, 1]");
        if (inner_config.log_interval_percent < 0.0)
            throw std::invalid_argument(
                "log interval percent must be non-negative");
        if (inner_config.reset_input.rate < 0.0) throw std::invalid_argument("reset input rate must be non-negative");

        if (std::abs(inner_config.alpha - 1) < FractionalScheme::ALPHA_EPSILON && inner_config.beta != 0.0) {
            throw std::invalid_argument("beta != 0.0 not supported for alpha = 1.0");
        }

        if (inner_config.dt > 1.0) {
            std::cerr << "[WARNING] dt > 1.0\n" << std::endl;
        }
    }

    inline Eigen::Index Builder::compute_starting_index() const {
        switch (inner_config.zero_input.type) {
            case ZeroInput::Type::Middle:
                return inner_config.num_intervals / 2;

            case ZeroInput::Type::Index:
                if (inner_config.zero_input.index < 0 || inner_config.zero_input.index > inner_config.num_intervals)
                    throw std::invalid_argument("initial_index must be in [0, I]");
                return inner_config.zero_input.index;

            case ZeroInput::Type::Distance: {
                if (inner_config.zero_input.distance < 0.0 || inner_config.zero_input.distance > inner_config.length)
                    throw std::invalid_argument("initial_distance must be in [0, length]");

                const auto grid_points_double = static_cast<double>(inner_config.num_intervals);
                const double ideal_index = grid_points_double * inner_config.zero_input.distance / inner_config.length;
                const auto real_index = static_cast<Eigen::Index>(std::round(ideal_index));
                const double actual_distance = static_cast<double>(real_index) * inner_config.length /
                                               grid_points_double;
                const double distance_difference = inner_config.zero_input.distance - actual_distance;

                if (std::abs(distance_difference) > 1e-9) {
                    std::cerr << "[WARNING] Target distance " << inner_config.zero_input.distance
                            << " snapped to grid index " << real_index
                            << " (discrepancy: " << distance_difference << ")\n";
                }

                if (real_index < 0 || real_index > inner_config.num_intervals)
                    throw std::logic_error("Calculated grid index is outside the grid boundaries");

                return real_index;
            }

            default:
                throw std::logic_error("Unhandled ZeroInput::Type enum value");
        }
    }

    inline Eigen::VectorXd Builder::compute_initial_state(const Eigen::Index starting_index) const {
        const Eigen::Index grid_size = inner_config.get_size();
        Eigen::VectorXd starting_values = Eigen::VectorXd::Zero(grid_size);

        switch (inner_config.values_input.type) {
            case ValuesInput::Type::Dirac:
                starting_values(starting_index) = static_cast<double>(inner_config.num_intervals) / inner_config.length;
                break;

            case ValuesInput::Type::Vector:
                if (inner_config.values_input.vector.size() != grid_size)
                    throw std::invalid_argument("initial values must have size I+1");
                starting_values = inner_config.values_input.vector;
                break;

            default:
                throw std::logic_error("Unhandled ValuesInput::Type enum value");
        }

        return starting_values;
    }

    inline std::vector<double> Builder::compute_force(const Eigen::Index starting_index) const {
        switch (inner_config.force_input.type) {
            case ForceInput::Type::Const: {
                const double value = inner_config.force_input.force_value;
                return std::vector<double>(inner_config.get_size(), value);
            }
            case ForceInput::Type::Function: {
                if (!inner_config.force_input.force_function)
                    throw std::invalid_argument("force function not set");

                const auto coords = inner_config.get_coordinates(starting_index);
                std::vector<double> force_values(coords.size());

                for (size_t i = 0; i < coords.size(); ++i) {
                    force_values[i] = inner_config.force_input.force_function(coords[i]);
                }
                return force_values;
            }

            case ForceInput::Type::Vector:
                if (inner_config.force_input.force_vector.size() != inner_config.get_size())
                    throw std::invalid_argument("force vector must have size I+1");
                return inner_config.force_input.force_vector;

            default:
                throw std::logic_error("Unhandled ForceInput::Type enum value");
        }
    }

    inline Eigen::Index Builder::compute_reset_index(const Eigen::Index starting_index) const {
        switch (inner_config.reset_input.type) {
            case ResetInput::Type::Index:
                if (inner_config.reset_input.index < 0 || inner_config.reset_input.index >= inner_config.get_size())
                    throw std::invalid_argument("reset input index must be in [0, I)");
                return inner_config.reset_input.index;
            case ResetInput::Type::Zero:
                return starting_index;

            default:
                throw std::logic_error("Unhandled ResetInput::Type enum value");
        }
    }


    inline Setup Builder::build_setup() const {
        validate_parameters();

        const Eigen::Index starting_index = compute_starting_index();
        const Eigen::Index reset_index = compute_reset_index(starting_index);

        auto force = compute_force(starting_index);
        auto initial_state = compute_initial_state(starting_index);

        const FractionalScheme fractional_scheme{inner_config};
        Eigen::MatrixXd step_matrix = fractional_scheme.build_step_matrix(force, reset_index);

        return Setup{
            .config = inner_config,
            .step_matrix = std::move(step_matrix),
            .initial_state = std::move(initial_state),
            .starting_index = starting_index,
            .force = std::move(force)
        };
    }
} // namespace Schemer
