import plotly.graph_objects as go
import argparse, sys, os, csv
import plotly.express as px
import numpy as np
#from sklearn import linear_model

def getopts():
  parser = argparse.ArgumentParser(description='Plot results for throughput test.')
  parser.add_argument('--html', action="store_true", help="save as html")
  parser.add_argument('--svg', action="store_true", help="save as svg")
  parser.add_argument("-o", default="results", help="output directory location")
  parser.add_argument("-i", default="outputs", help="input directory location")
  parser.add_argument("--fast", action='store_true', help="run the fast version of the benchmark")
  args = parser.parse_args()
  return args

input_directory = "outputs"
output_directory = "results"
fast = False

def plot(infiles, legends, symbols, colors, as_html, as_svg):
    fontdict = dict(family="Courier New, monospace", size = 20, color = "black")
    layout = go.Layout(
        xaxis = dict(
            title = 'Hardware Threads',
            tick0 = 1,
            dtick = 10,
            range=[0, 73]
        ),
        yaxis = dict (
            title = 'Time Taken (s)',
            type="log", tickmode="array", tickvals=[1, 10, 100],
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
          # font = dict(family = "Courier", size = 20, color = "black"),
        ),
        font=fontdict,
    )

    fig = go.Figure(layout=layout)

    for infile, legend, symbol, color in zip(infiles, legends, symbols, colors):
      with open(infile, 'r') as csvfile:
          reader = csv.DictReader(csvfile, delimiter=',')

          cores = []
          times = []
          for row in reader:
            cores.append(int(row["cores"]))
            times.append(float(row["time"]))

          cores, times = zip(*sorted(zip(cores, times), key=lambda tup: tup[1]))

          fig.add_trace(go.Scatter(
            x = cores,
            y = times,
            marker_symbol=symbol,
            marker_size=4,
            name = legend,
            mode = "markers",
            line=dict(color = color),
          ))

          # if legend == "Cowns (sequential)":
          #   # Create the line of best fit
          #   regr = linear_model.LinearRegression().fit(np.array(cores).reshape(-1, 1), np.array(times))

          #   cores = [i + 1 for i in range(72)]
          #   fig.add_trace(go.Scatter(
          #     x = cores,
          #     y = regr.predict(np.array(cores).reshape(-1, 1)).tolist(),
          #     mode = 'lines',
          #     line=dict(dash='dot', color = color, width=1),
          #     showlegend=False
          #   ))

    if fast:
      work = float(5)
    else:
      work = float(50)
    ideal = [ (cores+1, work / min(cores + 1, 50)) for cores in range(72)]
    (cores, times) = list(zip(*ideal))
    fig.add_trace(go.Scatter(
      x = cores,
      y = times,
      mode = 'lines',
      name = "Ideal",
      line=dict(color = colors[len(colors) - 1], width=1),
    ))

    if as_html:
      fig.write_html(output_directory + "/dining_scale.html")
    elif as_svg:
      fig.write_image(output_directory + "/dining_scale.svg")
      fig.write_image(output_directory + "/dining_scale.svg")
    else:
      # this is pretty stupid, but we do it twice because the print has something
      # on screen the first time
      fig.write_image(output_directory + "/dining_scale.pdf")
      fig.write_image(output_directory + "/dining_scale.pdf")

# This generates multiple graphs: each adding a new dataset to the previous datasets
def plot_inc(infiles, legends, symbols, colors):
    layout = go.Layout(
        xaxis = dict(
            title = 'Hardware Threads',
            tick0 = 1,
            dtick = 10,
            range=[0, 73]
        ),
        yaxis = dict (
            title = 'Time Taken (s)',
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
          font = dict(family = "Courier", size = 50, color = "black"),
        ),
        font=dict(
          family="Courier New, monospace",
          size=9,
          color="black",
        ),
    )

    for take in range(len(infiles) + 1):
        fig = go.Figure(layout=layout)

        for infile, legend, symbol, color in zip(infiles[:take], legends, symbols, colors):
          with open(infile, 'r') as csvfile:
              reader = csv.DictReader(csvfile, delimiter=',')

              cores = []
              times = []
              for row in reader:
                cores.append(int(row["cores"]))
                times.append(float(row["time"]))

              cores, times = zip(*sorted(zip(cores, times), key=lambda tup: tup[1]))

              fig.add_trace(go.Scatter(
                x = cores,
                y = times,
                marker_symbol=symbol,
                marker_size=4,
                name = legend,
                mode = "markers",
                line=dict(color = color),
              ))

              # if legend == "Cowns (sequential)":
              #   # Create the line of best fit
              #   regr = linear_model.LinearRegression().fit(np.array(cores).reshape(-1, 1), np.array(times))

              #   cores = [i + 1 for i in range(72)]
              #   fig.add_trace(go.Scatter(
              #     x = cores,
              #     y = regr.predict(np.array(cores).reshape(-1, 1)).tolist(),
              #     mode = 'lines',
              #     line=dict(dash='dot', color = color, width=1),
              #     showlegend=False
              #   ))

        ideal = [ (cores+1, float(50) / min(cores + 1, 50)) for cores in range(72)]
        (cores, times) = list(zip(*ideal))
        fig.add_trace(go.Scatter(
          x = cores,
          y = times,
          mode = 'lines',
          name = "Ideal",
          line=dict(color = colors[len(colors) - 1], width=1),
        ))

        fig.write_image(output_directory + f"/dining_scale-{take}.svg")
        fig.write_image(output_directory + f"/dining_scale-{take}.svg")

if __name__ == '__main__':
    args = getopts()
    input_directory = args.i
    output_directory = args.o
    fast = args.fast
    if not os.path.exists(output_directory):
        os.mkdir(output_directory)

    files_to_plot = [
      # input_directory + "/pthread_dining_opt_manual.csv",
      input_directory + "/pthread_dining_opt.csv",
      # input_directory + "/verona_dining_seq.csv",
      input_directory + "/verona_dining_opt.csv",
    ]
    symbols = [f"{symbol}-open" for symbol in ["triangle-up", "circle"]]
    colors = list(px.colors.qualitative.D3)
    colors = [colors[1], colors[4], colors[2]] # i like ideal to be green
    legends = ["Threads and Mutex", "Cowns and Behaviours"]
    plot(files_to_plot, legends, symbols, colors, False, False)

    # symbols = [f"{symbol}-open" for symbol in ["triangle-up", "x-thin", "circle", "cross-thin"]]
    # colors = list(px.colors.qualitative.D3)
    # colors = [colors[0], colors[1], colors[3], colors[4], colors[2]] # i like ideal to be green
    # legends = ["Threads and Mutex (manual)", "Threads and Mutex (std::lock)", "Cowns (sequential)", "Cowns (alternating)"]
    # plot_inc(files_to_plot, legends, symbols, colors)
