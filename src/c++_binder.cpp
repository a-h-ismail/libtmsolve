/*
Copyright (C) 2026 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifdef LOCAL_BUILD
#include "libtmsolve.h"
#else
#include <tmsolve/libtmsolve.h>
#endif
#include "c_complex_to_cpp.h"
#include <complex>
#include <format>
#include <math.h>
#include <vector>
#ifdef PYTHON_BINDINGS_BUILD
#include "nanobind/nanobind.h"
#include "nanobind/stl/complex.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/vector.h"

namespace nb = nanobind;
using namespace nb::literals;
#endif

namespace tmsolve
{
std::complex<double> to_complex(struct cpp_double input)
{
    std::complex<double> out(input.r, input.c);
    return out;
}

struct cpp_double from_complex(std::complex<double> input)
{
    return (struct cpp_double){input.real(), input.imag()};
}

std::complex<double> solve(std::string expr)
{
    auto result = tms_solve_e(expr.c_str(), ENABLE_CMPLX, NULL);
    if (isnan(result.r))
        throw std::runtime_error("Failed to get an answer.");
    return to_complex(result);
}

int64_t int_solve(std::string expr)
{
    int64_t result;
    int status = tms_int_solve_e(expr.c_str(), &result, 0, NULL);
    if (status != 0)
        throw std::runtime_error("Failed to get an answer.");
    return result;
}

std::vector<tms_int_factor> find_factors(int32_t value)
{
    tms_int_factor *out = tms_find_factors(value);
    std::vector<tms_int_factor> result;
    for (int i = 0; out[i].power != 0; ++i)
        result.push_back(out[i]);

    return result;
}

void set_var(std::string name, std::complex<double> value, bool is_constant = false)
{
    int status = tms_set_var(name.c_str(), from_complex(value), is_constant);
    if (status == -1)
        throw std::runtime_error(std::format("Can't modify variable \"{}\" because it is constant", name));
}

void set_int_var(std::string name, int64_t value, bool is_constant = false)
{
    int status = tms_set_int_var(name.c_str(), value, is_constant);
    if (status == -1)
        throw std::runtime_error(std::format("Can't modify variable \"{}\" because it is constant", name));
}

std::complex<double> get_var(std::string name)
{
    auto var = tms_get_var_by_name(name.c_str());
    if (var == NULL)
        throw std::runtime_error(std::format("Variable \"{}\" not found", name));

    return to_complex(var->value);
}

int64_t get_int_var(std::string name)
{
    auto var = tms_get_int_var_by_name(name.c_str());
    if (var == NULL)
        throw std::runtime_error(std::format("Variable \"{}\" not found", name));

    return var->value;
}
} // namespace tmsolve

#ifdef PYTHON_BINDINGS_BUILD
using namespace tmsolve;
NB_MODULE(tmsolve, m)
{
    nb::class_<tms_int_factor>(m, "int_factor")
        .def_rw("factor", &tms_int_factor::factor)
        .def_rw("power", &tms_int_factor::power);
    m.def("tmsolve_init", &tmsolve_init);
    m.def("solve", &solve, "expr"_a);
    m.def("int_solve", &int_solve, "expr"_a);
    m.def("factor", &find_factors, "value"_a);
    m.def("set_var", &set_var, "name"_a, "value"_a, "is_constant"_a = false);
    m.def("get_var", &get_var, "name"_a);
    m.def("set_int_var", &set_int_var, "name"_a, "value"_a, "is_constant"_a = false);
    m.def("get_int_var", &get_int_var, "name"_a);
}
#endif
