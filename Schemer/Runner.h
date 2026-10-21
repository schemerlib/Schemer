/**
 * @file Runner.h
 * @brief Runs time-dependent simulations and manages data output.
 */

#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "Setup.h"

namespace Schemer {
    /**
     * @class Runner
     * @brief Manages simulation progress, time stepping, and file saving.
     *
     * Controls the step-by-step execution of the simulation, keeps track of current
     * state values, and logs progress or saves output data to files.
     */
    class Runner {
    private:
        /// Setup used to create this simulation.
        Setup setup;

        /// Total number of time steps completed so far.
        int steps_taken = 0;

        /// Values across all grid points at the current time step.
        Eigen::VectorXd current_values;

        /**
         * @brief Writes current progress percentage to an output stream.
         * @param os Stream where log messages are written.
         * @param progress_percent Completion percentage (0 to 100).
         */
        static void log(std::ostream &os, double progress_percent);

        /**
         * @brief Writes configuration settings to an output stream.
         * @param os Stream where parameters are written.
         */
        void save_parameters(std::ostream &os) const;

        /**
         * @brief Writes current grid values to an output stream.
         * @param os Stream where current state values are written.
         */
        void save_current_values(std::ostream &os) const;

    public:
        /**
         * @brief Creates a simulation runner using a given setup.
         * @param setup Setup object containing settings, matrices, and initial values.
         */
        explicit Runner(Setup setup) : setup(std::move(setup)) { reset(); }

        ~Runner() = default;

        Runner(const Runner &) = default;

        Runner &operator=(const Runner &) = default;

        Runner(Runner &&) = default;

        Runner &operator=(Runner &&) = default;

        /**
         * @brief Resets the simulation back to step 0 and initial state values.
         */
        void reset() {
            current_values = setup.initial_state;
            steps_taken = 0;
        }

        /**
         * @brief Advances the simulation forward by a specified number of time steps.
         *
         * Steps through time, updates current grid values, logs progress, and
         * optionally writes history states to a stream at specified step intervals.
         *
         * @param steps Total number of time steps to run.
         * @param history_stream Optional output stream for saving history data (pass nullptr to skip).
         * @param save_every How often to save state history in steps (set to 0 to skip saving).
         */
        void run(int steps, std::ostream *history_stream = nullptr, int save_every = 0);

        /**
         * @brief Saves the current simulation state and settings to a file.
         * @param filepath Path to the output file on disk.
         */
        void save_result(const std::string &filepath) const;

        /**
         * @brief Gets a copy of the internal simulation setup.
         * @return Setup Copy of the setup configuration and matrices.
         */
        Setup get_setup() const { return setup; }

        /**
         * @brief Gets a copy of the current grid values vector.
         * @return Eigen::VectorXd Copy of current state values.
         */
        Eigen::VectorXd get_values() const { return current_values; }

        /**
        * @brief Gets a copy of the current grid values vector as a std::vector.
        * @return std::vector<double> Copy of current state values as std::vector.
        */
        std::vector<double> get_values_vector() const {
            return std::vector<double>(current_values.data(), current_values.data() + current_values.size());
        }
    };

    inline void Runner::log(std::ostream &os, const double progress_percent) {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);

        os << '['
                << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
                << "] Progress: "
                << progress_percent
                << "%" << std::endl;
    }

    inline void Runner::save_parameters(std::ostream &os) const {
        const auto &config = setup.config;

        os << '#' << "alpha: " << config.alpha << '\n';
        os << '#' << "beta: " << config.beta << '\n';
        os << '#' << "sigma: " << config.sigma << '\n';
        os << '#' << "length: " << config.length << '\n';

        os << '#' << "dt: " << config.dt << '\n';
        os << '#' << "num_intervals: " << config.num_intervals << '\n';
        os << '#' << "theta: " << config.theta << '\n';

        os << '#' << "delimiter: " << config.delimiter << '\n';
        os << '#' << "log_interval_percent: " << config.log_interval_percent << '\n';
        os << '#' << "log_file_path: " << config.log_file_path << '\n';
        os << '#' << "verbose: " << config.verbose << '\n';

        os << '#' << "initial_values: ";
        for (Eigen::Index i = 0; i < setup.initial_state.size(); i++)
            os << setup.initial_state(i) << config.delimiter;
        os << '\n';

        os << '#' << "force_variant: ";

        if (std::holds_alternative<double>(setup.force_variant)) {
            os << std::get<double>(setup.force_variant) << '\n';
        } else {
            for (const double force_value: std::get<std::vector<double> >(setup.force_variant)) {
                os << force_value << config.delimiter;
            }
            os << '\n';
        }

        os << '#' << "x_coordinates: ";
        auto coords = config.get_coordinates(setup.starting_index);
        for (const double position: coords) {
            os << position << config.delimiter;
        }
        os << std::endl;
    }

    inline void Runner::save_current_values(std::ostream &os) const {
        const auto &delimiter = setup.config.delimiter;
        for (Eigen::Index i = 0; i < current_values.size(); i++)
            os << current_values(i) << delimiter;
        os << std::endl;
    }

    inline void Runner::run(const int steps, std::ostream *history_stream, const int save_every) {
        if (steps < 0)
            throw std::invalid_argument("amount of steps cannot be negative");
        if (save_every < 0)
            throw std::invalid_argument("save_every cannot be negative");
        if (save_every > 0 && history_stream == nullptr)
            std::cerr << "[WARNING] history_stream not provided, history will not be saved";

        const bool should_log_history = history_stream != nullptr && save_every > 0;
        if (should_log_history) {
            if (history_stream->tellp() <= 0)
                save_parameters(*history_stream);
        }

        std::ofstream log_stream;
        bool should_log_progress = setup.config.log_interval_percent > 0;
        if (should_log_progress) {
            log_stream.open(setup.config.log_file_path);
            if (!log_stream) {
                std::cerr << "[WARNING] Failed opening log file " + setup.config.log_file_path <<
                        "logs will not be saved";
                should_log_progress = false;
            }
        }
        int log_interval = static_cast<int>(steps * setup.config.log_interval_percent / 100.0);
        if (log_interval <= 0) {
            log_interval = 1;
        }

        for (int step = 0; step < steps; step++) {
            if (should_log_progress && step % log_interval == 0)
                log(log_stream, 100.0 * step / steps);

            if (should_log_history && step % save_every == 0)
                save_current_values(*history_stream);

            current_values = setup.step_matrix * current_values;
            steps_taken++;
        }

        if (should_log_history)
            save_current_values(*history_stream);

        if (should_log_progress)
            log(log_stream, 100.0);
    }

    inline void Runner::save_result(const std::string &filepath) const {
        std::ofstream out_stream(filepath);
        if (!out_stream) {
            throw std::runtime_error("Failed to open save file: " + filepath);
        }
        save_parameters(out_stream);
        out_stream << '#' << "steps: " << steps_taken << std::endl;
        save_current_values(out_stream);
        if (setup.config.verbose > 0)
            std::cout << "Saved to " << filepath << std::endl;
    }
} // namespace Schemer
