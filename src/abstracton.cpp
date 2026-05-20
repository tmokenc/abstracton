#include <mata/nfa/nfa.hh>
#include <mata/nft/nft.hh>
#include <mata/nft/builder.hh>
#include <abstracton/mata_extensions.hpp>
#include <abstracton/abstracton.hpp>
#include <abstracton/utils/utils.hpp>

#define INIT_CLOCKS() std::chrono::steady_clock::time_point begin, end;
#define TICK() if (measure_time) { begin = std::chrono::steady_clock::now(); }
#define TOCK(message) if (measure_time) { end = std::chrono::steady_clock::now(); std::cout << "Time needed for " << message << ": " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl; }

using namespace mata;
using namespace mata::nfa;
using namespace mata::nft;

// input: DETERMINISTIC abstraction framework!
// TODO convert output to debug output
// TODO exclude all a where V(a) = emptyset (optional, but nicer...)
Nfa compute_ind(const Nft& abstraction_framework, const Nft& transition_relation, Alphabet& concrete_alphabet, Alphabet& abstract_alphabet, bool exclude_empty_abstractions, int verbosityLevel) {
    // project_1(Id intersect (V delta complement(inverse(V)))), then complement
    Nft v_delta {compose(abstraction_framework, transition_relation)};
    if (v_delta.levels.num_of_levels != 2) {
        std::cout << "compose function of nfts with ";
        std::cout << abstraction_framework.levels.num_of_levels;
        std::cout << ", ";
        std::cout << transition_relation.levels.num_of_levels;
        std::cout << " levels returned nft with ";
        std::cout << v_delta.levels.num_of_levels;
        std::cout << " levels." << std::endl;
        if (v_delta.is_lang_empty()) {
            std::cout << "language of nft result is empty.\n";
            // complement of ind is empty, ind is sigma star
            Nfa ind{ mata::nfa::builder::create_sigma_star_nfa(&abstract_alphabet) };
            std::cout << "sigma star nfa:\n";
            std::cout << ind.print_to_dot() << std::endl;
            if (exclude_empty_abstractions) {
                // intersect with pi_1(V)
                return mata::nfa::intersection(ind, project(abstraction_framework, 0));
            } else {
                return ind;
            }

        } else {
            std::cout << "language of nft result is not empty. Don't know what to do. Please inspect result NFT.\n";
            throw 2;
        }
    }
    std::vector<Alphabet*> alphabets {&abstract_alphabet, &concrete_alphabet};
    Nft v_complement {mata::ext::complement(abstraction_framework, nullptr, std::make_optional<std::vector<Alphabet*>>(alphabets), true)};
    logging::log(logging::VerbosityLevel::DEBUG, "complement of abstraction framework:", verbosityLevel);
    logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return v_complement.print_to_dot(); }, verbosityLevel);

    Nft product1 {compose(v_delta, v_complement, 1, 1)};
    if (product1.levels.num_of_levels != 2) {
        std::cout << "nft result of composition does not have 2 levels, need to handle.";
        throw 2;
    }
    Nft preprojection {mata::nft::intersection(create_identity(abstract_alphabet), product1)};
    Nfa projection{ project(preprojection, 0) };
    Nfa ind{ mata::nfa::complement(projection, abstract_alphabet) };
    if (exclude_empty_abstractions) {
        // intersect with pi_1(V)
        logging::log(logging::VerbosityLevel::DEBUG, "ind with empty abstractions:", verbosityLevel);
        logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return ind.print_to_dot(); }, verbosityLevel);
        return mata::nfa::intersection(ind, project(abstraction_framework, 0));
    } else {
        return ind;
    }
}
Nft compute_preach_complement(const Nft& abstraction_framework, const Nft& transition_relation, Alphabet& concrete_alphabet, Alphabet& abstract_alphabet, std::optional<const Nfa> ind, int verbosityLevel) {
    // inverse(V) id_Ind complement(V), then complement
    Nfa ind_result = ind.value_or(compute_ind(abstraction_framework, transition_relation, concrete_alphabet, abstract_alphabet, false, verbosityLevel));

    std::vector<Alphabet*> alphabets {&abstract_alphabet, &concrete_alphabet};
    Nft v_complement {mata::ext::complement(abstraction_framework, nullptr, std::make_optional<std::vector<Alphabet*>>(alphabets), true)}; // TODO only calculate once (not in ind and preach)

    Nft id_ind {create_identity(ind_result)};

    Nft v_id {compose(abstraction_framework, id_ind, 0, 0)};
    if (v_id.levels.num_of_levels != 2) {
        std::cout << "nft result of composition does not have 2 levels, need to handle.";
        throw 2;
    }
    Nft product {compose(v_id, v_complement)};
    if (product.levels.num_of_levels != 2) {
        std::cout << "nft result of composition does not have 2 levels, need to handle.";
        throw 2;
    }

    return product;
}

