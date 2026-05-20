#include "DodoParser.h"
#include <unordered_map>
#include <abstracton/utils/utils.hpp>

void dfs_explore(std::vector<std::vector<int>> const& adjacency_list, std::vector<int> &order, std::vector<bool> &visited, int node) {
    visited[node] = true;
    for (int s : adjacency_list[node]) {
        if (!visited[s]) {
            dfs_explore(adjacency_list, order, visited, s);
        }
    }
    order.push_back(node);
}

std::vector<int> topo_sort(std::vector<std::vector<int>> const& adjacency_list) {
    std::vector<int> order {};
    std::vector<bool> visited(adjacency_list.size(), false);
    for (int node = 0; node < adjacency_list.size(); node++) {
        if (!visited[node]) {
            dfs_explore(adjacency_list, order, visited, node);
        }
    }
    return order;
}

// mata::nfa::Nfa parseDodoNfa(Json::Value dfa, alphabet_encoding alphabet_enc);
mata::nfa::Nfa parseDodoNfa(Json::Value nfa, mata::OnTheFlyAlphabet* string_alphabet, int verbosityLevel) {
    // std::vector<char> alphabet = alphabet_enc.alphabet;
    // std::unordered_map<char, std::string> decode_alphabet = alphabet_enc.decoding;
    // std::unordered_map<std::string, char> encode_alphabet = alphabet_enc.encoding;
    // std::vector<char> alphabet {};
    // for (auto x : dfa["alphabet"]) {
    //     alphabet.push_back(encode_alphabet[x.asString()]);
    // }

    std::vector<int> states {};
    for (auto x : nfa["states"]) {
        states.push_back(x.asInt());
    }

    std::unordered_set<int> initialStates {};
    for (auto x : nfa["initialStates"]) {
        assert(x.isInt());
        initialStates.insert(x.asInt());
    }

    std::unordered_set<int> finalStates {};
    for (auto q : nfa["accepting"]) {
        finalStates.insert(q.asInt());
    }

    std::vector<std::tuple<int, std::string, int>> transitions;
    for (auto t : nfa["transitions"]) {
        int source = t["source"].asInt();

        std::string letter = t["letter"].asString();
        
        for (auto target : t["target"]) {
            assert(target.isInt());
            transitions.push_back(std::make_tuple(source, letter, target.asInt()));
        }
    }

    // std::vector<std::string> string_alphabet {};
    // for (auto t : nfa["alphabet"]) {
    //     string_alphabet.push_back(t.asString());
    // }

    std::unordered_map<int, int> indexOfState {};
    for (int i = 0; i < states.size(); i++) {
        indexOfState.emplace(states[i], i);
    }

    mata::utils::SparseSet<mata::nfa::State> initial_state_indices {};
    for (auto x : initialStates) {
        initial_state_indices.insert(indexOfState[x]);
    }

    mata::utils::SparseSet<mata::nfa::State> final_state_indices {};
    for (auto q : finalStates) {
        final_state_indices.insert(indexOfState[q]);
    }

    // build mata nfa
    mata::nfa::Nfa result(states.size(), initial_state_indices, final_state_indices, string_alphabet);

    for (auto t : transitions) {
        // TODO add "add" function of signature (State, string, State) to delta in mata if OnTheFlyAlphabet specified
        result.delta.add(std::get<0>(t), string_alphabet->translate_symb(std::get<1>(t)), std::get<2>(t));
    }


    return result;
}

std::pair<std::string, std::string> parsePair(std::string p, int verbosityLevel) {
    assert(p[0] == '[' && p[p.size() - 1] == ']');
    p = p.substr(1, p.size() - 2);
    size_t sep_index = p.find(",");
    return std::make_pair(p.substr(0, sep_index), p.substr(sep_index + 1, p.size() - sep_index - 1));
}

