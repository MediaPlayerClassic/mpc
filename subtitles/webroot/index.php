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

$db_text = trim(ereg_replace('[^a-zA-Z0-9]', '.', addslashes($text)));

if(!empty($db_text) && strlen($db_text) >= 3)
{
	$db->fetchAll(
		"select * from movie where id in ".
		"(select distinct movie_id from title ".
		" where title regexp '.*$db_text.*') ".
		"limit 100 ",
		$movies);

	chkerr();
}

// TODO: search on name || size || hash -> movies

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
			"order by t2.date desc, t2.disc_no asc", 
			$movies[$i]['subs']);

		chkerr();

		foreach($movies[$i]['subs'] as $j => $sub)
		{
			$movies[$i]['updated'] = max(strtotime($sub['date']), isset($movies[$i]['updated']) ? $movies[$i]['updated'] : 0);
			$movies[$i]['subs'][$j]['language'] = empty($isolang[$sub['iso639_2']]) ? 'Unknown' : $isolang[$sub['iso639_2']];
		}
		
		if(empty($movies[$i]['titles']) || empty($movies[$i]['subs']))
		{
			unset($movies[$i]);
		}
	}
	
	// TODO: maybe we should prefer movies having imdb link a bit more?
	function cmp($a, $b) {return $b['updated'] - $a['updated'];}
	usort($movies, 'cmp');

	if(empty($movies)) $smarty->assign('message', 'No matches were found');
	else $smarty->assign('movies', $movies);
}
else if(!empty($text))
{
	$smarty->assign('message', 'Text too short (min 3 chars)');
}

$smarty->assign('text', $text);
$smarty->assign('discs', $discs);

$smarty->assign('isolang', $isolang);
$smarty->assign('isolang_sel', $isolang_sel);

$smarty->assign('ticket', $_SESSION['ticket'] = rand(1, 10000000)); // ;)

$smarty->display('main.tpl');

?>