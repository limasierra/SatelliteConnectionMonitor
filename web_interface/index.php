<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- The above 3 meta tags *must* come first in the head; any other head content must come *after* these tags -->
    <meta name="description" content="Satellite connection monitor">
    <meta name="author" content="Laurent Seiler">

    <title>Satellite Connection Monitor</title>

    <!-- Bootstrap core CSS -->
    <link href="css/bootstrap.css" rel="stylesheet">

    <!-- NVD3 stuff -->
    <link href="css/nv.d3.css" rel="stylesheet" type="text/css">

    <!-- Custom styles for this template -->
    <link href="css/custom.css" rel="stylesheet">

    <!-- HTML5 shim and Respond.js for IE8 support of HTML5 elements and media queries -->
    <!--[if lt IE 9]>
      <script src="js/html5shiv.min.js"></script>
      <script src="js/respond.min.js"></script>
    <![endif]-->
  </head>

  <body>

    <nav class="navbar navbar-inverse navbar-fixed-top">
      <div class="container">
        <div class="navbar-header">
          <button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target="#navbar" aria-expanded="false" aria-controls="navbar">
            <span class="sr-only">Toggle navigation</span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
          </button>
          <a class="navbar-brand" href="#">
            <span>
              <img src="img/logo.png" style="margin:-9px 5px 0 0;">
              Satellite Connection Monitor
            </span>
          </a>
        </div>
        <div id="navbar" class="collapse navbar-collapse ">
          <div class="navbar-left">
            <p id="srv-status" class="navbar-text">
              Server status: <span id="srv-status-icon" class="glyphicon"></span>
            </p>
            <p class="navbar-text">
              Newest data: <span id="newest-data"></span>
            </p>
          </div>
          <div class="navbar-right">
            <div class="btn-group" role="group">
              <button type="button" class="btn btn-default navbar-btn" id="interval_minute">Minute</button>
              <button type="button" class="btn btn-default navbar-btn active" id="interval_ten_minutes">10 Minutes</button>
              <button type="button" class="btn btn-default navbar-btn" id="interval_hour">Hour</button>
              <button type="button" class="btn btn-default navbar-btn" id="interval_half_day">&#189; Day</button>
              <button type="button" class="btn btn-default navbar-btn" id="interval_day">Day</button>
            </div>
          </div>
        </div>
      </div>
    </nav>

    <div id="graph-container" class="container-fluid main-content">

      <svg id="main-graph"></svg>

    </div><!-- /.container -->


    <!-- Bootstrap core JavaScript
    ================================================== -->
    <!-- Placed at the end of the document so the pages load faster -->
    <script src="js/jquery.min.js"></script>
    <script src="js/bootstrap.min.js"></script>
    <script src="js/d3.min.js" charset="utf-8"></script>
    <script src="js/nv.d3.js" charset="utf-8"></script>
    <script src="js/stream_layers.js"></script>

    <script>
      function checkServerWatchdog() {
        $.get("check_watchdog.php", function(data) {
          data = +data;
          if (data <= 60) {
            $('#srv-status').removeClass("srv-status-down").addClass("srv-status-ok");
            $('#srv-status-icon').removeClass("glyphicon-remove").addClass("glyphicon-ok");
          } else {
            $('#srv-status').removeClass("srv-status-ok").addClass("srv-status-down");
            $('#srv-status-icon').removeClass("glyphicon-ok").addClass("glyphicon-remove");
          }
        });
        setTimeout(checkServerWatchdog, 10000);
      }
      checkServerWatchdog();
    </script>

    <script>
      var chart = nv.models.lineWithFocusChart();

      nv.addGraph(function() {
          chart.xAxis.showMaxMin(false)
                     .tickFormat(formatXAxis)
                     .ticks(d3.time.day);
          chart.xScale(d3.time.scale());
          chart.x2Axis.showMaxMin(false)
                      .tickFormat(formatXAxis);
          chart.yAxis.tickFormat(d3.format(',.2f'))
                     .axisLabel("Es/N0")
                     .ticks(20);
          chart.y2Axis.tickFormat(d3.format(',.2f'));
          chart.lines.forceY([0, 20]);
          chart.lines2.forceY([0, 20]);
          chart.useInteractiveGuideline(true);

          return chart;
      });

      interval = 'ten_minutes';
      periodicUpdate();

      $('#interval_minute')
        .click(function() { getEsnoData('minute', this); });
      $('#interval_ten_minutes')
        .click(function() { getEsnoData('ten_minutes', this); });
      $('#interval_hour')
        .click(function() { getEsnoData('hour', this); });
      $('#interval_half_day')
        .click(function() { getEsnoData('half_day', this); });
      $('#interval_day')
        .click(function() { getEsnoData('day', this); });

      function periodicUpdate() {
        getEsnoData();
        setTimeout(periodicUpdate, 60000);
      }

      function isFocusWindowAtCurrent() {
        return ((new Date(chart.xAxis.domain()[1])).getTime() == chart.x2Axis.domain()[1]);
      }

      function moveFocusToCurrent() {
        var scale_max_curr = (new Date(chart.x2Axis.domain()[1])).getTime();
        var brush = chart.brushExtent();
        if (brush == null) return;
        var diff = scale_max_curr - brush[1];
        brush[0] += diff;
        brush[1] += diff;
        chart.brushExtent(brush).update();
      }

      function updateNewestDataIndicator() {
        var scale_max = new Date(chart.x2Axis.domain()[1]);
        $("#newest-data").html(d3.time.format('%d.%m.%Y, %H:%M')(scale_max));
      }

      function getEsnoData(i, button) {
        if (typeof i === 'undefined')
          i = interval;
        if (typeof button !== 'undefined')
          $(button).addClass('active').siblings().removeClass('active');
        interval = i;

        var move_focus_window = (isFocusWindowAtCurrent()) ? true : false;

        $.getJSON("get_esno.php?interval=" + interval, function(data) {
          d3.select('#main-graph')
            .datum(data)
            .call(chart);

          if (move_focus_window)
            moveFocusToCurrent();

          updateNewestDataIndicator();
          nv.utils.windowResize(chart.update);
        });
      }

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

