import plotly.graph_objects as go
import argparse, sys, os, csv
import plotly.express as px
import numpy as np


def getopts():
  parser = argparse.ArgumentParser(description='Plot results for throughput test.')
  parser.add_argument("-o", default="results", help="output directory location")
  parser.add_argument("-i", default="outputs", help="input directory location")
  args = parser.parse_args()
  return args


if __name__ == '__main__':
  args = getopts()
  input_directory = args.i
  output_directory = args.o
  if not os.path.exists(output_directory):
      os.mkdir(output_directory)
  verona_results=input_directory + "/banking_scale.csv"
  with open(verona_results, newline='') as csvfile:
    reader = csv.reader(csvfile, delimiter=',')
    datum = []

    print("Loading data")
    for row in reader:
      paradigm, cores, benchmark, time = row
      datum.append({ "benchmark": benchmark.lower(), "paradigm": paradigm, "cores": cores, "time": float(time) })

    print("Plotting data")
    fig = px.box(datum, x="cores", y="time", boxmode="group")

    # Use logarithmic y-axis
    fig.update_yaxes(type="log")

    fontdict = dict(family="Courier New, monospace", size = 20, color = "black")

    layout = go.Layout(
        xaxis = dict(
            title = 'Hardware Threads',
            tick0 = 1,
            dtick = 10,
            range=[0, 41],
            showgrid=True
            # There's some interaction between ticks and gridlines
            # that means we have to turn gridlines on manually here
        ),
        yaxis = dict (
            title = 'Time Taken (ms)',
            type="log", tickmode="array",
            tickvals=[10, 100, 1000],
            # plotly handles log ranges in exponents rather than values
            # for some reason
            range=[1, 3],
            title_standoff = 0,
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
        font=fontdict,
      )

    fig.update_layout(layout)
    print("Writing output")
    fig.write_html(output_directory + f"/banking_scale.html")
    fig.write_image(output_directory + f"/banking_scale.pdf")
    fig.write_image(output_directory + f"/banking_scale.png")