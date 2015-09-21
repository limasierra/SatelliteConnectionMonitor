<?php

$mc_name = ["QPSK 1/4", "QPSK 1/3", "QPSK 2/5", "QPSK 1/2",
            "QPSK 3/5", "QPSK 2/3", "QPSK 3/4", "QPSK 4/5",
            "QPSK 5/6", "QPSK 8/9", "QPSK 9/10", "8PSK 3/5",
            "8PSK 2/3", "8PSK 3/4", "8PSK 5/6", "8PSK 8/9",
            "8PSK 9/10", "16APSK 2/3", "16APSK 3/4", "16APSK 4/5",
            "16APSK 5/6", "16APSK 8/9", "16APSK 9/10", "32APSK 3/4",
            "32APSK 4/5", "32APSK 5/6", "32APSK 8/9", "32APSK 9/10" ];

// Connect
$m = new MongoClient();

// Select a database
$db = $m->tc1;

// Select a collection (analogous to a relational database's table)
$collection = $db->mc;

// Send request to db and retrieve result
$cursor = $collection->find()->sort(['$natual' => -1])->limit(10000);
$cnt = $cursor->count();
$every_xth = $cnt / 100;

// Build array for buckets
$buckets = [];
foreach ($mc_name as $id => $mc) {
  $buckets[$id] = [];
}

// Preprocess the result to separate data for different
// MODCODs into different buckets
$step = 0;
foreach ($cursor as $id => $doc) {
  if (($step++ % $every_xth) != 0)
    continue;
  $ts = $doc["ts"]->sec * 1000;
  $mcs = $doc["arr"];
  foreach ($mcs as $mc_id => $mc_value) {
    $tmp = [$ts, $mc_value];
    array_push($buckets[$mc_id], $tmp);
  }
}

// Build a JSON objects to be returned
$all = [];
foreach ($buckets as $id => $values) {
  $arr = [];
  $arr["key"] = $mc_name[$id];
  $arr["values"] = $values;
  array_push($all, $arr);
}

echo json_encode($all);