Nft compute_preach(const Nft& abstraction_framework, const Nft& transition_relation, Alphabet& concrete_alphabet, Alphabet& abstract_alphabet, std::optional<const Nfa> ind, int verbosityLevel) {
    return mata::ext::complement(compute_preach_complement(abstraction_framework, transition_relation, concrete_alphabet, abstract_alphabet, ind, verbosityLevel), &concrete_alphabet);
}

std::vector<bool> check_abstract_safety_explicit(const mata::nfa::Nfa& initial_configurations, const mata::nft::Nft& preach, std::vector<mata::nfa::Nfa> unsafe_properties, int verbosityLevel, bool measure_time) {
    INIT_CLOCKS();
    TICK();
    Nfa preach_image = apply(preach, initial_configurations);
    TOCK("calculating image of initial configs under preach");
    std::vector<bool> result{};
    for (const mata::nfa::Nfa& unsafe_property : unsafe_properties) {
        TICK();
        result.push_back(intersection(preach_image, unsafe_property).is_lang_empty());
        TOCK("calculating intersection of preach image with unsafe configs");
    }
    return result;
}

std::vector<bool> check_abstract_safety_lazy(const mata::nfa::Nfa& initial_configurations, const mata::nft::Nft& preach_complement, std::vector<mata::nfa::Nfa> unsafe_properties, Alphabet& concrete_alphabet, std::string universality_alg, int verbosityLevel, bool measure_time) {
    INIT_CLOCKS();

    mata::nft::Nft initial_nft = mata::nft::builder::from_nfa_with_levels_advancing(initial_configurations, 1);
    std::vector<bool> result{};
    for (const mata::nfa::Nfa& unsafe_property : unsafe_properties) {
        TICK();
        mata::nft::Nft unsafe_property_nft = mata::nft::builder::from_nfa_with_levels_advancing(unsafe_property, 1);
        TOCK("constructing transducer for unsafe property");
        logging::log(logging::VerbosityLevel::DEBUG, "transducer for unsafe property:", verbosityLevel);
        logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return unsafe_property_nft.print_to_dot(); }, verbosityLevel);

        TICK();
        mata::nft::Nft initial_unsafe_pairs = mata::ext::relational_product_length_preserving_dont_care({initial_nft, unsafe_property_nft});
        TOCK("constructing transducer for (initial, unsafe) pairs");
        logging::log(logging::VerbosityLevel::DEBUG, "transducer for (initial, unsafe)-pairs:", verbosityLevel);
        logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return initial_unsafe_pairs.print_to_dot(); }, verbosityLevel);

        TICK();
        mata::nft::Nft initial_unsafe_complement = mata::ext::complement(initial_unsafe_pairs, &concrete_alphabet, std::nullopt, true);
        TOCK("constructing transducer for complement of (initial, unsafe) pairs");
        logging::log(logging::VerbosityLevel::DEBUG, "transducer for complement of (initial, unsafe)-pairs:", verbosityLevel);
        logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return initial_unsafe_complement.print_to_dot(); }, verbosityLevel);

        TICK();
        mata::nft::Nft union2 = mata::nft::union_nondet(preach_complement, initial_unsafe_complement);
        TOCK("building union of complements of preach and (initial, unsafe)-transducer");
        Run cex; // possible counterexample, could e.g. be printed or returned... TODO
        // TODO select best algorithm here...
        if (universality_alg == "lazy") {
            TICK();
            result.push_back(mata::ext::is_universal_lazy(union2, {&concrete_alphabet, &concrete_alphabet}, &cex));
            TOCK("checking universality using " + universality_alg + " algorithm");
        } else if (universality_alg == "antichains-inclusion") {
            TICK();
            result.push_back(mata::ext::is_universal_antichains_by_inclusion(union2, {&concrete_alphabet, &concrete_alphabet}, &cex));
            TOCK("checking universality using " + universality_alg + " algorithm");
        } else {
            if (universality_alg != "antichains") {
                logging::log(logging::VerbosityLevel::VERBOSE, "WARNING: unknown universality algorithm \"" + universality_alg + "\", defaulting to antichains.", verbosityLevel);
            }
            TICK();
            result.push_back(mata::ext::is_universal_antichains(union2, {&concrete_alphabet, &concrete_alphabet}, &cex));
            TOCK("checking universality using " + universality_alg + " algorithm");
        }

        if (!result[result.size() - 1]) {
            logging::log(logging::VerbosityLevel::VERBOSE, "counterexample (abstractly reachable):", verbosityLevel);
            logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return vec_to_string(cex.word); }, verbosityLevel);
        }
    }
    return result;
}
