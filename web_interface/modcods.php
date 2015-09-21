<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <link href="css/nv.d3.css" rel="stylesheet" type="text/css">
  <script src="https://cdnjs.cloudflare.com/ajax/libs/d3/3.5.2/d3.min.js" charset="utf-8"></script>
  <script src="js/nv.d3.js"></script>

  <style>
    text {
      font: 12px sans-serif;
    }
    svg {
      display: block;
    }
    html, body, svg {
      margin: 0px;
      padding: 0px;
      height: 100%;
      width: 100%;
    }
  </style>
</head>
<body class='with-3d-shadow with-transitions'>

<svg id="chart1"></svg>

<script>

  var histcatexplong = <?php include("get_modcod.php"); ?>;
  var colors = d3.scale.category20();

  var chart;
  nv.addGraph(function() {
    chart = nv.models.stackedAreaChart()
      .useInteractiveGuideline(true)
      .x(function(d) { return d[0] })
      .y(function(d) { return d[1] })
      .controlLabels({stacked: "Stacked"})
      .showLegend(false)
      .showControls(false)
      .duration(300)
      .style('expand');

    chart.xAxis.tickFormat(formatXAxis);
    chart.yAxis.tickFormat(d3.format(',.4f'));

    d3.select('#chart1')
      .datum(histcatexplong)
      .transition().duration(1000)
      .call(chart)
      .each('start', function() {
        setTimeout(function() {
          d3.selectAll('#chart1 *').each(function() {
            if(this.__transition__)
              this.__transition__.duration = 1;
          })
        }, 0)
      });

    nv.utils.windowResize(chart.update);
    return chart;
  });

  function formatXAxis(d) {
    // case for tooltip
    if (this === window)
        return d3.time.format('%a, %d %b %Y, %H:%M')(new Date(d));

    // case for axis
    return d3.time.format('%d.%m.%Y')(new Date(d));
  }

</script>
</body>
</html>
