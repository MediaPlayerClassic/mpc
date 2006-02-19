<?php

session_start();

require '../include/MySmarty.class.php';

$intypes = array(
	0 => 'detect',
	1 => '???');

$outtypes = array(
	0 => 'srt',
	1 => 'sub');
	
$smarty->assign('fps', 24);
$smarty->assign('intypes', $intypes);
$smarty->assign('outtypes', $outtypes);

$fps = !empty($_POST['fps']) ? stripslashes($_POST['fps']) : 24;
$intype = isset($_POST['intype']) ? intval($_POST['intype']) : 0;
$outtype = isset($_POST['outtype']) ? intval($_POST['outtype']) : 0;
$text = isset($_POST['text']) ? stripslashes($_POST['text']) : '';

if(!empty($_POST))
{
	$subs = array();

	if($intype == 0)
	{
		$rows = explode("\n", $text);
		$text = '';
		
		$sample = array_slice($rows, 0, min(10, count($rows)));
		foreach($sample as $row)
		{	
			if(preg_match('/^([0-9]{2})([0-9]{2})\.([0-9]+)-([0-9]{2})([0-9]{2})\.([0-9]+)/', $row, $matches))
			{
				$intype = 1;
				break;
			}
		}
		
		if($intype == 1)
		{
			foreach($rows as $row)
			{
				$row = trim($row);
				
				if(preg_match('/^([0-9]{2})([0-9]{2})\.([0-9]+)-([0-9]{2})([0-9]{2})\.([0-9]+)/', $row, $matches))
				{
					$start = (intval($matches[1])*60 + intval($matches[2])*60/100 + intval($matches[3])/$fps)*1000;
					$stop = (intval($matches[4])*60 + intval($matches[5])*60/100 + intval($matches[6])/$fps)*1000;
					$subs[] = array('start' => $start, 'stop' => $stop, 'rows' => array());
					$sub = end($subs);
				}
				else if(!empty($row))
				{
					if(empty($subs)) break;
					$subs[count($subs)-1]['rows'][] = $row;
				}
			}
		}
	}
	
	$text = array();
	
	foreach($subs as $i => $sub)
	{
		$start = $sub['start'];
		$stop = $sub['stop'];
		
		$ms1 = $sub['start'] % 1000; $sub['start'] /= 1000;
		$ss1 = $sub['start'] % 60; $sub['start'] /= 60;
		$mm1 = $sub['start'] % 60; $sub['start'] /= 60;
		$hh1 = $sub['start'];
		$ms2 = $sub['stop'] % 1000; $sub['stop'] /= 1000;
		$ss2 = $sub['stop'] % 60; $sub['stop'] /= 60;
		$mm2 = $sub['stop'] % 60; $sub['stop'] /= 60;
		$hh2 = $sub['stop'];

		if($outtype == 0)
		{
			$text[] = $i;
			$text[] = sprintf("%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d", 
				$hh1, $mm1, $ss1, $ms1, $hh2, $mm2, $ss2, $ms2);
			foreach($sub['rows'] as $row)
				$text[] = $row;
			$text[] = '';
		}
		else if($outtype == 1)
		{
			$text[] = sprintf("{%d}{%d}%s", 
				$start * $fps / 1000, 
				$stop * $fps / 1000, 
				implode('|', $sub['rows']));
		}
	}
	
	$text = implode("\r\n", $text);
	
	if(!empty($text))
	{
		header('Content-Type: application/octet-stream');
		header("Content-Disposition: attachment; filename=\"subtitle.{$outtypes[$outtype]}\"");
		header("Pragma: no-cache");
	
		if(!headers_sent() && extension_loaded("zlib")
		&& ereg("gzip", $_SERVER["HTTP_ACCEPT_ENCODING"]))
		{
			$text = gzencode($text, 9);
			
			header("Content-Encoding: gzip");
			header("Vary: Accept-Encoding");
			header("Content-Length: ".strlen($text));
		}
	
		echo $text;
		exit;
	}
	
	$smarty->assign('intype', $intype);
	$smarty->assign('outtype', $outtype);
	$smarty->assign('fps', $fps);
	$smarty->assign('text', $_POST['text']);
	$smarty->assign('conversion_error', true);
}

$smarty->display('main.tpl');

?>