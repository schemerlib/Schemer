/**
 * @file FractionalScheme.h
 * @brief Builds matrices for fractional diffusion simulations.
 */

#pragma once

#include "Config.h"
#include <Eigen/Dense>
#include <cmath>
#include <numbers>
#include <vector>
#include <variant>

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

        /**
         * @brief Construct a new FractionalScheme object with configuration settings.
         * @param config Reference to the Config object containing simulation settings.
         */
        explicit FractionalScheme(const Config &config);

        /**
         * @brief Builds the main spatial diffusion matrix.
         *
         * Calls build_M1(), build_M2(), or build_M3() depending on the value of \alpha.
         *
         * @return Eigen::MatrixXd The spatial diffusion matrix.
         */
        Eigen::MatrixXd build_diffusion_matrix() const;

        /**
         * @brief Builds the force matrix.
         *
         * Handles both constant forces (a single number) and variable forces (a value per grid point).
         *
         * @param force_variant Either a single value (`double`) or grid values (`std::vector<double>`).
         * @return Eigen::MatrixXd The force matrix.
         */
        Eigen::MatrixXd build_force_matrix(const std::variant<double, std::vector<double> > &force_variant) const;

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
         * @param force_variant Computed force representation.
         * @param reset_index Grid index where siphoned probability is concentrated.
         * @return Eigen::MatrixXd Time step update matrix.
         */
        Eigen::MatrixXd build_step_matrix(const std::variant<double, std::vector<double> > &force_variant,
                                          Eigen::Index reset_index) const;

    private:
        /// Reference to simulation settings.
        const Config &config;

        /// Weight for the left-side fractional derivative (\omega_L).
        double L_omega;

        /// Weight for the right-side fractional derivative (\omega_R).
        double R_omega;

        /// Weight factor used when \alpha = 1.
        double cauchyOmega;

        /// \alpha rounded up.
        double n;

        /**
         * @brief Calculates fractional weight coefficients (\lambda_k^{(\alpha)}).
         *
         * Generates a list of weight values used to construct the diffusion matrix.
         *
         * @return std::vector<double> List of calculated weights.
         */
        std::vector<double> get_lambdas() const;

        /**
         * @brief Builds the diffusion matrix for 0 < \alpha < 1.
         * @return Eigen::MatrixXd Diffusion matrix for \alpha < 1.
         */
        Eigen::MatrixXd build_M1() const;

        /**
         * @brief Builds the diffusion matrix for 1 < \alpha \le 2.
         * @return Eigen::MatrixXd Diffusion matrix for \alpha > 1.
         */
        Eigen::MatrixXd build_M2() const;

        /**
         * @brief Builds the diffusion matrix for \alpha = 1.
         * @return Eigen::MatrixXd Diffusion matrix for \alpha = 1.
         */
        Eigen::MatrixXd build_M3() const;

        /**
         * @brief Builds the force matrix for a constant force.
         * @param mi Constant force value.
         * @return Eigen::MatrixXd Constant force matrix.
         */
        Eigen::MatrixXd build_drift(double mi) const;

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

    inline FractionalScheme::FractionalScheme(const Config &config) : config(config) {
        const double alpha = config.alpha;
        const double dx = config.get_dx();
        n = std::ceil(alpha);

        cauchyOmega = config.sigma * config.dt / (dx * std::numbers::pi);

        const double omega = config.sigma * config.dt / (std::tgamma(n + 1 - alpha) * std::pow(dx, alpha));
        const double denominator = 2.0 * std::cos(std::numbers::pi * alpha / 2.0);
        L_omega = -omega * (1.0 + config.beta) / denominator;
        R_omega = -omega * (1.0 - config.beta) / denominator;
    }

    inline std::vector<double> FractionalScheme::get_lambdas() const {
        const Eigen::Index grid_size = config.get_size();
        std::vector<double> result(grid_size + 2);
        for (int i = 1; i < grid_size + 2; i++) {
            result[i] = std::pow(i, n - config.alpha) - std::pow(i - 1, n - config.alpha);
        }

        return result;
    }

    inline Eigen::MatrixXd FractionalScheme::build_M1() const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);
        const auto lambda = get_lambdas();

        const double v0 = lambda[1];
        M.diagonal().setConstant(-1.0 * (L_omega + R_omega) * v0);

        for (Eigen::Index i = 1; i < grid_size; i++) {
            const double value = lambda[i] - lambda[i + 1];
            M.diagonal(-i).setConstant(value * L_omega);
            M.diagonal(i).setConstant(value * R_omega);
        }

        return M;
    }

    inline Eigen::MatrixXd FractionalScheme::build_M2() const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);
        const auto lambda = get_lambdas();

        const double v0 = lambda[2] - 2.0 * lambda[1];
        M.diagonal().setConstant(-1.0 * (L_omega + R_omega) * v0);

        const double v1 = lambda[3] - 2 * lambda[2] + lambda[1];
        const double v2 = lambda[1];
        M.diagonal(-1).setConstant(-1.0 * (L_omega * v1 + R_omega * v2));
        M.diagonal(1).setConstant(-1.0 * (R_omega * v1 + L_omega * v2));

        for (Eigen::Index i = 2; i < grid_size; i++) {
            const double value = lambda[i + 2] - 2 * lambda[i + 1] + lambda[i];
            M.diagonal(-i).setConstant(-1.0 * value * L_omega);
            M.diagonal(i).setConstant(-1.0 * value * R_omega);
        }

        return M;
    }

    inline Eigen::MatrixXd FractionalScheme::build_M3() const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);

        M.diagonal().setConstant(2.0 * cauchyOmega);

        for (Eigen::Index i = 1; i < grid_size; i++) {
            const auto double_i = static_cast<double>(i);
            double value = 1.0 / (2.0 * double_i + 1.0) - 1.0 / (2.0 * (double_i - 1) + 1.0);
            value *= cauchyOmega;
            M.diagonal(-i).setConstant(value);
            M.diagonal(i).setConstant(value);
        }

        return M;
    }

    inline Eigen::MatrixXd FractionalScheme::build_drift(const double mi) const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd V = Eigen::MatrixXd::Zero(grid_size, grid_size);
        for (Eigen::Index i = 0; i < grid_size; i++) {
            V(i, i) = 3.0;
            if (i >= 1)
                V(i, i - 1) = -4.0;
            if (i >= 2)
                V(i, i - 2) = 1.0;
        }

        const double ni = mi * config.dt / (2.0 * config.get_dx());

        if (mi < 0)
            return -1.0 * ni * V.transpose();
        return ni * V;
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
        }

        if (grid_size > 1) {
            Eigen::Index last = grid_size - 1;
            V(last, last) = force[last];
            V(last, last - 1) = -force[last];
        }

        V *= config.dt / (2 * config.get_dx());
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

        double f_left = force[0];
        if (f_left < 0) {
            V(0, 0) -= f_left;
        }

        double f_right = force[grid_size - 1];
        if (f_right > 0) {
            Eigen::Index last = grid_size - 1;
            V(last, last) += f_right;
        }

        V *= config.dt / config.get_dx();
        return V;
    }

    inline Eigen::MatrixXd FractionalScheme::build_force_secondorderupwind(const std::vector<double> &force) const {
        const Eigen::Index grid_size = config.get_size();
        Eigen::MatrixXd V = Eigen::MatrixXd::Zero(grid_size, grid_size);

        for (Eigen::Index i = 0; i < grid_size; ++i) {
            double ai = force[i]; // Local velocity/force at node i

            if (ai >= 0) {
                if (i == 0) {
                    // Left boundary (Inflow): 1st-order fallback
                    V(i, i) += ai;
                } else if (i == 1) {
                    // Near boundary: 1st-order fallback (only 1 upstream node available)
                    V(i, i)     += ai;
                    V(i, i - 1) -= ai;
                } else {
                    // Full 2nd-order upwind: (3u_i - 4u_{i-1} + u_{i-2}) / (2*dx)
                    // Multiplying by 0.5 allows us to factor out dt/dx cleanly at the end.
                    double coeff = 0.5 * ai;
                    V(i, i)     += coeff * 3.0;
                    V(i, i - 1) += coeff * (-4.0);
                    V(i, i - 2) += coeff * 1.0;
                }
            } else {
                // Negative flow (moving right to left)
                if (i == grid_size - 1) {
                    // Right boundary (Inflow): 1st-order fallback
                    V(i, i) -= ai; // Since ai is negative, -ai adds a positive magnitude
                } else if (i == grid_size - 2) {
                    // Near boundary: 1st-order fallback (only 1 upstream node available)
                    V(i, i)     -= ai;
                    V(i, i + 1) += ai;
                } else {
                    // Full 2nd-order upwind: (-3u_i + 4u_{i+1} - u_{i+2}) / (2*dx)
                    double coeff = 0.5 * ai;
                    V(i, i)     += coeff * (-3.0);
                    V(i, i + 1) += coeff * 4.0;
                    V(i, i + 2) += coeff * (-1.0);
                }
            }
        }

        V *= config.dt / config.get_dx();
        return V;
    }

    inline Eigen::MatrixXd FractionalScheme::build_diffusion_matrix() const {
        if (std::abs(config.alpha - 1.0) < ALPHA_EPSILON)
            return build_M3();
        if (config.alpha < 1.0)
            return build_M1();
        return build_M2();
    }

    inline Eigen::MatrixXd FractionalScheme::build_force_matrix(
        const std::variant<double, std::vector<double> > &force_variant) const {
        if (std::holds_alternative<double>(force_variant)) {
            return build_drift(std::get<double>(force_variant));
        }

        const auto force_vector = std::get<std::vector<double> >(force_variant);
        switch (config.force_scheme) {
            case ForceSchemeType::SecondOrderCentered:
                return build_force_secondordercentered(force_vector);
            case ForceSchemeType::FirstOrderUpwind:
                return build_force_firstorderupwind(force_vector);
            case ForceSchemeType::SecondOrderUpwind:
                return build_force_secondorderupwind(force_vector);

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

    inline Eigen::MatrixXd FractionalScheme::build_step_matrix(
        const std::variant<double, std::vector<double> > &force_variant, const Eigen::Index reset_index) const {
        const Eigen::MatrixXd diffusion_matrix = build_diffusion_matrix();
        const Eigen::MatrixXd force_matrix = build_force_matrix(force_variant);
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
