<?php

session_start();

require '../include/MySmarty.class.php';
require '../include/DataBase.php';
require '../include/isolang.inc';

unset($_SESSION['ticket']);

$page = array(
	'start' => max(0, intval(getParam('start'))),
	'limit' => 10,
	'total' => 0
);

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
	
	if($db->count("movie_subtitle where id = $ms_id && userid = {$db->userid}") > 0)
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
			"select SQL_CALC_FOUND_ROWS * from movie where id in ".
			" (select distinct movie_id from movie_subtitle where subtitle_id in ".
			"  (select id from subtitle where id in ".
			"   (select distinct subtitle_id from file_subtitle where file_id in ".
			"    (select id from file where hash = '{$file['hash']}' && size = '{$file['size']}')))) ".
			"limit {$page['start']}, {$page['limit']} ");

		chkerr();
			
		while($row = $db->fetchRow())
			$movies[$row['id']] =  $row;
	}
}
else
{
	if(empty($text)) $text = '*';
	
	$db_text = ereg_replace('([_%])', '\\1', $text);
	$db_text = str_replace('*', '%', $db_text);
	$db_text = str_replace('?', '_', $db_text);
	$db_text = addslashes($db_text);
	if(!getParam('bw')) $db_text = '%'.$db_text;

	$db->query(
		"select SQL_CALC_FOUND_ROWS distinct t1.* from movie t1 ".
		"join movie_subtitle t2 on t1.id = t2.movie_id ".
		"where t1.id in ".
		"	( ".
			"select distinct movie_id from title where title like _utf8 '$db_text%' ".
			"union ".
			"select distinct movie_id from movie_subtitle where name like _utf8 '$db_text%' order by date desc ".
			") ".
		"and t1.id in (select distinct movie_id from movie_subtitle where subtitle_id in (select distinct id from subtitle)) ".
		"order by t2.date desc " .
		"limit {$page['start']}, {$page['limit']} ");
		
	while($row = $db->fetchRow())
		$movies[$row['id']] = $row;

	chkerr();
}

if(!empty($movies))
{
	$db->query("select FOUND_ROWS()");
	if($row = $db->fetchRow()) $page['total'] = $row[0];
	$page['count'] = count($movies);
	
	chkerr();
	
	$test_movie_id = "t1.movie_id in (".implode(',', array_keys($movies)).")";	

	// titles

	$db->query("select movie_id, title from title t1 where $test_movie_id");
	foreach($movies as $id => $movie) $movies[$id]['titles'] = array();
	while($row = $db->fetchRow()) $movies[$row['movie_id']]['titles'][] = $row['title'];
	
	chkerr();
	
	// subs

	$db->query(
		"select ".
		" t1.movie_id, t1.id as ms_id, t1.name, t1.userid, t1.date, t1.notes, t1.format, t1.iso639_2, ".
		" t2.id, t2.discs, t2.disc_no, t2.downloads, ".
		" t3.nick, t3.email, ".
		" (select count(*) from file_subtitle where subtitle_id = t2.id && file_id in (select id from file)) as has_file ".
		"from movie_subtitle t1 ".
		"join subtitle t2 on t1.subtitle_id = t2.id ".
		"left outer join user t3 on t1.userid = t3.userid ".
		"where $test_movie_id ".
		(!empty($discs)?" && t2.discs = '$discs' ":"").
		(!empty($isolang_sel)?" && t1.iso639_2 = '$isolang_sel' ":"").
		(!empty($format_sel)?" && t1.format = '$format_sel' ":"").
		(!empty($nick_sel)?" && t3.nick = '$nick_sel' ":"").
		"order by t1.date asc, t2.disc_no asc ");
	foreach($movies as $id => $movie) $movies[$id]['subs'] = array();
	while($row = $db->fetchRow()) $movies[$row['movie_id']]['subs'][] = $row;

	chkerr();

	foreach($movies as $id => $movie)
	{
		foreach($movies[$id]['subs'] as $j => $sub)
		{
			$movies[$id]['updated'] = max(strtotime($sub['date']), isset($movies[$id]['updated']) ? $movies[$id]['updated'] : 0);
			$movies[$id]['subs'][$j]['language'] = empty($isolang[$sub['iso639_2']]) ? 'Unknown' : $isolang[$sub['iso639_2']];
			$movies[$id]['subs'][$j]['files'] = array();
			
			if($movies[$id]['subs'][$j]['nick'] == null)
			{
				$movies[$id]['subs'][$j]['nick'] = 'Anonymous';
				$movies[$id]['subs'][$j]['email'] = '';
			}

			if(!empty($movies[$id]['subs'][$j]['has_file']))
			{			
				foreach($files as $file)
				{
					$cnt = $db->count(
						"file_subtitle where subtitle_id = {$movies[$id]['subs'][$j]['id']} && file_id in ".
						" (select id from file where hash = '{$file['hash']}' && size = '{$file['size']}') ");
					if($cnt > 0)
					{
						$movies[$id]['subs'][$j]['files'][] = $file;
						$movies[$id]['found_file'] = true;
						break;
					}
				}
			}
		}
		
		if(empty($movies[$id]['titles']) || empty($movies[$id]['subs']))
		{
			unset($movies[$id]);
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
	
	$page['index'] = array();
	
	if($page['limit'] < $page['total'])
	{
		$min = max(intval($page['start']/$page['limit']) - 9, 0);
		$max = min(intval($page['start']/$page['limit']) + 9, intval(($page['total']-1)/$page['limit']));
		for(; $min <= $max; $min++) $page['index'][] = $min*$page['limit'];
	}
	
	if(!empty($page['index']))
	{
		$page['has_less'] = $page['index'][0] > 0;
		$page['has_more'] = $page['index'][count($page['index'])-1] < ($page['total'] - $page['limit']);
		
		$cur = $page['start'] - $page['start']%$page['limit'];
		if($cur > 0) $page['prev'] = $cur - $page['limit'];
		if($cur + $page['limit'] < $page['total']) $page['next'] = $cur + $page['limit'];
	}
	
	$smarty->assign('page', $page);
}

$smarty->assign('text', $text);
$smarty->assign('discs', $discs);

$smarty->assign('isolang', $isolang);
$smarty->assign('isolang_sel', $isolang_sel);

$smarty->assign('format', $db->enumsetValues('movie_subtitle', 'format'));
$smarty->assign('format_sel', $format_sel);

$smarty->assign('ticket', $_SESSION['ticket'] = rand(1, 10000000)); // ;)

if(!empty($_REQUEST['player']))
{
	$smarty->assign('player', $_REQUEST['player']);
	$smarty->display('index.player.tpl');
	exit;
}

$index = array();
$index[] = array('mask' => "*", 'label' => 'All');
for($i = ord('A'); $i <= ord('Z'); $i++)
	$index[] = array('mask' => chr($i)."*", 'label' => chr($i));
$smarty->assign('index', $index);

$smarty->display('main.tpl');

?>