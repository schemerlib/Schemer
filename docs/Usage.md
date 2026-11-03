Usage {#usage}
======

This page shows how to use the library. Everything is available after including a single header:

```cpp
#include <Schemer/Schemer.h>
```

All classes live in the `Schemer` namespace. A typical program has three steps:

1. Configure the simulation with Schemer::Builder.
2. Build a Schemer::Runner and advance it in time with Schemer::Runner::run.
3. Read the state or write it to a file or stream.

Complete, runnable programs are in the `examples/` directory and are shown at the [end of this page](#usage_examples).

[TOC]

# Quick start {#usage_quick_start}

```cpp
#include <Schemer/Schemer.h>

int main() {
    Schemer::Runner runner = Schemer::Builder()
            .set_alpha(1.5)
            .set_length(20.0)
            .set_num_intervals(200)
            .set_dt(0.01)
            .set_log_interval_percent(0)
            .build();

    runner.run(500);                     // advance 500 steps, t = 500 * dt
    Eigen::VectorXd p = runner.get_values();
    runner.save_result("result.tsv");
}
```

By default the simulation starts from a Dirac delta at \f$x = 0\f$, which is placed in the middle of the domain. No force and no resetting are applied.

# Configuring a simulation {#usage_builder}

Every Schemer::Builder setter returns a reference to the builder, so calls can be chained. Parameters are only checked when you call `build()` or `build_setup()`. Invalid values throw `std::invalid_argument`.

## Physical parameters

| Method | Field | Default | Valid range | Meaning |
|--------|-------|---------|-------------|---------|
| `set_alpha(double)` | Schemer::Config::alpha | `0.0` (must be set) | \f$(0, 2)\f$ | Stability index. Smaller values give heavier tails. |
| `set_beta(double)` | Schemer::Config::beta | `0.0` | \f$[-1, 1]\f$ | Skewness. Must be `0` when \f$\alpha = 1\f$. |
| `set_sigma(double)` | Schemer::Config::sigma | `1.0` | \f$\ge 0\f$ | Scale (diffusion coefficient \f$K\f$). |

## Grid and time stepping

| Method | Field | Default | Meaning |
|--------|-------|---------|---------|
| `set_length(double)` | Schemer::Config::length | `40.0` | Length of the spatial domain. |
| `set_num_intervals(Eigen::Index)` | Schemer::Config::num_intervals | `1000` | Number of intervals \f$N\f$. The grid has \f$N + 1\f$ points. |
| `set_dt(double)` | Schemer::Config::dt | `0.001` | Time step \f$\Delta t\f$. A warning is printed if `dt > 1`. |
| `set_theta(double)` | Schemer::Config::theta | `0.5` | Time scheme: `0.0` explicit, `0.5` Crank–Nicolson, `1.0` implicit. |

The grid spacing is `Config::get_dx() = length / num_intervals`. The number of points is `Config::get_size() = num_intervals + 1`.

@note The step matrix is dense and of size \f$(N+1) \times (N+1)\f$. Memory use and build time grow quickly with `num_intervals`.

## Position of the origin

The origin \f$x = 0\f$ is where the initial Dirac delta is placed. It is also the default reset target.

- `set_zero_middle()` (default) puts it at index `num_intervals / 2`.
- `set_zero_index(Eigen::Index i)` puts it at grid index `i`, where \f$0 \le i \le N\f$.
- `set_zero_distance(double d)` puts it at distance `d` from the left edge, where \f$0 \le d \le L\f$. The distance is rounded to the nearest grid point, with a warning if it does not fall exactly on one.

## Initial conditions

The default is a discrete Dirac delta of height `num_intervals / length` at the origin. To start from any other profile, pass one value per grid point:

```cpp
builder.set_initial_conditions(eigen_vector);   // Eigen::VectorXd
builder.set_initial_conditions(std_vector);     // std::vector<double>
```

The vector must have exactly `num_intervals + 1` elements.

## External force

The equation contains the term \f$\partial_x \big(F(x) P(x,t)\big)\f$. The force \f$F\f$ can be given in three ways. The last one you set is used, and a warning is printed when it replaces an earlier one.

```cpp
builder.set_drift(1.0);                                     // constant force
builder.set_force([](double x) { return -2.0 * x; });       // function F(x)
builder.set_force(std::vector<double>(N + 1, 0.0));         // one value per grid point
```

The function is evaluated at the grid coordinates, measured from the origin. For position-dependent forces (a function or a vector), you can choose how the force matrix is discretised:

```cpp
builder.set_force_scheme(Schemer::ForceSchemeType::FirstOrderUpwind);     // more stable
builder.set_force_scheme(Schemer::ForceSchemeType::SecondOrderCentered);  // default, more accurate
```

## Stochastic resetting

Probability is removed from every point at a constant rate and added back at a reset target:

```cpp
builder.set_reset_rate(0.5);      // rate >= 0, 0 disables resetting (default)
builder.set_reset_index_zero();   // reset to the origin (default)
builder.set_reset_index(150);     // or reset to a given grid index
```

## Output and logging options

| Method | Default | Meaning |
|--------|---------|---------|
| `set_delimiter(std::string)` | `"\t"` | Column separator used in result and history output. |
| `set_log_interval_percent(double)` | `5.0` | Progress is logged every that many percent of a `run()` call. `0` disables logging. |
| `set_log_file_path(std::string)` | `"schemer_progress.log"` | File for progress messages. It is overwritten by each `run()` call. |
| `set_verbosity(int)` | `1` | `0` silences info messages, `1` prints "Saved to …", `>1` prints extra info. |

## Working with Config directly

All settings are stored in a Schemer::Config. You can read it, change it and pass it back. This is useful when running many similar simulations, as in a parameter sweep:

```cpp
Schemer::Config config = builder.get_config();
for (double alpha : {0.6, 1.2, 1.8}) {
    config.alpha = alpha;
    Schemer::Runner runner = Schemer::Builder().set_config(config).build();
    runner.run(1000);
}
```

You can also copy a Builder (`Schemer::Builder copy(base);`) to branch off variations from a shared base configuration.

# Running a simulation {#usage_runner}

`Builder::build()` returns a ready-to-use Schemer::Runner. If you need the precomputed data, call `Builder::build_setup()` instead. It returns a Schemer::Setup with the config, step matrix, initial state, origin index and force. You can then construct the runner yourself: `Schemer::Runner runner(setup);`.

| Method | Meaning |
|--------|---------|
| `run(int steps)` | Advance by `steps` time steps. |
| `run(int steps, std::ostream &history, int interval)` | Advance and write the state to `history` every `interval` steps. |
| `reset()` | Go back to step 0 and the initial state. |
| `get_values()` | Current state as `Eigen::VectorXd`. |
| `get_values_vector()` | Current state as `std::vector<double>`. |
| `get_setup()` | Copy of the Schemer::Setup. |

Calls to `run()` add up: `run(300); run(200);` gives the same state as `run(500);`.

# Output {#usage_output}

## Result files

`save_result(const std::string &path)` writes the configuration and the current state to a file. `save_result(std::ostream &)` does the same for any stream, such as `std::ostringstream` or `std::cout`. The format is:

```
#alpha: 1.5
#beta: 0
...
#initial_values: <N+1 values>
#force_variant: <drift value, or N+1 force values>
#x_coordinates: <N+1 positions>
#steps: 500
<N+1 values of the current state>
```

Header lines start with `#`, and values are separated by the configured delimiter. Each line of values also ends with a delimiter.

## History files

`run(steps, stream, interval)` first writes the same `#` header (without `#steps`), but only if the stream is still empty. It then writes one line per saved state: at steps `0, interval, 2*interval, …`, plus a final line after the last step. You can call `run()` several times with the same stream to append more rows. The first row of each later call repeats the last state of the previous call.

```cpp
std::ofstream history("history.csv");
runner.run(1000, history, 50);
```

## Your own format

Use the grid coordinates together with the state to write data in any layout you need:

```cpp
const Schemer::Setup setup = runner.get_setup();
const std::vector<double> x = setup.config.get_coordinates(setup.starting_index);
const std::vector<double> p = runner.get_values_vector();
const double dx = setup.config.get_dx();
// total probability: sum(p) * dx
```

# Examples {#usage_examples}

Build the examples with CMake:

```sh
cmake -S . -B build -DSCHEMER_BUILD_EXAMPLES=ON
cmake --build build
```

## Simple usage

Configure, run, read the state and save the result.

\include simple_usage.cpp

## Advanced usage

Drift, force functions and vectors, force schemes, stochastic resetting, a custom initial state and origin, editing a Config, `reset()`, `build_setup()` and error handling.

\include advanced_usage.cpp

## Output manipulation

History files, result files and streams, custom delimiters, progress logs, exporting your own CSV and computing statistics.

\include output_manipulation.cpp