mata::nft::Nft parseTransducer(Json::Value t, mata::OnTheFlyAlphabet* string_alphabet, int verbosityLevel) {
    /*std::vector<char> alphabet = alphabet_enc.alphabet;
    std::unordered_map<char, std::string> decode_alphabet = alphabet_enc.decoding;
    std::unordered_map<std::string, char> encode_alphabet = alphabet_enc.encoding;

    if (t["initialState"].size() > 1) {
        throw std::runtime_error("only one initialState allowed");
    }
    int initialState = t["initialState"][0].asInt();

    std::unordered_set<int> finalStates;
    for (auto q : t["accepting"]) {
        finalStates.insert(q.asInt());
    }

    transducerTransitions transitions;
    for (auto d : t["transitions"]) {
        int source = d["source"].asInt();
        std::vector<int> targets {};
        for (auto tar : d["target"]) {
            targets.push_back(tar.asInt());
        }
        auto letter = parsePair(d["letter"].asString());

        char a = encode_alphabet[letter.first];
        char b = encode_alphabet[letter.second];
        transitions.emplace(std::make_pair(source, std::make_pair(a, b)), targets);
    }
    
    return Transducer(transitions, initialState, finalStates);*/

    std::vector<int> states {};
    for (auto x : t["states"]) {
        assert(x.isInt());
        states.push_back(x.asInt());
    }

    std::unordered_set<int> initialStates {};
    for (auto x : t["initialStates"]) {
        assert(x.isInt());
        initialStates.insert(x.asInt());
    }

    std::unordered_set<int> finalStates {};
    for (auto q : t["accepting"]) {
        finalStates.insert(q.asInt());
    }

    std::vector<std::tuple<int, std::string, std::string, int>> transitions {};
    for (auto trans : t["transitions"]) {
        assert(trans["source"].isInt());
        int source = trans["source"].asInt();

        std::pair<std::string, std::string> letter_pair = parsePair(trans["letter"].asString());

        for (auto target : trans["target"]) {
            assert(target.isInt());
            transitions.push_back(std::make_tuple(source, letter_pair.first, letter_pair.second, target.asInt()));
        }
    }

    std::unordered_map<int, int> indexOfState {};
    for (int i = 0; i < states.size(); i++) {
        indexOfState.emplace(states[i], i);
    }

    mata::utils::SparseSet<mata::nft::State> initial_state_indices {};
    for (auto x : initialStates) {
        initial_state_indices.insert(indexOfState[x]);
    }

    mata::utils::SparseSet<mata::nft::State> final_state_indices {};
    for (auto q : finalStates) {
        final_state_indices.insert(indexOfState[q]);
    }

    // build mata nft (dodo benchmarks ALL have exactly 2 levels)
    // The alphabets pointer is filled in by parseDodoJSON once the AlphabetLevels owner exists.
    mata::nft::Nft result = mata::nft::Nft::with_levels(
            2, states.size(), initial_state_indices, final_state_indices);

    for (auto d : transitions) {
        // TODO add "add" function of signature (State, string, State) to delta in mata if OnTheFlyAlphabet specified
        result.add_transition(std::get<0>(d), {string_alphabet->translate_symb(std::get<1>(d)), string_alphabet->translate_symb(std::get<2>(d))}, std::get<3>(d));
    }

    return result;
}

// alphabet_encoding alphabetToCharAlphabet(std::vector<std::string> string_alphabet) {
//     std::unordered_map<char, std::string> decode {};
//     std::unordered_map<std::string, char> encode {};
//     std::vector<char> char_alphabet {};
//
//     // step 1: extract all length 1 strings to chars
//     std::vector<std::string> todo {};
//     for (std::string string_letter : string_alphabet) {
//         if (string_letter.size() == 1) {
//             char char_letter = string_letter[0];
//             char_alphabet.push_back(char_letter);
//             decode.emplace(std::make_pair(char_letter, string_letter));
//             encode.emplace(std::make_pair(string_letter, char_letter));
//         } else {
//             todo.push_back(string_letter);
//         }
//     }
//
//     int i = 0;
//     for (std::string string_letter : todo) {
//         while (std::find(char_alphabet.begin(), char_alphabet.end(), standardAlphabet(i)) != char_alphabet.end()) {
//             i++;
//             if (i >= 62) {
//                 throw std::runtime_error("Error: currently, only alphabets up to a size of 62 are supported");
//             }
//         }
//         char char_letter = standardAlphabet(i);
//         char_alphabet.push_back(char_letter);
//         decode.emplace(std::make_pair(char_letter, string_letter));
//         encode.emplace(std::make_pair(string_letter, char_letter));
//     }
//
//     return alphabet_encoding {
//         char_alphabet,
//         string_alphabet,
//         decode,
//         encode
//     };
// }

