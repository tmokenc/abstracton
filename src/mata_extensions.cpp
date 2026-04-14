#include <mata/nfa/nfa.hh>
#include <mata/nft/nft.hh>
#include <mata/nft/builder.hh>
#include <abstracton/mata_extensions.hpp>
#include <mata/nfa/algorithms.hh>
#include <abstracton/utils/utils.hpp>
#include <mata/utils/utils.hh>
#include <random>

using namespace mata;
using namespace mata::nfa;
using namespace mata::nft;

Nft create_identity(Alphabet& alphabet) {
    Nft result {};
    State initial {result.add_state()};
    result.initial.insert(initial);
    result.final.insert(initial);
    result.insert_identity(initial, &alphabet);
    result.alphabet = &alphabet;
    return result;
}

Nft create_identity(const Nfa& language) {
    return mata::nft::builder::from_nfa_with_levels_zero(language);
}

Nfa project(const Nft& nft, int level) {
    // TODO add possibility to do padding closure wrt. some padding symbol
    Nft nft_proj {mata::nft::project_to(nft, level)};
    assert(nft_proj.levels.num_of_levels == 1);
    return nft_proj.to_nfa_move();
}

mata::nfa::Nfa apply(const mata::nft::Nft& nft, const mata::nfa::Nfa& nfa, int level) {
    Nft image = nft.apply(nfa, level);
    assert(image.levels.num_of_levels == 1);
    return image.to_nfa_move();
}

namespace mata::ext {

    // TODO: implement functions that also move the nft to an nfa to save resources?

    Nft determinize(const Nft& nft) {
        int levels = nft.levels.num_of_levels;
        Nfa aut {nft.to_nfa_copy()};
        Nfa aut_det {determinize(aut)};
        Nft result =  mata::nft::builder::from_nfa_with_levels_advancing(aut_det, levels);
        result.alphabet = nft.alphabet;
        result.set_level_alphabets(nft.level_alphabets);
        return result;
    }

    Nft minimize(const Nft& nft) {
        int levels = nft.levels.num_of_levels;
        Nfa aut {nft.to_nfa_copy()};
        // TODO for mindet, brzozowski better?
        Nfa aut_det {determinize(aut)};
        aut_det = aut_det.trim();
        Nfa aut_min {mata::nfa::algorithms::minimize_hopcroft(aut_det)};
        Nft result =  mata::nft::builder::from_nfa_with_levels_advancing(aut_min, levels);
        result.alphabet = nft.alphabet;
        result.set_level_alphabets(nft.level_alphabets);
        return result;
    }

    std::vector<mata::utils::OrdVector<Symbol>> get_tape_symbols_to_work_with(const mata::nft::Nft& nft, const Alphabet* alphabet, const std::optional<const std::vector<Alphabet*>> alphabets) {
        mata::utils::OrdVector<Symbol> default_alphabet;
        if (alphabet != nullptr) {
            default_alphabet = alphabet->get_alphabet_symbols();
        } else if (nft.alphabet != nullptr) {
            default_alphabet = nft.alphabet->get_alphabet_symbols();
        } else {
            default_alphabet = nft.delta.get_used_symbols();
        }
        std::vector<mata::utils::OrdVector<Symbol>> result {};
        for (int i = 0; i < nft.levels.num_of_levels; ++i) {
            if (alphabets.has_value() && alphabets->operator[](i) != nullptr) {
                result.push_back(alphabets->operator[](i)->get_alphabet_symbols());
            } else {
                result.push_back(default_alphabet);
            }
        }
        return result;
    }

    void make_complete(mata::nft::Nft& nft, const std::vector<mata::utils::OrdVector<Symbol>>& symbols) {
        int levels = nft.levels.num_of_levels;

        // insert sink state (with multiple levels)
        std::vector<State> sinks {};
        for (int i = 0; i < levels; i++) {
            sinks.push_back(nft.add_state_with_level(i));
            // std::cout << "adding sink state " << sinks[i] << " at level " << i << std::endl;
        }

        const size_t num_of_states{ nft.num_of_states() };

        for (State state{ 0 }; state < num_of_states; ++state) {
            mata::utils::OrdVector<Symbol> used_symbols {};
            for (const SymbolPost& symbol_post : nft.delta[state]) {
                used_symbols.insert(symbol_post.symbol);
            }
            const mata::utils::OrdVector<Symbol> unused_symbols{ symbols[nft.levels[state]].difference(used_symbols) };
            const unsigned int state_level{ nft.levels[state] };
            const unsigned int next_level{ (state_level + 1) % levels };
            for (const Symbol symbol : unused_symbols) {
                // add to delta state -symbol-> sinks[i], iff state is in level i - 1
                nft.delta.add(state, symbol, sinks[next_level]);
            }
        }
    }

