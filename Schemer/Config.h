/**
 * @file Config.h
 * @brief Configuration settings for simulation runs.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <Eigen/Dense>

namespace Schemer {
/**
 * @struct ForceSchemeType
 * @brief Configuration for the force scheme used for building the force matrix.
 */
enum struct ForceSchemeType {
    SecondOrderCentered,
    FirstOrderUpwind,
    SecondOrderUpwind
};

/**
 * @struct ZeroInput
 * @brief Configuration for positioning the spatial origin (x = 0.0) on the grid.
 */
struct ZeroInput {
    /**
     * @enum Type
     * @brief Method used to position the spatial origin.
     */
    enum struct Type {
        Middle, ///< Place origin at the exact center of the grid.
        Index, ///< Place origin at a specific grid index.
        Distance ///< Place origin at a physical distance measured from the left edge.
    };

    Eigen::Index index = -1; ///< Grid index when type is Index.
    double distance = 0.0; ///< Distance from left edge when type is Distance.
    Type type = Type::Middle; ///< Selected positioning method.
};

/**
 * @struct ValuesInput
 * @brief Configuration for initial values applied across the grid at t = 0.
 */
struct ValuesInput {
    /**
     * @enum Type
     * @brief Type of starting values.
     */
    enum struct Type {
        Dirac, ///< Single peak (impulse) at the origin (x = 0.0).
        Vector ///< Custom starting values provided for all grid points.
    };

    Eigen::VectorXd vector = Eigen::VectorXd(); ///< Custom vector when type is Vector.
    Type type = Type::Dirac; ///< Selected value type.
};

/**
 * @struct ForceInput
 * @brief Configuration for external forces applied to the system.
 */
struct ForceInput {
    /**
     * @enum Type
     * @brief Type of external force.
     */
    enum struct Type {
        Const, ///< Constant force applied uniformly across the domain.
        Function, ///< Spatially varying force defined by a function f(x).
        Vector ///< Custom force value provided for each grid point.
    };

    std::function<double(double)> force_function; ///< Function when type is Function.
    std::vector<double> force_vector; ///< Vector when type is Vector.
    double force_value = 0.0; ///< Force value when type is Drift.
    Type type = Type::Const; ///< Selected force type.
};

/**
 * @struct ResetInput
 * @brief Configuration for where siphoned probability is concentrated during resetting.
 */
struct ResetInput {
    /**
     * @enum Type
     * @brief Method used to specify the reset target location.
     */
    enum struct Type {
        Zero, ///< Reset probability to the spatial origin (x = 0.0).
        Index ///< Reset probability to a specific grid index.
    };

    Eigen::Index index = 0; ///< Grid index when type is Index.

    /**
     * @brief Rate of probability siphoning for stochastic resetting.
     */
    double rate = 0.0;
    Type type = Type::Zero; ///< Selected reset target type.
};

/**
 * @struct Config
 * @brief Holds all simulation settings, physical parameters, grid specs, and file output options.
 */
struct Config {
    /// Configuration for external forces.
    ForceInput force_input;

    /**
     * @brief File path used for writing progress log messages.
     */
    std::string log_file_path = "schemer_progress.log";

    /**
     * @brief Column separator for output files.
     *
     * The string sequence used between data columns when saving results.
     */
    std::string delimiter = "\t";

    /// Configuration for positioning spatial origin.
    ZeroInput zero_input;

    /// Configuration for reset target location.
    ResetInput reset_input;

    /// Configuration for initial grid values.
    ValuesInput values_input;

    /**
     * @brief Stability parameter (\alpha).
     *
     * Controls the order of the fractional derivative, restricted to (0, 2].
     * A value of 2.0 gives standard Gaussian diffusion. Values less than 2.0
     * produce superdiffusion (Lévy flights) with heavy tails, making long jumps
     * more likely.
     */
    double alpha = 0.0;

    /**
     * @brief Skewness parameter (\beta).
     *
     * Controls the left-right bias of the diffusion, restricted to [-1, 1].
     * A value of 0 gives a balanced, symmetric spread, negative values lean left,
     * and positive values lean right.
     */
    double beta = 0.0;

    /**
     * @brief Scale parameter (\sigma).
     *
     * Controls the overall spread or width of the distribution. Must be positive (> 0).
     */
    double sigma = 1.0;

    /**
     * @brief Size of the 1D spatial domain.
     *
     * Defines the length of the grid in simulation units.
     */
    double length = 40.0;

    /**
     * @brief Time step size (\Delta t).
     *
     * The duration of a single step forward in time for numerical integration.
     */
    double dt = 0.001;

    /**
     * @brief Time stepping method (\theta).
     */
    double theta = 0.5;

    /**
     * @brief Log progress percentage.
     *
     * Completion percentage step between logging progress.
     */
    double log_interval_percent = 0.0;

    /**
     * @brief Number of grid intervals (N).
     *
     * Divides the domain into N segments, resulting in N + 1 total grid points.
     */
    Eigen::Index num_intervals = 1000;

    /// Chosen force matrix calculation scheme.
    ForceSchemeType force_scheme = ForceSchemeType::SecondOrderCentered;

    /**
     * @brief Calculates uniform grid step size (\Delta x).
     * @return double Grid step size.
     */
    constexpr double get_dx() const noexcept {
        return length / static_cast<double>(num_intervals);
    }

    /**
     * @brief Gets the total number of grid points (N + 1).
     * @return Eigen::Index Total grid point count.
     */
    constexpr Eigen::Index get_size() const noexcept {
        return num_intervals + 1;
    }

    /**
     * @brief Gets position coordinates for all grid points.
     * @param starting_index Grid point index corresponding to x = 0.0.
     * @return std::vector<double> List of spatial positions.
     */
    std::vector<double> get_coordinates(const Eigen::Index starting_index) const {
        const double dx = get_dx();
        const auto total_points = get_size();
        std::vector<double> coords(static_cast<std::size_t>(total_points));

        for (Eigen::Index i = 0; i < total_points; ++i) {
            coords[static_cast<std::size_t>(i)] = dx * static_cast<double>(i - starting_index);
        }
        return coords;
    }
};
} // namespace Schemer
