<?php

session_start();

require '../include/MySmarty.class.php';
require '../include/DataBase.php';
require '../include/isolang.inc';

unset($_SESSION['ticket']);

$text = getParam('text');
$discs = max(0, intval(getParam('discs')));
$isolang_sel = addslashes(getParam('isolang_sel'));
$format_sel = addslashes(getParam('format_sel'));

$files = array();

for($i = 0; !empty($_GET['name'][$i]) 
	&& !empty($_GET['hash'][$i]) && ereg('[0-9a-fA-F]{16}', $_GET['hash'][$i]) 
	&& !empty($_GET['size'][$i]) && ereg('[0-9a-fA-F]{16}', $_GET['size'][$i]);
	$i++)
{
	$name = $_GET['name'][$i];
	$hash = $_GET['hash'][$i];
	$size = $_GET['size'][$i];
	
	$files[] = array('name' => $name, 'hash' => $hash, 'size' => $size);
}

$smarty->assign('files', $files);

if(isset($_GET['del']))
{
	$ms_id = intval($_GET['del']);
	$succeeded = false;
	
	if($db->count("movie_subtitle where id = $ms_id && userid = {$db->userid} && userid") > 0)
	{
		$db->query("delete from movie_subtitle where id = $ms_id");
		$succeeded = true;
	}
	
	$smarty->assign('message', $succeeded ? 'Subtitle was removed successfully.' : 'Could not remove this subtitle!');
}
else if(!empty($files))
{
	foreach($files as $file)
	{
		$db->query( // close your eyes now...
			"select * from movie where id in ".
			" (select distinct movie_id from movie_subtitle where subtitle_id in ".
			"  (select id from subtitle where id in ".
			"   (select distinct subtitle_id from file_subtitle where file_id in ".
			"    (select id from file where hash = '{$file['hash']}' && size = '{$file['size']}')))) ".
			"limit 100 ");

		chkerr();
			
		while($row = $db->fetchRow())
			$movies[$row['id']] =  $row;
	}
}
else if(!empty($text))
{
	if(strlen($text) >= 3)
	{
		$db_text = ereg_replace('([_%])', '\\1', $text);
		$db_text = str_replace('*', '%', $db_text);
		$db_text = str_replace('?', '_', $db_text);
		$db_text = addslashes($db_text);

		$db->fetchAll(
			"select * from movie ".
			"where id in (select distinct movie_id from title where title like _utf8 '%$db_text%') ". // TODO: or id in (select distinct movie_id from movie_subtitle where name like _utf8 '%$db_text%')
			"and id in (select distinct movie_id from movie_subtitle where subtitle_id in (select distinct id from subtitle)) ".
			"limit 100 ",
			$movies);

		chkerr();
	}
	else
	{
		$smarty->assign('message', 'Text too short (min 3 chars)');
	}
}

if(!empty($movies))
{
	$users = array();

	foreach($movies as $i => $movie)
	{
		$movies[$i]['titles'] = array();

		$db->query("select * from title where movie_id = {$movie['id']}");
		while($row = $db->fetchRow()) $movies[$i]['titles'][] = $row['title'];

		chkerr();

		$movies[$i]['subs'] = array();

		$db->fetchAll(
			"select t1.id as ms_id, t1.name, t1.userid, t1.date, t1.notes, ".
			" t2.id, t2.discs, t2.disc_no, t2.format, t2.iso639_2, t2.downloads ".
			"from movie_subtitle t1 ".
			"join subtitle t2 on t1.subtitle_id = t2.id ".
			"where t1.movie_id = {$movie['id']} ".
			(!empty($discs)?" && discs = '$discs' ":"").
			(!empty($isolang_sel)?" && iso639_2 = '$isolang_sel' ":"").
			(!empty($format_sel)?" && format = '$format_sel' ":"").
			"order by t1.date desc, t2.disc_no asc ", 
			$movies[$i]['subs']);
			
		chkerr();
		
		foreach($movies[$i]['subs'] as $j => $sub)
		{
			$movies[$i]['updated'] = max(strtotime($sub['date']), isset($movies[$i]['updated']) ? $movies[$i]['updated'] : 0);
			$movies[$i]['subs'][$j]['language'] = empty($isolang[$sub['iso639_2']]) ? 'Unknown' : $isolang[$sub['iso639_2']];
			$movies[$i]['subs'][$j]['files'] = array();
			
			$userid = intval($sub['userid']);
			
			if(!isset($users[$userid]))
			{
				$db->query("select nick, email from user where userid = $userid");
				if($row = $db->fetchRow()) $users[$userid] = $row;
			}
			
			if(isset($users[$userid]))
			{
				$movies[$i]['subs'][$j]['nick'] = $users[$userid]['nick'];
				$movies[$i]['subs'][$j]['email'] = $users[$userid]['email'];
			}
			else
			{
				$movies[$i]['subs'][$j]['nick'] = 'Anonymous';
				$movies[$i]['subs'][$j]['email'] = '';
			}

			if($db->count("file_subtitle where subtitle_id = {$movies[$i]['subs'][$j]['id']} && file_id in (select id from file)") > 0)
			{
				$movies[$i]['subs'][$j]['has_file'] = true;
			
				foreach($files as $file)
				{
					$cnt = $db->count(
						"file_subtitle where subtitle_id = {$movies[$i]['subs'][$j]['id']} && file_id in ".
						" (select id from file where hash = '{$file['hash']}' && size = '{$file['size']}') ");
					if($cnt > 0)
					{
						$movies[$i]['subs'][$j]['files'][] = $file;
						$movies[$i]['found_file'] = true;
						break;
					}
				}
			}
		}
		
		if(empty($movies[$i]['titles']) || empty($movies[$i]['subs']))
		{
			unset($movies[$i]);
		}
	}
	
	// TODO: maybe we should prefer movies having imdb link a bit more?
	
	function cmp($a, $b)
	{
		if(isset($a['found_file']) && !isset($b['found_file'])) return -1;
		if(!isset($a['found_file']) && isset($b['found_file'])) return +1;
		return $b['updated'] - $a['updated'];
	}
	
	usort($movies, 'cmp');
}

if(isset($movies))
{
	if(empty($movies)) $smarty->assign('message', 'No matches were found');
	$smarty->assign('movies', $movies);
}

$smarty->assign('text', $text);
$smarty->assign('discs', $discs);

$smarty->assign('isolang', $isolang);
$smarty->assign('isolang_sel', $isolang_sel);

$smarty->assign('format', $db->enumsetValues('subtitle', 'format'));
$smarty->assign('format_sel', $format_sel);

$smarty->assign('ticket', $_SESSION['ticket'] = rand(1, 10000000)); // ;)

if(!empty($_REQUEST['player']))
{
	$smarty->assign('player', $_REQUEST['player']);
	$smarty->display('index.player.tpl');
	exit;
}

$smarty->display('main.tpl');

?>