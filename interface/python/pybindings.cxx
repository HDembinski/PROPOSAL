
#include "PROPOSAL/math/Vector3D.h"
#include "PROPOSAL/version.h"
#include "PROPOSAL/crosssection/CrossSection.h"
#include "PROPOSAL/propagation_utility/Interaction.h"
#include "PROPOSAL/Propagator.h"
#include "PROPOSAL/propagation_utility/InteractionBuilder.h"
#include "PROPOSAL/propagation_utility/ContRandBuilder.h"
#include "PROPOSAL/propagation_utility/TimeBuilder.h"
#include "PROPOSAL/propagation_utility/DecayBuilder.h"
#include "PROPOSAL/propagation_utility/PropagationUtility.h"
#include "PROPOSAL/EnergyCutSettings.h"

#include "pyBindings.h"

namespace py = pybind11;
using namespace PROPOSAL;


void init_components(py::module& m);
void init_medium(py::module& m);
void init_particle(py::module& m);
void init_decay(py::module& m);
void init_geometry(py::module& m);
void init_parametrization(py::module& m);
void init_crosssection(py::module& m);
void init_scattering(py::module& m);
void init_math(py::module&);
void init_secondaries(py::module&);

PYBIND11_MODULE(proposal, m)
{
    m.doc() = R"pbdoc(
        .. currentmodule:: proposal
    )pbdoc";

    init_components(m);
    init_medium(m);
    init_particle(m);
    init_decay(m);
    init_geometry(m);
    init_parametrization(m);
    init_crosssection(m);
    init_scattering(m);
    init_math(m);
    init_secondaries(m);

    m.attr("__version__") = &PROPOSAL_VERSION;

    py::class_<Vector3D, std::shared_ptr<Vector3D>>(m, "Vector3D")
        .def(py::init<>())
        .def(py::init<double, double, double>(), py::arg("x"), py::arg("y"),
                py::arg("z"))
        .def(py::init<const Vector3D&>())
        .def("__str__", &py_print<Vector3D>)
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * float())
        .def(float() * py::self)
        .def(py::self * py::self)
        .def(-py::self)
        .def_property_readonly("x", &Vector3D::GetX)
        .def_property_readonly("y", &Vector3D::GetY)
        .def_property_readonly("z", &Vector3D::GetZ)
        .def_property_readonly("radius", &Vector3D::GetRadius)
        .def_property_readonly("phi", &Vector3D::GetPhi)
        .def_property_readonly("theta", &Vector3D::GetTheta)
        .def("set_cartesian_coordinates", &Vector3D::SetCartesianCoordinates)
        .def("set_spherical_coordinates", &Vector3D::SetSphericalCoordinates)
        .def("normalize", &Vector3D::normalise)
        .def("magnitude", &Vector3D::magnitude)
        .def("cartesian_from_spherical",
                &Vector3D::CalculateCartesianFromSpherical)
        .def("spherical_from_cartesian",
                &Vector3D::CalculateSphericalCoordinates);

    py::class_<EnergyCutSettings, std::shared_ptr<EnergyCutSettings>>(m,
            "EnergyCutSettings",
            R"pbdoc(
            Settings for the lower integration limit.
            Losses below the cut will be handeled continously and the
            other stochasticaly.

            .. math::

                \text{cut} = \begin{cases} e_\text{cut} & E * v_\text{cut} \geq e_\text{cut} \\ v_\text{cut} & \, \text{else} \end{cases}
        )pbdoc")
        .def(py::init<double, double, bool>(), py::arg("ecut"), py::arg("vcut"), py::arg("continuous_random"),
                R"pbdoc(
                    Set the cut values manualy.

                    Args:
                        ecut (float): static energy cut.
                        vcut (float): relativ energy cut.
                )pbdoc")
        .def(py::init<const EnergyCutSettings&>())
                                      .def("__str__", &py_print<EnergyCutSettings>)
                                      .def_property_readonly("ecut", &EnergyCutSettings::GetEcut,
                                              R"pbdoc(
                    Return set e_cut.

                    Returns:
                        float: e_cut
                )pbdoc")
                                      .def_property_readonly("vcut", &EnergyCutSettings::GetVcut,
                                              R"pbdoc(
                    Return set v_cut.

                    Returns:
                        float: v_cut
                )pbdoc")
                                          .def("cut", overload_cast_<double>()(&EnergyCutSettings::GetCut, py::const_))
                                          .def("cut", overload_cast_<std::tuple<double, double> const&, double>()(&EnergyCutSettings::GetCut, py::const_));

    py::class_<Interaction, std::shared_ptr<Interaction>>(m, "Interaction")
        .def("energy_interaction", py::vectorize(&Interaction::EnergyInteraction),
                py::arg("energy"), py::arg("random number"))
        .def("rates", &Interaction::Rates, py::arg("energy"))
        .def("sample_loss", &Interaction::SampleLoss,
                py::arg("energy"), py::arg("rates"), py::arg("random number"))
        .def("mean_free_path", py::vectorize(&Interaction::MeanFreePath),
                py::arg("energy"))
        .def_readwrite_static("interpol_def", &Interaction::interpol_def);

    m.def("make_interaction", [](crosssection_list_t<ParticleDef, Medium> cross, bool interpolate){
            return shared_ptr<Interaction>(make_interaction(cross, interpolate));
            });

    py::class_<Displacement, std::shared_ptr<Displacement>>(m, "Displacement")
        .def("solve_track_integral", py::vectorize(&Displacement::SolveTrackIntegral), py::arg("upper_lim"), py::arg("lower_lim"))
        .def("upper_limit_track_integral", py::vectorize(&Displacement::UpperLimitTrackIntegral), py::arg("energy"), py::arg("distance"))
        .def("function_to_integral", py::vectorize(&Displacement::FunctionToIntegral), py::arg("energy"))
        .def_readwrite_static("interpol_def", &Displacement::interpol_def);

    m.def("make_displacement", [](crosssection_list_t<ParticleDef, Medium> cross, bool interpolate){
            return shared_ptr<Displacement>(make_displacement(cross, interpolate));
            });

    py::class_<InterpolationDef, std::shared_ptr<InterpolationDef>>(m,
            "InterpolationDef",
            R"pbdoc(
                The set standard values have been optimized for performance
                and accuracy. They should not be changed any further
                without a good reason.
                Example:
                    For speed savings it makes sense to specify a path to
                    the tables, to reuse build tables if possible.
                    Sometimes it is usefull to look in the tables for a check.
                    To do this binary tables can be diabled.
                    >>> interpolDef = pp.InterpolationDef()
                    >>> interpolDef.do_binary_tables = False
                    >>> interpolDef.path_to_tables = "./custom/table/path"
                    >>> interpolDef.path_to_tables_readonly = "./custom/table/path"
            )pbdoc")
        .def(py::init<>())
        .def_readwrite_static("path_to_tables",
                &InterpolationDef::path_to_tables,
                R"pbdoc(
                Path where tables can be written from memory to disk to
                reuse it if possible.
            )pbdoc")
        .def_readwrite_static("path_to_tables_readonly",
                &InterpolationDef::path_to_tables_readonly,
                R"pbdoc(
                Path where tables can be read from disk to avoid to rebuild
                it.
            )pbdoc")
        .def_readwrite_static("do_binary_tables", &InterpolationDef::do_binary_tables,
                R"pbdoc(
                Should binary tables be used to store the data.
                This will increase performance, but are not readable for a
                crosscheck by human. Default: xxx
            )pbdoc")
        .def_readwrite_static("just_use_readonly_path",
                &InterpolationDef::just_use_readonly_path,
                R"pbdoc(
                Just the readonly path to the interpolation tables is used.
                This will stop the program, if the required table is not
                in the readonly path. The (writable) path_to_tables will be
                ignored. Default: xxx
            )pbdoc");

        py::class_<ContRand, std::shared_ptr<ContRand>>(m, "ContinuousRandomizer",
                R"pbdoc(
                If :math:`v_\text{cut}` is large, the spectrum is not continously any
                more. Particles which has no stochastic loss crossing the medium has
                all the same energy :math:`E_\text{peak}` after propagating through
                the medium.  This produce a gap of size of minimal stochastic loss.
                This effect can be reduced using continous randomization.

                .. figure:: figures/mu_continuous.png
                    :scale: 50%
                    :align: center

                    Propagate a muon with 100 TeV through 300 m Rock.

                The average energy loss from continous energy losses, is
                estimated by the energy-integral.

                .. math::

                    \int^{E_f}_{E_i} \frac{\sigma(E)}{-f(E)} d\text{E} = -\text{log}(\xi)

                Since probability of a energy loss :math:`e_{lost} < e_{cut}`
                at distance is finite, it produce fluctuations in average
                energy losses.

                This small losses can be added in form of a perturbation from
                average energy loss.
            )pbdoc")
        .def("variance", py::vectorize(&ContRand::Variance), py::arg("initial_energy"), py::arg("final_energy"))
        .def("randomize", py::vectorize(&ContRand::EnergyRandomize),
                py::arg("initial_energy"), py::arg("final_energy"), py::arg("rand"),
                R"pbdoc(
                    Calculates the stochastical smering of the distribution based on
                    the second momentum of the parametrizations, the final and intial
                    energy.

                    Note:
                        A normal distribution with uppere defined variance will be
                        asumed. The cumulative distibution function has the form:

                        .. math::

                            \text{cdf}(E) = \frac{1}{2} \left(1 + \text{erf} \left(
                                \frac{E}{ \sqrt{ 2 \sigma^2 } } \right) \right)

                        There will be sampled a deviation form mean energy :math:`a`
                        between :math:`\text{cdf}(E_\text{i} - E_\text{f})` and
                        :math:`\text{cdf}(E_\text{mass} - E_\text{f})` and finaly the
                        energy updated.

                        .. math::

                            E_\text{f} = \sigma \cdot a + E_\text{f}

                    Args:
                        initial_energy (float): energy befor stochastical loss
                        final_energy (float): energy after stochastical loss
                        rand (float): random number between 0 and 1

                    Returns:
                        float: randomized final energy
                )pbdoc")
        .def_readwrite_static("interpol_def", &ContRand::interpol_def);

        m.def("make_contrand", [](crosssection_list_t<ParticleDef, Medium> cross, bool interpolate){
                return shared_ptr<ContRand>(make_contrand(cross, interpolate));
                });

    py::class_<Decay, std::shared_ptr<Decay>>(m, "Decay")
        .def("energy_decay", py::vectorize(&Decay::EnergyDecay), py::arg("energy"), py::arg("rnd"), py::arg("density"))
        .def_readwrite_static("interpol_def", &Decay::interpol_def);

    m.def("make_decay", [](crosssection_list_t<ParticleDef, Medium> cross, ParticleDef const& particle, bool interpolate){
            return shared_ptr<Decay>(make_decay(cross, particle, interpolate));
            });

    py::class_<Time, std::shared_ptr<Time>>(m, "Time")
        .def("elapsed", &Time::TimeElapsed, py::arg("initial_energy"), py::arg("final_energy"), py::arg("distance"), py::arg("density"))
        .def_readwrite_static("interpol_def", &Time::interpol_def);

    m.def("make_time", [](crosssection_list_t<ParticleDef, Medium> cross, ParticleDef const& particle, bool interpolate){
            return shared_ptr<Time>(make_time(cross, particle, interpolate));
            });

    py::class_<PropagationUtility::Collection, std::shared_ptr<PropagationUtility::Collection>>(m, "PropagationUtilityCollection")
        .def(py::init<>())
        .def_readwrite("interaction", &PropagationUtility::Collection::interaction_calc)
        .def_readwrite("displacement", &PropagationUtility::Collection::displacement_calc)
        .def_readwrite("time", &PropagationUtility::Collection::time_calc)
        .def_readwrite("scattering", &PropagationUtility::Collection::scattering)
        .def_readwrite("decay", &PropagationUtility::Collection::decay_calc)
        .def_readwrite("cont_rand", &PropagationUtility::Collection::cont_rand);

    py::class_<PropagationUtility, std::shared_ptr<PropagationUtility>>(m, "PropagationUtility")
        .def(py::init<PropagationUtility::Collection const&>(), py::arg("collection"))
        .def("energy_stochasticloss", &PropagationUtility::EnergyStochasticloss)
        .def("energy_decay", &PropagationUtility::EnergyDecay)
        .def("energy_interaction", &PropagationUtility::EnergyInteraction)
        .def("energy_randomize", &PropagationUtility::EnergyRandomize)
        .def("energy_distance", &PropagationUtility::EnergyDistance)
        .def("length_continuous", &PropagationUtility::LengthContinuous)
        .def("length_continuous", &PropagationUtility::LengthContinuous)
        .def("directions_scatter", &PropagationUtility::DirectionsScatter);


    /* .def(py::init<const Utility&, const InterpolationDef>(), */
    /*     py::arg("utility"), py::arg("interpolation_def"), */
    /*     R"pbdoc( */
        /*             Initalize a continous randomization calculator. */
        /*             This may take some minutes because for all parametrization */
        /*             the continous randomization interpolation tables have to be */
        /*             build. */

        /*             Note: */
        /*                 .. math:: */

        /*                     \langle ( \Delta ( \Delta E ) )^2 \rangle = */
        /*                         \int^{e_\text{cut}}_{e_0} \frac{d\text{E}}{-f(E)} */
        /*                         \left( \int_0^{e_{cut}} e^2 p(e;E) d\text{e} \right) */

        /*             Args: */
        /*                 interpolation_def (interpolation_def): specify the number of interpolation points for cont-integral */
        /*                 utility (utility): specify the parametrization and energy cuts */
        /*         )pbdoc") */


    /* py::enum_<Sector::ParticleLocation::Enum>(m, "ParticleLocation") */
    /*     .value("infront_detector", Sector::ParticleLocation::InfrontDetector) */
    /*     .value("inside_detector", Sector::ParticleLocation::InsideDetector) */
    /*     .value("behind_detector", Sector::ParticleLocation::BehindDetector); */

    /* py::class_<Sector::Definition, std::shared_ptr<Sector::Definition>>(m, */
    /*     "SectorDefinition", */
    /*     R"pbdoc( */
    /*             Sector definition is a container that collects all important */
    /*             settings for propagation through a sector. There could for */
    /*             example specify the used cross sections or the energy cut */
    /*             settings. */

    /*             An example for multiple SectorDefinitions is the standard */
    /*             implementation of propagation. Three sectors are generated, */
    /*             which differ in the energy cut settings, to reduce computing */
    /*             power without a measurable loss of accuracy. Infront of the */
    /*             detector energy cut will be like :math:`v_\text{cut} = 0.05`. */
    /*             Inside the detector the energy cut :math:`e_\text{cut} = 500 */
    /*             \text{MeV}` is chosen so that particles below the detector */
    /*             resolution are statistically sampled. Outside of the detector no */
    /*             cuts are made to find a decay point of the particle in first */
    /*             approximation. */
    /*         )pbdoc") */
    /*     .def(py::init<>()) */
    /*     .def("__str__", &py_print<Sector::Definition>) */
    /*     .def_readwrite("cut_settings", &Sector::Definition::cut_settings, */
    /*         R"pbdoc( */
    /*                 Definition of the :meth:`EnergyCutSettings` */
    /*             )pbdoc") */
    /*     .def_property("medium", &Sector::Definition::GetMedium, */
    /*         &Sector::Definition::SetMedium, */
    /*         R"pbdoc( */
    /*                 Definition of the :meth:`~proposal.medium.Medium` */
    /*             )pbdoc") */
    /*     .def_property("geometry", &Sector::Definition::GetGeometry, */
    /*         &Sector::Definition::SetGeometry, */
    /*         R"pbdoc( */
    /*                 Definiton of the :meth:`~proposal.geometry.Geometry` */
    /*             )pbdoc") */
    /*     .def_readwrite("do_stochastic_loss_weighting", */
    /*         &Sector::Definition::do_stochastic_loss_weighting, */
    /*         R"pbdoc( */
    /*                 Boolean value whether the probability of producing a */
    /*                 stochastic loss should be adjusted with a factor, */
    /*                 defaults to False. */
    /*             )pbdoc") */
    /*     .def_readwrite("stochastic_loss_weighting", */
    /*         &Sector::Definition::stochastic_loss_weighting, */
    /*         R"pbdoc( */
    /*                 Factor used to scale the probability of producing a */
    /*                 stochastic loss, defaults to 1.0. */
    /*             )pbdoc") */
    /*     .def_readwrite("stopping_decay", &Sector::Definition::stopping_decay, */
    /*         R"pbdoc( */

    /*             )pbdoc") */
    /*     .def_readwrite("do_continuous_randomization", */
    /*         &Sector::Definition::do_continuous_randomization, */
    /*         R"pbdoc( */
    /*                 Boolean if continous randomization should be done if */
    /*                 interpolation if is used, defaults to true. */
    /*             )pbdoc") */
    /*     .def_readwrite("do_continuous_energy_loss_output", */
    /*         &Sector::Definition::do_continuous_energy_loss_output, */
    /*         R"pbdoc( */

    /*             )pbdoc") */
    /*     .def_readwrite("do_exact_time_calculation", */
    /*         &Sector::Definition::do_exact_time_calculation, */
    /*         R"pbdoc( */
    /*                 Boolean if particle speed could be approach by the speed */
    /*                 of light or should be calculated by solving the track */
    /*                 integral, defaults to false. */

    /*                 If the energy is in the order of the rest energy of the */
    /*                 particle, the assumption that the particle moves at the */
    /*                 speed of light becomes increasingly worse. Than it make */
    /*                 sense to calculate the time integral. */

    /*                 .. math:: */

    /*                         t_\text{f} = t_\text{i} + \int_{x_\text{i}}^{x_\text{f}} */
    /*                             \frac{ \text{dx} }{ v(x) } */
    /*             )pbdoc") */
    /*     .def_readwrite("scattering_model", */
    /*         &Sector::Definition::scattering_model, */
    /*         R"pbdoc( */
    /*                 Definition of the scattering modell of type :meth:`~proposal.scattering.ScatteringModel` */
    /*                 or deactivate scattering. */

    /*                 Example: */
    /*                     Deactivating scattering can be achieved with: */

    /*                     >>> sec = proposal.SectorDefinition() */
    /*                     >>> sec.scattering_model = proposal.scattering.ScatteringModel.NoScattering */
    /*             )pbdoc") */
    /*     .def_readwrite("particle_location", &Sector::Definition::location, */
    /*         R"pbdoc( */
    /*                 Definition of the relationship of the sectors to each */
    /*                 other of type :meth:`ParticleLocation`. */
    /*             )pbdoc") */
    /*     .def_readwrite("crosssection_defs", &Sector::Definition::utility_def, */
    /*         R"pbdoc( */
    /*                 Definition of the crosssection of type :meth:`~proposal.UtilityDefinition` */
    /*             )pbdoc"); */

    /* py::class_<Sector, std::shared_ptr<Sector>>(m, "Sector", R"pbdoc( */
    /*         A sector is characterized by its homogeneous attitudes. */
    /*         Within a sector there are no boundaries to consider. */
    /*     )pbdoc") */
    /*     .def(py::init<ParticleDef&, const Sector::Definition&>(), */
    /*         py::arg("particle_def"), py::arg("sector_definition")) */
    /*     .def(py::init<ParticleDef&, const Sector::Definition&, */
    /*              const InterpolationDef&>(), */
    /*         py::arg("particle_def"), py::arg("sector_definition"), */
    /*         py::arg("interpolation_def")) */
    /*     .def("energy_decay", &Sector::EnergyDecay, py::arg("initial_energy"), */
    /*         py::arg("rnd")) */
    /*     .def("energy_interaction", &Sector::EnergyInteraction, */
    /*         py::arg("initial_energy"), py::arg("rnd")) */
    /*     .def("energy_minimal", &Sector::EnergyMinimal, */
    /*         py::arg("initial_energy"), py::arg("cut")) */
    /*     .def("energy_distance", &Sector::EnergyDistance, */
    /*         py::arg("initial_energy"), py::arg("distance")) */
    /*     .def("make_stochastic_loss", &Sector::MakeStochasticLoss, */
    /*         py::arg("minimal_energy")) */
    /*     .def("propagate", &Sector::Propagate, */
    /*         py::arg("particle_condition"), py::arg("max_distance"), py::arg("min_energy")); */

    /* py::class_<RandomGenerator, std::unique_ptr<RandomGenerator, py::nodelete>>( */
    /*     m, "RandomGenerator") */
    /*     .def("random_double", &RandomGenerator::RandomDouble) */
    /*     .def("set_seed", &RandomGenerator::SetSeed, py::arg("seed") = 0) */
    /*     .def("set_random_function", &RandomGenerator::SetRandomNumberGenerator, */
    /*         py::arg("function")) */
    /*     .def_static( */
    /*         "get", &RandomGenerator::Get, py::return_value_policy::reference); */

    /* py::class_<Propagator, std::shared_ptr<Propagator>>(m, "Propagator") */
    /*     .def(py::init<ParticleDef const&, std::vector<Sector>>()) */
    /*     .def(py::init<GammaDef, std::string const&>()) */
    /*     .def(py::init<EMinusDef, std::string const&>()) */
    /*     .def(py::init<EPlusDef, std::string const&>()) */
    /*     .def(py::init<MuMinusDef, std::string const&>()) */
    /*     .def(py::init<MuPlusDef, std::string const&>()) */
    /*     .def(py::init<TauMinusDef, std::string const&>()) */
    /*     .def(py::init<TauPlusDef, std::string const&>()) */
    /*     .def("propagate", &Propagator::Propagate, py::arg("initial_particle"), py::arg("max_distance"), py::arg("min_energy")); */

    /*     .def( */
    /*         py::init<const ParticleDef&, const std::vector<Sector::Definition>&, */
    /*             std::shared_ptr<const Geometry>, const InterpolationDef&>(), */
    /*         py::arg("particle_def"), py::arg("sector_defs"), */
    /*         py::arg("detector"), py::arg("interpolation_def"), R"pbdoc( */
    /*                 Function Docstring. */

    /*                 Args: */
    /*                     particle_def (proposal.particle.ParticleDef): definition of the particle to propagate describing the basic properties. */
    /*                     sector_defs (List[proposal.SectorDefinition]): list of the sectors describing the environment around the detector. */
    /*                     detector (proposal.geometry.Geometry): geometry of the detector. */
    /*                     interpolation_def (proposal.InterpolationDef): definition of the Interpolation tables like path to store them, etc. */
    /*             )pbdoc") */
    /*     .def(py::init<const ParticleDef&, */
    /*              const std::vector<Sector::Definition>&, std::shared_ptr<const Geometry>>(), */
    /*         py::arg("particle_def"), py::arg("sector_defs"), */
    /*         py::arg("detector")) */
    /*     .def(py::init<const ParticleDef&, const std::string&>(), */
    /*         py::arg("particle_def"), py::arg("config_file")) */
    /*     .def("propagate", &Propagator::Propagate, */
    /*         py::arg("particle_condition"), */
    /*         py::arg("max_distance_cm") = 1e20, */
    /*         py::arg("minimal_energy") = 0., */
    /*         py::return_value_policy::reference, */
    /*         R"pbdoc( */
    /*                 Propagate a particle through sectors and produce stochastic */
    /*                 losses, untill propagated distance is reached. */

    /*                 Args: */
    /*                     max_distance_cm (float): Maximum distance a particle is */
    /*                         propagated before it is considered lost. */

    /*                 Returns: */
    /*                     list(DynamicData): list of stochastic losses parameters */
    /*                     list(list): list of stochastic losses parameters */

    /*                 Example: */
    /*                     Propagate 1000 particle with an inital energy of 100 */
    /*                     Tev and save the losses in daughters. */

    /*                     >>> for i in range(int(1e3)): */
    /*                     >>>   mu.energy = 1e8 */
    /*                     >>>   mu.propagated_distance = 0 */
    /*                     >>>   mu.position = pp.Vector3D(0, 0, 0) */
    /*                     >>>   mu.direction = pp.Vector3D(0, 0, -1) */
    /*                     >>>   daughters = prop.propagate() */

    /*                 Further basic condition are set at the next point of */
    /*                 interaction during generation. */
    /*                 The aim is to introduce forced stochastic losses */
    /*                 so that the particle can be propagated through homogeneous */
    /*                 sectors. */
    /*                 A more percise description of propagation through a */
    /*                 homoegenous sector can be found in ???. */

    /*                 .. figure:: figures/sector.png */
    /*                     :height: 200px */
    /*                     :align: center */

    /*                     If the next interaction point is outside the actuell */
    /*                     sector, the particle is forced to an interaction at the */
    /*                     sector boundary. Thus it can be assumed that the */
    /*                     particle is porpagated by a homogeneous medium. */

    /*                 The propagated particle will be forced to interact in the */
    /*                 closest point to the dector center. */
    /*                 This will be stored in the closest_approach variable of */
    /*                 the propagated particle. */

    /*                 The propagation terminate if the maximal distance is */
    /*                 reached. */
    /*                 The energy loss of the particle (e_lost) in the detector */
    /*                 will be calculated and the produced secondary particles */
    /*                 returned. */
    /*             )pbdoc") */
    /*     .def_property_readonly("particle_def", &Propagator::GetParticleDef, */
    /*         R"pbdoc( */
    /*                 Get the internal particle definition to use its properties. */

    /*                 Returns: */
    /*                     ParticleDefinition: the definition of the propagated particle. */
    /*             )pbdoc") */
    /*     .def_property_readonly("sector", &Propagator::GetCurrentSector, */
    /*         R"pbdoc( */
    /*                 "Get the current sector" */

    /*                 Returns: */
    /*                     Sector: the current sector, where the particle is at the moment. */
    /*             )pbdoc") */
    /*     .def_property_readonly("detector", &Propagator::GetDetector, */
    /*         R"pbdoc( */
    /*                 Get the detector geometry. */

    /*                 Returns: */
    /*                     Geometry: the geometry of the detector. */
    /*             )pbdoc"); */


    /* py::class_<PropagatorService, std::shared_ptr<PropagatorService>>( */
    /*     m, "PropagatorService") */
    /*     .def(py::init<>()) */
    /*     .def("propagate", &PropagatorService::Propagate, */
    /*         py::arg("particle_definition"), py::arg("particle_condition"), */
    /*         py::arg("max_distance") = 1e20, py::arg("min_energy") = 1e20) */
    /*     .def("register_propagator", &PropagatorService::RegisterPropagator, */
    /*         py::arg("propagator")); */
}

// #undef COMPONENT_DEF
// #undef MEDIUM_DEF
// #undef AXIS_DEF
// #undef BREMS_DEF
// #undef PHOTO_REAL_DEF
// #undef PHOTO_Q2_DEF
// #undef PHOTO_Q2_INTERPOL_DEF
// #undef EPAIR_DEF
// #undef EPAIR_INTERPOL_DEF
// #undef MUPAIR_DEF
// #undef MUPAIR_INTERPOL_DEF
