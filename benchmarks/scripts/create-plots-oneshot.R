library(tidyverse)

# Auto-discover every *.csv in results/. The basename (sans .csv) is used as
# the algorithm label throughout, so adding a new bench script + log + csv
# automatically shows up in the plots without changing this file.
csv_paths <- list.files("results", pattern = "\\.csv$", full.names = TRUE)
stopifnot(length(csv_paths) > 0)

results <- map_dfr(csv_paths, function(p) {
  read_csv(p, show_col_types = FALSE) |>
    mutate(algorithm = tools::file_path_sans_ext(basename(p)))
})

# Per-algorithm output summary (replaces the old hard-coded mata vs dodo prints).
for (algo in sort(unique(results$algorithm))) {
  cat("=== ", algo, " ===\n", sep = "")
  print(factor(results$output[results$algorithm == algo]) |> summary())
}

# Cumulative plot: one geom_step per algorithm.
results |>
  filter(!is.na(time)) |>
  mutate(time = as.numeric(time)) |>
  arrange(algorithm, time) |>
  group_by(algorithm) |>
  mutate(solved_cumulatively = row_number()) |>
  ungroup() |>
  ggplot(aes(x = time, y = solved_cumulatively, color = algorithm)) +
  geom_step() +
  xlab("time (s)") +
  ylab("number of instances solved in time") +
  labs(color = "algorithm")
ggsave("cumulative.png")

# Pairwise scatter: pick the first algorithm alphabetically as the reference
# x-axis and facet every other algorithm against it on y. Skipped if fewer
# than two algorithms were found.
if (length(csv_paths) >= 2) {
  algos <- sort(unique(results$algorithm))
  ref <- algos[1]
  others <- algos[-1]

  wide <- results |>
    mutate(time = as.numeric(time)) |>
    select(name, property, interpretation, algorithm, time) |>
    pivot_wider(names_from = algorithm, values_from = time)

  wide |>
    pivot_longer(all_of(others), names_to = "algorithm", values_to = "time_other") |>
    ggplot(aes(x = .data[[ref]], y = time_other, color = interpretation)) +
    geom_point() +
    scale_x_log10() +
    scale_y_log10() +
    geom_function(fun = function(x) x) +
    geom_function(fun = function(x) 10 * x, color = "green") +
    geom_function(fun = function(x) 0.1 * x, color = "red") +
    facet_wrap(~ algorithm) +
    xlab(paste0("time (s) — ", ref)) +
    ylab("time (s) — other algorithm")
  ggsave("comparison.png")
}
