#pragma once

#include <cassert>
#include <iostream>
#include <fstream>
#if __has_include(<jsoncpp/json/json.h>)
#include <jsoncpp/json/json.h>
#elif __has_include(<json/json.h>)
#include <json/json.h>
#else
#error "Could not find a jsoncpp header"
#endif
#include <stdexcept>
#include <mata/alphabet.hh>
#include <mata/nfa/nfa.hh>
#include <mata/nft/nft.hh>
#include <mata/nfa/delta.hh>
#include <memory>

#include <abstracton/utils/utils.hpp>

// struct alphabet_encoding {
//     std::vector<char> alphabet;
//     std::vector<std::string> string_alphabet;
//     std::unordered_map<char, std::string> decoding;
//     std::unordered_map<std::string, char> encoding;
// };

void dfs_explore(std::vector<std::vector<int>> const& adjacency_list, std::vector<int> &order, std::vector<bool> &visited, int node);

std::vector<int> topo_sort(std::vector<std::vector<int>> const& adjacency_list);

// mata::nfa::Nfa parseDodoNfa(Json::Value dfa, alphabet_encoding alphabet_enc);
mata::nfa::Nfa parseDodoNfa(Json::Value nfa, mata::OnTheFlyAlphabet* string_alphabet, int verbosityLevel = logging::DEFAULT_VERBOSITY_LEVEL);

std::pair<std::string, std::string> parsePair(std::string p, int verbosityLevel = logging::DEFAULT_VERBOSITY_LEVEL);

mata::nft::Nft parseTransducer(Json::Value t, int verbosityLevel = logging::DEFAULT_VERBOSITY_LEVEL);

// alphabet_encoding alphabetToCharAlphabet(std::vector<std::string> string_alphabet);

struct DodoParserResult {
    // alphabet_encoding char_alphabet_triple;
    std::shared_ptr<mata::OnTheFlyAlphabet> string_alphabet;
    // Owns the AlphabetLevels object pointed to by transitionRelation.alphabets.
    // Stored as shared_ptr so DodoParserResult remains copyable.
    std::shared_ptr<mata::AlphabetLevels> alphabet_levels;
    mata::nfa::Nfa initialConfig;
    std::vector<mata::nfa::Nfa> properties;
    std::vector<std::string> propertyNames;
    mata::nft::Nft transitionRelation;
};

DodoParserResult parseDodoJSON(std::string filepath, int verbosityLevel = logging::DEFAULT_VERBOSITY_LEVEL);
