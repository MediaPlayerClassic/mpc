<?php

define('ROOT_DIR', dirname(__FILE__).'/..'); 
define('TEMPLATE_DIR', ROOT_DIR.'/templates'); 
define('COMPILE_DIR', ROOT_DIR.'/templates_c'); 
define('CONFIG_DIR', ROOT_DIR.'/configs'); 
define('CACHE_DIR', ROOT_DIR.'/cache'); 
require ROOT_DIR.'/libs/Smarty.class.php';

class MySmarty extends Smarty
{
	function MySmarty()
	{
		$this->Smarty(); 

		$this->template_dir = TEMPLATE_DIR; 
		$this->compile_dir = COMPILE_DIR; 
		$this->config_dir = CONFIG_DIR; 
		$this->cache_dir = CACHE_DIR; 

		if(isset($_SERVER['REMOTE_ADDR']) && $_SERVER['REMOTE_ADDR'] == '127.0.0.1')
		{
			$this->compile_check = true;
			$this->debugging = true;
		}
		
		$this->assign('template', preg_replace('/(.+)\.php$/i', '\1.tpl', basename($_SERVER['PHP_SELF'])));
	}
}

$smarty = new MySmarty();

function RedirAndExit($path)
{
	if(empty($path)) exit;

	$http = isset($_SERVER["HTTPS"]) && $_SERVER["HTTPS"] == 'on' ? 'https://' : 'http://';
	
	$url = parse_url($path);
	$dir = str_replace("\\", '/', dirname($url['path']));
	if(empty($dir) || $dir[0] == '.' || $dir[0] != '/') $dir = str_replace("\\", '/', dirname($_SERVER['PHP_SELF']));
	if($dir == '/') $dir = '';

	header('Location: '.$http.$_SERVER['HTTP_HOST'].$dir.'/'.basename($path));
	exit;
}

function error404()
{
	header("HTTP/1.0 404 Not Found");
	exit;
}

function getParam($name, $i = null)
{
	$ret = false;
	if(isset($_POST[$name]))
	{
		$ret = $_POST[$name];
	}
	else if(isset($_GET[$name]))
	{
		$ret = $_GET[$name];
	}
	else if(isset($_SESSION['POST'][$name]))
	{
		$ret = $_SESSION['POST'][$name];
	}
	else if(isset($_COOKIE[$name]))
	{
		$ret = $_COOKIE[$name];
	}
	if($i !== null) $ret = is_array($ret) ? $ret[$i] : false;
	else if(is_array($ret)) return false;
	$ret = stripslashes($ret);
	return $ret;
}

$browser = isset($_SERVER['HTTP_USER_AGENT']) ? $_SERVER['HTTP_USER_AGENT'] : 'MSIE';
if(ereg("Opera", $browser)) $browser = "Opera";
else if(ereg("MSIE", $browser)) $browser = "MSIE";
else if(ereg("Mozilla", $browser)) $browser = "Mozilla";
$smarty->assign('browser', $browser);
unset($browser);

?>