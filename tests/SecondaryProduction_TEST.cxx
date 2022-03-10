#include "gtest/gtest.h"
#include <cmath>

#include "PROPOSAL/secondaries/parametrization/ionization/NaivIonization.h"
#include "PROPOSAL/secondaries/parametrization/photomupairproduction/PhotoMuPairProductionBurkhardtKelnerKokoulin.h"
#include "PROPOSAL/secondaries/parametrization/photopairproduction/PhotoPairProductionKochMotz.h"
#include "PROPOSAL/secondaries/parametrization/photopairproduction/PhotoPairProductionTsai.h"
#include "PROPOSAL/Constants.h"
#include "PROPOSAL/math/RandomGenerator.h"

using namespace PROPOSAL;

TEST(NaivIonization, EnergyMomentumConservation)
{
    auto E_i = 1e2;

    auto param = secondaries::NaivIonization(EPlusDef(), StandardRock());
    auto loss = StochasticLoss((int)InteractionType::Ioniz, 1e1, Cartesian3D(0, 0, 0), Cartesian3D(0, 0, 1), 0, 0, E_i);

    auto rnd = std::vector<double>{0.1909};
    auto secs = param.CalculateSecondaries(loss, Components::StandardRock(), rnd);

    auto p_sum = Cartesian3D(0., 0., 0.);
    double E_sum = 0.;

    for (auto sec : secs) {
        p_sum = p_sum + sec.GetMomentum() * sec.direction;
        E_sum += sec.energy;
    }

    EXPECT_EQ(secs.size(), 2);

    // check momentum conservation
    EXPECT_NEAR(p_sum.GetX(), 0., COMPUTER_PRECISION);
    EXPECT_NEAR(p_sum.GetY(), 0., COMPUTER_PRECISION);
    EXPECT_NEAR(p_sum.GetZ(), std::sqrt((E_i + ME) * (E_i - ME)), COMPUTER_PRECISION);

    // check energy conservation
    EXPECT_NEAR(E_sum, E_i + ME, (E_i + ME) * COMPUTER_PRECISION);

}

auto GetPhotoPairProductionParamList(ParticleDef particle, Medium medium) {
    auto param_list = std::vector<std::unique_ptr<secondaries::PhotoPairProduction>> {};
    param_list.push_back(std::make_unique<secondaries::PhotoPairProductionKochMotzForwardPeaked>(particle, medium));
    param_list.push_back(std::make_unique<secondaries::PhotoPairProductionTsaiForwardPeaked>(particle, medium));
    param_list.push_back(std::make_unique<secondaries::PhotoPairProductionTsai>(particle, medium));
    return param_list;
}

TEST(PhotoPairProduction, EnergyConservation)
{
    RandomGenerator::Get().SetSeed(2);

    auto E_i = 1e5;
    auto particle = GammaDef();
    auto medium = StandardRock();
    auto init_direction = Cartesian3D(0, 0, 1);
    auto init_position = Cartesian3D(0, 0, 0);
    auto loss = StochasticLoss((int)InteractionType::Photopair, E_i, init_position, init_direction, 0, 0, E_i);
    auto rnd = std::vector<double>{RandomGenerator::Get().RandomDouble(),
                                   RandomGenerator::Get().RandomDouble(),
                                   RandomGenerator::Get().RandomDouble(),
                                   RandomGenerator::Get().RandomDouble(),
                                   RandomGenerator::Get().RandomDouble()};

    auto param_list = GetPhotoPairProductionParamList(particle, medium);

    for (auto& param: param_list) {
        auto secs = param->CalculateSecondaries(loss, Components::StandardRock(), rnd);

        double E_sum = 0.;
        for (auto sec : secs) {
            E_sum += sec.energy;
            // direction should be changed, but position must stay identical
            EXPECT_NE(sec.direction, init_direction);
            EXPECT_EQ(sec.position, init_position);
        }

        // expect two secondary particles
        EXPECT_EQ(secs.size(), 2);
        // check energy conservation
        EXPECT_NEAR(E_sum, E_i, E_i * COMPUTER_PRECISION);
        // secondary particle should lay in one shared plane
        auto azimuth_difference
            = secs.at(0).direction.GetSphericalCoordinates().at(1) - secs.at(1).direction.GetSphericalCoordinates().at(1);
        EXPECT_NEAR(std::abs(azimuth_difference), PI, COMPUTER_PRECISION);
    }
}

TEST(PhotoMuPairProduction, EnergyConservation)
{
    RandomGenerator::Get().SetSeed(2);
    auto particle = GammaDef();
    auto medium = StandardRock();
    auto param = secondaries::PhotoMuPairProductionBurkhardtKelnerKokoulin(particle, medium);

    auto E_i = 1e6;
    auto init_direction = Cartesian3D(0, 0, 1);
    auto init_position = Cartesian3D(0, 0, 0);
    auto loss = StochasticLoss((int)InteractionType::Photopair, E_i, init_position, init_direction, 0, 0, E_i);

    std::vector<double> rnd;
    for (int i=0; i < param.RequiredRandomNumbers(); i++) {
        rnd.push_back(RandomGenerator::Get().RandomDouble());
    }

    auto secs = param.CalculateSecondaries(loss, medium.GetComponents().front(), rnd);

    double E_sum = 0.;
    double charge_sum = 0.;
    for (auto sec : secs) {
        E_sum += sec.energy;
        // direction should be changed, but position must stay identical
        EXPECT_NE(sec.direction, init_direction);
        EXPECT_EQ(sec.position, init_position);
    }

    // expect two secondary particles
    EXPECT_EQ(secs.size(), 2);
    // expect net charge of secondaries to be zero
    EXPECT_EQ(charge_sum, 0);
    // check energy conservation
    EXPECT_NEAR(E_sum, E_i, E_i * COMPUTER_PRECISION);
    // secondary particle should lay in one shared plane
    auto azimuth_difference = secs.at(0).direction.GetSphericalCoordinates().at(1) - secs.at(1).direction.GetSphericalCoordinates().at(1);
    EXPECT_NEAR(std::abs(azimuth_difference), PI, COMPUTER_PRECISION);

}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
