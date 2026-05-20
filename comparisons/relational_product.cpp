#include <abstracton/mata_extensions.hpp>
#include <chrono>
#include <random>

int main() {
    for (int i = 0; i < 5; ++i) {
        size_t num_of_levels = 2;
        size_t num_of_states = 5;
        std::vector<size_t> alphabet_sizes = {2, 3};
        double states_trans_ratio_per_symbol = 3.0;
        double final_state_density = 0.2;
        std::optional<unsigned int> seed = std::make_optional(std::random_device{}());
        mata::nft::Nft aut1 = mata::ext::builder::create_random_nft_tabakov_vardi(
            num_of_levels,
            num_of_states,
            alphabet_sizes,
            states_trans_ratio_per_symbol,
            final_state_density,
            seed
        );
        mata::nft::Nft aut2 = mata::ext::builder::create_random_nft_tabakov_vardi(
            num_of_levels,
            num_of_states,
            alphabet_sizes,
            states_trans_ratio_per_symbol,
            final_state_density,
            std::make_optional(seed.value() + 1)
        );

        mata::EnumAlphabet alph0 = {0, 1};
        mata::EnumAlphabet alph1 = {0, 1, 2};
        mata::AlphabetLevels alphabet_levels{std::vector<mata::Alphabet*>{&alph0, &alph1}};
        aut1.alphabets = &alphabet_levels;
        aut2.alphabets = &alphabet_levels;

        std::cout << "Seed: " << seed.value() << std::endl;

        std::cout << "Using alphabets:" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        mata::nft::Nft prod1 = mata::ext::relational_product_length_preserving({aut1, aut2});
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Size: " << prod1.num_of_states() << std::endl;
        std::cout << "Calculated in " << duration.count() << " microseconds" << std::endl;

        std::cout << "Always using DONT_CARE:" << std::endl;
        auto start2 = std::chrono::high_resolution_clock::now();
        mata::nft::Nft prod2 = mata::ext::relational_product_length_preserving_dont_care({aut1, aut2});
        auto end2 = std::chrono::high_resolution_clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
        std::cout << "Size: " << prod2.num_of_states() << std::endl;
        std::cout << "Calculated in " << duration2.count() << " microseconds" << std::endl;

        std::cout << "minimizing first result..." << std::endl;
        prod1 = mata::ext::minimize(prod1);
        std::cout << "new size: " << prod1.num_of_states() << std::endl;
        std::cout << "minimizing second result..." << std::endl;
        prod2 = mata::ext::minimize(prod2);
        std::cout << "new size: " << prod2.num_of_states() << std::endl;
        std::cout << "checking agreement of results..." << std::endl;
        if (!are_equivalent(prod1, prod2)) {
            std::cout << "ERROR: different answers for seed " << seed.value() << std::endl;
            return 0;
        }
    }
}
