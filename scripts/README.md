# Scripts for generating graphs in the paper.

There are two scripts for running experiments:

* `run_savina.py`: This runs Savina for both Verona and Pony, and saves the results.
* `run_dining.py`: This runs the dining philosophers benchmark for both Verona and std::lock, and saves the results.

There are four scripts for generate tables/graphs for the paper:

* `produce_table_actor.py`: Generates LaTeX table for comparing performance of Pony with BoC (Actor).
* `produce_table_boc_full.py`: Generates LaTeX table for comparing performance of BoC (Actor) with BoC (Full).
* `produce_graph_dining.py`: Generates the graph for the scaling of the dining philosophers benchmark.
* `produce_graph_banking_scale.py`: Generates the graph for the scaling of banking example from Savina.

