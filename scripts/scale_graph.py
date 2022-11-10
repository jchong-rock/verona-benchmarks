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

    layout = go.Layout(
        xaxis = dict(
            title = 'Hardware Threads',
            tick0 = 1,
            dtick = 10,
            range=[0, 41]
        ),
        yaxis = dict (
            title = 'Time Taken (ms)',
            type="log", tickmode="array", tickvals=[1, 10, 100],
            title_standoff = 0
        ),
        hovermode='closest',
        legend=dict(
          orientation="h",
          yanchor="top",
          y=0.99,
          xanchor="right",
          x=0.995,
          bgcolor="LightSteelBlue",
          bordercolor="Black",
          borderwidth=1,
          # itemsizing = "constant",
        ),
        font=dict(
          family="Courier New, monospace",
          size=9,
          color="black",
        ),
    )

    fig.update_layout(layout)
            

    fig.write_html(f"output/scale.html")
    fig.write_image(f"output/scale.pdf")
    fig.write_image(f"output/scale.png")
