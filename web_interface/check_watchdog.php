<?php

$connection = new MongoClient();

$collection = $connection->tc1->sys;

$document = $collection->findOne(["key" => "watchdog_ts"]);

$curr_ts = time();
$watchdog_ts = $document["val"]->sec;

echo $curr_ts - $watchdog_ts;