    void make_complete(mata::nft::Nft& nft, const Alphabet* alphabet, const std::optional<const std::vector<Alphabet*>> alphabets) {
        make_complete(nft, get_tape_symbols_to_work_with(nft, alphabet, alphabets));
    }

    Nft complement(const Nft& aut, const std::vector<mata::utils::OrdVector<Symbol>>& symbols, bool minimize_during_determinization) {
        Nft result;

        if (aut.initial.empty()) {
            result = Nft::with_levels(aut.levels.num_of_levels, 1, {0}, {});
        } else if (minimize_during_determinization) {
            result = mata::ext::minimize(aut);
        } else {
            result = mata::ext::determinize(aut);
        }

        make_complete(result, symbols);
        utils::SparseSet<State> new_final_states{};

        const size_t num_of_states{ result.num_of_states() };

        for (State state{ 0 }; state < num_of_states; ++state) {
            if (result.levels[state] == 0 && !result.final.contains(state)) {
                //std::cout << "state " << state << " has level " << result.levels[state] << " and is not final" << std::endl;
                new_final_states.insert(state);
            }
        }

        result.final = new_final_states;
        result.alphabet = aut.alphabet;
        result.set_level_alphabets(aut.level_alphabets);
        return result;
    }

    mata::nft::Nft complement(const mata::nft::Nft& nft, const Alphabet* alphabet, const std::optional<const std::vector<Alphabet*>> alphabets, bool minimize_during_determinization) {
        return complement(nft, get_tape_symbols_to_work_with(nft, alphabet, alphabets), minimize_during_determinization);
    }

    mata::nft::Nft create_sigma_star_nft(int number_of_levels, Alphabet* alphabet, const std::optional<const std::vector<Alphabet*>> alphabets) {
        if (number_of_levels == 0) {
            if (alphabets.has_value()) {
                return mata::nft::Nft::with_levels(0, 1, {0}, {0}, alphabets.value());
            }
            return mata::nft::Nft::with_levels(0, 1, {0}, {0}, alphabet);
        }

        Nft result = alphabets.has_value()
                ? mata::nft::Nft::with_levels(number_of_levels, number_of_levels, {0}, {0}, alphabets.value())
                : mata::nft::Nft::with_levels(number_of_levels, number_of_levels, {0}, {0}, alphabet);

        for (int i = 0; i < number_of_levels; ++i) {
            result.levels[i] = i;
            mata::utils::OrdVector<Symbol> s = {DONT_CARE};
            if (alphabets.has_value() && alphabets->operator[](i) != nullptr) {
                s = alphabets->operator[](i)->get_alphabet_symbols();
            } else if (alphabet != nullptr) {
                s = alphabet->get_alphabet_symbols();
            }
            for (const Symbol& sym : s) {
                result.delta.add(i, sym, (i + 1) % number_of_levels);
            }
        }

        return result;
    }

