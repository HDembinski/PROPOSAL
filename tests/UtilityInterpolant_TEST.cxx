#include "gtest/gtest.h"

#include "PROPOSAL/propagation_utility/PropagationUtility.h"

using namespace PROPOSAL;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(IntegrationFromLowerLim, Calculate){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, lower_lim, 0), 200, 0., 1e14, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e5;
    double E_f = 1e4;
    double analytical = std::log(E_i) - std::log(E_f);
    EXPECT_NEAR(interpolant.Calculate(E_i, E_f, 0.), analytical, analytical*1e-5);
}

TEST(IntegrationFromLowerLim, ApproximationCase){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, lower_lim, 0), 200, 0., 1e14, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e7;
    double E_f = 9999999;
    EXPECT_TRUE(E_i - E_f < E_i * IPREC); //This is the condition we want to check in this UnitTest
    double analytical = std::log(E_i) - std::log(E_f);
    EXPECT_NEAR(interpolant.Calculate(E_i, E_f, 0.), analytical, analytical*1e-5);
}

TEST(IntegrationFromLowerLim, GetUpperLimit){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, lower_lim, 0), 200, 0., 1e14, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e5;
    double xi = 0.5;
    double analytical_upper = E_i * std::exp(-xi);
    EXPECT_NEAR(interpolant.GetUpperLimit(E_i, xi), analytical_upper, analytical_upper*1e-5);
}

TEST(IntegrationFromLowerLim, PlausabilityChecks){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, lower_lim, 0), 200, 0., 1e14, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    auto energies = std::array<double, 6>{1e3, 1e5, 1e7, 1e9, 1e11, 1e13};
    for(auto E_i : energies){
        for(auto E_f : energies){
            if(E_i < E_f){
                continue;
            }
            double xi = interpolant.Calculate(E_i, E_f, 0.);
            double upper = interpolant.GetUpperLimit(E_i, xi);
            EXPECT_NEAR(upper, E_f, E_f * 1e-5);
        }
    }
}

TEST(IntegrationFromMax, Calculate){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);
    double xmax = 1e14;

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, xmax, 0), 200, 0., xmax, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e5;
    double E_f = 1e4;
    double analytical = std::log(E_i) - std::log(E_f);
    EXPECT_NEAR(interpolant.Calculate(E_i, E_f, 0.), analytical, analytical*1e-5);
}

TEST(IntegrationFromMax, ApproximationCase){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);
    double xmax = 1e14;

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, xmax, 0), 200, 0., xmax, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e7;
    double E_f = 9999999;
    EXPECT_TRUE(E_i - E_f < E_i * IPREC); //This is the condition we want to check in this UnitTest
    double analytical = std::log(E_i) - std::log(E_f);
    EXPECT_NEAR(interpolant.Calculate(E_i, E_f, 0.), analytical, analytical*1e-5);
}

TEST(IntegrationFromMax, GetUpperLimit){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);
    double xmax = 1e14;

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, xmax, 0), 200, 0., xmax, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e5;
    double xi = 0.5;
    double analytical_upper = E_i * std::exp(-xi);
    EXPECT_NEAR(interpolant.GetUpperLimit(E_i, xi), analytical_upper, analytical_upper*1e-5);
}

TEST(IntegrationFromMax, PlausabilityChecks){
    auto integrand = [](double x)->double {return -1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);
    double xmax = 1e14;

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, xmax, 0), 200, 0., xmax, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    auto energies = std::array<double, 6>{1e3, 1e5, 1e7, 1e9, 1e11, 1e13};
    for(auto E_i : energies){
        for(auto E_f : energies){
            if(E_i < E_f){
                continue;
            }
            double xi = interpolant.Calculate(E_i, E_f, 0.);
            double upper = interpolant.GetUpperLimit(E_i, xi);
            EXPECT_NEAR(upper, E_f, E_f * 1e-5);
        }
    }
}

TEST(PositiveFunction, Calculate){
    auto integrand = [](double x)->double {return 1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, lower_lim, 0), 200, 0., 1e14, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e5;
    double E_f = 1e4;
    double analytical = std::log(E_f) - std::log(E_i);
    EXPECT_NEAR(interpolant.Calculate(E_i, E_f, 0.), analytical, std::abs(analytical*1e-5));
}

TEST(PositiveFunction, ApproximationCase){
    auto integrand = [](double x)->double {return 1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, lower_lim, 0), 200, 0., 1e14, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e7;
    double E_f = 9999999;
    EXPECT_TRUE(E_i - E_f < E_i * IPREC); //This is the condition we want to check in this UnitTest
    double analytical = std::log(E_f) - std::log(E_i);
    EXPECT_NEAR(interpolant.Calculate(E_i, E_f, 0.), analytical, std::abs(analytical*1e-5));
}

TEST(PositiveFunction, GetUpperLimit){
    auto integrand = [](double x)->double {return 1/x;};
    double lower_lim = 100;

    auto integral = UtilityIntegral(integrand, lower_lim);

    Interpolant1DBuilder::Definition interpol_def(
            std::bind(&UtilityIntegral::Calculate, integral, std::placeholders::_1, lower_lim, 0), 200, 0., 1e14, 5, false, false, true, 5, false, false, false);

    auto interpolant = UtilityInterpolant(integrand, lower_lim);
    interpolant.BuildTables("unittest_interpolant", 4325465, interpol_def);

    double E_i = 1e5;
    double xi = 0.5;
    double analytical_upper = E_i * std::exp(xi);
    EXPECT_NEAR(interpolant.GetUpperLimit(E_i, xi), analytical_upper, analytical_upper*1e-5);
}