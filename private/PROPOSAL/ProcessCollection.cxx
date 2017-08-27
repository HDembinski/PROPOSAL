/*
 * ProcessCollection.cxx
 *
 *  Created on: 29.04.2013
 *      Author: koehne
 */

// #include <cmath>

// #include <boost/function.hpp>
#include <boost/bind.hpp>

#include "PROPOSAL/ProcessCollection.h"
#include "PROPOSAL/Output.h"
#include "PROPOSAL/methods.h"
#include "PROPOSAL/Constants.h"


using namespace std;
using namespace PROPOSAL;


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//-------------------------public member functions----------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

double ProcessCollection::Propagate(PROPOSALParticle& particle, double distance )
{
    bool    flag;
    double  displacement;

    double propagated_distance  =   0;

    double  initial_energy  =   particle.GetEnergy();
    double  final_energy    =   particle.GetEnergy();

    bool particle_interaction = false;

    pair<double, ParticleType::Enum> decay;
    std::vector<PROPOSALParticle*> decay_products;

    pair<double, ParticleType::Enum> energy_loss;


    //TODO(mario): check Fri 2017/08/25
    // int secondary_id    =   0;

    //first: final energy befor first interaction second: energy at which the
    // particle decay
    //first and second are compared to decide if interaction happens or decay
    pair<double,double> energy_till_stochastic_;


    if(distance < 0)
    {
        distance   =   0;
    }

    if(initial_energy <= particle.GetLow() || distance==0)
    {
        flag    =   false;
    }
    else
    {
        flag    =   true;
    }

    while(flag)
    {
        energy_till_stochastic_ = CalculateEnergyTillStochastic(particle, initial_energy );
        if(energy_till_stochastic_.first > energy_till_stochastic_.second)
        {
            particle_interaction   =   true;
            final_energy            =   energy_till_stochastic_.first;
        }
        else
        {
            particle_interaction   =   false;
            final_energy            =   energy_till_stochastic_.second;

        }

        //Calculate the displacement according to initial energy initial_energy and final_energy
        displacement =
            CalculateDisplacement(
                particle, initial_energy, final_energy, density_correction_ * (distance - propagated_distance)) /
            density_correction_;

        // The first interaction or decay happens behind the distance we want to propagate
        // So we calculate the final energy using only continuous losses
        if( displacement > distance - propagated_distance )
        {
            displacement  =   distance - propagated_distance;

            final_energy  =   CalculateFinalEnergy(particle, initial_energy, density_correction_*displacement);

        }
        //Advance the Particle according to the displacement
        //Initial energy and final energy are needed if Molier Scattering is enabled
        AdvanceParticle(particle, displacement, initial_energy, final_energy);

        propagated_distance +=  displacement;

        if(abs(distance - propagated_distance) < abs(distance)*COMPUTER_PRECISION)
        {
            propagated_distance = distance;  // computer precision control
        }
        //Randomize the continuous energy loss if this option is enabled
        if( do_continuous_randomization_)
        {
            if(final_energy != particle.GetLow())
            {
                final_energy  = Randomize( initial_energy, final_energy );
            }

        }
        // Lower limit of particle energy is reached or
        // or complete particle is propagated the whole distance
        if( final_energy == particle.GetLow() || propagated_distance == distance)
        {
            break;
        }

        //Set the particle energy to the current energy before making
        //stochatic losses or decay
        particle.SetEnergy( final_energy );

        if(particle_interaction)
        {
            energy_loss     =   MakeStochasticLoss(particle);
            if (energy_loss.second == ParticleType::unknown)
            {
                // in this case, no cross section is chosen, so there is no interaction
                // due to the parameterization of the cross section cutoffs
                log_debug("no interaction due to the parameterization of the cross section cutoffs. final energy: %f\n", final_energy);
                initial_energy = final_energy;
                continue;
            }
            final_energy    -=  energy_loss.first;
            // log_debug("Energyloss: %f\t%s", energy_loss.first, PROPOSALParticle::GetName(energy_loss.second).c_str());
            // //TODO(mario): hack Thu 2017/08/24
            Output::getInstance().FillSecondaryVector(&particle, ParticleDef(BremsDef::Get()), energy_loss.first, 0);
            // secondary_id    =   particle.GetParticleId() + 1;
            // Output::getInstance().FillSecondaryVector(&particle, secondary_id, energy_loss, 0);
        }
        else
        {
            DecayChannel* mode = &particle.GetDecayTable().SelectChannel();
            decay_products = mode->Decay(&particle);
            Output::getInstance().FillSecondaryVector(decay_products);

            //TODO(mario): Delete decay products Tue 2017/08/22

            // decay           =   current_collection_->MakeDecay();
            // final_energy    =   0;
            // log_debug("Decay of particle: %s", particle_->GetName().c_str());
            // secondary_id    = particle_->GetParticleId()  +   1;
            // Output::getInstance().FillSecondaryVector(particle_, secondary_id, decay ,0);

        }

        //break if the lower limit of particle energy is reached
        if(final_energy <= particle.GetLow())
        {
            break;
        }

        //Next round: update the inital energy
        initial_energy  =   final_energy;

    }

    // if(stopping_decay_)
    // {
    //     if(propagated_distance!=distance && final_energy!=0 && particle_->GetLifetime()>=0)
    //     {
    //         particle_->SetEnergy(particle_->GetMass());
    //
    //         double t    =   particle_->GetT() -particle_->GetLifetime()*log(RandomDouble());
    //         double product_energy   =   0;
    //
    //         pair<double, ParticleType::Enum> decay_to_store;
    //         secondary_id    =   particle_->GetParticleId() + 1;
    //
    //         particle_->SetT( t );
    //
    //         if(particle_->GetType()==2)
    //         {
    //             // --------------------------------------------------------------------- //
    //             // Calculate random numbers before passing to a fuction, because
    //             // the order of argument evaluation is unspecified in c++ standards and
    //             // therfor depend on the compiler.
    //             // --------------------------------------------------------------------- //
    //
    //             double rnd1 = RandomDouble();
    //             double rnd2 = RandomDouble();
    //
    //             product_energy  =   current_collection_->GetDecay()->CalculateProductEnergy(rnd1, 0.5, rnd2);
    //         }
    //         else
    //         {
    //             product_energy  =   current_collection_->GetDecay()->CalculateProductEnergy(RandomDouble(), 0.5, 0.5);
    //         }
    //
    //         decay_to_store.first    =   product_energy;
    //         decay_to_store.second   =   current_collection_->GetDecay()->GetOut();
    //
    //         final_energy  =   0;
    //
    //         Output::getInstance().FillSecondaryVector(particle_,secondary_id, decay_to_store, final_energy);
    //     }
    // }

    particle.SetEnergy(final_energy);

    //Particle reached the border, final energy is returned
    if(propagated_distance==distance)
    {
        return final_energy;
    }
    //The particle stopped/decayed, the propageted distance is return with a minus sign
    else
    {
        return -propagated_distance;
    }
    //Should never be here
    return 0;
}

std::pair<double,double> ProcessCollection::CalculateEnergyTillStochastic(const PROPOSALParticle& particle, double initial_energy )
{
    double rndd    =-  log(RandomDouble());
    double rndi    =-  log(RandomDouble());

    double rndiMin = 0;
    double rnddMin = 0;

    pair<double,double> final;

    //solving the tracking integral
    if(particle.GetLifetime() < 0)
    {
        rnddMin =   0;
    }
    else
    {
        rnddMin =   CalculateTrackingIntegal(particle, initial_energy, rndd, false)/density_correction_;
    }

    rndiMin =   CalculateTrackingIntegal(particle, initial_energy, rndi, true);
    //evaluating the energy loss
    if(rndd >= rnddMin || rnddMin<=0)
    {
        final.second =   particle.GetLow();
    }
    else
    {
        final.second =   CalculateFinalEnergy(particle, initial_energy, rndd*density_correction_, false );
    }

    if(rndi >= rndiMin || rndiMin <= 0)
    {
        final.first =   particle.GetLow();
    }
    else
    {
        final.first =   CalculateFinalEnergy(particle, initial_energy, rndi, true );
    }

    return final;
}

double ProcessCollection::CalculateDisplacement(const PROPOSALParticle& particle, double ei, double ef, double dist)
{
    if(do_interpolation_)
    {
        if(fabs(ei-ef) > fabs(ei)*HALF_PRECISION)
        {
            double aux;

            ini_    =   interpolant_->Interpolate(ei);
            aux     =   ini_ - interpolant_->Interpolate(ef);

            if(fabs(aux) > fabs(ini_)*HALF_PRECISION)
            {
                return max(aux, 0.0);
            }

        }

        ini_    =   0;
        return max((interpolant_diff_->Interpolate((ei + ef)/2))*(ef - ei), 0.0);

    }
    else
    {
        return integral_->IntegrateWithRandomRatio(ei, ef, boost::bind(&ProcessCollection::FunctionToIntegral, this, boost::ref(particle), _1), 4, -dist);
    }

}