    /// universality check using Antichains
    bool is_universal_antichains (
        const Nft&                      aut,
        const std::vector<Alphabet*>    alphabets,
        Run*                            cex)
    {

        using WorklistType = std::list<StateSet>;
        using ProcessedType = std::list<StateSet>;

        auto subsumes = [](const StateSet& lhs, const StateSet& rhs) {
            if (lhs.size() > rhs.size()) { // bigger set cannot be subset
                return false;
            }

            return std::ranges::includes(rhs, lhs);
        };

        // check initial states
        if (are_disjoint(aut.initial, aut.final)) {
            if (nullptr != cex) { cex->word.clear(); }
            return false;
        }

        // initialize
        WorklistType worklist = { StateSet(aut.initial) };
        ProcessedType processed = { StateSet(aut.initial) };
        std::vector<mata::utils::OrdVector<Symbol>> alph_symbols{};
        for (int i{ 0 }; i < alphabets.size(); ++i) {
            alph_symbols.push_back(alphabets[i]->get_alphabet_symbols());
        }

        // 'paths[s] == t' denotes that state 's' was accessed from state 't',
        // 'paths[s] == s' means that 's' is an initial state
        std::map<StateSet, std::pair<StateSet, Symbol>> paths =
            { {StateSet(aut.initial), {StateSet(aut.initial), 0}} };

        while (!worklist.empty()) {
            // get a next state
            StateSet state;

            // process parameters
            // TODO: set correctly!!!!
            constexpr bool is_dfs = true;
            if (is_dfs) {
                state = *worklist.rbegin();
                worklist.pop_back();
            } else { // BFS
                state = *worklist.begin();
                worklist.pop_front();
            }

            // a symbol x is called possible in stateset S iff for every s in S, x is in the alphabet at the level of s.
            // this can be calculated as intersection of all alphabets for which there exists some s in S which is at the level of the alphabet.
            std::vector<State> state_as_vector = state.to_vector();
            // state is guaranteed to have at least one state (empty sets have empty intersection with final states and are never added to worklist)
            mata::utils::OrdVector<Symbol> possible_symbols{alph_symbols[aut.levels[state_as_vector[0]]]};
            for (int i{ 1 }; i < state_as_vector.size(); ++i) {
                // if level is the same, alphabet is also the same and intersection can be skipped
                if (aut.levels[state_as_vector[0]] != aut.levels[state_as_vector[i]]) {
                    possible_symbols = possible_symbols.intersection(alph_symbols[aut.levels[state_as_vector[i]]]);
                }
            }

            // process it
            for (Symbol symb : possible_symbols) {
                StateSet succ = aut.post(state, symb);
                bool all_are_at_level_0 = true;
                for (State s : succ) {
                    if (aut.levels[s] != 0) {
                        all_are_at_level_0 = false;
                        break;
                    }
                }
                if (all_are_at_level_0 && !aut.final.intersects_with(succ)) {
                    if (nullptr != cex) {
                        cex->word.clear();
                        cex->word.push_back(symb);
                        StateSet trav = state;
                        while (paths[trav].first != trav)
                        { // go back until initial state
                            cex->word.push_back(paths[trav].second);
                            trav = paths[trav].first;
                        }

                        std::ranges::reverse(cex->word);
                    }

                    return false;
                }

                bool is_subsumed = false;
                for (const auto& anti_state : processed) {
                    // trying to find a smaller state in processed
                    if (subsumes(anti_state, succ)) {
                        is_subsumed = true;
                        break;
                    }
                }

                if (is_subsumed) { continue; }

                // prune data structures and insert succ inside
                for (std::list<StateSet>* ds : {&processed, &worklist}) {
                    auto it = ds->begin();
                    while (it != ds->end()) {
                        if (subsumes(succ, *it)) {
                            auto to_remove = it;
                            ++it;
                            ds->erase(to_remove);
                        } else {
                            ++it;
                        }
                    }

                    // TODO: set pushing strategy
                    ds->push_back(succ);
                }

                // also set that succ was accessed from state
                paths[succ] = {state, symb};
            }
        }

        return true;
    }

    bool is_universal_antichains_by_inclusion(const Nft& aut, const std::vector<Alphabet*> alphabets, Run* cex) {
        Nft univ = create_sigma_star_nft(aut.levels.num_of_levels, nullptr, std::make_optional(alphabets));

        return mata::nft::algorithms::is_included_antichains(univ, aut, nullptr, cex);
    }

