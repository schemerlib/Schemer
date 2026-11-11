/**
 * @file FractionalScheme.h
 * @brief Builds matrices for fractional diffusion simulations.
 */

#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

#include <Eigen/Dense>

#include "Config.h"

namespace Schemer {
    /**
     * @class FractionalScheme
     * @brief Builds diffusion and force matrices for time stepping.
     *
     * Calculates fractional derivative matrices based on alpha, beta, and grid spacing.
     */
    class FractionalScheme {
    public:
        /**
         * @brief Small tolerance value used to check if \alpha = 1.
         */
        static constexpr double ALPHA_EPSILON = 1e-8;

        explicit FractionalScheme(Config config)
            : config(std::move(config)), n(static_cast<int>(std::ceil(this->config.alpha))) {}

        FractionalScheme(const FractionalScheme &) = default;

        FractionalScheme(FractionalScheme &&) = default;

        FractionalScheme &operator=(const FractionalScheme &) = default;

        FractionalScheme &operator=(FractionalScheme &&) = default;

        /**
         * @brief Builds the main spatial diffusion matrix.
         * @return Eigen::MatrixXd The spatial diffusion matrix.
         */
        Eigen::MatrixXd build_diffusion_matrix() const;

        /**
         * @brief Builds the force matrix.
         * @param force grid values (`std::vector<double>`).
         * @return Eigen::MatrixXd The force matrix.
         */
        Eigen::MatrixXd build_force_matrix(const std::vector<double> &force) const;

        /**
         * @brief Builds the probability reset rate matrix.
         *
         * Siphons probability at a constant rate from all spatial points
         * and concentrates it at the specified target index.
         *
         * @param reset_index The grid index where siphoned probability is concentrated.
         * @return Eigen::MatrixXd The reset rate matrix.
         */
        Eigen::MatrixXd build_reset_matrix(Eigen::Index reset_index) const;

        /**
         * @brief Builds time-stepping matrix based on current settings and forces.
         * @param force Computed force representation.
         * @param reset_index Grid index where siphoned probability is concentrated.
         * @return Eigen::MatrixXd Time step update matrix.
         */
        Eigen::MatrixXd build_step_matrix(const std::vector<double> &force, Eigen::Index reset_index) const;

    private:
        Config config;
        int n;

        bool is_alpha_one() const {
            return std::abs(config.alpha - 1.0) < ALPHA_EPSILON;
        }

        /**
         * @brief Calculates fractional weight coefficients (\lambda_k^{(\alpha)}).
         * @return std::vector<double> List of calculated weights.
         */
        std::vector<double> get_lambdas() const;

        /**
         * @brief Holds calculated left and right fractional weights.
         */
        struct FractionalOmegas {
            double L_omega;
            double R_omega;
        };

        /**
         * @brief Calculates fractional weights (L_omega, R_omega) for alpha != 1.
         */
        FractionalOmegas calculate_fractional_omegas() const;

        /**
         * @brief Calculates the Cauchy weight factor for alpha == 1.
         */
        double calculate_cauchy_omega() const;

        /**
         * @brief Builds the diffusion matrix for 0 < \alpha < 1.
         * @return Eigen::MatrixXd Diffusion matrix for \alpha < 1.
         */
        Eigen::MatrixXd build_subdiffusion() const;

        /**
         * @brief Builds the diffusion matrix for \alpha = 1.
         * @return Eigen::MatrixXd Diffusion matrix for \alpha = 1.
         */
        Eigen::MatrixXd build_superdiffusion() const;

        /**
         * @brief Builds the diffusion matrix for 1 < \alpha \le 2.
         * @return Eigen::MatrixXd Diffusion matrix for \alpha > 1.
         */
        Eigen::MatrixXd build_cauchydiffusion() const;

        /**
         * @brief Builds the force matrix for a variable force across grid points using second order centered scheme.
         * @param force Force value at each grid point.
         * @return Eigen::MatrixXd Variable force matrix.
         */
        Eigen::MatrixXd build_force_secondordercentered(const std::vector<double> &force) const;

        /**
         * @brief Builds the force matrix for a variable force across grid points using first order upwind scheme.
         * @param force Force value at each grid point.
         * @return Eigen::MatrixXd Variable force matrix.
         */
        Eigen::MatrixXd build_force_firstorderupwind(const std::vector<double> &force) const;

        /**
         * @brief Builds the force matrix for a variable force across grid points using second order upwind scheme.
         * @param force Force value at each grid point.
         * @return Eigen::MatrixXd Variable force matrix.
         */
        Eigen::MatrixXd build_force_secondorderupwind(const std::vector<double> &force) const;
    };

