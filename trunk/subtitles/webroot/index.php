<?php

require '../include/MySmarty.class.php';
require '../include/DataBase.php';
require '../include/isolang.inc';

session_start();
session_unset();

$db = new SubtitlesDB();

$text = getParam('text');
$discs = max(0, intval(getParam('discs')));
$isolang_sel = addslashes(getParam('isolang'));

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

if(!empty($files))
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
		$db_text = addslashes(ereg_replace('[^a-zA-Z0-9]+', '.+', $text));
	
		$db->fetchAll(
			"select * from movie ".
			"where id in (select distinct movie_id from title where title regexp '.*$db_text.*') ".
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
	foreach($movies as $i => $movie)
	{
		$movies[$i]['titles'] = array();

		$db->query("select * from title where movie_id = {$movie['id']}");
		while($row = $db->fetchRow()) $movies[$i]['titles'][] = $row['title'];

		chkerr();

		$movies[$i]['subs'] = array();
		
		$db->fetchAll(
			"select t2.* from movie_subtitle t1 ".
			"join subtitle t2 on t1.subtitle_id = t2.id ".
			"where t1.movie_id = {$movie['id']} ".
			(!empty($isolang_sel)?" && iso639_2 = '$isolang_sel' ":"").
			(!empty($discs)?" && discs = '$discs' ":"").
			"order by t2.date desc, t2.disc_no asc ", 
			$movies[$i]['subs']);

		chkerr();

		foreach($movies[$i]['subs'] as $j => $sub)
		{
			$movies[$i]['updated'] = max(strtotime($sub['date']), isset($movies[$i]['updated']) ? $movies[$i]['updated'] : 0);
			$movies[$i]['subs'][$j]['language'] = empty($isolang[$sub['iso639_2']]) ? 'Unknown' : $isolang[$sub['iso639_2']];
			$movies[$i]['subs'][$j]['files'] = array();

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

$smarty->assign('ticket', $_SESSION['ticket'] = rand(1, 10000000)); // ;)

if(!empty($_REQUEST['player']))
{
	$smarty->assign('player', $_REQUEST['player']);
	$smarty->display('index.tpl');
	exit;
}

$smarty->display('main.tpl');

?>