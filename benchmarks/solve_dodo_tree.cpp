#include "DodoParser.h"

#include <abstracton/interpretations.hpp>
#include <abstracton/mata_extensions.hpp>
#include <abstracton/utils/utils.hpp>
#include <mata/nfa/builder.hh>
#include <mata/nft/lazy.hh>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#define TICK()                                                                 \
  if (measure_time) {                                                          \
    begin = std::chrono::steady_clock::now();                                  \
  }
#define TOCK(message)                                                          \
  if (measure_time) {                                                          \
    end = std::chrono::steady_clock::now();                                    \
    std::cout << "Time needed for " << message << ": "                         \
              << std::chrono::duration_cast<std::chrono::microseconds>(end -   \
                                                                       begin)  \
                     .count()                                                  \
              << "[\xC2\xB5s]" << std::endl;                                   \
  }

namespace {

std::vector<int>
get_selected_property_indices(const std::vector<std::string> &property_names,
                              const std::optional<std::string> &property) {
  std::vector<int> indices{};

  if (property.has_value()) {
    for (size_t i = 0; i < property_names.size(); ++i) {
      if (property_names[i] == property.value()) {
        indices.push_back(static_cast<int>(i));
      }
    }
    return indices;
  }

  indices.reserve(property_names.size());
  for (size_t i = 0; i < property_names.size(); ++i) {
    indices.push_back(static_cast<int>(i));
  }
  return indices;
}

} // namespace

