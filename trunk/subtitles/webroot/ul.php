<?php

require '../include/MySmarty.class.php';
require '../include/DataBase.php';
require '../include/isolang.inc';


// FIXME: $_SESSION['titles_imdb'][$imdb_id] <- $_SESSION['titles_imdb'.$imdb_id]
// ['titles_imdb'][$imdb_id] doesn't work with php 4.3.4

session_start();

if(isset($_GET['clearimdb']))
{
	$imdb_id = intval($_GET['clearimdb']);
	unset($_SESSION['titles_imdb'.$imdb_id]);
	RedirAndExit($_SERVER['PHP_SELF']);
}

if(!empty($_GET))
{
	session_unset();
	$_SESSION['POST'] = $_GET;
	
	$_SESSION['file'] = array();
	for($i = 0; !empty($_GET['name'][$i]) 
		&& !empty($_GET['hash'][$i]) && ereg('[0-9a-fA-F]{16}', $_GET['hash'][$i]) 
		&& !empty($_GET['size'][$i]) && ereg('[0-9a-fA-F]{16}', $_GET['size'][$i]);
		$i++)
	{
		$file['name'] = $_GET['name'][$i];
		$file['hash'] = $_GET['hash'][$i];
		$file['size'] = $_GET['size'][$i];
		sscanf($_GET['size'][$i], "%x", $file['intsize']);
		$_SESSION['file'][$i+1] = $file;
		// TODO: search imdb on name || size || hash -> imdb
	}

	RedirAndExit($_SERVER['PHP_SELF']);
}

$db = new SubtitlesDB();

$maxtitles = 4;
$maxsubs = 8;

//

function getIMDbTitles($imdb)
{
	$titles = array();

	if(($str = @file_get_contents(rtrim($imdb, '/')))
	|| ($str = @file_get_contents(eregi_replace('\.com/', '.com.nyud.net:8090/', rtrim($imdb, '/')))))
	{
		$str = str_replace("&#32;", "", $str);
		$str = str_replace("\r", "", $str);
		$str = str_replace("\n", "|", $str);
		if(preg_match('/<title>(.+)<\/title>/i', $str, $regs))
			$titles[] = trim($regs[1]);

		// TODO: stripos
		$aka = '<b class="ch">Also Known As';
		if(($str = stristr($str, $aka))
		&& ($str = substr($str, strlen($aka), strpos($str, '|') - strlen($aka))))
		{
			$tmp = explode('<br>', $str);
			foreach($tmp as $title)
			{
				$title = trim(strip_tags($title));
				if($i = strpos($title, ') ')) $title = substr($title, 0, $i+1);
				if(!empty($title) && strlen($title) > 1) $titles[] = $title;
			}
		}
	}
	
	return $titles;
}

function mergeTitles($a, $b)
{
	$ret = array();
	foreach(array_merge($a, $b) as $title)
	{
		$skip = false;
		foreach($ret as $i => $title0)
		{
			if(stristr($title, $title0) !== false) {$ret[$i] = $title; $skip = true;}
			else if(stristr($title0, $title) !== false) $skip = true;
		}
		if(!$skip) $ret[] = $title;
	}
	return $ret;
}

function storeMovie($imdb, $titles)
{
	$db_titles = array();
	foreach($titles as $title)
		$db_titles[] = addslashes($title);

	$movie_id = 0;

	global $db;
	$db->query("select * from movie where imdb = $imdb && imdb != 0 ");

	if($row = $db->fetchRow())
	{
		$movie_id = $row['id'];
	}
	else
	{
		$db->query("insert into movie (imdb) values ($imdb) ");
		$movie_id = $db->fetchLastInsertId();
	}

	foreach($db_titles as $db_title)
		if($db->count("title where movie_id = $movie_id && title = '$db_title'") == 0)
			$db->query("insert into title (movie_id, title) values ($movie_id, '$db_title') ");

	return $movie_id;
}
/*

set_time_limit(0);
for($i = 200000; $i < 1000000; $i++)
{
	if($db->count("title where movie_id in (select id from movie where imdb = $i)") > 0)
		continue;

//	$tmp = getIMDbTitles(sprintf("http://imdb.com.nyud.net:8090/title/tt%07d/", $i));
	$tmp = getIMDbTitles(sprintf("http://imdb.com/title/tt%07d/", $i));
	if(empty($tmp)) 
		continue;

	echo $i."\r\n";
	var_dump($tmp);
	echo "\r\n";
	storeMovie($i, $tmp);
//	sleep(3);
}
exit;
*/

