#include "PROPOSAL/scattering/stochastic_deflection/nuclearInteraction/NaivNuclearInteraction.h"
#include "PROPOSAL/Constants.h"
#include "math.h"

using namespace PROPOSAL;

std::array<double, 2>
stochastic_deflection::NaivNuclearInteraction::CalculateStochasticDeflection(
        double e_i, double e_f, std::vector<double> const& rnd) const
{
    // ------- NuclInt 
    auto proton_mass = 938.272; // Proton mass in MeV
    auto muon_mass = 105.558;
    // All energies should be in units of GeV
    e_i = e_i / 1000.0;
    e_f = e_f / 1000.0;
    proton_mass = proton_mass / 1000.0;
    muon_mass = muon_mass / 1000.0;

    auto m_0 = std::sqrt(0.4);
    auto epsilon = e_i - e_f; 
    auto y = epsilon / e_i; 
    auto t_max = 2 * proton_mass * epsilon; 
    auto t_min = muon_mass * muon_mass * y * y / (1.0 - y);
    auto t_1 = std::min(epsilon * epsilon, m_0 * m_0);
    auto t_p = (t_max * t_1) / ((t_max + t_1) * pow(((t_max * (t_min + t_1)) / (t_min * (t_max + t_1))), rnd[0]) - t_max); 
    auto sin2 = (t_p - t_min) / (4 * (e_i * e_f - muon_mass * muon_mass) - 2 * t_min);
    auto theta_muon = 2 * std::asin(std::sqrt(sin2));


    //TODO: PropagationUtility will call cos() on the first return value
    return std::array<double, 2> { theta_muon, 2 * PI * rnd[1] };
}