void ProcessCollection::AdvanceParticle(PROPOSALParticle& particle, double dr, double ei, double ef)
{

    double dist = particle.GetPropagatedDistance();
    double time = particle.GetT();
    Vector3D position = particle.GetPosition();

    dist   +=  dr;

    if(do_exact_time_calculation_)
    {
        time   +=  CalculateParticleTime(particle, ei, ef)/ density_correction_;
    }
    else
    {
        time   +=  dr/SPEED;
    }


    //TODO(mario): Adjucst the whole scatteing class Thu 2017/08/24
    if(do_scattering_)
    {
        scattering_->Scatter(dr, ei, ef);
    }

    // if(scattering_model_!=-1)
    // {
    //     switch(scattering_model_)
    //     {
    //         case 0:
    //             current_collection_->GetScattering()->Scatter(dr,ei,ef);
    //             break;
    //
    //         case 1:
    //             scatteringFirstOrder_->Scatter(dr, particle_, current_collection_->GetMedium());
    //             break;
    //
    //         case 2:
    //             scatteringFirstOrderMoliere_->Scatter(dr, particle_, current_collection_->GetMedium());
    //             break;
    //         default:
    //             log_error("Never should be here! scattering_model = %i !",scattering_model_);
    //     }
    //
    // }
    // else
    // {
    //     position = position + dr*particle.GetDirection();
    //     particle.SetPosition(position);
    // }

    particle.SetPropagatedDistance(dist);
    particle.SetT(time);
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::CalculateTrackingIntegal(const PROPOSALParticle& particle, double initial_energy, double rnd, bool particle_interaction)
{
    if(do_interpolation_)
    {
        if(particle_interaction)
        {
            storeDif_.at(1) =   interpol_prop_interaction_->Interpolate(initial_energy);
        }
        else
        {
            storeDif_.at(0) =   interpol_prop_decay_->Interpolate(initial_energy);
        }

        if(up_&&particle_interaction)
        {
            if(particle_interaction)
            {
                return max(storeDif_.at(1), 0.0);
            }
            else
            {
                return max(storeDif_.at(0), 0.0);
            }
        }
        else
        {
            if(particle_interaction)
            {
                return max(bigLow_.at(1)-storeDif_.at(1), 0.0);
            }
            else
            {
                return max(bigLow_.at(0)-storeDif_.at(0), 0.0);
            }
        }
    }
    else
    {

        if(particle_interaction)
        {
            return prop_interaction_->IntegrateWithRandomRatio(initial_energy, particle.GetLow(), boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::ref(particle), _1), 4, -rnd);
        }
        else
        {
            return prop_decay_->IntegrateWithRandomRatio(initial_energy, particle.GetLow(), boost::bind(&ProcessCollection::FunctionToPropIntegralDecay, this, boost::ref(particle), _1), 4, -rnd);
        }
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

//Formerly: double CrossSections::getef(double ei, double dist)
double ProcessCollection::CalculateFinalEnergy(const PROPOSALParticle& particle, double ei, double dist)
{
    if(do_interpolation_)
    {
        if(ini_ != 0)
        {
            double aux;
            aux     =   interpolant_->FindLimit(ini_-dist);

            if(fabs(aux) > fabs(ei)*HALF_PRECISION)
            {
                return min( max(aux, particle.GetLow()), ei);
            }
        }

        return min( max(ei+dist/interpolant_diff_->Interpolate(ei + dist/(2*interpolant_diff_->Interpolate(ei))), particle.GetLow()), ei);
    }
    else
    {
        return integral_->GetUpperLimit();
    }

}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

double ProcessCollection::CalculateFinalEnergy(const PROPOSALParticle& particle, double ei, double rnd, bool particle_interaction)
{
    if( do_interpolation_ )
    {
        if( particle_interaction )
        {
            if( abs(rnd) > abs(storeDif_.at(1))*HALF_PRECISION)
            {
                double aux;

                if(up_&&particle_interaction)
                {
                    if(particle_interaction)
                    {
                        aux =   interpol_prop_interaction_->FindLimit(storeDif_.at(1)-rnd);
                    }
                    else
                    {
                        aux =   interpol_prop_decay_->FindLimit(storeDif_.at(0)-rnd);
                    }
                }
                else
                {
                    if(particle_interaction)
                    {
                        aux =   interpol_prop_interaction_->FindLimit(storeDif_.at(1)+rnd);
                    }
                    else
                    {
                        aux =   interpol_prop_decay_->FindLimit(storeDif_.at(0)+rnd);
                    }
                }
                if(abs(ei-aux) > abs(ei)*HALF_PRECISION)
                {
                    return min(max(aux, particle.GetLow()), ei);
                }
            }
        }
        else
        {
            if(abs(rnd) > abs(storeDif_.at(0))*HALF_PRECISION)
            {
                double aux;

                if(up_&&particle_interaction)
                {
                    if(particle_interaction)
                    {
                        aux =   interpol_prop_interaction_->FindLimit(storeDif_.at(1)-rnd);
                    }
                    else
                    {
                        aux =   interpol_prop_decay_->FindLimit(storeDif_.at(0)-rnd);
                    }
                }
                else
                {
                    if(particle_interaction)
                    {
                        aux =   interpol_prop_interaction_->FindLimit(storeDif_.at(1)+rnd);
                    }
                    else
                    {
                        aux =   interpol_prop_decay_->FindLimit(storeDif_.at(0)+rnd);
                    }
                }

                if(abs(ei-aux) > abs(ei)*HALF_PRECISION)
                {
                    return min(max(aux, particle.GetLow()), ei);
                }
            }
        }

        if(particle_interaction)
        {
            return min(max(ei + rnd/interpol_prop_interaction_diff_->Interpolate(ei + rnd/(2*interpol_prop_interaction_diff_->Interpolate(ei))), particle.GetLow()), ei);
        }
        else
        {
            return min(max(ei + rnd/interpol_prop_decay_diff_->Interpolate(ei + rnd/(2*interpol_prop_decay_diff_->Interpolate(ei))), particle.GetLow()), ei);
        }
    }
    else
    {
        if(particle_interaction)
        {
            return prop_interaction_->GetUpperLimit();
        }
        else
        {
            return prop_decay_->GetUpperLimit();
        }

    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


// pair<double, ParticleType::Enum> ProcessCollection::MakeStochasticLoss()
// {
//     // --------------------------------------------------------------------- //
//     // Calculate random numbers before passing to a fuction, because
//     // the order of argument evaluation is unspecified in c++ standards and
//     // therefore depend on the compiler.
//     // --------------------------------------------------------------------- //
//
//     double rnd1 = MathModel::RandomDouble();
//     double rnd2 = MathModel::RandomDouble();
//     double rnd3 = MathModel::RandomDouble();
//
//     return this->MakeStochasticLoss(rnd1, rnd2, rnd3);
// }


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


pair<double, ParticleType::Enum> ProcessCollection::MakeStochasticLoss(const PROPOSALParticle& particle)
{
    double rnd1 = RandomGenerator::Get().RandomDouble();
    double rnd2 = RandomGenerator::Get().RandomDouble();
    double rnd3 = RandomGenerator::Get().RandomDouble();

    double total_rate          =    0;
    double total_rate_weighted =    0;
    double rates_sum           =    0;

    // return 0 and unknown, if there is no interaction
    pair<double, ParticleType::Enum> energy_loss;
    energy_loss.first  = 0.;
    energy_loss.second = ParticleType::unknown;

    std::vector<double> rates;

    rates.resize( crosssections_.size() );

    if(do_weighting_)
    {
        if(particle.GetPropagatedDistance() > weighting_starts_at_)
        {
            double exp      =   abs(weighting_order_);
            double power    =   pow(rnd2, exp);

            if(weighting_order_>0)
            {
                rnd2    =   1 - power*rnd2;
            }
            else
            {
                rnd2    =   power*rnd2;
            }

            weighting_order_        =   (1 + exp)*power;
            weighting_starts_at_    =   particle.GetPropagatedDistance();
            do_weighting_           =   false;
        }
    }
    // if (particle_->GetEnergy() < 650) printf("energy: %f\n", particle_->GetEnergy());
    for(unsigned int i = 0 ; i < GetCrosssections().size(); i++)
    {
        rates.at(i) =  crosssections_.at(i)->CalculatedNdx( particle, rnd2 );
        total_rate  +=  rates.at(i);
        // if (rates.at(i) == 0) printf("%i = 0, energy: %f\n", i, particle_->GetEnergy());
        log_debug("Rate for %s = %f",crosssections_.at(i)->GetName().c_str(), rates.at(i));

    }

    total_rate_weighted = total_rate*rnd1;

    log_debug("Total rate = %f, total rate weighted = %f",total_rate ,total_rate_weighted);

    for(unsigned int i = 0 ; i < rates.size(); i++)
    {
        rates_sum   += rates.at(i);

        if(rates_sum > total_rate_weighted)
        {
            energy_loss.first   =   crosssections_.at(i)->CalculateStochasticLoss(particle, rnd2,rnd3);
            energy_loss.second  =   crosssections_.at(i)->GetType();
            break;
        }
    }

    return energy_loss;

}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

// DecayChannel::DecayProducts ProcessCollection::MakeDecay()
// {
//     const DecayChannel* mode = particle_->GetDecayTable().SelectChannel();
//     return mode->Decay(particle_);
// }

// pair<double, ParticleType::Enum> ProcessCollection::MakeDecay()
// {
//     // --------------------------------------------------------------------- //
//     // Calculate random numbers before passing to a fuction, because
//     // the order of argument evaluation is unspecified in c++ standards and
//     // therfor depend on the compiler.
//     // --------------------------------------------------------------------- //
//
//     pair<double, ParticleType::Enum> decay_pair;
//     const DecayChannel* mode = particle_->GetDecayTable().SelectChannel();
//     decay_pair.first = mode->Decay(particle_);
//
//     // double rnd1 = MathModel::RandomDouble();
//     // double rnd2 = MathModel::RandomDouble();
//     // double rnd3 = MathModel::RandomDouble();
//     //
//     // return MakeDecay(rnd1, rnd2, rnd3);
// }


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//



// pair<double, ParticleType::Enum> ProcessCollection::MakeDecay(double rnd1,double rnd2, double rnd3)
// {
//     pair<double, ParticleType::Enum> decay;
//
//     if(particle_->GetType() == ParticleType::TauPlus || particle_->GetType() == ParticleType::TauMinus)
//     {
//         decay.first     =   decay_->CalculateProductEnergy(rnd1, rnd2, rnd3);
//     }
//     else
//     {
//         decay.first     =   decay_->CalculateProductEnergy(rnd2, rnd3, 0.5);
//     }
//
//     decay.second    =   decay_->GetOut();
//
//     return decay;
// }


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::CalculateParticleTime(const PROPOSALParticle& particle, double ei, double ef)
{
    if(do_time_interpolation_)
    {
        if(abs(ei-ef) > abs(ei)*HALF_PRECISION)
        {
            double aux  =   interpol_time_particle_->Interpolate(ei);
            double aux2 =   aux - interpol_time_particle_->Interpolate(ef);

            if(abs(aux2) > abs(aux)*HALF_PRECISION)
            {
                return aux2;
            }
        }

        return interpol_time_particle_diff_->Interpolate( (ei+ef)/2 )*(ef-ei);
    }
    else
    {
        return time_particle_->Integrate(ei, ef, boost::bind(&ProcessCollection::FunctionToTimeIntegral, this, boost::ref(particle), _1),4);
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::Randomize(double initial_energy, double final_energy)
{
    double rnd = MathModel::RandomDouble();
    return randomizer_->Randomize( *particle_, crosssections_, initial_energy, final_energy, rnd );
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//--------------------Enable and Disable interpolation------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::EnableInterpolation(PROPOSALParticle& particle, std::string path, bool raw)
{
    if(do_interpolation_)return;

    EnableDEdxInterpolation(particle, path,raw);
    EnableDNdxInterpolation(particle, path,raw);

    double energy = particle.GetEnergy();

    bool reading_worked =   true;
    bool storing_failed =   false;

    double a = abs(-prop_interaction_->Integrate(particle.GetLow(), particle.GetLow()*10, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1),4));
    double b = abs(-prop_interaction_->Integrate(BIGENERGY, BIGENERGY/10, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1),4));

    if( a < b)
    {
        up_  =   true;
    }
    else
    {
        up_  =   false;
    }

    particle.SetEnergy(energy); //(mario) Previous if condition changes the particle energy

    // charged anti leptons have the same cross sections like charged leptons
    // (except of diffractive Bremsstrahlung, where one can analyse the interference term if implemented)
    // so they use the same interpolation tables
    string particle_name = particle.GetName();

    if(!path.empty())
    {
        stringstream filename;
        filename<<path<<"/Collection"
                <<"_"<<particle_name
                <<"_mass_"<<particle.GetMass()
                <<"_charge_"<<particle.GetCharge()
                <<"_lifetime_"<<particle.GetLifetime()
                <<"_"<<medium_->GetName()
                <<"_"<<medium_->GetMassDensity()
                <<"_"<<cut_settings_->GetEcut()
                <<"_"<<cut_settings_->GetVcut();

        for(unsigned int i =0; i<crosssections_.size(); i++)
        {
            switch (crosssections_.at(i)->GetType())
            {
                case ParticleType::Brems:
                    filename << "_b"
                        << "_" << crosssections_.at(i)->GetParametrization()
                        << "_" << crosssections_.at(i)->GetLpmEffectEnabled();
                    break;
                case ParticleType::DeltaE:
                    filename << "_i";
                    break;
                case ParticleType::EPair:
                    filename << "_e"
                        << "_" << crosssections_.at(i)->GetLpmEffectEnabled();
                    break;
                case ParticleType::NuclInt:
                    filename << "_p"
                        << "_" << crosssections_.at(i)->GetParametrization();
                    break;
                default:
                    log_fatal("Unknown cross section");
                    exit(1);
            }
            filename<< "_" << crosssections_.at(i)->GetMultiplier()
                    << "_" << crosssections_.at(i)->GetEnergyCutSettings()->GetEcut()
                    << "_" << crosssections_.at(i)->GetEnergyCutSettings()->GetVcut();
        }

        if(!raw)
            filename<<".txt";

        if( FileExist(filename.str()) )
        {
            log_info("ProcessCollection parametrisation tables will be read from file:\t%s",filename.str().c_str());
            ifstream input;

            if(raw)
            {
                input.open(filename.str().c_str(), ios::binary);
            }
            else
            {
                input.open(filename.str().c_str());
            }

            interpolant_        =   new Interpolant();
            interpolant_diff_   =   new Interpolant();

            reading_worked = interpolant_->Load(input,raw);
            reading_worked = interpolant_diff_->Load(input,raw);


            interpol_prop_decay_            =   new Interpolant();
            interpol_prop_decay_diff_       =   new Interpolant();
            interpol_prop_interaction_      =   new Interpolant();
            interpol_prop_interaction_diff_ =   new Interpolant();

            reading_worked = interpol_prop_decay_->Load(input,raw);
            reading_worked = interpol_prop_decay_diff_->Load(input,raw);
            reading_worked = interpol_prop_interaction_->Load(input,raw);
            reading_worked = interpol_prop_interaction_diff_->Load(input,raw);

            input.close();
        }
        if(!FileExist(filename.str()) || !reading_worked )
        {

            if(!reading_worked)
            {
                log_info("File %s is corrupted! Write it again!",filename.str().c_str());
            }

            log_info("ProcessCollection parametrisation tables will be saved to file:\t%s",filename.str().c_str());

            if(abs(-prop_interaction_->Integrate(particle.GetLow(), particle.GetLow()*10, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1),4))
                    < abs(-prop_interaction_->Integrate(BIGENERGY, BIGENERGY/10, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1),4)))
            {
                up_  =   true;
            }
            else
            {
                up_  =   false;
            }

            particle.SetEnergy(energy);

            ofstream output;

            if(raw)
            {
                output.open(filename.str().c_str(), ios::binary);
            }
            else
            {
                output.open(filename.str().c_str());
            }

            if(output.good())
            {
                output.precision(16);

                interpolant_        =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToBuildInterpolant, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
                interpolant_diff_   =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToIntegral, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);

                particle.SetEnergy(energy);

                interpol_prop_decay_            =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::InterpolPropDecay, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
                interpol_prop_decay_diff_       =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralDecay, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
                interpol_prop_interaction_      =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::InterpolPropInteraction, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
                interpol_prop_interaction_diff_ =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);

                particle.SetEnergy(energy);

                interpolant_->Save(output,raw);
                interpolant_diff_->Save(output,raw);
                interpol_prop_decay_->Save(output,raw);
                interpol_prop_decay_diff_->Save(output,raw);
                interpol_prop_interaction_->Save(output,raw);
                interpol_prop_interaction_diff_->Save(output,raw);

            }
            else
            {
                storing_failed  =   true;
                log_warn("Can not open file %s for writing! Table will not be stored!",filename.str().c_str());
            }
            particle.SetEnergy(energy);

            output.close();
        }
    }
    if(path.empty() || storing_failed)
    {
        log_info("ProcessCollection parametrisation tables will be stored in memory!");

        double energy = particle.GetEnergy();

        interpolant_        =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToBuildInterpolant, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
        interpolant_diff_   =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToIntegral, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);

        particle.SetEnergy(energy);

        interpol_prop_decay_            =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::InterpolPropDecay, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
        interpol_prop_decay_diff_       =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralDecay, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
        interpol_prop_interaction_      =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::InterpolPropInteraction, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
        interpol_prop_interaction_diff_ =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);

        particle.SetEnergy(energy);

    }

    if(do_continuous_randomization_)
    {
        randomizer_->EnableDE2dxInterpolation(*particle_, crosssections_, path ,raw);
        randomizer_->EnableDE2deInterpolation(*particle_, crosssections_, path,raw);
    }

    if(do_exact_time_calculation_)
    {
        EnableParticleTimeInterpolation(particle, path,raw);
    }

    if(do_scattering_)
    {
        scattering_->EnableInterpolation(path);
    }

    bigLow_.at(0)=interpol_prop_decay_->Interpolate(particle.GetLow());
    bigLow_.at(1)=interpol_prop_interaction_->Interpolate(particle.GetLow());

    do_interpolation_=true;

}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//



