#include "DodoParser.h"
#include <format>
#include <abstracton/interpretations.hpp>
#include <abstracton/abstracton.hpp>
#include <abstracton/utils/utils.hpp>
#include <abstracton/mata_extensions.hpp>
#include <chrono>

#define TICK() if (measure_time) { begin = std::chrono::steady_clock::now(); }
#define TOCK(message) if (measure_time) { end = std::chrono::steady_clock::now(); std::cout << "Time needed for " << message << ": " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl; }

int main(int argc, char** argv) {
    using namespace logging;

    if (argc < 2) {
        std::cout << "SYNOPSIS\n";
        std::cout << "\tsolve_dodo FILENAME [OPTIONS]\n";
        std::cout << "ARGUMENTS\n";
        std::cout << "\tFILENAME: path to filename of dodo problem instance\n";
        std::cout << "\tOPTIONS\n";
        std::cout << "\t\t-i ITYPE: which interpretation to use:\n";
        std::cout << "\t\t\tt: trap\n";
        std::cout << "\t\t\ts: siphon\n";
        std::cout << "\t\t\tf: flow\n";
        std::cout << "\t\t...if no ITYPE is given, the algorithm is run for all of the above.\n";
        std::cout << "\t\t-v NUM\n";
        std::cout << "\t\t\tsets verbosity to NUM, which may be any of the following four:\n";
        std::cout << "\t\t\t  0: QUIET\n";
        std::cout << "\t\t\t  1: NORMAL\n";
        std::cout << "\t\t\t  2: VERBOSE\n";
        std::cout << "\t\t\t  3: DEBUG\n";
        std::cout << "\t\t-p PROPERTY\n";
        std::cout << "\t\t\tonly checks abstract safety for property PROPERTY\n";
        std::cout << "\t\t--minimize-input\n";
        std::cout << "\t\t\talso minimize input (automata for initial configurations, transition relation and unsafe configurations)\n";
        std::cout << "\t\t--universality-alg ALG\n";
        std::cout << "\t\t\tChoose algorithm to be used for checking abstract safety. ALG can be one of:\n";
        std::cout << "\t\t\t - antichains (default): use lazy construction with subsumption checking\n";
        std::cout << "\t\t\t - antichains-inclusion: first reduce to inclusion problem and then use antichains\n";
        std::cout << "\t\t\t - lazy:                 use lazy construction without subsumption checking\n";
        std::cout << "\t\t\t - explicit:             explicitly construct PReach\n";
        std::cout << "\t\t\tExample: solve_dodo input.json --universality-alg antichains-inclusion\n";
        std::cout << "\t\t\tExplicitly construct intersection PReach(initial) with final\n";
        std::cout << "\t\t--measure-time\n";
        std::cout << "\t\t\tUse system time to measure performance of some steps\n";
        return 0;
    }
    std::vector<enum SetInterpretation> interpretations{};
    int verbosityLevel = VerbosityLevel::NORMAL;
    std::optional<std::string> property = std::nullopt;
    std::vector<std::string> propertyNames{};
    bool minimize_input = false;
    std::string universality_alg{"antichains"};
    bool measure_time = false;
    std::string filename{};
    // no switch for strings in c++...so if else branches:
    for (int i{ 1 }; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-i" || arg == "--interpretation") {
            if (i + 1 < argc) {
                std::string itype(argv[i + 1]);
                ++i;
                if (itype == "t" || itype == "trap") {
                    interpretations.push_back(Trap);
                } else if (itype == "s" || itype == "siphon") {
                    interpretations.push_back(Siphon);
                } else if (itype == "f" || itype == "flow") {
                    interpretations.push_back(Flow);
                } else {
                    std::cout << "ITYPE must be any of the following: t, s, f\n";
                }
            } else {
                std::cout << "need to specify ITYPE after " << argv[i] << " option";
                std::cout << ", which must be any of the following: t, s, f\n";
            }
        } else if (arg == "-v" || arg == "--verbosity") {
            if (i + 1 < argc && atoi(argv[i + 1]) >= 0 && atoi(argv[i + 1]) <= 3) {
                verbosityLevel = atoi(argv[i + 1]);
                ++i;
            } else {
                std::cout << "need to give verbosity level 0, 1, 2 or 3 after " << argv[i] << " option\n";
                return 0;
            }
        } else if (arg == "-p" || arg == "--property") {
            if (i + 1 < argc) {
                property = std::make_optional(std::string(argv[i + 1]));
                ++i;
            } else {
                std::cout << "need to name property after " << argv[i] << " option\n";
                return 0;
            }
        } else if (arg == "--minimize-input") {
            minimize_input = true;
        } else if (arg == "--universality-alg") {
            std::vector<std::string> possibilities{"antichains", "antichains-inclusion", "lazy", "explicit"};
            if (i + 1 < argc &&
                    std::find(possibilities.begin(), possibilities.end(), std::string(argv[i + 1])) != possibilities.end()) {
                universality_alg = std::string(argv[i + 1]);
                ++i;
            } else {
                std::cout << "need to name algorithm after " << argv[i] << " option. Possible choices:\n";
                for (const std::string& possibility : possibilities) {
                    std::cout << "\t" << possibility << std::endl;
                }
                return 0;
            }
        } else if (arg == "--measure-time") {
            measure_time = true;
        } else if (arg.find(".json") != std::string::npos) {
            filename = arg;
        } else {
            std::cout << "unknown argument \"" << argv[i] << "\"\n";
            return 0;
        }
    }
    if (interpretations.empty()) {
        log(VerbosityLevel::VERBOSE, "did not receive particular interpretation, defaulting to checking all three of t, s, f", verbosityLevel);
        interpretations = {Trap, Siphon, Flow};
    }

    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point end;

    TICK();
    DodoParserResult dpr = parseDodoJSON(filename, verbosityLevel);
    TOCK("parsing json file");

    log(VerbosityLevel::VERBOSE, "parsed file " + filename + ".", verbosityLevel);
    log(VerbosityLevel::DEBUG, "alphabet: " + stream_to_string(dpr.string_alphabet->get_alphabet_symbols()), verbosityLevel);
    assert(dpr.transitionRelation.alphabets != nullptr
           && dpr.string_alphabet->is_equal(dpr.transitionRelation.alphabets->for_level(0)));
    log(VerbosityLevel::DEBUG, "initial configurations:", verbosityLevel);
    logexp(VerbosityLevel::DEBUG, [&]() { return dpr.initialConfig.print_to_dot(); }, verbosityLevel);
    log(VerbosityLevel::DEBUG, "transition relation:", verbosityLevel);
    logexp(VerbosityLevel::DEBUG, [&]() { return dpr.transitionRelation.print_to_dot(); }, verbosityLevel);
    assert(dpr.properties.size() == dpr.propertyNames.size());
    log(VerbosityLevel::DEBUG, "properties:", verbosityLevel);
    for (int i{ 0 }; i < dpr.properties.size(); ++i) {
        log(VerbosityLevel::DEBUG, dpr.propertyNames[i], verbosityLevel);
        logexp(VerbosityLevel::DEBUG, [&]() { return dpr.properties[i].print_to_dot(); }, verbosityLevel);
    }

    // TODO only minimize relevant input, e.g. don't minimize properties that should not be checked
    if (minimize_input) {
        log(VerbosityLevel::VERBOSE, "minimizing input...", verbosityLevel);

        log(VerbosityLevel::VERBOSE, std::format("automaton for initialConfig has {} states.", dpr.initialConfig.num_of_states()), verbosityLevel);
        TICK();
        dpr.initialConfig = minimize_nfa(dpr.initialConfig);
        TOCK("minimizing initial configs");
        log(VerbosityLevel::VERBOSE, std::format("minimized automaton for initialConfig has {} states.", dpr.initialConfig.num_of_states()), verbosityLevel);

        log(VerbosityLevel::VERBOSE, std::format("automaton for transitionRelation has {} states.", dpr.transitionRelation.num_of_states()), verbosityLevel);
        TICK();
        dpr.transitionRelation = mata::ext::minimize(dpr.transitionRelation);
        TOCK("minimizing transition relation");
        log(VerbosityLevel::VERBOSE, std::format("minimized automaton for transitionRelation has {} states.", dpr.transitionRelation.num_of_states()), verbosityLevel);

        for (int i{ 0 }; i < dpr.properties.size(); ++i) {
            if (!property.has_value() || property.value() == dpr.propertyNames[i]) {
                log(VerbosityLevel::VERBOSE, std::format("automaton for property {} has {} states.", dpr.propertyNames[i], dpr.properties[i].num_of_states()), verbosityLevel);
                TICK();
                dpr.properties[i] = minimize_nfa(dpr.properties[i]);
                TOCK(std::format("minimizing property {}", dpr.propertyNames[i]));
                log(VerbosityLevel::VERBOSE, std::format("minimized automaton for property {} has {} states.", dpr.propertyNames[i], dpr.properties[i].num_of_states()), verbosityLevel);
            }
        }
    }

    for (enum SetInterpretation itype : interpretations) {
        log(VerbosityLevel::QUIET, "using " + to_string(itype) + " interpretation", verbosityLevel);
        // build interpretation
        TICK();
        std::pair<mata::nft::Nft, std::shared_ptr<mata::OnTheFlyAlphabet>> ipa = trapInterpretation(dpr.string_alphabet.get(), itype);
        TOCK("constructing " + to_string(itype) + " interpretation transducer");
        mata::nft::Nft interpretation = ipa.first;
        std::shared_ptr<mata::OnTheFlyAlphabet> powerset_alphabet_ptr = ipa.second;
        logexp(VerbosityLevel::DEBUG, [&]() { return interpretation.print_to_dot(); }, verbosityLevel);

        // compute ind
        log(VerbosityLevel::NORMAL, "computing ind...", verbosityLevel);

        TICK();
        mata::nfa::Nfa ind {compute_ind(interpretation, dpr.transitionRelation, *dpr.string_alphabet, *powerset_alphabet_ptr, false)};
        TOCK("computing ind");

        log(VerbosityLevel::NORMAL, std::format("automaton for ind has {} states.", ind.num_of_states()), verbosityLevel);

        TICK();
        ind = mata::nfa::minimize(ind);
        TOCK("minimizing ind");

        log(VerbosityLevel::NORMAL, std::format("minimized automaton for ind has {} states.", ind.num_of_states()), verbosityLevel);
        log(VerbosityLevel::NORMAL, "...done.", verbosityLevel);
        log(VerbosityLevel::VERBOSE, "automaton for ind:", verbosityLevel);
        logexp(VerbosityLevel::VERBOSE, [&]() { return ind.print_to_dot(); }, verbosityLevel);

        // already construct properties that need to be checked (needed in both branches of if)
        std::vector<mata::nfa::Nfa> properties_to_check{};
        if (property.has_value()) {
            for (int i{ 0 }; i < dpr.propertyNames.size(); ++i) {
                if (dpr.propertyNames[i] == property.value()) {
                    properties_to_check.push_back(dpr.properties[i]);
                    propertyNames.push_back(dpr.propertyNames[i]);
                }
            }
        } else {
            properties_to_check = dpr.properties;
            propertyNames = dpr.propertyNames;
        }

        std::vector<bool> result;
        if (universality_alg == "explicit") {
            // compute preach
            log(VerbosityLevel::NORMAL, "computing preach...", verbosityLevel);

            TICK();
            mata::nft::Nft preach {compute_preach(interpretation, dpr.transitionRelation, *dpr.string_alphabet, *powerset_alphabet_ptr, std::make_optional<const mata::nfa::Nfa>(ind))};
            TOCK("computing preach");

            log(VerbosityLevel::NORMAL, std::format("automaton for preach has {} states.", preach.num_of_states()), verbosityLevel);

            TICK();
            preach = preach.trim();
            TOCK("trimming preach");
            TICK();
            preach = mata::ext::minimize(preach);
            TOCK("minimizing preach");

            log(VerbosityLevel::NORMAL, std::format("minimized automaton for preach has {} states.", preach.num_of_states()), verbosityLevel);
            log(VerbosityLevel::NORMAL, "...done.", verbosityLevel);
            log(VerbosityLevel::VERBOSE, "automaton for preach:", verbosityLevel);
            logexp(VerbosityLevel::VERBOSE, [&]() { return preach.print_to_dot(); }, verbosityLevel);

            // check whether safety can be proven, i.e. whether abstract safety holds
            log(VerbosityLevel::NORMAL, "trying to prove abstract safety...", verbosityLevel);
            TICK();
            result = check_abstract_safety_explicit(dpr.initialConfig, preach, properties_to_check, verbosityLevel, measure_time);
            TOCK("checking abstract safety");
        } else {
            // lazily check intersection of preach using complement of preach
            log(VerbosityLevel::NORMAL, "computing complement of preach...", verbosityLevel);

            TICK();
            mata::nft::Nft preach_comp {compute_preach_complement(interpretation, dpr.transitionRelation, *dpr.string_alphabet, *powerset_alphabet_ptr, std::make_optional<const mata::nfa::Nfa>(ind))};
            TOCK("computing complement of preach");

            log(VerbosityLevel::NORMAL, std::format("automaton for complement of preach has {} states.", preach_comp.num_of_states()), verbosityLevel);
            log(VerbosityLevel::NORMAL, std::format("...of which, {} states are at level 0.", preach_comp.num_of_states_with_level(0)), verbosityLevel);

            TICK();
            preach_comp = preach_comp.trim();
            TOCK("trimming complement of preach");
            // TICK();
            // preach_comp = mata::ext::minimize(preach_comp); // TODO probably should just delete this??? (Not needed for antichains, and determinizes...)
            //
            // TOCK("minimizing complement of preach");

            // log(VerbosityLevel::NORMAL, std::format("minimized automaton for complement of preach has {} states.", preach_comp.num_of_states()), verbosityLevel);
            log(VerbosityLevel::NORMAL, "...done.", verbosityLevel);
            log(VerbosityLevel::VERBOSE, "automaton for complement of preach:", verbosityLevel);
            logexp(VerbosityLevel::VERBOSE, [&]() { return preach_comp.print_to_dot(); }, verbosityLevel);

            // check whether safety can be proven, i.e. whether abstract safety holds
            log(VerbosityLevel::NORMAL, "trying to prove abstract safety...", verbosityLevel);
            TICK();
            result = check_abstract_safety_lazy(dpr.initialConfig, preach_comp, properties_to_check, *dpr.string_alphabet, universality_alg, verbosityLevel, measure_time);
            TOCK("checking abstract safety");
        }

        log(VerbosityLevel::NORMAL, "...done.", verbosityLevel);
        log(VerbosityLevel::QUIET, "Result: " + vec_to_string(result), verbosityLevel);
        logexp(VerbosityLevel::QUIET, [&]() {
            std::ostringstream oss;
            std::vector<std::string> proven {};
            std::vector<std::string> unproven {};
            for (int i{ 0 }; i < result.size(); ++i) {
                if (result[i]) {
                    proven.push_back(propertyNames[i]);
                } else {
                    unproven.push_back(propertyNames[i]);
                }
            }
            oss << "This means that the following properties could be separated from the initial configurations:\n";
            oss << "\t" << vec_to_string(proven) << "\n";
            oss << "While the following properties could not be separated from the initial configurations:\n";
            oss << "\t" << vec_to_string(unproven) << "\n";
            return oss.str();
        }, verbosityLevel, false, false);
    }

    return 0;
}
