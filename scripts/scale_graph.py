import plotly.graph_objects as go
import argparse, sys, os, csv
import plotly.express as px
import numpy as np




verona_results=sys.argv[1]
with open(verona_results, newline='') as csvfile:
  reader = csv.reader(csvfile, delimiter=',')
  datum = []

  for row in reader:
    paradigm, cores, benchmark, time = row
    datum.append({ "benchmark": benchmark.lower(), "paradigm": paradigm, "cores": cores, "time": float(time) })

    fig = px.box(datum, x="cores", y="time", boxmode="group")

    # Use logarithmic y-axis
    fig.update_yaxes(type="log")

    fig.update_layout(xaxis_title="Cores", yaxis_title="Time in ms")

    fig.write_html(f"output/scale.html")
    fig.write_image(f"output/scale.pdf")
    fig.write_image(f"output/scale.png")
