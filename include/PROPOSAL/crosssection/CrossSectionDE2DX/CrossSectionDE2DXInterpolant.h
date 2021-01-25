
#pragma once

#include "PROPOSAL/crosssection/CrossSectionDE2DX/CrossSectionDE2DXIntegral.h"

#include <type_traits>

#include "CubicInterpolation/CubicSplines.h"
#include "CubicInterpolation/Interpolant.h"

namespace PROPOSAL {

double transform_relativ_loss(double v_cut, double v_max, double v);
double retransform_relativ_loss(double v_cut, double v_max, double v);

template <typename T1, typename T2, typename... Args>
auto build_de2dx_def(T1 const& param, T2 const& p_def, Args... args)
{
    auto de2dx
        = std::make_shared<CrossSectionDE2DXIntegral>(param, p_def, args...);
    auto def = cubic_splines::CubicSplines<double>::Definition();
    def.axis = std::make_unique<cubic_splines::ExpAxis<double>>(
        param.GetLowerEnergyLim(p_def), 1e14, (size_t)100);
    def.f = [de2dx](double E) { return de2dx->Calculate(E); };
    return def;
}

class CrossSectionDE2DXInterpolant : public CrossSectionDE2DX {

    cubic_splines::Interpolant<cubic_splines::CubicSplines<double>> interpolant;

    std::string gen_name()
    {
        return std::string("de2dx_") + std::to_string(GetHash())
            + std::string(".txt");
    }

public:
    template <typename... Args>
    CrossSectionDE2DXInterpolant(Args... args)
        : CrossSectionDE2DX(args...)
        , interpolant(build_de2dx_def(args...), "/tmp", gen_name())
    {
    }

    double Calculate(double E) final;
};
} // namespace PROPOSAL