    bool is_universal_lazy(const Nft& aut, const std::vector<Alphabet*> alphabets, Run* cex) {
        using WorklistType = std::list<StateSet>;
        using ProcessedType = std::unordered_set<StateSet>;

        // check initial states
        if (are_disjoint(aut.initial, aut.final)) {
            if (nullptr != cex) { cex->word.clear(); }
            return false;
        }

        // initialize
        WorklistType worklist = { StateSet(aut.initial) };
        ProcessedType processed = { StateSet(aut.initial) };
        std::vector<mata::utils::OrdVector<Symbol>> alph_symbols{};
        for (int i{ 0 }; i < alphabets.size(); ++i) {
            alph_symbols.push_back(alphabets[i]->get_alphabet_symbols());
        }

        // 'paths[s] == t' denotes that state 's' was accessed from state 't',
        // 'paths[s] == s' means that 's' is an initial state
        std::map<StateSet, std::pair<StateSet, Symbol>> paths =
            { {StateSet(aut.initial), {StateSet(aut.initial), 0}} };

        while (!worklist.empty()) {
            // get a next state
            StateSet state;

            // process parameters
            // TODO: set correctly!!!!
            constexpr bool is_dfs = true;
            if (is_dfs) {
                state = *worklist.rbegin();
                worklist.pop_back();
            } else { // BFS
                state = *worklist.begin();
                worklist.pop_front();
            }

            // a symbol x is called possible in stateset S iff for every s in S, x is in the alphabet at the level of s.
            // this can be calculated as intersection of all alphabets for which there exists some s in S which is at the level of the alphabet.
            std::vector<State> state_as_vector = state.to_vector();
            // state is guaranteed to have at least one state (empty sets have empty intersection with final states and are never added to worklist)
            mata::utils::OrdVector<Symbol> possible_symbols{alph_symbols[aut.levels[state_as_vector[0]]]};
            for (int i{ 1 }; i < state_as_vector.size(); ++i) {
                // if level is the same, alphabet is also the same and intersection can be skipped
                if (aut.levels[state_as_vector[0]] != aut.levels[state_as_vector[i]]) {
                    possible_symbols = possible_symbols.intersection(alph_symbols[aut.levels[state_as_vector[i]]]);
                }
            }

            // process it
            for (Symbol symb : possible_symbols) {
                StateSet succ = aut.post(state, symb);
                bool all_are_at_level_0 = true;
                for (State s : succ) {
                    if (aut.levels[s] != 0) {
                        all_are_at_level_0 = false;
                        break;
                    }
                }
                if (all_are_at_level_0 && !aut.final.intersects_with(succ)) {
                    if (nullptr != cex) {
                        cex->word.clear();
                        cex->word.push_back(symb);
                        StateSet trav = state;
                        while (paths[trav].first != trav)
                        { // go back until initial state
                            cex->word.push_back(paths[trav].second);
                            trav = paths[trav].first;
                        }

                        std::ranges::reverse(cex->word);
                    }

                    return false;
                }

                processed.insert(state);
                if (!processed.contains(succ)) {
                    worklist.push_back(succ);
                }

                // also set that succ was accessed from state
                paths[succ] = {state, symb};
            }
        }

        return true;
    }