if(isset($_POST['update']) || isset($_POST['submit']))
{
	$_SESSION['POST'] = $_POST;
	
	// validation

	unset($_SESSION['err']);
	
	$titles = array();
	for($i = 0; $i < $maxtitles; $i++)
		if($title = trim(strip_tags(getParam('title', $i))))
			$titles[] = $title;
	$imdb = trim(getParam('imdb'));
	$nick = strip_tags(getParam('nick'));
	$email = strip_tags(getParam('email'));

	if(!empty($imdb))
	{
		if(eregi('/title/tt([0-9]+)', $imdb, $regs))
		{
			$imdb_id = intval($regs[1]);
			$titles_imdb = array();
			
			if(empty($titles_imdb))
			{
				if(!empty($_SESSION['titles_imdb'.$imdb_id]))
					$titles_imdb = $_SESSION['titles_imdb'.$imdb_id];
			}

			if(empty($titles_imdb))
			{
				$db->query("select title from title where movie_id in (select id from movie where imdb = $imdb_id)");
				while($row = $db->fetchRow()) $titles_imdb[] = $row['title'];
				$_SESSION['titles_imdb'.$imdb_id] = $titles_imdb;
			}

			if(empty($titles_imdb))
			{
				$titles_imdb = getIMDbTitles($imdb);
				$_SESSION['titles_imdb'.$imdb_id] = $titles_imdb;
				storeMovie($imdb_id, $titles_imdb);
			}

//var_dump($titles_imdb);
//exit;
			$imdb = $imdb_id;
			$titles = mergeTitles($titles_imdb, $titles);
		}

		if(empty($titles_imdb)) $_SESSION['err']['imdb'] = true;
	}
	else
	{
		$imdb = 0;
	}
	
	if(empty($titles)) $_SESSION['err']['title'][0] = true;
	if(empty($nick)) $_SESSION['err']['nick'] = true;
	if(!empty($email) && !ereg('^[_a-z0-9-]+(\.[_a-z0-9-]+)*@[a-z0-9-]+(\.[a-z0-9-]{1,})*\.([a-z]{2,}){1}$', $email)) $_SESSION['err']['email'] = true;

	$_SESSION['err']['nosub'] = true;

	for($i = 0; $i < $maxsubs; $i++)
	{
		if(empty($_FILES['sub']['tmp_name'][$i])) continue;
		
		$format_sel = getParam('format_sel', $i);
		$isolang_sel = getParam('isolang_sel', $i);
		$discs = intval(getParam('discs', $i));
		$disc_no = intval(getParam('disc_no', $i));
		$file_sel = intval(getParam('file_sel', $i));
			
		if(empty($format_sel)) $_SESSION['err']['format_sel'][$i] = true;
		if(empty($isolang_sel)) $_SESSION['err']['isolang_sel'][$i] = true;
		if($discs < 1 || $discs > 127 || $disc_no < 1 || $disc_no > 127 || $disc_no > $discs) $_SESSION['err']['disc_no'][$i] = true;
		if(!empty($_SESSION['file']) && empty($_SESSION['file'][$file_sel])) $_SESSION['err']['file_sel'][$i] = true;
	
		if(!isset($_SESSION['err']['format_sel'][$i])
		&& !isset($_SESSION['err']['isolang_sel'][$i])
		&& !isset($_SESSION['err']['disc_no'][$i])
		&& !isset($_SESSION['err']['file_sel'][$i]))
		{
			unset($_SESSION['err']['nosub']);
		}	
	}
	
	if(!empty($_SESSION['err']) || isset($_POST['update']))
	{
		RedirAndExit($_SERVER['PHP_SELF']);
	}
	
	//

	$db_nick = addslashes($nick);
	$db_email = addslashes($email);

	$movie_id = storeMovie($imdb, $titles);

	for($i = 0; $i < $maxsubs; $i++)
	{
		if(empty($_FILES['sub']['tmp_name'][$i])) continue;
		
		$sub = @file_get_contents($_FILES['sub']['tmp_name'][$i]);
		$db_sub = addslashes(gzcompress($sub, 9));
		$db_name = addslashes(basename($_FILES['sub']['name'][$i]));
		$db_hash = md5($sub);
		$db_mime = addslashes($_FILES['sub']['type'][$i]);
		$format_sel = getParam('format_sel', $i); // TODO: verify this
		$isolang_sel = getParam('isolang_sel', $i); // TODO: verify this
		$discs = intval(getParam('discs', $i));
		$disc_no = intval(getParam('disc_no', $i));
		$file_sel = intval(getParam('file_sel', $i));
		$db_notes = addslashes(strip_tags(getParam('notes', $i)));

		$db->query("select id from subtitle where hash = '$db_hash'");

		if($row = $db->fetchRow())
		{
			$subtitle_id = $row[0];
		}
		else
		{
			$db->query(
				"insert into subtitle (discs, disc_no, sub, name, hash, mime, format, iso639_2, nick, email, date, notes) ".
				"values ($discs, $disc_no, '$db_sub', '$db_name', '$db_hash', '$db_mime', '$format_sel', '$isolang_sel', '$db_nick', '$db_email', NOW(), '$db_notes')");

			$subtitle_id = $db->fetchLastInsertId();
		}

		chkerr();
		
		if($db->count("movie_subtitle where movie_id = $movie_id && subtitle_id = $subtitle_id") == 0)
			$db->query("insert into movie_subtitle (movie_id, subtitle_id) values($movie_id, $subtitle_id)");

		chkerr();
		
		if(isset($_SESSION['file'][$file_sel]))
		{
			$file = $_SESSION['file'][$file_sel];
			
			$db_name = $file['name'];
			$hash = $file['hash'];
			$size = $file['size'];

			$db->query("select * from file where name = '$db_name' && hash = '$hash' && size = '$size'");
			if($row = $db->fetchRow()) $file_id = $row['id'];
			else {$db->query("insert into file (name, hash, size) values ('$db_name', '$hash', '$size')"); $file_id = $db->fetchLastInsertId();}
			
			chkerr();

			if($db->count("file_subtitle where file_id = $file_id && subtitle_id = $subtitle_id") == 0)
				$db->query("insert into file_subtitle (file_id, subtitle_id) values($file_id, $subtitle_id)");
	
			chkerr();
		}
	}
	
	if(!empty($email) && !empty($nick))
	{
		$db->query("update subtitle set nick = '$db_nick' where email = '$db_email'");
	}

	RedirAndExit('index.php?text='.urlencode($titles[0]));
}