void ProcessCollection::EnableDEdxInterpolation(PROPOSALParticle& particle, std::string path, bool raw)
{
    for(unsigned int i =0 ; i < crosssections_.size() ; i++)
    {
        crosssections_.at(i)->EnableDEdxInterpolation(particle, path,raw);
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//



void ProcessCollection::EnableDNdxInterpolation(PROPOSALParticle& particle, std::string path, bool raw)
{
    for(unsigned int i =0 ; i < crosssections_.size() ; i++)
    {
        crosssections_.at(i)->EnableDNdxInterpolation(particle, path,raw);
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableDEdxInterpolation()
{
    for(unsigned int i =0 ; i < crosssections_.size() ; i++)
    {
        crosssections_.at(i)->DisableDEdxInterpolation();
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableDNdxInterpolation()
{
    for(unsigned int i =0 ; i < crosssections_.size() ; i++)
    {
        crosssections_.at(i)->DisableDNdxInterpolation();
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableInterpolation()
{
    do_interpolation_  =   false;
    DisableDEdxInterpolation();
    DisableDNdxInterpolation();
    DisableParticleTimeInterpolation();

}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::EnableParticleTimeInterpolation(PROPOSALParticle& particle, std::string path, bool raw)
{

    if(do_time_interpolation_)return;

    bool reading_worked =   true;
    bool storing_failed =   false;

    // charged anti leptons have the same cross sections like charged leptons
    // (except of diffractive Bremsstrahlung, where one can analyse the interference term if implemented)
    // so they use the same interpolation tables
    string particle_name = particle.GetName();

    if(!path.empty())
    {
        stringstream filename;
        filename<<path<<"/Time"
                <<"_"<<particle_name
                <<"_mass_"<<particle.GetMass()
                <<"_charge_"<<particle.GetCharge()
                <<"_lifetime_"<<particle.GetLifetime()
                <<"_"<<medium_->GetName()
                <<"_"<<medium_->GetMassDensity()
                <<"_"<<cut_settings_->GetEcut()
                <<"_"<<cut_settings_->GetVcut();

        for(unsigned int i =0; i<crosssections_.size(); i++)
        {
            switch (crosssections_.at(i)->GetType())
            {
                case ParticleType::Brems:
                    filename << "_b"
                        << "_" << crosssections_.at(i)->GetParametrization()
                        << "_" << crosssections_.at(i)->GetLpmEffectEnabled();
                    break;
                case ParticleType::DeltaE:
                    filename << "_i";
                    break;
                case ParticleType::EPair:
                    filename << "_e"
                        << "_" << crosssections_.at(i)->GetLpmEffectEnabled();
                    break;
                case ParticleType::NuclInt:
                    filename << "_p"
                        << "_" << crosssections_.at(i)->GetParametrization();
                    break;
                default:
                    log_fatal("Unknown cross section");
                    exit(1);
            }
            filename<< "_" << crosssections_.at(i)->GetMultiplier()
                    << "_" << crosssections_.at(i)->GetEnergyCutSettings()->GetEcut()
                    << "_" << crosssections_.at(i)->GetEnergyCutSettings()->GetVcut();

        }

        if(!raw)
            filename<<".txt";

        if( FileExist(filename.str()) )
        {
            log_debug("Particle time parametrisation tables will be read from file:\t%s",filename.str().c_str());
            ifstream input;

            if(raw)
            {
                input.open(filename.str().c_str(), ios::binary);
            }
            else
            {
                input.open(filename.str().c_str());
            }

            interpol_time_particle_         = new Interpolant();
            interpol_time_particle_diff_    = new Interpolant();

            reading_worked = interpol_time_particle_->Load(input,raw);
            reading_worked = interpol_time_particle_diff_->Load(input,raw);

            input.close();
        }
        if(!FileExist(filename.str()) || !reading_worked )
        {
            if(!reading_worked)
            {
                log_info("File %s is corrupted! Write it again!",filename.str().c_str());
            }

            log_info("Particle time parametrisation tables will be saved to file:\t%s",filename.str().c_str());

            double energy = particle.GetEnergy();

            ofstream output;

            if(raw)
            {
                output.open(filename.str().c_str(), ios::binary);
            }
            else
            {
                output.open(filename.str().c_str());
            }

            if(output.good())
            {
                output.precision(16);

                interpol_time_particle_         =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToTimeIntegral, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
                interpol_time_particle_diff_    =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);

                interpol_time_particle_->Save(output,raw);
                interpol_time_particle_diff_->Save(output,raw);

            }
            else
            {
                storing_failed  =   true;
                log_warn("Can not open file %s for writing! Table will not be stored!",filename.str().c_str());
            }
            particle.SetEnergy(energy);

            output.close();
        }
    }
    if(path.empty() || storing_failed)
    {
        double energy = particle.GetEnergy();

        interpol_time_particle_         =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToTimeIntegral, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);
        interpol_time_particle_diff_    =   new Interpolant(NUM3, particle.GetLow(), BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1), order_of_interpolation_, false, false, true, order_of_interpolation_, false, false, false);

        particle.SetEnergy(energy);
    }

    do_time_interpolation_ =true;

}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableParticleTimeInterpolation()
{
    delete interpol_time_particle_;
    delete interpol_time_particle_diff_;

    interpol_time_particle_         = NULL;
    interpol_time_particle_diff_    = NULL;

    do_time_interpolation_ =false;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//-------------------------lpm effect /randomization--------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//



void ProcessCollection::EnableLpmEffect()
{
    lpm_effect_enabled_ =true;
    for(unsigned int i=0;i<crosssections_.size();i++){
        crosssections_.at(i)->EnableLpmEffect(lpm_effect_enabled_);
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableLpmEffect()
{
    lpm_effect_enabled_ =false;
    for(unsigned int i=0;i<crosssections_.size();i++){
        crosssections_.at(i)->EnableLpmEffect(lpm_effect_enabled_);
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::EnableContinuousRandomization()
{
    randomizer_ =   new ContinuousRandomization();
    do_continuous_randomization_     =   true;
}
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableContinuousRandomization()
{
    delete  randomizer_;
    randomizer_ =   NULL;
    do_continuous_randomization_     =   false;
}



//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableScattering()
{
    delete  scattering_;
    scattering_        =   NULL;
    do_scattering_      =   false;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::EnableScattering()
{
    scattering_     =   new Scattering(crosssections_);
    do_scattering_      =   true;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::EnableExactTimeCalculation()
{
    do_exact_time_calculation_   =   true;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::DisableExactTimeCalculation()
{
    do_exact_time_calculation_   =   false;
}



//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//--------------------------------constructors--------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


//Standard constructor
ProcessCollection::ProcessCollection()
    :order_of_interpolation_     ( 5 )
    ,do_interpolation_           ( false )
    ,lpm_effect_enabled_         ( false )
    ,ini_                        ( 0 )
    ,debug_                      ( false )
    ,do_weighting_               ( false )
    ,weighting_order_            ( 0 )
    ,weighting_starts_at_        ( 0 )
    ,enable_randomization_       ( false )
    ,do_continuous_randomization_( false )
    ,do_scattering_              ( false )
    ,location_                   ( 0 )
    ,density_correction_         ( 1. )
    ,do_time_interpolation_      ( false )
    ,do_exact_time_calculation_   ( false )
    ,up_                         ( false )
    ,bigLow_                     ( 2,0 )
    ,storeDif_                   ( 2,0 )
    ,crosssections_              ( )
{

    interpolant_                    = NULL;
    interpolant_diff_               = NULL;
    particle_                       = new PROPOSALParticle();
    backup_particle_                = particle_;
    medium_                         = new Water();
    cut_settings_                   = new EnergyCutSettings();
    integral_                       = new Integral(IROMB, IMAXS, IPREC2);
    prop_decay_                     = new Integral(IROMB, IMAXS, IPREC2);
    prop_interaction_               = new Integral(IROMB, IMAXS, IPREC2);
    decay_                          = new Decay();
    time_particle_                  = new Integral(IROMB, IMAXS, IPREC2);

    interpol_time_particle_         = NULL;
    interpol_time_particle_diff_    = NULL;
    interpol_prop_decay_            = NULL;
    interpol_prop_decay_diff_       = NULL;
    interpol_prop_interaction_      = NULL;
    interpol_prop_interaction_diff_ = NULL;
    randomizer_                     = NULL;
    geometry_                       = NULL;
    scattering_                 = NULL;

}

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


//Copyconstructor
ProcessCollection::ProcessCollection(const ProcessCollection &collection)
    : MathModel(collection)
    ,order_of_interpolation_     ( collection.order_of_interpolation_ )
    ,do_interpolation_           ( collection.do_interpolation_ )
    ,lpm_effect_enabled_         ( collection.lpm_effect_enabled_ )
    ,ini_                        ( collection.ini_ )
    ,debug_                      ( collection.debug_ )
    ,do_weighting_               ( collection.do_weighting_ )
    ,weighting_order_            ( collection.weighting_order_ )
    ,weighting_starts_at_        ( collection.weighting_starts_at_ )
    ,enable_randomization_       ( collection.enable_randomization_ )
    ,do_continuous_randomization_( collection.do_continuous_randomization_ )
    ,do_scattering_              ( collection.do_scattering_ )
    ,location_                   ( collection.location_ )
    ,density_correction_         ( collection.density_correction_ )
    ,do_time_interpolation_      ( collection.do_time_interpolation_ )
    ,do_exact_time_calculation_   ( collection.do_exact_time_calculation_ )
    ,up_                         ( collection.up_)
    ,bigLow_                     ( collection.bigLow_ )
    ,storeDif_                   ( collection.storeDif_ )
    ,particle_                   ( new PROPOSALParticle(*collection.particle_) )
    ,backup_particle_            ( new PROPOSALParticle(*collection.backup_particle_) )
    ,medium_                     ( collection.medium_->clone() )
    ,integral_                   ( new Integral(*collection.integral_) )
    ,cut_settings_               ( new EnergyCutSettings(*collection.cut_settings_) )
    ,decay_                      ( new Decay(*collection.decay_) )
    ,prop_decay_                 ( new Integral(*collection.prop_decay_) )
    ,prop_interaction_           ( new Integral(*collection.prop_interaction_) )
    ,time_particle_              ( new Integral(*collection.time_particle_) )

{
    crosssections_.resize(collection.crosssections_.size());

    for(unsigned int i =0; i<collection.crosssections_.size(); i++)
    {
        switch (collection.crosssections_.at(i)->GetType())
        {
            case ParticleType::Brems:
                crosssections_.at(i) = new Bremsstrahlung( *(Bremsstrahlung*)collection.crosssections_.at(i) );
                break;
            // case ParticleType::DeltaE:
            //     crosssections_.at(i) = new Ionization( *(Ionization*)collection.crosssections_.at(i) );
            //     break;
            // case ParticleType::EPair:
            //     crosssections_.at(i) = new Epairproduction( *(Epairproduction*)collection.crosssections_.at(i) );
            //     break;
            case ParticleType::NuclInt:
                crosssections_.at(i) = new Photonuclear( *(Photonuclear*)collection.crosssections_.at(i) );
                break;
            default:
                log_fatal("Unknown cross section");
                exit(1);
        }
    }

    if(collection.interpolant_ != NULL)
    {
        interpolant_ = new Interpolant(*collection.interpolant_) ;
    }
    else
    {
        interpolant_ = NULL;
    }

    if(collection.interpolant_diff_ != NULL)
    {
        interpolant_diff_ = new Interpolant(*collection.interpolant_diff_) ;
    }
    else
    {
        interpolant_diff_ = NULL;
    }

    if(collection.interpol_prop_decay_ != NULL)
    {
        interpol_prop_decay_ = new Interpolant(*collection.interpol_prop_decay_) ;
    }
    else
    {
        interpol_prop_decay_ = NULL;
    }

    if(collection.interpol_prop_decay_diff_ != NULL)
    {
        interpol_prop_decay_diff_ = new Interpolant(*collection.interpol_prop_decay_diff_) ;
    }
    else
    {
        interpol_prop_decay_diff_ = NULL;
    }

    if(collection.interpol_prop_interaction_ != NULL)
    {
        interpol_prop_interaction_ = new Interpolant(*collection.interpol_prop_interaction_) ;
    }
    else
    {
        interpol_prop_interaction_ = NULL;
    }

    if(collection.interpol_prop_interaction_diff_ != NULL)
    {
        interpol_prop_interaction_diff_ = new Interpolant(*collection.interpol_prop_interaction_diff_) ;
    }
    else
    {
        interpol_prop_interaction_diff_ = NULL;
    }

    if(collection.interpol_time_particle_ != NULL)
    {
        interpol_time_particle_ = new Interpolant(*collection.interpol_time_particle_) ;
    }
    else
    {
        interpol_time_particle_ = NULL;
    }

    if(collection.interpol_time_particle_diff_ != NULL)
    {
        interpol_time_particle_diff_ = new Interpolant(*collection.interpol_time_particle_diff_) ;
    }
    else
    {
        interpol_time_particle_diff_ = NULL;
    }

    if(collection.randomizer_ != NULL)
    {
        randomizer_ = new ContinuousRandomization(*collection.randomizer_) ;
    }
    else
    {
        randomizer_ = NULL;
    }

    if(collection.scattering_ != NULL)
    {
        scattering_ = new Scattering(*collection.scattering_) ;
    }
    else
    {
        scattering_ = NULL;
    }

    if(collection.geometry_ != NULL)
    {
        // geometry_ = new Geometry(*collection.geometry_) ;
        geometry_ = collection.geometry_->clone();
    }
    else
    {
        geometry_ = NULL;
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


ProcessCollection::ProcessCollection(PROPOSALParticle *particle, Medium *medium, EnergyCutSettings* cut_settings)
    :order_of_interpolation_     ( 5 )
    ,do_interpolation_           ( false )
    ,lpm_effect_enabled_         ( false )
    ,ini_                        ( 0 )
    ,debug_                      ( false )
    ,do_weighting_               ( false )
    ,weighting_order_            ( 0 )
    ,weighting_starts_at_        ( 0 )
    ,enable_randomization_       ( false )
    ,do_continuous_randomization_( false )
    ,do_scattering_              ( false )
    ,location_                   ( 0 )
    ,density_correction_         ( 1. )
    ,do_time_interpolation_      ( false )
    ,do_exact_time_calculation_   ( false )
    ,up_                         ( false )
    ,bigLow_                     ( 2,0 )
    ,storeDif_                   ( 2,0 )
{

    particle_           =   particle;
    backup_particle_    =   particle_;
    medium_             =   medium;
    cut_settings_       =   cut_settings;

    integral_           =   new Integral(IROMB, IMAXS, IPREC2);
    prop_decay_         =   new Integral(IROMB, IMAXS, IPREC2);
    prop_interaction_   =   new Integral(IROMB, IMAXS, IPREC2);

    // crosssections_.resize(4);
    crosssections_.resize(1);
    crosssections_.at(0) = new Bremsstrahlung(medium_, cut_settings_);
    // crosssections_.at(0) = new Ionization(particle_, medium_, cut_settings_);
    // crosssections_.at(1) = new Bremsstrahlung(medium_, cut_settings_);
    // crosssections_.at(2) = new Photonuclear(particle_, medium_, cut_settings_);
    // crosssections_.at(3) = new Epairproduction(particle_, medium_, cut_settings_);

    decay_               = new Decay();
    time_particle_       = new Integral(IROMB, IMAXS, IPREC2);

    interpolant_                    = NULL;
    interpolant_diff_               = NULL;
    interpol_prop_decay_            = NULL;
    interpol_prop_decay_diff_       = NULL;
    interpol_prop_interaction_      = NULL;
    interpol_prop_interaction_diff_ = NULL;
    randomizer_                     = NULL;
    geometry_                       = NULL;
    interpol_time_particle_         = NULL;
    interpol_time_particle_diff_    = NULL;
    scattering_                 = NULL;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//-------------------------operators and swap function------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


ProcessCollection& ProcessCollection::operator=(const ProcessCollection &collection)
{
    if (this != &collection)
    {
      ProcessCollection tmp(collection);
      swap(tmp);
    }
    return *this;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


bool ProcessCollection::operator==(const ProcessCollection &collection) const
{
    if( order_of_interpolation_     != collection.order_of_interpolation_ )  return false;
    if( do_interpolation_           != collection.do_interpolation_ )        return false;
    if( lpm_effect_enabled_         != collection.lpm_effect_enabled_ )      return false;
    if( ini_                        != collection.ini_ )                     return false;
    if( debug_                      != collection.debug_ )                   return false;
    if( *particle_                  != *collection.particle_ )               return false;
    if( *medium_                    != *collection.medium_ )                 return false;
    if( *integral_                  != *collection.integral_ )               return false;
    if( *cut_settings_              != *collection.cut_settings_ )           return false;
    if( *prop_decay_                != *collection.prop_decay_ )             return false;
    if( *prop_interaction_          != *collection.prop_interaction_ )       return false;
    if( up_                         != collection.up_)                       return false;
    if( do_weighting_               != collection.do_weighting_ )            return false;
    if( weighting_order_            != collection.weighting_order_ )         return false;
    if( weighting_starts_at_        != collection.weighting_starts_at_ )     return false;
    if( enable_randomization_       != collection.enable_randomization_ )    return false;
    if( do_continuous_randomization_!= collection.do_continuous_randomization_ )return false;
    if( do_scattering_              != collection.do_scattering_ )           return false;
    if( location_                   != collection.location_ )                return false;
    if( density_correction_         != collection.density_correction_ )      return false;
    if( do_exact_time_calculation_   != collection.do_exact_time_calculation_ )return false;
    if( do_time_interpolation_      != collection.do_time_interpolation_ )   return false;
    if( *time_particle_             != *collection.time_particle_ )          return false;

    if( *decay_                     != *collection.decay_ )                  return false;

    if( crosssections_.size()       != collection.crosssections_.size() )    return false;
    if( bigLow_.size()              != collection.bigLow_.size() )           return false;
    if( storeDif_.size()            != collection.storeDif_.size() )         return false;

    for(unsigned int i =0; i<collection.bigLow_.size(); i++)
    {
        if( bigLow_.at(i) !=  collection.bigLow_.at(i) )        return false;
    }

    for(unsigned int i =0; i<collection.storeDif_.size(); i++)
    {
        if( storeDif_.at(i) !=  collection.storeDif_.at(i) )    return false;
    }

    for(unsigned int i =0; i<collection.crosssections_.size(); i++)
    {
        switch (collection.crosssections_.at(i)->GetType())
        {
            case ParticleType::Brems:
                if( *(Bremsstrahlung*)crosssections_.at(i) !=  *(Bremsstrahlung*)collection.crosssections_.at(i) ) return false;
                break;
            // case ParticleType::DeltaE:
            //     if( *(Ionization*)crosssections_.at(i) != *(Ionization*)collection.crosssections_.at(i) ) return false;
            //     break;
            // case ParticleType::EPair:
            //     if( *(Epairproduction*)crosssections_.at(i) !=  *(Epairproduction*)collection.crosssections_.at(i) ) return false;
            //     break;
            case ParticleType::NuclInt:
                if( *(Photonuclear*)crosssections_.at(i) !=  *(Photonuclear*)collection.crosssections_.at(i) )  return false;
                break;
            default:
                log_fatal("Unknown cross section");
                exit(1);
        }
    }

    if( interpolant_ != NULL && collection.interpolant_ != NULL)
    {
        if( *interpolant_   != *collection.interpolant_)                                        return false;
    }
    else if( interpolant_ != collection.interpolant_)                                           return false;

    if( interpolant_diff_ != NULL && collection.interpolant_diff_ != NULL)
    {
        if( *interpolant_diff_   != *collection.interpolant_diff_)                              return false;
    }
    else if( interpolant_diff_ != collection.interpolant_diff_)                                 return false;

    if( interpol_prop_decay_ != NULL && collection.interpol_prop_decay_ != NULL)
    {
        if( *interpol_prop_decay_   != *collection.interpol_prop_decay_)                        return false;
    }
    else if( interpol_prop_decay_ != collection.interpol_prop_decay_)                           return false;

    if( interpol_prop_decay_diff_ != NULL && collection.interpol_prop_decay_diff_ != NULL)
    {
        if( *interpol_prop_decay_diff_   != *collection.interpol_prop_decay_diff_)              return false;
    }
    else if( interpol_prop_decay_diff_ != collection.interpol_prop_decay_diff_)                 return false;

    if( interpol_prop_interaction_ != NULL && collection.interpol_prop_interaction_ != NULL)
    {
        if( *interpol_prop_interaction_   != *collection.interpol_prop_interaction_)            return false;
    }
    else if( interpol_prop_interaction_ != collection.interpol_prop_interaction_)               return false;

    if( interpol_prop_interaction_diff_ != NULL && collection.interpol_prop_interaction_diff_ != NULL)
    {
        if( *interpol_prop_interaction_diff_   != *collection.interpol_prop_interaction_diff_)  return false;
    }
    else if( interpol_prop_interaction_diff_ != collection.interpol_prop_interaction_diff_)     return false;

    if( interpol_time_particle_diff_ != NULL && collection.interpol_time_particle_diff_ != NULL)
    {
        if( *interpol_time_particle_diff_   != *collection.interpol_time_particle_diff_)        return false;
    }
    else if( interpol_time_particle_diff_ != collection.interpol_time_particle_diff_)           return false;

    if( interpol_time_particle_ != NULL && collection.interpol_time_particle_ != NULL)
    {
        if( *interpol_time_particle_   != *collection.interpol_time_particle_)                  return false;
    }
    else if( interpol_time_particle_ != collection.interpol_time_particle_)                     return false;

    if( randomizer_ != NULL && collection.randomizer_ != NULL)
    {
        if( *randomizer_   != *collection.randomizer_)                                        return false;
    }
    else if( randomizer_ != collection.randomizer_)                                           return false;

    if( scattering_ != NULL && collection.scattering_ != NULL)
    {
        if( *scattering_   != *collection.scattering_)                                        return false;
    }
    else if( scattering_ != collection.scattering_)                                           return false;

    if( geometry_ != NULL && collection.geometry_ != NULL)
    {
        if( *geometry_   != *collection.geometry_)                                        return false;
    }
    else if( geometry_ != collection.geometry_)                                           return false;

    //else
    return true;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


bool ProcessCollection::operator!=(const ProcessCollection &collection) const
{
    return !(*this == collection);
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

namespace PROPOSAL
{

ostream& operator<<(ostream& os, ProcessCollection const& collection)
{

    os<<"------------ProcessCollection( "<<&collection<<" )------------"<<endl;
    os<<"------------------------------------------------------"<<endl;
    os<<"Particle type:\t\t\t\t\t"<<collection.particle_->GetName()<<endl;
    os<<"Order of interpolation:\t\t\t\t"<<collection.order_of_interpolation_<<endl;
    os<<"Interpolation enabled:\t\t\t\t"<<collection.do_interpolation_<<endl;
    os<<"Continuous randomization enabled:\t\t"<<collection.do_continuous_randomization_<<endl;
    os<<"Moliere scattering enabled:\t\t\t"<<collection.do_scattering_<<endl;
    os<<"Exact time calculation enabled:\t\t\t"<<collection.do_exact_time_calculation_<<endl;
    os<<"Location (0=infront, 1=inside, 2=behind)\t"<<collection.location_<<endl;
    os<<"Density correction factor:\t\t\t"<<collection.density_correction_<<endl;

    os<<"\n"<<*collection.medium_<<"\n"<<endl;
    os<<*collection.cut_settings_<<"\n"<<endl;

    if(collection.geometry_ != NULL)
        os<<*collection.geometry_<<"\n"<<endl;

    os<<"------------------------------------------------------"<<endl;
    os<<"------------------------------------------------------"<<endl;
    return os;
}

}  // namespace PROPOSAL

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::swap(ProcessCollection &collection)
{
    using std::swap;

    PROPOSALParticle tmp_particle1(*collection.particle_);
    PROPOSALParticle tmp_particle2(*particle_);

    EnergyCutSettings tmp_cuts1(*collection.cut_settings_);
    EnergyCutSettings tmp_cuts2(*cut_settings_);

    vector<CrossSections*> tmp_cross1(collection.crosssections_);
    vector<CrossSections*> tmp_cross2(crosssections_);

    swap( order_of_interpolation_     , collection.order_of_interpolation_ );
    swap( do_interpolation_           , collection.do_interpolation_ );
    swap( lpm_effect_enabled_         , collection.lpm_effect_enabled_ );
    swap( ini_                        , collection.ini_ );
    swap( debug_                      , collection.debug_ );
    swap( up_                         , collection.up_ );
    swap( do_weighting_               , collection.do_weighting_ );
    swap( weighting_order_            , collection.weighting_order_ );
    swap( weighting_starts_at_        , collection.weighting_starts_at_ );
    swap( do_continuous_randomization_, collection.do_continuous_randomization_ );
    swap( enable_randomization_       , collection.enable_randomization_ );
    swap( location_                   , collection.location_ );
    swap( density_correction_         , collection.density_correction_ );
    swap( do_time_interpolation_      , collection.do_time_interpolation_ );
    swap( do_exact_time_calculation_   , collection.do_exact_time_calculation_ );


    particle_->swap( *collection.particle_ );       //particle pointer swap
    medium_->swap( *collection.medium_ );
    integral_->swap( *collection.integral_ );
    cut_settings_->swap( *collection.cut_settings_ );
    crosssections_.swap(collection.crosssections_); //particle pointer swap
    prop_decay_->swap( *collection.prop_decay_ );
    prop_interaction_->swap( *collection.prop_interaction_ );
    decay_->swap(*collection.decay_);               //particle pointer swap

    time_particle_->swap(*collection.time_particle_ );

    storeDif_.swap(collection.storeDif_);
    bigLow_.swap(collection.bigLow_);

    if( randomizer_ != NULL && collection.randomizer_ != NULL)
    {
        randomizer_->swap(*collection.randomizer_);
    }
    else if( randomizer_ == NULL && collection.randomizer_ != NULL)
    {
        randomizer_ = new ContinuousRandomization(*collection.randomizer_);
        collection.randomizer_ = NULL;
    }
    else if( randomizer_ != NULL && collection.randomizer_ == NULL)
    {
        collection.randomizer_ = new ContinuousRandomization(*randomizer_);
        randomizer_ = NULL;
    }

    if( scattering_ != NULL && collection.scattering_ != NULL)
    {
        scattering_->swap(*collection.scattering_);
    }
    else if( scattering_ == NULL && collection.scattering_ != NULL)
    {
        scattering_ = new Scattering(*collection.scattering_);
        collection.scattering_ = NULL;
    }
    else if( scattering_ != NULL && collection.scattering_ == NULL)
    {
        collection.scattering_ = new Scattering(*scattering_);
        scattering_ = NULL;
    }

    if( geometry_ != NULL && collection.geometry_ != NULL)
    {
        geometry_->swap(*collection.geometry_);
    }
    else if( geometry_ == NULL && collection.geometry_ != NULL)
    {
        // geometry_ = new Geometry(*collection.geometry_);
        geometry_ = collection.geometry_->clone();
        collection.geometry_ = NULL;
    }
    else if( geometry_ != NULL && collection.geometry_ == NULL)
    {
        // collection.geometry_ = new Geometry(*geometry_);
        geometry_ = collection.geometry_->clone();
        geometry_ = NULL;
    }


    // Set pointers again (to many swapping above....)
    // SetParticle( new PROPOSALParticle(tmp_particle1) );
    // collection.SetParticle( new PROPOSALParticle(tmp_particle2) );

    SetCutSettings(  new EnergyCutSettings(tmp_cuts1) );
    collection.SetCutSettings( new EnergyCutSettings(tmp_cuts2) );

    SetCrosssections(  tmp_cross1 );
    collection.SetCrosssections(  tmp_cross2 );

    if( interpolant_ != NULL && collection.interpolant_ != NULL)
    {
        interpolant_->swap(*collection.interpolant_);
    }
    else if( interpolant_ == NULL && collection.interpolant_ != NULL)
    {
        interpolant_ = new Interpolant(*collection.interpolant_);
        collection.interpolant_ = NULL;
    }
    else if( interpolant_ != NULL && collection.interpolant_ == NULL)
    {
        collection.interpolant_ = new Interpolant(*interpolant_);
        interpolant_ = NULL;
    }

    if( interpolant_diff_ != NULL && collection.interpolant_diff_ != NULL)
    {
        interpolant_diff_->swap(*collection.interpolant_diff_);
    }
    else if( interpolant_diff_ == NULL && collection.interpolant_diff_ != NULL)
    {
        interpolant_diff_ = new Interpolant(*collection.interpolant_diff_);
        collection.interpolant_diff_ = NULL;
    }
    else if( interpolant_diff_ != NULL && collection.interpolant_diff_ == NULL)
    {
        collection.interpolant_diff_ = new Interpolant(*interpolant_diff_);
        interpolant_diff_ = NULL;
    }

    if( interpol_prop_decay_ != NULL && collection.interpol_prop_decay_ != NULL)
    {
        interpol_prop_decay_->swap(*collection.interpol_prop_decay_);
    }
    else if( interpol_prop_decay_ == NULL && collection.interpol_prop_decay_ != NULL)
    {
        interpol_prop_decay_ = new Interpolant(*collection.interpol_prop_decay_);
        collection.interpol_prop_decay_ = NULL;
    }
    else if( interpol_prop_decay_ != NULL && collection.interpol_prop_decay_ == NULL)
    {
        collection.interpol_prop_decay_ = new Interpolant(*interpol_prop_decay_);
        interpol_prop_decay_ = NULL;
    }

    if( interpol_prop_decay_diff_ != NULL && collection.interpol_prop_decay_diff_ != NULL)
    {
        interpol_prop_decay_diff_->swap(*collection.interpol_prop_decay_diff_);
    }
    else if( interpol_prop_decay_diff_ == NULL && collection.interpol_prop_decay_diff_ != NULL)
    {
        interpol_prop_decay_diff_ = new Interpolant(*collection.interpol_prop_decay_diff_);
        collection.interpol_prop_decay_diff_ = NULL;
    }
    else if( interpol_prop_decay_diff_ != NULL && collection.interpol_prop_decay_diff_ == NULL)
    {
        collection.interpol_prop_decay_diff_ = new Interpolant(*interpol_prop_decay_diff_);
        interpol_prop_decay_diff_ = NULL;
    }

    if( interpol_prop_interaction_ != NULL && collection.interpol_prop_interaction_ != NULL)
    {
        interpol_prop_interaction_->swap(*collection.interpol_prop_interaction_);
    }
    else if( interpol_prop_interaction_ == NULL && collection.interpol_prop_interaction_ != NULL)
    {
        interpol_prop_interaction_ = new Interpolant(*collection.interpol_prop_interaction_);
        collection.interpol_prop_interaction_ = NULL;
    }
    else if( interpol_prop_interaction_ != NULL && collection.interpol_prop_interaction_ == NULL)
    {
        collection.interpol_prop_interaction_ = new Interpolant(*interpol_prop_interaction_);
        interpol_prop_interaction_ = NULL;
    }

    if( interpol_prop_interaction_diff_ != NULL && collection.interpol_prop_interaction_diff_ != NULL)
    {
        interpol_prop_interaction_diff_->swap(*collection.interpol_prop_interaction_diff_);
    }
    else if( interpol_prop_interaction_diff_ == NULL && collection.interpol_prop_interaction_diff_ != NULL)
    {
        interpol_prop_interaction_diff_ = new Interpolant(*collection.interpol_prop_interaction_diff_);
        collection.interpol_prop_interaction_diff_ = NULL;
    }
    else if( interpol_prop_interaction_diff_ != NULL && collection.interpol_prop_interaction_diff_ == NULL)
    {
        collection.interpol_prop_interaction_diff_ = new Interpolant(*interpol_prop_interaction_diff_);
        interpol_prop_interaction_diff_ = NULL;
    }

    if( interpol_time_particle_ != NULL && collection.interpol_time_particle_ != NULL)
    {
        interpol_time_particle_->swap(*collection.interpol_time_particle_);
    }
    else if( interpol_time_particle_ == NULL && collection.interpol_time_particle_ != NULL)
    {
        interpol_time_particle_ = new Interpolant(*collection.interpol_time_particle_);
        collection.interpol_time_particle_ = NULL;
    }
    else if( interpol_time_particle_ != NULL && collection.interpol_time_particle_ == NULL)
    {
        collection.interpol_time_particle_ = new Interpolant(*interpol_time_particle_);
        interpol_time_particle_ = NULL;
    }

    if( interpol_time_particle_diff_ != NULL && collection.interpol_time_particle_diff_ != NULL)
    {
        interpol_time_particle_diff_->swap(*collection.interpol_time_particle_diff_);
    }
    else if( interpol_time_particle_diff_ == NULL && collection.interpol_time_particle_diff_ != NULL)
    {
        interpol_time_particle_diff_ = new Interpolant(*collection.interpol_time_particle_diff_);
        collection.interpol_time_particle_diff_ = NULL;
    }
    else if( interpol_time_particle_diff_ != NULL && collection.interpol_time_particle_diff_ == NULL)
    {
        collection.interpol_time_particle_diff_ = new Interpolant(*interpol_time_particle_diff_);
        interpol_time_particle_diff_ = NULL;
    }

}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//-------------------------Functions to interpolate---------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::FunctionToBuildInterpolant(const PROPOSALParticle& particle, double energy)
{
    return integral_->Integrate(energy, particle.GetLow(), boost::bind(&ProcessCollection::FunctionToIntegral, this, boost::cref(particle), _1),4);
}



//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


// double ProcessCollection::FunctionToBuildInterpolantDiff(double energy)
// {
//     return FunctionToIntegral(energy);
// }


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::InterpolPropDecay(const PROPOSALParticle& particle, double energy)
{
    return -prop_decay_->Integrate(energy, BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralDecay, this, boost::cref(particle), _1),4);
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


// double ProcessCollection::InterpolPropDecayDiff(double energy)
// {
//     return FunctionToPropIntegralDecay(energy);
// }


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::InterpolPropInteraction(const PROPOSALParticle& particle, double energy)
{
    if(up_)
    {
        return prop_interaction_->Integrate(energy, particle.GetLow(), boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1),4);
    }
    else
    {
        return -prop_interaction_->Integrate(energy, BIGENERGY, boost::bind(&ProcessCollection::FunctionToPropIntegralInteraction, this, boost::cref(particle), _1),4);
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


// double ProcessCollection::InterpolPropInteractionDiff(double energy)
// {
//     return FunctionToPropIntegralInteraction(energy);
// }


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


// double ProcessCollection::InterpolTimeParticle(double energy)
// {
//     return FunctionToTimeIntegral(energy);
// }


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::InterpolTimeParticleDiff(const PROPOSALParticle& particle, double energy)
{
    return time_particle_->Integrate(energy, particle.GetLow(), boost::bind(&ProcessCollection::FunctionToTimeIntegral, this, boost::cref(particle), _1),4);
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//--------------------------Functions to integrate----------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::FunctionToTimeIntegral(const PROPOSALParticle& particle, double energy)
{
    double aux;

    aux     =   FunctionToIntegral(particle, energy);
    aux     *=  particle.GetEnergy()/(particle.GetMomentum()*SPEED);
    return aux;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::FunctionToPropIntegralDecay(const PROPOSALParticle& particle, double energy)
{
    double aux;
    double decay;

    aux =   FunctionToIntegral(particle, energy);

    decay  =   decay_->MakeDecay(particle);

    log_debug(" + %f",particle.GetEnergy());


    return aux*decay;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::FunctionToPropIntegralInteraction(const PROPOSALParticle& particle, double energy)
{
    double aux;
    double rate = 0;
    double total_rate = 0.5;

    aux =   FunctionToIntegral(particle, energy);

    for( unsigned int i = 0; i < crosssections_.size(); i++)
    {
        rate  =   crosssections_.at(i)->CalculatedNdx(particle);

        log_debug("Rate for %s = %f",crosssections_.at(i)->GetName().c_str(),rate);

        total_rate += rate;

    }
    return aux*total_rate;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


double ProcessCollection::FunctionToIntegral(const PROPOSALParticle& particle, double energy)
{
    double result;
    double aux;

    PROPOSALParticle temp_particle(particle);
    temp_particle.SetEnergy(energy);

    result  =    0.5;

    for(unsigned int i =0;i<crosssections_.size();i++)
    {
        aux     =   crosssections_.at(i)->CalculatedEdx(temp_particle);
        result  +=  aux;

        log_debug("energy %f , dE/dx = %f",particle.GetEnergy() ,aux);

    }

    return -1/result;
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//---------------------------------Setter-------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::SetCutSettings(EnergyCutSettings* cutSettings)
{
    cut_settings_ = cutSettings;
    for(unsigned int i = 0 ; i < crosssections_.size() ; i++)
    {
        crosssections_.at(i)->SetEnergyCutSettings(cut_settings_);
    }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::SetMedium(Medium* medium)
{
    medium_ = medium;
    for(unsigned int i = 0 ; i < crosssections_.size() ; i++)
    {
        crosssections_.at(i)->SetMedium(medium_);
    }
    // if(do_continuous_randomization_)
    // {
    //     randomizer_->SetMedium(medium_);
    // }
}


//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


// void ProcessCollection::SetParticle(PROPOSALParticle* particle)
// {
//     particle_ = particle;
//
//     for(unsigned int i = 0 ; i < crosssections_.size() ; i++)
//     {
//         crosssections_.at(i)->SetParticle(particle);
//     }
//     if(do_continuous_randomization_)
//     {
//         randomizer_->SetParticle(particle_);
//     }
//     if(do_scattering_)
//     {
//         scattering_->SetParticle(particle_);
//     }
// }

// PROPOSALParticle *ProcessCollection::GetBackup_particle() const
// {
//     return backup_particle_;
// }
//
// void ProcessCollection::SetBackup_particle(PROPOSALParticle *backup_particle)
// {
//     backup_particle_ = backup_particle;
// }
//
// void ProcessCollection::RestoreBackup_particle()
// {
//     particle_ = backup_particle_;
//
//     for(unsigned int i = 0 ; i < crosssections_.size() ; i++)
//     {
//         crosssections_.at(i)->RestoreBackup_particle();
//     }
//     if(do_continuous_randomization_)
//     {
//         randomizer_->RestoreBackup_particle();
//     }
//     if(do_scattering_)
//     {
//         scattering_->RestoreBackup_particle();
//     }
// }

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


void ProcessCollection::SetCrosssections(
        std::vector<CrossSections*> crosssections) {
    crosssections_ = crosssections;
    // if(do_continuous_randomization_)
    // {
    //     randomizer_->SetCrosssections(crosssections);
    // }
    if(do_scattering_)
    {
        scattering_->SetCrosssections(crosssections);
    }
}

void ProcessCollection::SetDebug(bool debug) {
    debug_ = debug;
}

void ProcessCollection::SetDoInterpolation(bool doInterpolation) {
    do_interpolation_ = doInterpolation;
}

void ProcessCollection::SetIni(double ini) {
    ini_ = ini;
}

void ProcessCollection::SetIntegral(Integral* integral) {
    integral_ = integral;
}

void ProcessCollection::SetInterpolant(Interpolant* interpolant) {
    interpolant_ = interpolant;
}

void ProcessCollection::SetInterpolantDiff(
        Interpolant* interpolantDiff) {
    interpolant_diff_ = interpolantDiff;
}

void ProcessCollection::SetLpmEffectEnabled(bool lpmEffectEnabled) {
    lpm_effect_enabled_ = lpmEffectEnabled;
}

void ProcessCollection::SetOrderOfInterpolation(int orderOfInterpolation) {
    order_of_interpolation_ = orderOfInterpolation;
}

void ProcessCollection::SetLocation(int location)
{
    if(location<0 || location > 2)
    {
        log_error("Invalid location! Must be 0,1,2 (infront, inside, behind). Set to 0!");
        location_   =   0;
    }
    else
    {
        location_   =   location;
    }
}

void ProcessCollection::SetGeometry(Geometry *geometry){
    geometry_   =   geometry;
}

void ProcessCollection::SetDensityCorrection(double density_correction){
    density_correction_ =   density_correction;
}

void ProcessCollection::SetEnableRandomization(bool enable_randomization){
    enable_randomization_ =   enable_randomization;
}

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//---------------------------------Destructor---------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//


ProcessCollection::~ProcessCollection(){}