    inline std::vector<double> FractionalScheme::get_lambdas() const {
        const Eigen::Index grid_size = config.get_size();
        const int n = static_cast<int>(std::ceil(config.alpha));
        std::vector<double> result(grid_size + 2);
        for (int i = 1; i < grid_size + 2; i++) {
            result[i] = std::pow(i, n - config.alpha) - std::pow(i - 1, n - config.alpha);
        }
        return result;
    }

    inline FractionalScheme::FractionalOmegas FractionalScheme::calculate_fractional_omegas() const {
        const double dx = config.get_dx();
        const double omega = config.sigma * config.dt / (std::tgamma(n + 1 - config.alpha) * std::pow(dx, config.alpha));
        const double denominator = 2.0 * std::cos(std::numbers::pi * config.alpha / 2.0);

        return {
            .L_omega = -omega * (1.0 + config.beta) / denominator,
            .R_omega = -omega * (1.0 - config.beta) / denominator
        };
    }

    inline double FractionalScheme::calculate_cauchy_omega() const {
        const double dx = config.get_dx();
        return 2.0 * config.sigma * config.dt / (dx * std::numbers::pi);
    }

    inline Eigen::MatrixXd FractionalScheme::build_diffusion_matrix() const {
        if (is_alpha_one()) {
            return build_cauchydiffusion();
        }
        return (config.alpha < 1.0) ? build_subdiffusion() : build_superdiffusion();
    }

