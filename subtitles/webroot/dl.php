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
$db->query("select sub, name, mime from subtitle where id = $id");
if(!($row = $db->fetchRow())) error404();

$db->query("update subtitle set downloads = downloads+1 where id = $id");

header("Content-Type: {$row['mime']}");
header("Content-Disposition: attachment; filename=\"{$row['name']}\"");
header("Pragma: no-cache");
echo gzuncompress($row['sub']);
exit;

?>