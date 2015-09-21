<?php

// Connect
$m = new MongoClient();

// Select a database
$db = $m->tc1;

// Select a collection (analogous to a relational database's table)
$collection = $db->sdd;

// Get URL variables
$interval_name = "minute";
if (isset($_GET["interval"]))
  $interval_name = $_GET["interval"];

// Set the appropriate Mongo request buckets for this interval
$interval_sub = [];
switch ($interval_name) {
case "minute":
  $interval_sup = [ '$dateToString' => [ 'format' => '%Y%m%d%H%M', 'date' => '$ts', ] ];
  break;
case "ten_minutes":
  $interval_sup = [ '$dateToString' => [ 'format' => '%Y%m%d%H', 'date' => '$ts', ] ];
  $interval_sub = [
    '$subtract' => [
      ['$minute' => '$ts'],
      ['$mod' => [['$minute' => '$ts'], 10]],
    ],
  ];
  break;
case "hour":
  $interval_sup = [ '$dateToString' => [ 'format' => '%Y%m%d%H', 'date' => '$ts', ] ];
  break;
case "half_day":
  $interval_sup = [ '$dateToString' => [ 'format' => '%Y%m%d', 'date' => '$ts', ] ];
  $interval_sub = [
    '$subtract' => [
      ['$hour' => '$ts'],
      ['$mod' => [['$hour' => '$ts'], 12]],
    ],
  ];
  break;
case "day":
  $interval_sup = [ '$dateToString' => [ 'format' => '%Y%m%d', 'date' => '$ts', ] ];
  break;
default:
  exit(1);
}

// Build full request for MongoDB
$selection = [
  [
    '$group' => [
      '_id' => [
        'interval_sup' => $interval_sup,
        'interval_sub' => $interval_sub,
        'rx' => '$rx',
        'ns' => '$ns',
      ],
      'ts' => ['$first' => '$ts'],
      'esno' => ['$avg' => '$esno'],
    ],
  ],
  [
    '$sort' => ['ts' => -1],
  ],
  [
    '$limit' => 10000,
  ],
];


// Send request to db and retrieve result
$cursor = $collection->aggregate($selection);

// Preprocess the result to separate data for different
// network segments into different buckets
$buckets = [];
foreach ($cursor["result"] as $document) {
    $ns = $document["_id"]["ns"] . " on " . $document["_id"]["rx"];
    $val = [];
    if(!array_key_exists($ns, $buckets))
      $buckets[$ns] = [];
    array_push($buckets[$ns], $document);
}

ksort($buckets);

// Build a JSON objects to be returned
$all = [];
foreach ($buckets as $ns => $values) {
  $arr = [];
  $arr["values"] = [];
  foreach ($values as $k => $v) {
    $tmp = [];
    $tmp["x"] = $v["ts"]->sec * 1000;
    if ($v["esno"] == 0)  // Produce empty space in graph if EsNo
      $tmp["y"] = NULL;   // is null. Remove these 3 lines if this
    else                  // behaviour is not desired.
      $tmp["y"] = $v["esno"];
    array_unshift($arr["values"], $tmp);
  }
  $arr["key"] = $ns;
  array_push($all, $arr);
}

echo json_encode($all);