// subs

$subs = array();
for($i = 0; $i < $maxsubs; $i++) $subs[] = $i;
$smarty->assign('subs', $subs);

function assign($param, $limit = 0)
{
	global $smarty;
	
	if($limit > 0)
	{
		$tmp = array();
		for($i = 0; $i < $limit; $i++) $tmp[$i] = getParam($param, $i);
		$smarty->assign($param, $tmp);
	}
	else
	{
		$smarty->assign($param, $tmp = getParam($param));
	}

	return $tmp;
}

function assign_cookie($param)
{
	global $smarty;
	$value = getParam($param);
	if($value !== false) setcookie($param, $value, time()+60*60*24*30, '/');
	$smarty->assign($param, $value);
	return $value;
}

// titles, imdb

assign('title', $maxtitles);
assign('imdb');

if(eregi('/title/tt([0-9]+)', getParam('imdb'), $regs) && !empty($_SESSION['titles_imdb'.intval($regs[1])]))
{
	$imdb_id = intval($regs[1]);
	$smarty->assign('imdb_id', $imdb_id);
	$smarty->assign('titles_imdb', $_SESSION['titles_imdb'.$imdb_id]);
}

// nick, email

assign_cookie('nick');
assign_cookie('email');

// subs

$smarty->assign('isolang', $isolang);
assign('isolang_sel', $maxsubs);
$smarty->assign('format', $db->enumsetValues('subtitle', 'format'));
assign('format_sel', $maxsubs);
assign('discs', $maxsubs);
assign('disc_no', $maxsubs);
$smarty->assign('file', !empty($_SESSION['file']) ? $_SESSION['file'] : false);
assign('file_sel', $maxsubs);
assign('notes', $maxsubs);

// err

if(isset($_SESSION['err'])) 
	$smarty->assign('err', $_SESSION['err']);

//

$smarty->display('main.tpl');

?>