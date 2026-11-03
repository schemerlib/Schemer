\mainpage SchemerLib

Schemer is a header-only C++20 library that solves the one-dimensional space-fractional Fokker–Planck equation with an optional external force and stochastic resetting. It provides a builder-based API for configuring a simulation, running time steps, and saving results.

[TOC]

# The equation {#main_equation}

Schemer computes the time evolution of a probability density \f$P(x,t)\f$ governed by

\f{equation}{
\frac{\partial}{\partial t} P(x,t)
= \underbrace{\frac{\partial}{\partial x} \big( F(x)\,P(x,t) \big)}_{\text{force}}
+ \underbrace{K \cdot \frac{\partial^\alpha}{\partial |x|^\alpha} P(x,t)}_{\text{fractional diffusion}}
\underbrace{-\, r\,P(x,t) + r\,\delta(x - x_r)}_{\text{stochastic resetting}}
\f}

with the initial condition \f$P(x, 0) = \delta(x)\f$ by default, or any profile you supply.

The process described is a Lévy flight inside any external potential (or with a drift) with optional resetting. Applied numerical scheme is tailored for researching First-passage statistics.

## Terms and parameters {#main_parameters}

Every symbol in the equation corresponds to one setting of Schemer::Builder (stored in Schemer::Config):

| Symbol | Meaning | Builder method | Config field | Range / default |
|--------|---------|----------------|--------------|-----------------|
| \f$\alpha\f$ | Stability index, the order of the fractional derivative | `set_alpha()` | Schemer::Config::alpha | \f$(0, 2)\f$, required |
| \f$\beta\f$ | Skewness of the fractional operator | `set_beta()` | Schemer::Config::beta | \f$[-1, 1]\f$, default \f$0\f$ (must be \f$0\f$ for \f$\alpha = 1\f$) |
| \f$K\f$ | Scale (fractional diffusion coefficient) | `set_sigma()` | Schemer::Config::sigma | \f$\ge 0\f$, default \f$1\f$ |
| \f$F(x)\f$ | External force | `set_drift()`, `set_force()` | Schemer::Config::force_input | default \f$F \equiv 0\f$ |
| \f$r\f$ | Resetting rate | `set_reset_rate()` | Schemer::ResetInput::rate | \f$\ge 0\f$, default \f$0\f$ (off) |
| \f$x_r\f$ | Resetting position | `set_reset_index_zero()`, `set_reset_index()` | Schemer::Config::reset_input | default \f$x_r = 0\f$ |
| \f$P(x,0)\f$ | Initial condition | `set_initial_conditions()` | Schemer::Config::values_input | default \f$\delta(x)\f$ |

# Numerical method {#main_method}

- **Grid.** The domain of length \f$L\f$ (`set_length()`) is divided into \f$N\f$ intervals (`set_num_intervals()`), giving \f$N+1\f$ points \f$x_i = (i - i_0)\,\Delta x\f$ with \f$\Delta x = L/N\f$. The origin index \f$i_0\f$ is set with `set_zero_middle()`, `set_zero_index()` or `set_zero_distance()`.
- **Space.** The fractional term becomes a dense matrix whose form depends on whether \f$0 < \alpha < 1\f$, \f$\alpha = 1\f$, or \f$1 < \alpha < 2\f$ (see Schemer::FractionalScheme). A position-dependent force is discretised with either Schemer::ForceSchemeType::FirstOrderUpwind ( more stable) or Schemer::ForceSchemeType::SecondOrderCentered (default, more accurate).
- **Time.** With all spatial terms collected into an operator \f$A\f$, each step of size \f$\Delta t\f$ (`set_dt()`) uses the \f$\theta\f$-scheme
  \f{equation}{
  \big(I + \theta\,\Delta t\,A\big)\,P^{n+1} = \big(I - (1-\theta)\,\Delta t\,A\big)\,P^{n},
  \f}
  where \f$\theta = 0\f$ is explicit, \f$\theta = 1/2\f$ (default) is Crank–Nicolson and \f$\theta = 1\f$ is implicit (`set_theta()`). The resulting step matrix is computed once, when the simulation is built.
- **Boundaries.** Absorbing boundary conditions are used, probability that leaves the domain is lost. While acceptable for first passage time, longer simulations' results may become inaccurate.
- **Stability.** The stability of resulting matrix is heavily dependent on multiple parametres, especially \f$\alpha\f$ and force \f$F(x)\f$. Using Schemer::ForceSchemeType::FirstOrderUpwind the matrix is more stable at the cost of accuracy.

# Getting started {#main_getting_started}

```cpp
#include <Schemer/Schemer.h>

int main() {
    Schemer::Runner runner = Schemer::Builder()
            .set_alpha(1.5)      // α
            .set_beta(0.0)       // β
            .set_sigma(1.0)      // K
            .set_drift(0.5)      // F(x) = 0.5
            .set_length(20.0)
            .set_num_intervals(200)
            .set_dt(0.01)
            .build();

    runner.run(500);                    // P(x, t = 5)
    runner.save_result("result.tsv");
}
```

For installation see the [README](https://github.com/schemerlib/Schemer#installation). For the full API walkthrough and complete examples see @subpage usage.

# References {#main_references}

1. A. Padash, A. V. Chechkin, B. Dybiec, I. Pavlyukevich, B. Shokri, R. Metzler, *First-passage properties of asymmetric Lévy flights*, Journal of Physics A: Mathematical and Theoretical **52**(45), 454004 (2019).
2. A. Padash, K. Capała, H. Kantz, B. Dybiec, B. Shokri, R. Metzler, A. V. Chechkin, *First-passage statistics for Lévy flights with a drift*, Journal of Physics A: Mathematical and Theoretical **58**(18), 185002 (2025).

# Authors and license {#main_authors}

Authors:
- **Antoni Maślanka**
- **Kacper Jakubiak**

Schemer is distributed under the MIT License.