int main(int argc, char **argv) {
  using namespace logging;
  using namespace mata::nft::lazy;

  if (argc < 2) {
    std::cout << "SYNOPSIS\n";
    std::cout << "\tsolve_dodo_tree FILENAME [OPTIONS]\n";
    std::cout << "ARGUMENTS\n";
    std::cout << "\tFILENAME: path to filename of dodo problem instance\n";
    std::cout << "\tOPTIONS\n";
    std::cout << "\t\t-i ITYPE: which interpretation to use:\n";
    std::cout << "\t\t\tt: trap\n";
    std::cout << "\t\t\ts: siphon\n";
    std::cout << "\t\t\tf: flow\n";
    std::cout << "\t\t...if no ITYPE is given, the algorithm is run for all of "
                 "the above.\n";
    std::cout << "\t\t-v NUM\n";
    std::cout << "\t\t\tsets verbosity to NUM, which may be any of the "
                 "following four:\n";
    std::cout << "\t\t\t  0: QUIET\n";
    std::cout << "\t\t\t  1: NORMAL\n";
    std::cout << "\t\t\t  2: VERBOSE\n";
    std::cout << "\t\t\t  3: DEBUG\n";
    std::cout << "\t\t-p PROPERTY\n";
    std::cout << "\t\t\tonly checks abstract safety for property PROPERTY\n";
    std::cout << "\t\t--minimize-input\n";
    std::cout
        << "\t\t\talso minimize input (automata for initial configurations, "
           "transition relation and unsafe configurations)\n";
    std::cout << "\t\t--measure-time\n";
    std::cout << "\t\t\tUse system time to measure performance of some steps\n";
    return 0;
  }

  std::vector<enum SetInterpretation> interpretations{};
  int verbosity_level = VerbosityLevel::NORMAL;
  std::optional<std::string> property = std::nullopt;
  bool minimize_input = false;
  bool measure_time = false;
  std::string filename{};

  for (int i{1}; i < argc; ++i) {
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
          return 0;
        }
      } else {
        std::cout << "need to specify ITYPE after " << argv[i] << " option";
        std::cout << ", which must be any of the following: t, s, f\n";
        return 0;
      }
    } else if (arg == "-v" || arg == "--verbosity") {
      if (i + 1 < argc && atoi(argv[i + 1]) >= 0 && atoi(argv[i + 1]) <= 3) {
        verbosity_level = atoi(argv[i + 1]);
        ++i;
      } else {
        std::cout << "need to give verbosity level 0, 1, 2 or 3 after "
                  << argv[i] << " option\n";
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
    log(VerbosityLevel::VERBOSE,
        "did not receive particular interpretation, defaulting to checking all "
        "three of t, s, f",
        verbosity_level);
    interpretations = {Trap, Siphon, Flow};
  }

  std::chrono::steady_clock::time_point begin;
  std::chrono::steady_clock::time_point end;

  TICK();
  DodoParserResult dpr = parseDodoJSON(filename, verbosity_level);
  TOCK("parsing json file");

  if (minimize_input) {
    log(VerbosityLevel::VERBOSE, "minimizing input...", verbosity_level);

    TICK();
    dpr.initialConfig = minimize_nfa(dpr.initialConfig);
    TOCK("minimizing initial configs");

    TICK();
    dpr.transitionRelation = mata::ext::minimize(dpr.transitionRelation);
    TOCK("minimizing transition relation");

    for (mata::nfa::Nfa &property_aut : dpr.properties) {
      TICK();
      property_aut = minimize_nfa(property_aut);
      TOCK("minimizing unsafe property");
    }
  }

  const std::vector<int> selected_property_indices =
      get_selected_property_indices(dpr.propertyNames, property);
  if (property.has_value() && selected_property_indices.empty()) {
    std::cout << "could not find property \"" << property.value() << "\"\n";
    return 0;
  }

  for (enum SetInterpretation interpretation_type : interpretations) {
    log(VerbosityLevel::QUIET,
        "using " + to_string(interpretation_type) + " interpretation",
        verbosity_level);

    TICK();
    std::pair<mata::nft::Nft, std::shared_ptr<mata::OnTheFlyAlphabet>>
        interpretation_result =
            trapInterpretation(dpr.string_alphabet.get(), interpretation_type);
    TOCK("constructing interpretation transducer");

    mata::nft::Nft interpretation = interpretation_result.first;
    std::shared_ptr<mata::OnTheFlyAlphabet> abstract_alphabet =
        interpretation_result.second;

    TICK();
    SymbolicAutomataTree tree;

    const Term initial_term = tree.make_term(dpr.initialConfig);
    const Term delta_term = tree.make_term(dpr.transitionRelation);
    const Term v_term = tree.make_term(interpretation);
    const Term abstract_universal_term = tree.make_term(
        mata::nfa::builder::create_sigma_star_nfa(abstract_alphabet.get()));

    const Term not_v_term = tree.complement(v_term);
    const Term v_delta_term = tree.compose(v_term, delta_term);
    const Term abstract_conflicts =
        tree.compose(v_delta_term, not_v_term, {1}, {1});
    const Term bad_abstract_pairs = tree.intersect(
        tree.identity(abstract_universal_term), abstract_conflicts);
    const Term bad_abstract_sources =
        Term{tree.project(bad_abstract_pairs, {0})};
    const Term ind = tree.complement(bad_abstract_sources);

    const Term id_ind = tree.identity(ind);
    const Term preach_bad =
        tree.compose(tree.compose(v_term, id_ind, {0}, {0}), not_v_term);
    const Term preach = tree.complement(preach_bad);
    const Term reachable_under_preach = tree.post_image(initial_term, preach);
    TOCK("constructing symbolic tree");

    std::vector<bool> result{};
    std::vector<std::string> property_names{};
    result.reserve(selected_property_indices.size());
    property_names.reserve(selected_property_indices.size());

    for (const int property_index : selected_property_indices) {
      const std::string &property_name = dpr.propertyNames[property_index];
      log(VerbosityLevel::NORMAL,
          "trying to prove abstract safety for property " + property_name +
              "...",
          verbosity_level);

      const Term unsafe_term = tree.make_term(dpr.properties[property_index]);
      const Term unsafe_reachable =
          tree.intersect(reachable_under_preach, unsafe_term);

      TICK();
      const bool safe = tree.is_empty(unsafe_reachable, *dpr.string_alphabet);
      TOCK("final lazy emptiness for property " + property_name);

      result.push_back(safe);
      property_names.push_back(property_name);

      log(VerbosityLevel::NORMAL,
          std::string("...") + (safe ? "safe" : "unsafe"), verbosity_level);
    }

    log(VerbosityLevel::QUIET, "Result: " + vec_to_string(result),
        verbosity_level);
    logexp(
        VerbosityLevel::QUIET,
        [&]() {
          std::ostringstream oss;
          std::vector<std::string> proven{};
          std::vector<std::string> unproven{};
          for (size_t i{0}; i < result.size(); ++i) {
            if (result[i]) {
              proven.push_back(property_names[i]);
            } else {
              unproven.push_back(property_names[i]);
            }
          }
          oss << "This means that the following properties could be separated "
                 "from the initial configurations:\n";
          oss << "\t" << vec_to_string(proven) << "\n";
          oss << "While the following properties could not be separated from "
                 "the initial configurations:\n";
          oss << "\t" << vec_to_string(unproven) << "\n";
          return oss.str();
        },
        verbosity_level, false, false);
  }

  return 0;
}