    inline Eigen::MatrixXd FractionalScheme::build_subdiffusion() const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);

        const auto [L_omega, R_omega] = calculate_fractional_omegas();
        const auto lambdas = get_lambdas();

        const double v0 = lambdas[1];
        M.diagonal().setConstant(-1.0 * (L_omega + R_omega) * v0);

        for (Eigen::Index i = 1; i < grid_size; i++) {
            const double value = lambdas[i] - lambdas[i + 1];
            M.diagonal(-i).setConstant(value * L_omega);
            M.diagonal(i).setConstant(value * R_omega);
        }

        return M;
    }

    inline Eigen::MatrixXd FractionalScheme::build_superdiffusion() const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);

        const auto [L_omega, R_omega] = calculate_fractional_omegas();

        const auto lambdas = get_lambdas();

        const double v0 = lambdas[2] - 2.0 * lambdas[1];
        M.diagonal().setConstant(-1.0 * (L_omega + R_omega) * v0);

        const double v1 = lambdas[3] - 2.0 * lambdas[2] + lambdas[1];
        const double v2 = lambdas[1];
        M.diagonal(-1).setConstant(-1.0 * (L_omega * v1 + R_omega * v2));
        M.diagonal(1).setConstant(-1.0 * (R_omega * v1 + L_omega * v2));

        for (Eigen::Index i = 2; i < grid_size; i++) {
            const double value = lambdas[i + 2] - 2.0 * lambdas[i + 1] + lambdas[i];
            M.diagonal(-i).setConstant(-1.0 * value * L_omega);
            M.diagonal(i).setConstant(-1.0 * value * R_omega);
        }

        return M;
    }

    inline Eigen::MatrixXd FractionalScheme::build_cauchydiffusion() const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);

        const double cauchyOmega = calculate_cauchy_omega();

        M.diagonal().setConstant(2.0 * cauchyOmega);

        for (Eigen::Index i = 1; i < grid_size; i++) {
            const auto double_i = static_cast<double>(i);
            double value = 1.0 / (2.0 * double_i + 1.0) - 1.0 / (2.0 * (double_i - 1.0) + 1.0);
            value *= cauchyOmega;
            M.diagonal(-i).setConstant(value);
            M.diagonal(i).setConstant(value);
        }

        return M;
    }

    inline Eigen::MatrixXd FractionalScheme::build_force_secondordercentered(const std::vector<double> &force) const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd V = Eigen::MatrixXd::Zero(grid_size, grid_size);

        for (Eigen::Index i = 1; i < grid_size - 1; i++) {
            V(i, i - 1) -= force[i];
            V(i, i + 1) += force[i];
            V(i, i) += force[i + 1] - force[i - 1];
        }

        if (grid_size > 1) {
            V(0, 0) = -force[0];
            V(0, 1) = force[0];
            Eigen::Index last = grid_size - 1;
            V(last, last) = force[last];
            V(last, last - 1) = -force[last];
        }

        V *= config.dt / (2.0 * config.get_dx());
        return V;
    }

    inline Eigen::MatrixXd FractionalScheme::build_force_firstorderupwind(const std::vector<double> &force) const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd V = Eigen::MatrixXd::Zero(grid_size, grid_size);

        for (Eigen::Index i = 0; i < grid_size - 1; i++) {
            double f_half = 0.5 * (force[i] + force[i + 1]);

            if (f_half >= 0) {
                V(i, i) += f_half;
                V(i + 1, i) -= f_half;
            } else {
                V(i, i + 1) += f_half;
                V(i + 1, i + 1) -= f_half;
            }
        }

        if (force[0] < 0) {
            V(0, 0) -= force[0];
        }
        if (force[grid_size - 1] > 0) {
            V(grid_size - 1, grid_size - 1) += force[grid_size - 1];
        }

        V *= config.dt / config.get_dx();
        return V;
    }

    inline Eigen::MatrixXd FractionalScheme::build_force_secondorderupwind(const std::vector<double> &force) const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd V = Eigen::MatrixXd::Zero(grid_size, grid_size);

        for (Eigen::Index i = 0; i < grid_size - 1; ++i) {
            double f_half = 0.5 * (force[i] + force[i + 1]);

            if (f_half >= 0) {
                if (i == 0) {
                    V(i, i) += f_half;
                    V(i + 1, i) -= f_half;
                } else {
                    double c_i = 1.5 * f_half;
                    double c_im1 = -0.5 * f_half;

                    V(i, i) += c_i;
                    V(i, i - 1) += c_im1;
                    V(i + 1, i) -= c_i;
                    V(i + 1, i - 1) -= c_im1;
                }
            } else {
                if (i == grid_size - 2) {
                    V(i, i + 1) += f_half;
                    V(i + 1, i + 1) -= f_half;
                } else {
                    double c_ip1 = 1.5 * f_half;
                    double c_ip2 = -0.5 * f_half;

                    V(i, i + 1) += c_ip1;
                    V(i, i + 2) += c_ip2;
                    V(i + 1, i + 1) -= c_ip1;
                    V(i + 1, i + 2) -= c_ip2;
                }
            }
        }

        if (force[0] < 0) {
            V(0, 0) -= force[0];
        }
        if (force[grid_size - 1] > 0) {
            V(grid_size - 1, grid_size - 1) += force[grid_size - 1];
        }

        V *= config.dt / config.get_dx();
        return V;
    }

    inline Eigen::MatrixXd FractionalScheme::build_force_matrix(const std::vector<double> &force) const {
        switch (config.force_scheme) {
            case ForceSchemeType::SecondOrderCentered:
                return build_force_secondordercentered(force);
            case ForceSchemeType::FirstOrderUpwind:
                return build_force_firstorderupwind(force);
            case ForceSchemeType::SecondOrderUpwind:
                return build_force_secondorderupwind(force);
            default:
                throw std::logic_error("Unhandled ForceSchemeType enum value");
        }
    }

    inline Eigen::MatrixXd FractionalScheme::build_reset_matrix(const Eigen::Index reset_index) const {
        const Eigen::Index grid_size = config.get_size();
        const double reset_rate = config.reset_input.rate * config.dt;

        Eigen::MatrixXd reset_matrix = reset_rate * Eigen::MatrixXd::Identity(grid_size, grid_size);
        for (Eigen::Index i = 0; i < grid_size; ++i) {
            reset_matrix(reset_index, i) -= reset_rate;
        }

        return reset_matrix;
    }

    inline Eigen::MatrixXd FractionalScheme::build_step_matrix(const std::vector<double> &force, const Eigen::Index reset_index) const {
        const Eigen::MatrixXd diffusion_matrix = build_diffusion_matrix();
        const Eigen::MatrixXd force_matrix = build_force_matrix(force);
        const Eigen::MatrixXd reset_rate_matrix = build_reset_matrix(reset_index);

        const Eigen::MatrixXd total_operator = diffusion_matrix + force_matrix + reset_rate_matrix;

        const Eigen::Index grid_size = config.get_size();
        const Eigen::MatrixXd id = Eigen::MatrixXd::Identity(grid_size, grid_size);

        const Eigen::MatrixXd Lhs = id + config.theta * total_operator;
        const Eigen::MatrixXd Rhs = id - (1.0 - config.theta) * total_operator;

        const Eigen::PartialPivLU<Eigen::MatrixXd> solver(Lhs);
        return solver.solve(Rhs);
    }
} // namespace Schemer