    Nft insert_tapes(const Nft& aut, const std::vector<int> inserted_tape_indices, const std::vector<Alphabet*> inserted_tape_alphabets) {
        assert(inserted_tape_indices.size() == inserted_tape_alphabets.size());
        for (int i{ 0 }; i < inserted_tape_indices.size(); ++i) {
            assert(inserted_tape_indices[i] < aut.levels.num_of_levels + inserted_tape_indices.size());
            if (i + 1 < inserted_tape_indices.size()) {
                assert(inserted_tape_indices[i] < inserted_tape_indices[i + 1]);
            }
        }

        Nft result = Nft::with_levels(aut.levels.num_of_levels + inserted_tape_indices.size(), aut.num_of_states(), {}, {});
        result.alphabet = aut.alphabet;
        std::vector<Alphabet*> new_alphabets(result.levels.num_of_levels, nullptr);

        std::vector<int> old_to_new_levels{};
        {
            int i = 0;
            int j = 0;
            while (i + j < result.levels.num_of_levels) {
                if (i + j == inserted_tape_indices[j]) {
                    // i + j is an inserted level
                    new_alphabets[i + j] = inserted_tape_alphabets[j];
                    j++;
                } else {
                    // i + j is the next old level
                    old_to_new_levels.push_back(i + j);
                    if (!aut.level_alphabets.empty()) {
                        new_alphabets[i + j] = aut.level_alphabets[i];
                    }
                    i++;
                }
            }
        }

        // insert old states, keeping their indices (e.g. State 0 will remain State 0), updating their level
        for (State s : aut.get_reachable_states()) {
            result.add_state_with_level(s, old_to_new_levels[aut.levels[s]]);
        }

        // iterate over old delta, and insert it while also inserting new tapes in between as necessary
        for (const State& s : aut.get_reachable_states()) {
            int source_level = aut.levels[s];
            int num_of_inserted_levels_in_between = (source_level + 1 == aut.levels.num_of_levels) ?
                    (result.levels.num_of_levels - 1 - old_to_new_levels[source_level]) + old_to_new_levels[0] :
                    old_to_new_levels[source_level + 1] - old_to_new_levels[source_level] - 1;
            if (old_to_new_levels[source_level] == 0 && aut.final.contains(s)) {
                result.final.insert(s);
            }
            if (num_of_inserted_levels_in_between == 0) {
                // would like to just set result.delta[s] = aut.delta[s]...
                for (const SymbolPost& sp : aut.delta[s]) {
                    result.delta.add(s, sp.symbol, sp.targets);
                }
                continue;
            }
            // insert levels in between
            for (const SymbolPost& sp : aut.delta[s]) {
                std::vector<State> in_between_states{};
                std::vector<int> in_between_levels{};
                for (int i{ 0 }; i < num_of_inserted_levels_in_between; ++i) {
                    int in_between_level = (old_to_new_levels[source_level] + 1 + i) % result.levels.num_of_levels;
                    in_between_states.push_back(result.add_state_with_level(in_between_level));
                    in_between_levels.push_back(in_between_level);

                    // any intermediate state on level 0 on a path to some final state becomes a new final state
                    if (in_between_level == 0 && aut.final.intersects_with(sp.targets)) {
                        result.final.insert(in_between_states[in_between_states.size() - 1]);
                    }
                }
                // add transitions with complete alphabets between in-between-states
                for (int i{ 0 }; i < num_of_inserted_levels_in_between - 1; ++i) {
                    if (new_alphabets[in_between_levels[i]] == nullptr) {
                        result.delta.add(in_between_states[i], DONT_CARE, in_between_states[i + 1]);
                    } else {
                        for (const Symbol x : new_alphabets[in_between_levels[i]]->get_alphabet_symbols()) {
                            result.delta.add(in_between_states[i], x, in_between_states[i + 1]);
                        }
                    }
                }
                // go from original source to first in-between-state with old symbol
                result.delta.add(s, sp.symbol, in_between_states[0]);
                // go from last in-between-state with any symbol to old target state
                if (new_alphabets[in_between_levels[in_between_levels.size() - 1]] == nullptr) {
                    result.delta.add(in_between_states[in_between_states.size() - 1], DONT_CARE, sp.targets);
                } else {
                    for (const Symbol x : new_alphabets[in_between_levels[in_between_levels.size() - 1]]->get_alphabet_symbols()) {
                        result.delta.add(in_between_states[in_between_states.size() - 1], x, sp.targets);
                    }
                }
            }
        }

        // determine initial states
        if (old_to_new_levels[0] == 0) {
            result.initial = aut.initial;
        } else {
            for (const State& s : aut.initial) {
                int num_of_intermediate_states = old_to_new_levels[0];
                std::vector<State> intermediate_states{};
                for (int i{ 0 }; i < num_of_intermediate_states; ++i) {
                    intermediate_states.push_back(result.add_state_with_level(i));
                }
                // add transitions between new intermediate states
                for (int i{ 0 }; i < num_of_intermediate_states - 1; ++i) {
                    if (new_alphabets[i] == nullptr) {
                        result.delta.add(intermediate_states[i], DONT_CARE, intermediate_states[i + 1]);
                    } else {
                        for (const Symbol& x : new_alphabets[i]->get_alphabet_symbols()) {
                            result.delta.add(intermediate_states[i], x, intermediate_states[i + 1]);
                        }
                    }
                }
                // add transitions to old initial states
                if (new_alphabets[num_of_intermediate_states - 1] == nullptr) {
                    result.delta.add(intermediate_states[num_of_intermediate_states - 1], DONT_CARE, s);
                } else {
                    for (const Symbol& x : new_alphabets[num_of_intermediate_states - 1]->get_alphabet_symbols()) {
                        result.delta.add(intermediate_states[num_of_intermediate_states - 1], x, s);
                    }
                }
                result.initial.insert(intermediate_states[0]);
                if (aut.final.contains(s)) {
                    result.final.insert(intermediate_states[0]);
                }
            }
        }

        // update alphabets
        result.set_level_alphabets(new_alphabets);

        return result;
    }