DodoParserResult parseDodoJSON(std::string filepath, int verbosityLevel) {
    std::ifstream ifs(filepath);
    if (!ifs.good()) {
        throw std::runtime_error("file " + filepath + " does not exist");
    }
    Json::Reader reader;
    Json::Value obj;
    reader.parse(ifs, obj);

    // Read alphabet
    std::vector<std::string> string_alphabet_vec {};
    for (auto x : obj["alphabet"]) {
        string_alphabet_vec.push_back(x.asString());
    }
    mata::OnTheFlyAlphabet string_alphabet(string_alphabet_vec);
    std::shared_ptr<mata::OnTheFlyAlphabet> string_alphabet_ptr = std::make_shared<mata::OnTheFlyAlphabet>(string_alphabet);

    // // convert to char alphabet
    // alphabet_encoding alphabet_enc = alphabetToCharAlphabet(string_alphabet);
    // std::vector<char> alphabet = alphabet_enc.alphabet;
    logging::log(logging::VerbosityLevel::DEBUG, "parser: alphabet vec " + vec_to_string(string_alphabet_vec), verbosityLevel);
    logging::log(logging::VerbosityLevel::DEBUG, "parser: alphabet " + stream_to_string(string_alphabet_ptr->get_alphabet_symbols()) + " initialized!", verbosityLevel);

    // Parse transducer
    mata::nft::Nft t = parseTransducer(obj["transducer"], string_alphabet_ptr.get(), verbosityLevel);
    logging::log(logging::VerbosityLevel::DEBUG, "transducer:", verbosityLevel);
    logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return t.print_to_dot(); }, verbosityLevel);

    // Parse initial nfa
    mata::nfa::Nfa initialConfig = parseDodoNfa(obj["initial"], string_alphabet_ptr.get());
    logging::log(logging::VerbosityLevel::DEBUG, "initial:", verbosityLevel);
    logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return initialConfig.print_to_dot(); }, verbosityLevel);

    // Parse properties
    std::vector<mata::nfa::Nfa> properties {};
    std::vector<std::string> propertyNames {};
    for (auto p : obj["properties"].getMemberNames()) {
        properties.push_back(parseDodoNfa(obj["properties"][p], string_alphabet_ptr.get()));
        propertyNames.push_back(p);
        logging::log(logging::VerbosityLevel::DEBUG, "property \"" + p + "\":", verbosityLevel);
        logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return properties[properties.size() - 1].print_to_dot(); }, verbosityLevel);
    }


    logging::log(logging::VerbosityLevel::DEBUG, "parser: alphabet " + stream_to_string(string_alphabet_ptr->get_alphabet_symbols()) + " still intact!", verbosityLevel);
    logging::log(logging::VerbosityLevel::DEBUG, "parser: transition relation:", verbosityLevel);
    logging::logexp(logging::VerbosityLevel::DEBUG, [&]() { return t.print_to_dot(); }, verbosityLevel);

    // Wrap the shared string alphabet as an AlphabetLevels in Global mode (same alphabet on every
    // level of the transducer) and own it inside DodoParserResult so its address stays stable.
    auto alphabet_levels_ptr = std::make_shared<mata::AlphabetLevels>(string_alphabet_ptr.get());
    t.alphabets = alphabet_levels_ptr.get();
    assert(t.alphabets != nullptr && string_alphabet_ptr->is_equal(t.alphabets->for_level(0)));

    return DodoParserResult {
        string_alphabet_ptr,
        alphabet_levels_ptr,
        initialConfig,
        properties,
        propertyNames,
        t
    };
}
