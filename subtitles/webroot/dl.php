<?php

require '../include/MySmarty.class.php';
require '../include/DataBase.php';

session_start();

if(empty($_GET['id']) 
|| empty($_GET['ticket']) || empty($_SESSION['ticket'])
|| $_GET['ticket'] != $_SESSION['ticket'])
	error404();

$db = new SubtitlesDB();

$id = intval($_GET['id']);
$db->query("select hash, name, mime from subtitle where id = $id");
if(!($row = $db->fetchRow())) error404();

$hash = $row['hash'];
$name = $row['name'];
$mime = $row['mime'];

$fn = "../subcache/$hash";

@mkdir("../subcache");

if(!($sub = @file_get_contents($fn)) || empty($sub))
{
	$db->query("select sub from subtitle where id = $id");
	if(!($row = $db->fetchRow())) error404();

	$sub = $row['sub'];
	if($fp = fopen($fn, "wb")) {fwrite($fp, $sub); fclose($fp);}
}

$db->query("update subtitle set downloads = downloads+1 where id = $id");

header("Content-Type: $mime}");
header("Content-Disposition: attachment; filename=\"$name\"");
header("Pragma: no-cache");
echo gzuncompress($sub);
exit;

?>