    Nft relational_product_length_preserving(const std::vector<mata::nft::Nft> nfts) {
        if (nfts.size() == 0) {
            // return nft accepting only epsilon
            //return Nft::with_levels(0, 1, {0}, {0});
            throw std::runtime_error("mata currently does not support creating nfts with 0 levels");
        }
        if (nfts.size() == 1) {
            // theoretically not necessary, but reduces brain power needed in later loop
            return nfts[0];
        }
        size_t total_number_of_tapes = 0;
        std::vector<size_t> start_indices(nfts.size(), 0);
        for (int i = 0; i < nfts.size(); ++i) {
            start_indices[i] = total_number_of_tapes;
            total_number_of_tapes += nfts[i].levels.num_of_levels;
        }
        start_indices.push_back(total_number_of_tapes); // so that start_indices[i], start_indices[i + 1] can be used as range for all tapes i

        // create alphabets (TODO: replace with DONT_CARE?)
        std::vector<Alphabet*> alphabets(total_number_of_tapes, nullptr);
        for (int i = 0; i < nfts.size(); ++i) {
            for (int j = 0; j < nfts[i].levels.num_of_levels; ++j) {
                if (!nfts[i].level_alphabets.empty()) {
                    alphabets[start_indices[i] + j] = nfts[i].level_alphabets[j];
                } else if (nfts[i].alphabet != nullptr) {
                    alphabets[start_indices[i] + j] = nfts[i].alphabet;
                } else {
                    // alphabets[start_indices[i] + j] = new Alphabet{nfts[i].delta.get_used_symbols()};
                }
            }
        }

        Nft result;
        for (int i = 0; i < nfts.size(); ++i) {
            // set nft levels to false (do not insert new tapes here)
            std::vector<int> inserted_tape_indices{};
            std::vector<Alphabet*> inserted_tape_alphabets{};
            for (int j = 0; j < total_number_of_tapes; ++j) {
                if (!(j >= start_indices[i] && j < start_indices[i + 1])) {
                    inserted_tape_indices.push_back(j);
                    inserted_tape_alphabets.push_back(alphabets[j]);
                }
            }

            Nft padded_nft = mata::ext::insert_tapes(nfts[i], inserted_tape_indices, inserted_tape_alphabets);
            if (i == 0) {
                result = padded_nft;
            } else {
                result = mata::nft::intersection(result, padded_nft);
            }
        }

        return result;
    }

    Nft relational_product_length_preserving_dont_care(const std::vector<mata::nft::Nft> nfts) {
        if (nfts.size() == 0) {
            // return nft accepting only epsilon
            //return Nft::with_levels(0, 1, {0}, {0});
            throw std::runtime_error("mata currently does not support creating nfts with 0 levels");
        }
        if (nfts.size() == 1) {
            // theoretically not necessary, but reduces brain power needed in later loop
            return nfts[0];
        }
        size_t total_number_of_tapes = 0;
        std::vector<size_t> start_indices(nfts.size(), 0);
        for (int i = 0; i < nfts.size(); ++i) {
            start_indices[i] = total_number_of_tapes;
            total_number_of_tapes += nfts[i].levels.num_of_levels;
        }
        start_indices.push_back(total_number_of_tapes); // so that start_indices[i], start_indices[i + 1] can be used as range for all tapes i

        BoolVector bv(total_number_of_tapes, true);
        Nft result;
        for (int i = 0; i < nfts.size(); ++i) {
            // set nft levels to false (do not insert new tapes here)
            for (int j = start_indices[i]; j < start_indices[i + 1]; ++j) {
                bv[j] = false;
            }

            Nft padded_nft = mata::nft::insert_levels(nfts[i], bv);
            if (i == 0) {
                result = padded_nft;
            } else {
                result = mata::nft::intersection(result, padded_nft);
            }

            // unset nft levels, so the BoolVector can be used in-place
            for (int j = start_indices[i]; j < start_indices[i + 1]; ++j) {
                bv[j] = true;
            }
        }

        return result;
    }

    void padding_closure(Nfa& nfa, Symbol padding_symbol) {
        // compute predecessors wrt. delta (restricted to padding_symbol)
        // -> construct graph with edges (i, j) meaning that (j, padding_symbol, i) is in delta
        std::unordered_map<State, std::unordered_set<State>> predecessors {};

        std::unordered_set<State> visited {};
        std::unordered_set<State> worklist {};

        for (State i : nfa.initial) {
            worklist.insert(i);
        }

        while (!worklist.empty()) {
            // pop element from worklist
            State q = *worklist.begin();
            worklist.erase(worklist.begin());
            visited.insert(q);

            // explore successors of q and add them to correct sets
            for (const SymbolPost& sp : nfa.delta[q]) {
                for (const State target : sp.targets) {
                    if (sp.symbol == padding_symbol) {
                        predecessors[target].insert(q);
                    }
                    if (!visited.contains(target)) {
                        worklist.insert(target);
                    }
                }
            }
        }

        // DEBUG output
        std::cout << "visited:" << std::endl;
        for (auto i : visited) {
            std::cout << i;
        }
        std::cout << std::endl;
        std::cout << "pred size " << predecessors.size() << std::endl;
        for (const auto& [key, values] : predecessors) {
            std::cout << key << " : { ";
            for (int v : values) {
                std::cout << v << " ";
            }
            std::cout << "}\n";
        }

        // dfs on predecessors to find states that need to be final
        visited.clear();
        worklist.clear();

        for (State i : nfa.final) {
            worklist.insert(i);
        }

        while (!worklist.empty()) {
            // pop element from worklist
            State q = *worklist.begin();
            worklist.erase(worklist.begin());
            visited.insert(q);

            // make q final in nfa
            nfa.final.insert(q);

            // explore predecessors of q
            for (const State& p : predecessors[q]) {
                if (!visited.contains(p)) {
                    worklist.insert(p);
                }
            }
        }
    }

    // code copied and adapted from mata's create_random_nfa_tabakov_vardi
    Nft builder::create_random_nft_tabakov_vardi(const size_t num_of_levels, const size_t num_of_states, const std::vector<size_t>& alphabet_sizes, const double states_trans_ratio_per_symbol, const double final_state_density, const std::optional<unsigned int> seed) {
        if (num_of_states == 0) {
            return Nft::with_levels(num_of_levels);
        }
        if (states_trans_ratio_per_symbol < 0 || static_cast<size_t>(states_trans_ratio_per_symbol) > num_of_states) {
            // Maximum of num_of_states^2 unique transitions for one symbol can be created.
            throw std::runtime_error("Transition density must be in range [0, num_of_states]");
        }
        if (final_state_density < 0 || final_state_density > 1) {
            // Maximum of num_of_states final states can be created.
            throw std::runtime_error("Final state density must be in range (0, 1]");
        }
        if (num_of_levels != alphabet_sizes.size()) {
            // did not specify alphabet size uniquely for each alphabet
            throw std::runtime_error("Must give exactly one alphabet size for each level");
        }

        Nft nft = Nft::with_levels(num_of_levels, num_of_states, { 0 }, { 0 }, nullptr);

        // Initialize the random number generator
        unsigned int seed_val = seed.value_or(std::random_device{}());
        std::mt19937 gen(seed_val); // Mersenne Twister engine

        // Unique final state generator
        std::vector<State> states(num_of_states);
        std::iota(states.begin(), states.end(), 0);
        std::shuffle(states.begin() + 1, states.end(), gen); // Starting from 1, because 0 is always final state.

        // Create final states
        const size_t num_of_final_states{ static_cast<size_t>(std::round(static_cast<double>(num_of_states) * final_state_density)) };
        for (size_t i = 0; i < num_of_final_states; ++i) {
            nft.final.insert(states[i]);
        }

        // Unique transition generator
        std::vector<State> one_dimensional_transition_matrix(num_of_states * num_of_states);
        std::iota(one_dimensional_transition_matrix.begin(), one_dimensional_transition_matrix.end(), 0);

        // Create transitions
        // Using std::min because, in some universe, casting and rounding might cause the number of transitions to exceed the number of possible transitions by 1
        // and then an access to the non-existing element of one_dimensional_transition_matrix would occur.
        const size_t num_of_transitions_per_symbol{ std::min(static_cast<size_t>(std::round(static_cast<double>(num_of_states) * states_trans_ratio_per_symbol)), one_dimensional_transition_matrix.size()) };
        std::vector<unsigned int> tuple_bound;
        tuple_bound.reserve(alphabet_sizes.size());
        for (auto x : alphabet_sizes)
            tuple_bound.push_back(static_cast<unsigned int>(x));

        for (auto symbol_vec : BoundedTuples(tuple_bound)) {
            std::shuffle(one_dimensional_transition_matrix.begin(), one_dimensional_transition_matrix.end(), gen);
            for (size_t i = 0; i < num_of_transitions_per_symbol; ++i) {
                const State source{ one_dimensional_transition_matrix[i] / num_of_states };
                const State target{ one_dimensional_transition_matrix[i] % num_of_states };
                nft.add_transition(source, symbol_vec, target);
            }
        }
        return nft;
    }
}
